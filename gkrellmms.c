/*  GKrellMMS: GKrellM XMMS Plugin
 |  Copyright (C) 2000-2004 Sander Klein Lebbink
 |
 |  Original Author:  Sander Klein Lebbink <sander@cerberus.demon.nl>
 |  Current Maintainer: Sjoerd Simons <sjoerd@luon.net>
 |  Latest versions might be found at:  http://gkrellm.luon.net/
 |
 |  This program is free software which I release under the GNU General Public
 |  License. You may redistribute and/or modify this program under the terms
 |  of that license as published by the Free Software Foundation; either
 |  version 2 of the License, or (at your option) any later version.
 |
 |  This program is distributed in the hope that it will be useful,
 |  but WITHOUT ANY WARRANTY; without even the implied warranty of
 |  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 |  GNU General Public License for more details.
 |
 |  To get a copy of the GNU General Public License,  write to the
 |  Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "gkrellmms.h"

#include "pixmaps/prev.xpm"
#include "pixmaps/play_pause.xpm"
#include "pixmaps/stop.xpm"
#include "pixmaps/next.xpm" 
#include "pixmaps/eject.xpm"

#include "pixmaps/prev_clicked.xpm"
#include "pixmaps/play_pause_clicked.xpm"
#include "pixmaps/stop_clicked.xpm"
#include "pixmaps/next_clicked.xpm" 
#include "pixmaps/eject_clicked.xpm"

typedef struct {
  GkrellmPiximage *image;
  GkrellmDecalbutton *button;
  gint x,y,w,h;
  double x_scale,y_scale;
} ControlButton;

static ControlButton prev_button, 
                     play_button, 
                     stop_button,
                     next_button,
                     eject_button;

/* Define xmms-vars */
static gint where_to_jump;

static gboolean xmms_playing = FALSE;
static GkrellmMonitor *monitor;

/* Define gkrellm-vars */
static GkrellmTextstyle *ts, *ts_alt;
static GkrellmStyle *style;
static GkrellmStyle *scroll_style;

/* for scrolling text */
static GkrellmPanel *scroll_panel = NULL;
static GkrellmDecal *scroll_text;
static GkrellmDecal *scroll_in_motion;
static gint scroll_motion_x, x_scroll;
static GtkTooltips *scrolling_tooltip;
static gchar *scrolling_tooltip_text;

/* for time-bar */
static GkrellmPanel *control_panel;
static GkrellmKrell *time_krell, *slider_in_motion;

static GkrellmDecal *xmms_decal,*led_decal;
static gint  led_off_index = 0,
             led_running_index = 1,
             led_paused_index = 2,
             led_playing_index= 3;

static gint	scroll_separator_len;

static gboolean	gkrellmrc_button_placement;
static gboolean got_motion;
static gint style_id;
static GkrellmTicks *pGK;

DBusGProxy* proxy = NULL;

/* drag and drop support */

enum {
  DROP_PLAINTEXT,
  DROP_URLENCODED,
  DROP_STRING,
  NUM_DROP_TYPES
};

static GtkTargetEntry drop_types[] =
{
  {"text/plain",    0, DROP_PLAINTEXT},
  {"text/uri-list", 0, DROP_URLENCODED},
  {"STRING",        0, DROP_STRING}
};


void do_xmms_command(gint i) {
  if (!xmms_running) return ;

  switch (i) {
    case gkrellmms_prev: audacious_remote_playlist_prev(proxy); break;
    case gkrellmms_play:
      if (audacious_remote_is_playing(proxy) && 
          !audacious_remote_is_paused(proxy)) 
        audacious_remote_pause(proxy);
      else
        audacious_remote_play(proxy);
      break;
    case gkrellmms_paus: audacious_remote_pause(proxy); break;
    case gkrellmms_stop:
      audacious_remote_stop(proxy);
// FIXME      time_krell->previous = t = 0;
      break;
    case gkrellmms_next: audacious_remote_playlist_next(proxy); break;
    case gkrellmms_eject:
      if (eject_opens_playlist)
        pl_show_playlist();
      else
        audacious_remote_eject(proxy);
      break;
  }
}

static void
cb_control_button(GkrellmDecalbutton *button) {
  gint control_id = GPOINTER_TO_INT(button->data);
  if (!xmms_running && control_id == gkrellmms_play) xmms_start_func();
  else do_xmms_command(control_id);
}

static void
set_panel_status(void) {
	if (!xmms_running || !scroll_enable) gkrellm_panel_hide(scroll_panel); 
  else gkrellm_panel_show(scroll_panel);

  gkrellm_set_button_sensitive(prev_button.button, xmms_running);
	gkrellm_set_button_sensitive(stop_button.button, xmms_running);
	gkrellm_set_button_sensitive(next_button.button, xmms_running);
	gkrellm_set_button_sensitive(eject_button.button, xmms_running);
}

void
gkrellmms_set_scroll_separator_len(void)
	{
	scroll_separator_len = gkrellm_gdk_string_width(
							scroll_text->text_style.font,
							scroll_separator);
	}


static gchar *
get_scrolling_title_text(gint *ret_width, gboolean reset)
	{
	gint          cur_time, cur_position;
	gchar         *cur_title;
	static gint   time, position, width;
	static gchar	*title, *scrolling_text;

	cur_time = pl_get_current_time();
	cur_position = pl_get_current_position();
	cur_title = pl_get_current_title();
	if (   !scrolling_text || reset
			|| cur_time != time || cur_position != position
			|| gkrellm_dup_string(&title, cur_title)
	   )
		{
		time = cur_time;
		position = cur_position;
		g_free(scrolling_text);
		if (time > 0)
			scrolling_text = g_strdup_printf("%d. %s (%d:%02d)",
						position, title, time / (60 * 1000), time/1000 % 60);
		else
				scrolling_text = g_strdup_printf("%d. %s", position, title);
		width = gkrellm_gdk_string_width(scroll_text->text_style.font,
							scrolling_text);
		}
	if (ret_width)
		*ret_width = width;
	if (reset)
		gkrellmms_set_scroll_separator_len();
	return scrolling_text;
	}

static void
update_gkrellmms() {
	GkrellmMargin *margin;
	static gint	output_time, len, w;
	static gint	prev_position = -1;
	gchar		*scrolling_title_text;
	gchar		*prev_scrolling_tooltip_text;
	gchar		*tooltip_utf8 = NULL, *tooltip_locale = NULL;
	gchar		*more_scrolled;
	gchar		*xmms_string;
	gint		rate;
	gint		freq;
	gint		nch;
  gint    time;
  gint    playlist_changed = FALSE;
  gint    position_changed = FALSE;
  gint slider_position = 0;
	static gint	on_index = 0, off_index = 0, led_status = 0,led_on = FALSE;

	if (pGK->second_tick) 
    set_panel_status();

  playlist_changed = update_playlist();

	if ((xmms_running = audacious_remote_is_running(proxy))) {
    /* position is changed in the playlist when the playlist is changed too !
     */
    position_changed = 
      pl_get_current_position() != prev_position || playlist_changed;
    prev_position = pl_get_current_position();
		xmms_playing = audacious_remote_is_playing(proxy);

		if (scroll_panel) {
			/* Scrollbar */
      if (scroll_enable && !scroll_in_motion) {
				margin = gkrellm_get_style_margins(style);
				w = gkrellm_chart_width() - margin->left - margin->right - 2;

				scrolling_title_text = get_scrolling_title_text(&len, FALSE);
				time = pl_get_current_time();
				if (scrolling_tooltip != NULL) {
					audacious_remote_get_info(proxy, &rate, &freq, &nch);
					prev_scrolling_tooltip_text = scrolling_tooltip_text;
          scrolling_tooltip_text = g_strdup_printf("%s\n%d%s - %dHz - %s",
				  scrolling_title_text, rate / 1000,
				    	(time  == -1) ? "bpm" : "kb/s", freq,
							(nch == 1) ? "mono" : "stereo");

					if (!strcmp(prev_scrolling_tooltip_text, scrolling_tooltip_text)) { 
            gkrellm_locale_dup_string(&tooltip_utf8,scrolling_tooltip_text,
                                      &tooltip_locale);
            gtk_tooltips_set_tip(scrolling_tooltip, scroll_panel->drawing_area,
								                  tooltip_utf8, NULL);
            g_free(tooltip_utf8);
						g_free(tooltip_locale);
					}
					g_free(prev_scrolling_tooltip_text);
			  }
#if defined(GKRELLM_HAVE_DECAL_SCROLL_TEXT)
				/* gkrellm >= 2.2.0 has a scroll text decal which minimizes slow
				|  Pango renders as the decal text is scrolled (offset is changed).
				*/
				if (len > w) {
					more_scrolled = g_strdup_printf("%s%s",
							scrolling_title_text, scroll_separator);
			  	x_scroll = (x_scroll + 1) % (len + scroll_separator_len);
					gkrellm_decal_scroll_text_set_text(scroll_panel,
									scroll_text, more_scrolled);
					gkrellm_decal_scroll_text_horizontal_loop(scroll_text, TRUE);
					gkrellm_decal_text_set_offset(scroll_text, -x_scroll, 0);
		    }
				else {
					more_scrolled = g_strdup("");
					x_scroll = 0;
					gkrellm_decal_text_set_offset(scroll_text, 0, 0);
					gkrellm_draw_decal_text(scroll_panel, scroll_text,
									scrolling_title_text, 0);
				}
#else
			  if (len > w) {
			  	more_scrolled = g_strdup_printf("%s%s%s",
			  	scrolling_title_text, scroll_separator, scrolling_title_text);
			  	x_scroll = (x_scroll + 1) % (len + scroll_separator_len);
			  	scroll_text->x_off = w - x_scroll - len;
		    	} else {
			    	more_scrolled = g_strdup("");
			    	x_scroll = scroll_text->x_off = 0;
		    }

		  	gkrellm_draw_decal_text(scroll_panel, scroll_text,
						(len > w) ? more_scrolled : scrolling_title_text,
						(gulong) (len > w) ? (w - x_scroll - len) : -1);
#endif
		  	gkrellm_draw_panel_layers(scroll_panel);

		  	g_free(more_scrolled);
		  }
	  }

		/* The time bar has a max.-scale of 100, so count the output-time
		| to a max. of 100
		*/
	}

	/* Also draw xmms-status when xmms isn't running,
	|  but don't while seeking
	*/
	if (slider_in_motion == NULL) {
		xmms_decal->x_off = 1;
		if(xmms_running && (xmms_playing || position_changed) && draw_time){ 
		  output_time = audacious_remote_get_output_time(proxy);
      /* calculate slider position */
  	  slider_position = pl_get_current_time() ? 
                             ((output_time * 100) / pl_get_current_time()) : 0;
      if (slider_position < 0)
        slider_position = 0;
      else if (slider_position > 100)
        slider_position = 100;

      /* render timer string */
			if (time_format || pl_get_current_time() <= 0) {
				xmms_string = g_strdup_printf("%02d:%02d",
						output_time / 60000, (output_time / 1000) % 60);
      } else {
				xmms_string = g_strdup_printf(
						draw_minus ? "-%02d:%02d" : "%02d:%02d",
						(pl_get_current_time() - output_time) / 60000,
						((pl_get_current_time() - output_time) / 1000) % 60);
				if (draw_minus)
					xmms_decal->x_off = -1;
				}
			} else {
        xmms_string = g_strdup(gkrellmms_label);
      }

		gkrellm_draw_decal_text(control_panel, xmms_decal, xmms_string, -1);

    /* calculate if led should be on or off */
    if (led_status < 0) {
      /* want to blink for two seconds */
      led_status = gkrellm_update_HZ() * 2;
    }
    led_on = (--led_status) > gkrellm_update_HZ();

		if (!xmms_running)  {
      on_index = led_playing_index;
      off_index = led_off_index;
    } else if (audacious_remote_is_paused(proxy)) {
			on_index = led_off_index;	/* invert the duty cycle */
			off_index = led_paused_index;
		} else if (!xmms_playing) {
			on_index = led_running_index;
			off_index = led_off_index;
		} else	/* Playing */ {
			led_on = TRUE; /* no blinking */
			on_index = led_playing_index;
		}
		gkrellm_draw_decal_pixmap(control_panel, led_decal,
				led_on ? on_index : off_index);

		gkrellm_update_krell(control_panel, time_krell, (gulong) slider_position);
		gkrellm_draw_panel_layers(control_panel);
		g_free(xmms_string);
  }
}

static gint
panel_expose_event(GtkWidget *widget, GdkEventExpose *ev, GkrellmPanel *p) {
  gdk_draw_pixmap(widget->window,
                  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                  p->pixmap,
                  ev->area.x, ev->area.y, ev->area.x,
                  ev->area.y, ev->area.width, ev->area.height);
  return FALSE;
}

static gint
slider_motion(GtkWidget *widget, GdkEventMotion *ev, gpointer data) {
  gint   z;
  GdkModifierType state;

  if (slider_in_motion != NULL) {
    /* Check if button is still pressed, in case missed button_release */
    state = ev->state;
    if (!(state & GDK_BUTTON1_MASK))
    {
      slider_in_motion = NULL;
      return TRUE;
    }
    z = ev->x * time_krell->full_scale / (gkrellm_chart_width() - 1);
    if (z < 0)
      z = 0;
    else if (z > 100)
      z = 100;

    time_krell->previous = 0;
    gkrellm_update_krell(control_panel, time_krell, (gulong) z);
    gkrellm_draw_panel_layers(control_panel);

    where_to_jump = (z * pl_get_current_time()) / 100;
    if (where_to_jump >= pl_get_current_time()) where_to_jump = pl_get_current_time() - 1;
    got_motion = TRUE;
  }
  return TRUE;
}

static void 
drag_data_received(GtkWidget *window,GdkDragContext *context, gint x, gint y,
                   GtkSelectionData *data,guint info,guint time,gpointer date) {
  if (data->data) {
    audacious_remote_playlist_clear(proxy);
    audacious_remote_playlist_add_url_string(proxy,(gchar *)data->data);
    audacious_remote_play(proxy);
    update_playlist();
  }
}

static gint
panel_button_release(GtkWidget *widget, GdkEventButton *ev, gpointer data) {
  gint z;
  gint timer;
  time_t lt;

  if (slider_in_motion) {
    if (!got_motion) {
      /* Also jump to time if you've clicked once on the Krell */
      z = ev->x * time_krell->full_scale / (gkrellm_chart_width() - 1);
      if (z < 0)
        z = 0;
      else if (z > 100)
        z = 100;
      where_to_jump = (z * pl_get_current_time()) / 100;
      if (where_to_jump >= pl_get_current_time()) 
        where_to_jump = pl_get_current_time() -1;

      time_krell->previous = 0;
      gkrellm_update_krell(time_bar, time_krell, (gulong) z);
      gkrellm_draw_panel_layers(time_bar);
    }

    if (where_to_jump > pl_get_current_time()) return FALSE;

    /*
     | Let gkrellm sleep for about 0 seconds when sending this commands
     | when xmms isn't playing, so that xmms received the functions for
     | sure, and gkrellm doesn't do weird stuff on it.
     | Maybe it's not neccasary if you've got a fast computer, but on my
     | P200 I really notice some bugs without it.
     */
    if (!xmms_playing)
      audacious_remote_play(proxy);
    timer = time(&lt);

    /* Do nothing, wait until xmms really plays;
     | stop waiting after 10 seconds.
     */
    /* FIXME ugly evil code */
    while (!audacious_remote_is_playing(proxy) && ((time(&lt) - timer) < 10))
    {
      usleep(0);
    }

    audacious_remote_jump_to_time(proxy, where_to_jump);

    timer = localtime(&lt)->tm_sec;

    /* Wait till really jumped before we continue. */
    while ((audacious_remote_get_output_time(proxy) / 1000) 
        != (where_to_jump / 1000) && ((time(&lt) - timer) < 10))
      usleep(0);
  }
  else if((slider_in_motion != NULL) && !xmms_playing)
    audacious_remote_play(proxy);

  slider_in_motion = NULL;
  got_motion = FALSE;
  return FALSE;
}

static gint
panel_button_press(GtkWidget *widget, GdkEventButton *ev, gpointer data) {
  switch (ev->button)
  {
    case 1:
      if (   xmms_running
          && ev->y >= time_krell->y0
          && ev->y < time_krell->y0 + time_krell->h_frame 
         )
        slider_in_motion = time_krell;
      else
        slider_in_motion = NULL;
      if ((ev->type == GDK_2BUTTON_PRESS) && !xmms_running)
        xmms_start_func();
      break;
    case 2:
      if (xmms_running && audacious_remote_is_playing(proxy))
      {
        if (krell_mmb_pause)
          audacious_remote_pause(proxy);
        else
          audacious_remote_stop(proxy);
      }
      else if (xmms_running)
        audacious_remote_play(proxy);
      else
        xmms_start_func();
      break;
    case 3:
      options_menu(ev);
      break;
  }
  return FALSE;
}

static void
scroll_bar_press(GtkWidget *widget, GdkEventButton *ev, gpointer data) {
  if (ev->button == 1) {
    scroll_in_motion = scroll_text;
    scroll_motion_x = ev->x;
  }
}

static void
scroll_bar_release(GtkWidget *widget, GdkEventButton *ev, gpointer data)
{
  scroll_in_motion = NULL;
}

static void
scroll_bar_motion(GtkWidget *widget, GdkEventButton *ev, gpointer data) {
  GkrellmMargin *margin;
  gint w, len, change;
  GdkModifierType state;
  gchar *scrolling_title_text;
  gchar *more_scrolled;

  if (scroll_in_motion != NULL) {
    /* Check if button is still pressed, in case missed button_release */
    state = ev->state;
    if (!(state & GDK_BUTTON1_MASK))
    {
      scroll_in_motion = NULL;
      return;
    }

    margin = gkrellm_get_style_margins(style);
    w = gkrellm_chart_width() - margin->left - margin->right - 2;

    change = scroll_motion_x - ev->x;
    scroll_motion_x = ev->x;

		scrolling_title_text = get_scrolling_title_text(&len, FALSE);

    if (len <= w) {
      scroll_in_motion = NULL;
      return;
    }

		x_scroll = (x_scroll + change) % (len + scroll_separator_len);
		if (x_scroll < 0)
			x_scroll = len + scroll_separator_len;

#if defined(GKRELLM_HAVE_DECAL_SCROLL_TEXT)
    more_scrolled = g_strdup_printf("%s%s",
						scrolling_title_text, scroll_separator);

		gkrellm_decal_scroll_text_set_text(scroll_panel,
						scroll_text, more_scrolled);
		gkrellm_decal_scroll_text_horizontal_loop(scroll_text, TRUE);
		gkrellm_decal_text_set_offset(scroll_text, -x_scroll, 0);
#else
    more_scrolled = g_strdup_printf("%s%s%s",
      scrolling_title_text, scroll_separator, scrolling_title_text);

    scroll_text->x_off = w - x_scroll - len;
    gkrellm_draw_decal_text(scroll_panel, scroll_text, more_scrolled, 
        (gulong) w - x_scroll - len);
#endif

    gkrellm_draw_panel_layers(scroll_panel);
    g_free(more_scrolled);

  }
}

static void
stack_in_out_images(GkrellmPiximage *out_image, GkrellmPiximage *in_image)
	{
	GdkPixbuf	*pixbuf, *src_pixbuf;
	gint		w, h;

	src_pixbuf = out_image->pixbuf;
	w = gdk_pixbuf_get_width(src_pixbuf);
	h = gdk_pixbuf_get_height(src_pixbuf);

	pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
				gdk_pixbuf_get_has_alpha(src_pixbuf), 8, w, 2 * h);
	gdk_pixbuf_copy_area(src_pixbuf, 0, 0, w, h,
				pixbuf, 0, 0);
	gdk_pixbuf_copy_area(in_image->pixbuf, 0, 0, w, h,
				pixbuf, 0, h);
	g_object_unref(out_image->pixbuf);
	out_image->pixbuf = pixbuf;
	gkrellm_destroy_piximage(in_image);
	}

static void
load_button(ControlButton *button, gchar *button_name,
			gchar *out, gchar **out_xpm,
			gchar *in, gchar **in_xpm)
	{
	GkrellmPiximage	*clicked_image = NULL;

	if (!gkrellm_load_piximage(button_name, NULL, &button->image, STYLE_NAME))
		{
		gkrellm_load_piximage(out, out_xpm, &button->image, STYLE_NAME);
		gkrellm_load_piximage(in, in_xpm, &clicked_image, STYLE_NAME);
		stack_in_out_images(button->image, clicked_image);
		}
	/* Default size may be gkrellmrc reassigned later.
	*/
	button->w = gdk_pixbuf_get_width(button->image->pixbuf);
	button->h = gdk_pixbuf_get_height(button->image->pixbuf) / 2;
	}

static void
load_button_images(void)
	{
	load_button(&prev_button, "prev_button", "prev", prev_xpm,
				"prev_clicked", prev_clicked_xpm);
	load_button(&play_button, "play_button", "play_pause", play_pause_xpm,
				"play_pause_clicked", play_pause_clicked_xpm);
	load_button(&stop_button, "stop_button", "stop", stop_xpm,
				"stop_clicked", stop_clicked_xpm);
	load_button(&next_button, "next_button", "next", next_xpm,
				"next_clicked", next_clicked_xpm);
	load_button(&eject_button, "eject_button", "eject", eject_xpm,
				"eject_clicked", eject_clicked_xpm);
	}

static gint
set_x_position(gint x, gchar *anchor)
	{
#if defined(GKRELLM_HAVE_THEME_SCALE)
	x = x * gkrellm_get_theme_scale();
#endif
	/* left anchor is the default */
	if (anchor[0] == 'c' || anchor[0] == 'C')
		x += gkrellm_chart_width() / 2;
	else if (anchor[0] == 'r' || anchor[0] == 'R')
		x = gkrellm_chart_width() - x - 1;
	return x;
	}

  /* Look for custom button placement and size from the gkrellmrc.  The
  |  button final size may later change yet again if user has set theme_scale.
  */
static void
button_position(ControlButton *button, gchar *name)
	{
	gchar	*s, anchor[8];
	gint	x, w, h;

	if ((s = gkrellm_get_gkrellmrc_string(name)) != NULL)
		{
		anchor[0] = 'l';
		if (sscanf(s, "%d %d %d %d %8s", &x, &button->y, &w, &h, anchor) >= 4)
			{
			button->x = set_x_position(x, anchor);
			if (w > 0)
				button->w = w;
			if (h > 0)
				button->h = h;
			}
#if defined(GKRELLM_HAVE_THEME_SCALE)
		button->y = button->y * gkrellm_get_theme_scale();
#endif
		gkrellmrc_button_placement = TRUE;
		}
	}

static void
decal_position(GkrellmDecal *decal, gchar *name, gboolean text)
	{
	gchar	*s, anchor[8];
	gint	x, y;

	anchor[0] = 'l';
	if (   (s = gkrellm_get_gkrellmrc_string(name)) != NULL
		&& sscanf(s, "%d %d %8s", &x, &y, anchor) >= 2
	   )
		{
		x = set_x_position(x, anchor);
#if defined(GKRELLM_HAVE_THEME_SCALE)
		y = y * gkrellm_get_theme_scale();
#endif

#if !GKRELLM_CHECK_VERSION(2,1,100)
		if (text)
			y -= decal->y_baseline;
#endif
		gkrellm_move_decal(control_panel, decal, x, y);
		}
	}

static void
layout_control_panel(GkrellmStyle *style, gint yoff)
	{
	prev_button.y = play_button.y = stop_button.y = yoff;
	next_button.y = eject_button.y = yoff;

	/* Next, look for custom theming button placements and sizes.
	*/
	gkrellmrc_button_placement = FALSE;
	button_position(&prev_button, "gkrellmms_prev_button_position");
	button_position(&play_button, "gkrellmms_play_button_position");
	button_position(&stop_button, "gkrellmms_stop_button_position");
	button_position(&next_button, "gkrellmms_next_button_position");
	button_position(&eject_button, "gkrellmms_eject_button_position");

	decal_position(xmms_decal, "gkrellmms_label_position", TRUE);
	decal_position(led_decal, "gkrellmms_led_position", FALSE);
	}

  /* If the mouse is in the button decal and the mouse position is not on a
  |  transparent pixel (look at the pixbuf), return TRUE.
  */
static gboolean
cb_in_button(GkrellmDecalbutton *b, GdkEventButton *ev, ControlButton *cbut)
	{
	GdkPixbuf		*pixbuf;
	GkrellmDecal	*d;
	guchar			*pixels, alpha;
	gint			rowstride, x, y;

	d = b->decal;
	if (gkrellm_in_decal(d, ev))
		{
		pixbuf = cbut->image->pixbuf;
		if (!gdk_pixbuf_get_has_alpha(pixbuf))
			return TRUE;
		x = ev->x - d->x;
		x = x / cbut->x_scale;
		y = ev->y - d->y;
		y = y / cbut->y_scale;
		pixels = gdk_pixbuf_get_pixels(pixbuf);
		rowstride = gdk_pixbuf_get_rowstride(pixbuf);
		pixels += y * rowstride + 4 * x;
		alpha = *(pixels + 3);
//		printf("in button x=%d y=%d alpha=%x\n", x, y, alpha);
		if (alpha > 0)
			return TRUE;
		}
	return FALSE;
	}

static void
move_buttons(void)
	{
	gint	x, w;

	if (!gkrellmrc_button_placement)
		{
		/* Place buttons according to GKrellMMS 0.5.5
		*/
		w = prev_button.w + play_button.w + stop_button.w
				+ next_button.w + eject_button.w;
		x = (gkrellm_chart_width() - w) / 2;
		if (x < 0)
			x = 0;
		prev_button.x = x;
		play_button.x = prev_button.x + prev_button.w;
		stop_button.x = play_button.x + play_button.w;
		next_button.x = stop_button.x + stop_button.w;
		eject_button.x = next_button.x + next_button.w;
		}
	gkrellm_move_decal(control_panel, prev_button.button->decal,
			prev_button.x, prev_button.y);
	gkrellm_move_decal(control_panel, play_button.button->decal,
			play_button.x, play_button.y);
	gkrellm_move_decal(control_panel, stop_button.button->decal,
			stop_button.x, stop_button.y);
	gkrellm_move_decal(control_panel, next_button.button->decal,
			next_button.x, next_button.y);
	gkrellm_move_decal(control_panel, eject_button.button->decal,
			eject_button.x, eject_button.y);
	}

static void
make_button(ControlButton *cbut, gint fn_id)
	{
	cbut->button = gkrellm_make_scaled_button(control_panel, cbut->image,
			cb_control_button, GINT_TO_POINTER(fn_id), FALSE, FALSE, 2, 0, 1,
			cbut->x, cbut->y, cbut->w, cbut->h);

	/* The button final size may be different from the requested size if the
	|  user has set a theme_scale other than 1.0.
	*/
	cbut->w = cbut->button->decal->w;
	cbut->h = cbut->button->decal->h;

	/* x_scale and y_scale are the ratio of the button final size to the
	|  original button image size so cb_in_button() can map mouse events from
	|  the button decal to the button image.
	*/
	cbut->x_scale = (double) cbut->w /
				(double) gdk_pixbuf_get_width(cbut->image->pixbuf);
	cbut->y_scale = (double) cbut->h /
				((double) gdk_pixbuf_get_height(cbut->image->pixbuf) / 2.0);
	gkrellm_set_in_button_callback(cbut->button, cb_in_button, cbut);
	}

static void
create_gkrellmms(GtkWidget *vbox, gint first_create) {
	GkrellmMargin		*m, mgn;
	GkrellmPiximage		*led_image = NULL;
#if !defined(GKRELLM_HAVE_THEME_SCALE)
	static GdkPixmap	*led_pixmap;
	static GdkBitmap	*led_mask;
#endif
	gint y = 0, y1, w;
	static GkrellmPiximage *bg_scroll_image;

	if (first_create) {
		xmms_running = audacious_remote_is_running(proxy);
		if (auto_main_close && xmms_running)
			audacious_remote_main_win_toggle(proxy, FALSE);
		if (xmms_autostart && !xmms_running)  xmms_start_func();
    		pl_init();
		control_panel = gkrellm_panel_new0();
	} else {
		update_playlist();
	}

	style = gkrellm_meter_style(DEFAULT_STYLE);
	if (scroll_style) g_free(scroll_style);
	scroll_style = gkrellm_copy_style(style);

	ts = gkrellm_meter_textstyle(GKRELLMMS_STYLE);
	ts_alt = gkrellm_meter_alt_textstyle(GKRELLMMS_STYLE);

	/* Scroll Bar */
	if (first_create) scroll_panel = gkrellm_panel_new0();

	if (bg_scroll_image) {		/* Avoid memory leaks at theme changes */
		gkrellm_destroy_piximage(bg_scroll_image);
		bg_scroll_image = NULL;
	}
  gkrellm_load_piximage("bg_scroll", NULL, &bg_scroll_image, STYLE_NAME);
  if (bg_scroll_image)
		gkrellm_set_gkrellmrc_piximage_border("gkrellmms_bg_scroll",
					bg_scroll_image, scroll_style);

	mgn = *gkrellm_get_style_margins(scroll_style);
	if (gkrellm_get_gkrellmrc_integer("gkrellmms_scroll_margin", &mgn.left))
		mgn.right = mgn.left;
	gkrellm_get_gkrellmrc_integer("gkrellmms_scroll_top_margin", &mgn.top);
	gkrellm_get_gkrellmrc_integer("gkrellmms_scroll_bottom_margin",
				&mgn.bottom);
	gkrellm_set_style_margins(scroll_style, &mgn);

  /* Text decal for scrolling title */
  scroll_text = gkrellm_create_decal_text(scroll_panel, "Apif0",
				ts_alt, scroll_style, -1, -1, -1);

	if (bg_scroll_image)
		gkrellm_panel_bg_piximage_override(scroll_panel, bg_scroll_image);
    gkrellm_panel_configure(scroll_panel, NULL, scroll_style);
    gkrellm_panel_create(vbox, monitor, scroll_panel);

    if (scrolling_tooltip == NULL) {
		scrolling_tooltip = gtk_tooltips_new();
		scrolling_tooltip_text = g_strdup("audacious");
		gtk_tooltips_set_tip(scrolling_tooltip, scroll_panel->drawing_area,
					scrolling_tooltip_text, NULL);
		gtk_tooltips_set_delay(scrolling_tooltip, 750);
		}


	/* Control panel */
	time_krell = gkrellm_create_krell(control_panel,
				gkrellm_krell_meter_piximage(DEFAULT_STYLE), style);
	gkrellm_monotonic_krell_values(time_krell, FALSE);
	gkrellm_set_krell_full_scale(time_krell, 100, 1);

	m = gkrellm_get_style_margins(style);
	w = gkrellm_gdk_string_width(ts->font, "-000:00xxx");
	xmms_decal = gkrellm_create_decal_text(control_panel, (gchar *) "A0", ts,
				style, -1, -1, w);
	xmms_decal->x += m->left;

	if (gkrellm_load_piximage("led_indicator", NULL, &led_image, STYLE_NAME))
		{
#if defined(GKRELLM_HAVE_THEME_SCALE)
		led_decal = gkrellm_make_scaled_decal_pixmap(control_panel, led_image,
				style, 4, 0, -1, 0, 0);

#else
		gkrellm_scale_piximage_to_pixmap(led_image, &led_pixmap, &led_mask,
				0, 0);
		led_decal = gkrellm_create_decal_pixmap(control_panel,
				led_pixmap, led_mask, 4, style, 0, -1);
#endif
		led_off_index = 0;
		led_running_index = 1;
		led_paused_index = 2;
		led_playing_index = 3;
	} else {
		led_decal = gkrellm_create_decal_pixmap(control_panel,
				gkrellm_decal_misc_pixmap(), gkrellm_decal_misc_mask(),
				N_MISC_DECALS, style, 0, -1);
		led_off_index = D_MISC_LED0;
		led_running_index = D_MISC_LED1;
		led_paused_index = D_MISC_LED1;
		led_playing_index = D_MISC_LED1;
	}
	led_decal->x = gkrellm_chart_width() - led_decal->w - m->right;

	gkrellm_draw_decal_text(control_panel, xmms_decal, gkrellmms_label, -1);
	gkrellm_draw_decal_pixmap(control_panel, led_decal, led_off_index);
	gkrellm_update_krell(control_panel, time_krell, (gulong) 0);

	if (enable_buttonbar) {
		/* Above decal and krell x,y settings are as the default original
		|  gkrellmss 0.5.5.  Now look for custom theming.
		*/
		load_button_images();
		y = xmms_decal->y + xmms_decal->h;
		y1 = time_krell->y0 + time_krell->h_frame;
		if (y1 > y)
			y = y1;
		layout_control_panel(style, y + 3);

		make_button(&prev_button,  gkrellmms_prev);
		make_button(&play_button,  gkrellmms_play);
		make_button(&stop_button,  gkrellmms_stop);
		make_button(&next_button,  gkrellmms_next);
		make_button(&eject_button, gkrellmms_eject);
		move_buttons();
  	}
	gkrellm_panel_configure(control_panel, NULL, style);
	gkrellm_panel_create(vbox, monitor, control_panel);

	/* Make led and label decals drawn on top of buttons.
	*/
	gkrellm_remove_decal(control_panel, xmms_decal);
	gkrellm_remove_decal(control_panel, led_decal);
	gkrellm_insert_decal(control_panel, led_decal, TRUE);
	gkrellm_insert_decal(control_panel, xmms_decal, TRUE);	/* On top */

	get_scrolling_title_text(NULL, TRUE);		/* Sync with theme changes */
	gkrellm_draw_panel_layers(control_panel);

	set_panel_status();

	if (first_create) {
		g_signal_connect(G_OBJECT(scroll_panel->drawing_area),
			"expose_event", G_CALLBACK(panel_expose_event), scroll_panel);
		g_signal_connect(G_OBJECT(scroll_panel->drawing_area),
			"button_press_event", G_CALLBACK(scroll_bar_press), NULL);
		g_signal_connect(G_OBJECT(scroll_panel->drawing_area),
			"button_release_event", G_CALLBACK( scroll_bar_release), NULL);
		g_signal_connect(G_OBJECT(scroll_panel->drawing_area),
			"motion_notify_event", G_CALLBACK(scroll_bar_motion), NULL);

		g_signal_connect(G_OBJECT(control_panel->drawing_area),
			"expose_event", G_CALLBACK( panel_expose_event), control_panel);
		g_signal_connect(G_OBJECT(control_panel->drawing_area),
			"button_press_event", G_CALLBACK( panel_button_press), NULL);
		g_signal_connect(G_OBJECT(control_panel->drawing_area),
			"button_release_event", G_CALLBACK(panel_button_release), NULL);
		g_signal_connect(G_OBJECT(control_panel->drawing_area),
			"motion_notify_event", G_CALLBACK(slider_motion), NULL);

    gtk_drag_dest_set(vbox,GTK_DEST_DEFAULT_ALL,
                      drop_types,NUM_DROP_TYPES,GDK_ACTION_COPY);
    g_signal_connect(G_OBJECT(vbox),"drag_data_received",
                     G_CALLBACK(drag_data_received),NULL);
                         
	}
}

void mainwin_back_func()
{
  if (auto_main_close && xmms_running)
    audacious_remote_main_win_toggle(proxy, TRUE);
}

static GkrellmMonitor  plugin_mon  =
{
  "GkrellMMS",             /* Name, for config tab.                   */
  0,                       /* Id, 0 if a plugin                       */
  create_gkrellmms,        /* The create_plugin() function            */
  update_gkrellmms,        /* The update_plugin() function            */

  create_gkrellmms_config, /* Create_plugin_tab() config func */
  apply_gkrellmms_config,  /* The apply_plugin_config() function      */

  save_gkrellmms_config,   /* The save_plugin_config() function */
  load_gkrellmms_config,   /* The load_plugin_config() function */
  CONFIG_KEYWORD,          /* Config Keyword                    */

  NULL,                    /* Undefined 2 */
  NULL,                    /* Undefined 1 */
  NULL,                    /* Undefined 0 */

  GKRELLMMS_PLACE,         /* Insert plugin before this monitor     */
  NULL,                    /* Handle if a plugin, filled by GkrellM */
  NULL                     /* Path if a plugin, filled by GkrellM   */
};

GkrellmMonitor *
gkrellmms_get_monitor(void) {
	return monitor;
}

GkrellmMonitor *
gkrellm_init_plugin(void) {
  gchar *tmp;
  GError *error = NULL;
  static DBusGConnection *connection = NULL;

#ifdef ENABLE_NLS
   bind_textdomain_codeset(PACKAGE, "UTF-8");
#endif /* ENABLE_NLS */

	/* Popup-menu set */
	running_factory = options_menu_factory(1);
	not_running_factory = options_menu_factory(0);



  connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);

  proxy = dbus_g_proxy_new_for_name(connection, AUDACIOUS_DBUS_SERVICE,
                                    AUDACIOUS_DBUS_PATH,
                                    AUDACIOUS_DBUS_INTERFACE);

  /* Default settings */
  control_panel = NULL;
  xmms_running = FALSE;
  x_scroll = 0;
  total_plist_time = 0;
  scrolling_tooltip = NULL;

  tmp = g_strdup_printf("%s/%s", gkrellm_homedir(), GKRELLM_DATA_DIR);

  playlist_file = g_strdup_printf("%s/gkrellmms_playlist", tmp);
  position_file = g_strdup_printf("%s/gkrellmms_position", tmp);
  time_file = g_strdup_printf("%s/gkrellmms_time", tmp);

  g_free(tmp);

  playlist_dir = g_strdup(gkrellm_homedir());
  files_directory = g_strdup("/");
  gkrellmms_label = g_strdup("audacious");
  scroll_enable = TRUE;
  scroll_separator = g_strdup(SCROLL_SEPARATOR);
  draw_time = 1;
  xmms_exec_command = g_strdup("audacious");
  xmms_autostart = 0;
  auto_main_close = 0;
  auto_hide_all = 0;
  eject_opens_playlist = 1;
  krell_mmb_pause = 1;
  time_format = 1;
  auto_play_start = 0;
  auto_seek = 0;
  always_load_info = 0;
  draw_minus = 1;
  enable_buttonbar = TRUE;

  g_atexit(mainwin_back_func);

    style_id = gkrellm_add_meter_style(&plugin_mon, STYLE_NAME);
	pGK = gkrellm_ticks();

  /* Done */
	monitor = &plugin_mon;
  return &plugin_mon;
}
