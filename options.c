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

static GtkWidget *exec_entry,
                 *files_entry,
                 *playlist_dir_entry,
                 *label_entry,
                 *separator_entry,
                 *session_entry,
                 *xmms_start_entry,
                 *scroll_enable_entry,
                 *main_close_entry,
                 *hide_all_entry,
                 *auto_play_entry,
                 *draw_minus_entry,
/*
                 *scroll_command_entry,
*/
                 *always_load_entry;

static gint eject_thing;
static gint time_thing;
static gint pause_thing;
static gint time_fmt_thing;
static gint always_load_thing;

void toggles_func (GtkWidget *w, gpointer what)
{
  gint type;
  type = GPOINTER_TO_INT(what);

  switch (type)
  {
    case gkrellmms_mainwin:
      xmms_remote_main_win_toggle(xmms_session,
        !xmms_remote_is_main_win(xmms_session));
      break;
    case gkrellmms_playlist:
      xmms_remote_pl_win_toggle(xmms_session,
        !xmms_remote_is_pl_win(xmms_session));
      break;
    case gkrellmms_eq:
      xmms_remote_eq_win_toggle(xmms_session,
        !xmms_remote_is_eq_win(xmms_session));
      break;
    case gkrellmms_repeat:
      xmms_remote_toggle_repeat(xmms_session);
      break;
    case gkrellmms_shuffle:
      xmms_remote_toggle_shuffle(xmms_session);
      break;
    case gkrellmms_eject:
      xmms_remote_eject(xmms_session);
      break;
    case gkrellmms_prefs:
      xmms_remote_show_prefs_box(xmms_session);
      break;
    default:
      do_xmms_command(type);
      break;
  }
}

void aot_func(GtkWidget *w, gpointer data)
{
  xmms_remote_toggle_aot(xmms_session, GPOINTER_TO_INT(data));
}

void save_time(gint);
void save_playlist(gchar *, gint);

void xmms_start_func ()
{
  gint timer;
  time_t lt;
	gchar	**argv = 0;
	GError *err = NULL;
	gboolean res;

	if (!g_shell_parse_argv(xmms_exec_command, NULL, &argv, &err)) {
    gkrellm_message_window(_("GKrellMMS Error"),
				err->message, NULL);
		g_error_free(err);
		return;
		}
	res = g_spawn_async(files_directory, argv, NULL,
					G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &err);
  if (!res && err) {
    gkrellm_message_window(_("GKrellMMS Error"),
				err->message, NULL);
		g_error_free(err);
		}

	/*
	 | wait until xmms has started, but no longer than 10 secs.
	 | This is so that there are actually windows to hide when
	 | we get to down to the auto_hide_all part.
	 */
  /* FIXME supposed ugly evil code */
  timer = time(&lt);
  while (!xmms_remote_is_running(xmms_session) && ((time(&lt) - timer) < 10))
				usleep(0);

	xmms_running = xmms_remote_is_running(xmms_session);

	if (auto_hide_all && xmms_running) {
    xmms_remote_main_win_toggle(xmms_session, FALSE);
    xmms_remote_pl_win_toggle(xmms_session, FALSE);
    xmms_remote_eq_win_toggle(xmms_session, FALSE);
	}
}

void start_func (GtkWidget *w, gpointer data)
{
  xmms_start_func();
}

void quit_func (GtkWidget *w, gpointer data)
{
  time_t timer;
  time_t lt;

  time(&lt);
  timer = lt;

  xmms_remote_quit(xmms_session);
  while (xmms_remote_is_running(xmms_session)
         && ((time(&lt) - timer) < 10)) {
    /* Do nothing; wait until xmms really quits, but not longer than 10sec! */
    usleep(0);
  }
  update_playlist();
}

static void open_playlist_cb(GtkWidget *widget, gpointer data) {
  pl_show_playlist();
}

static void load_playlist_cb(GtkWidget *widget, gpointer data) {
  pl_open_playlist();
}

static void open_options_cb(GtkWidget *widget, gpointer data) {
	gkrellm_open_config_window(gkrellmms_get_monitor());
}


static GtkItemFactoryEntry gkrellmms_factory[] =
{
  {"/-",                            NULL, NULL,          0,        "<Separator>"},
  {N_("/Toggles..."),                   NULL, NULL,          0,        "<Branch>"},
  {N_("/Toggles.../Main Window"),       NULL, toggles_func,  gkrellmms_mainwin,  "<Item>"},
  {N_("/Toggles.../Playlist"),          NULL, toggles_func,  gkrellmms_playlist, "<Item>"},
  {N_("/Toggles.../EQ"),                NULL, toggles_func,  gkrellmms_eq,       "<Item>"},
  {N_("/Toggles.../Repeat"),            NULL, toggles_func,  gkrellmms_repeat,   "<Item>"},
  {N_("/Toggles.../Shuffle"),           NULL, toggles_func,  gkrellmms_shuffle,  "<Item>"},
  {N_("/Toggles.../-"),                 NULL, NULL,          0,        "<Separator>"},
  {N_("/Toggles.../Always on top on"),  NULL, aot_func,      ON,       "<Item>"},
  {N_("/Toggles.../Always on top off"), NULL, aot_func,      OFF,      "<Item>"},
  {"/-",                                NULL, NULL,          0,        "<Separator>"},
  {"/Xmms...",                          NULL, NULL,          0,        "<Branch>"},
  {N_("/Xmms.../Previous"),             NULL, toggles_func,  gkrellmms_prev,     "<Item>"},
  {N_("/Xmms.../Play"),                 NULL, toggles_func,  gkrellmms_play,     "<Item>"},
  {N_("/Xmms.../Pause"),                NULL, toggles_func,  gkrellmms_paus,     "<Item>"},
  {N_("/Xmms.../Stop"),                 NULL, toggles_func,  gkrellmms_stop,     "<Item>"},
  {N_("/Xmms.../Next"),                 NULL, toggles_func,  gkrellmms_next,     "<Item>"},
  {"/-",                                NULL, NULL,          0,        "<Separator>"},
  {N_("/Playlist Editor"),              NULL, open_playlist_cb, 0,        "<Item>"},
  {N_("/GKrellMMS Options"),            NULL, open_options_cb, 0,        "<Item>"},
  {"/-",                                NULL, NULL,          0,        "<Separator>"},
  {N_("/Open file(s)"),                 NULL, toggles_func,  gkrellmms_eject,    "<Item>"},
  {N_("/Open Playlist"),                NULL, load_playlist_cb,  0,        "<Item>"},
  {N_("/XMMS Prefs"),                   NULL, toggles_func,  gkrellmms_prefs,    "<Item>"},
  {"/-",                                NULL, NULL,          0,        "<Separator>"},
  {N_("/Quit XMMS"),                    NULL, quit_func,     0,        "<Item>"},
  {"/-",                                NULL, NULL,          0,        "<Separator>"},
};

static GtkItemFactoryEntry gkrellmms_factory_norun[] =
{
  {"/-",                           NULL, NULL,          0,    "<Separator>"},
  {N_("/Launch XMMS"),                 NULL, start_func,    0,    "<Item>"},
  {"/-",                           NULL, NULL,          0,    "<Separator>"},
};

GtkItemFactory *options_menu_factory(gint run_menu)
{
  gint n,i;
  GtkItemFactory *music_factory;
  GtkAccelGroup  *axel;
  axel = gtk_accel_group_new();

  gtk_window_add_accel_group(GTK_WINDOW(gkrellm_get_top_window()), axel);

  music_factory = gtk_item_factory_new(GTK_TYPE_MENU, "<Main>", axel);

  if (run_menu)
  {
    n = sizeof(gkrellmms_factory) / sizeof(GtkItemFactoryEntry);
    for(i = 0; i < n; i++)
        gkrellmms_factory[i].path = _(gkrellmms_factory[i].path);
    gtk_item_factory_create_items(music_factory, n, gkrellmms_factory, NULL);
  } else {
    n = sizeof(gkrellmms_factory_norun) / sizeof(GtkItemFactoryEntry);
    for(i = 0; i < n; i++)
        gkrellmms_factory_norun[i].path = _(gkrellmms_factory_norun[i].path);
    gtk_item_factory_create_items(music_factory, n, gkrellmms_factory_norun, NULL);
  }

  return music_factory;
}

void options_menu(GdkEventButton *ev)
{
  gtk_menu_popup(GTK_MENU(xmms_running ?
                 running_factory->widget : not_running_factory->widget),
                 NULL, NULL, NULL, NULL,
                 ev->button, ev->time);
}
  
/* Configs */

void apply_gkrellmms_config()
{
  gint prev_scroll_enable;

  /* Entry's */
  g_free(xmms_exec_command);
  xmms_exec_command = g_strdup(gtk_entry_get_text(GTK_ENTRY(exec_entry)));

  g_free(files_directory);
  files_directory = g_strdup(gtk_entry_get_text(GTK_ENTRY(files_entry)));

  g_free(playlist_dir);
  playlist_dir = g_strdup(gtk_entry_get_text(GTK_ENTRY(playlist_dir_entry)));

  g_free(gkrellmms_label);
  gkrellmms_label = g_strdup(gtk_entry_get_text(GTK_ENTRY(label_entry)));

  g_free(scroll_separator);
  scroll_separator = g_strdup(gtk_entry_get_text(GTK_ENTRY(separator_entry)));
	gkrellmms_set_scroll_separator_len();

  xmms_session = gtk_spin_button_get_value_as_int(
                                                GTK_SPIN_BUTTON(session_entry));

  /* Toggles */
  prev_scroll_enable = scroll_enable;
  scroll_enable = GTK_TOGGLE_BUTTON(scroll_enable_entry)->active;
  xmms_autostart = GTK_TOGGLE_BUTTON(xmms_start_entry)->active;
  auto_main_close = GTK_TOGGLE_BUTTON(main_close_entry)->active;
  auto_hide_all = GTK_TOGGLE_BUTTON(hide_all_entry)->active;
  auto_play_start = GTK_TOGGLE_BUTTON(auto_play_entry)->active;
  draw_minus = GTK_TOGGLE_BUTTON(draw_minus_entry)->active;

  /* Switches */
  eject_opens_playlist = eject_thing;
  krell_mmb_pause = pause_thing;
  draw_time = time_thing;
  time_format = time_fmt_thing;
  always_load_info = always_load_thing;
}

void save_gkrellmms_config(FILE *f)
{
  fprintf(f, "%s scroll_enable %d\n", CONFIG_KEYWORD, scroll_enable);
  fprintf(f, "%s xmms_session %d\n", CONFIG_KEYWORD, xmms_session);
  fprintf(f, "%s draw_time %d\n", CONFIG_KEYWORD, draw_time);
  fprintf(f, "%s xmms_autostart %d\n", CONFIG_KEYWORD, xmms_autostart);
  fprintf(f, "%s auto_main_close %d\n", CONFIG_KEYWORD, auto_main_close);
  fprintf(f, "%s auto_hide_all %d\n", CONFIG_KEYWORD, auto_hide_all);
  fprintf(f, "%s xmms_exec_command %s\n", CONFIG_KEYWORD, xmms_exec_command);
  fprintf(f, "%s files_directory %s\n", CONFIG_KEYWORD, files_directory);
  fprintf(f, "%s playlist_dir %s\n", CONFIG_KEYWORD, playlist_dir);
  fprintf(f, "%s gkrellmms_label %s\n", CONFIG_KEYWORD, gkrellmms_label);
  fprintf(f, "%s scroll_separator \"%s\"\n", CONFIG_KEYWORD, scroll_separator);
  fprintf(f, "%s eject_opens_playlist %d\n", CONFIG_KEYWORD, eject_opens_playlist);
  fprintf(f, "%s krell_mmb_pause %d\n", CONFIG_KEYWORD, krell_mmb_pause);
  fprintf(f, "%s time_format %d\n", CONFIG_KEYWORD, time_format);
  fprintf(f, "%s auto_play_start %d\n", CONFIG_KEYWORD, auto_play_start);
  fprintf(f, "%s always_load_info %d\n", CONFIG_KEYWORD, always_load_info);
  fprintf(f, "%s draw_minus %d\n", CONFIG_KEYWORD, draw_minus);
}

void load_gkrellmms_config(gchar *arg)
{
  gchar config[64], item[256], command[64];
  gint n;
  n = sscanf(arg, "%s %[^\n]", config, item);
  if (n == 2)
  {
    if (strcmp(config, "scroll_enable") == 0)
      sscanf(item, "%d\n", &scroll_enable);
    else if (strcmp(config, "xmms_session") == 0)
      sscanf(item, "%d\n", &xmms_session);
    else if (strcmp(config, "xmms_autostart") == 0)
      sscanf(item, "%d\n", &xmms_autostart);
    else if (strcmp(config, "auto_main_close") == 0)
      sscanf(item, "%d\n", &auto_main_close);
    else if (strcmp(config, "auto_hide_all") == 0)
      sscanf(item, "%d\n", &auto_hide_all);
    else if (strcmp(config, "eject_opens_playlist") == 0)
      sscanf(item, "%d\n", &eject_opens_playlist);
    else if (strcmp(config, "draw_time") == 0)
      sscanf(item, "%d\n", &draw_time);
    else if (strcmp(config, "krell_mmb_pause") == 0)
      sscanf(item, "%d\n", &krell_mmb_pause);
    else if (strcmp(config, "time_format") == 0)
      sscanf(item, "%d\n", &time_format);
    else if (strcmp(config, "auto_play_start") == 0)
      sscanf(item, "%d\n", &auto_play_start);
    else if (strcmp(config, "always_load_info") == 0)
      sscanf(item, "%d\n", &always_load_info);
    else if (strcmp(config, "draw_minus") == 0)
      sscanf(item, "%d\n", &draw_minus);
    else if (strcmp(config, "gkrellmms_label") == 0)
    {
      sscanf(item, "%s\n", command);
      g_free(gkrellmms_label);
      gkrellmms_label = g_strdup(command);
    }
    else if (strcmp(config, "scroll_separator") == 0)
    {
         glong sprt_str, sprt_len; 
         gchar *scroll_temp = item;

         sprt_str = 0;
         while (item[sprt_str] != '"') sprt_str++; sprt_str++;

         sprt_len = sprt_str;
         while (item[sprt_len] != '"') sprt_len++;
         sprt_len = sprt_len - sprt_str;
         
         scroll_separator = malloc(sizeof(char) * (sprt_len + 1));
         memset(scroll_separator, '\0', sprt_len + 1);
         scroll_temp += sprt_str;
         memcpy(scroll_separator, scroll_temp, sprt_len);
    }
    else if (strcmp(config, "xmms_exec_command") == 0)
    {
      xmms_exec_command = g_strdup(item);
    }
    else if (strcmp(config, "playlist_dir") == 0)
    {
      playlist_dir = g_strdup(item);
    }
    else if (strcmp(config, "files_directory") == 0)
    {
      files_directory = g_strdup(item);
    }
  }

  if(!gkrellm_get_gkrellmrc_integer("gkrellmms_show_buttons",&enable_buttonbar))
    enable_buttonbar = TRUE;
}

void eject_type_set(GtkWidget *w, gpointer data)
{
  eject_thing = GPOINTER_TO_INT(data);
}

void time_type_set(GtkWidget *w, gpointer data)
{
  time_thing = GPOINTER_TO_INT(data);
}

static void pause_type_set(GtkWidget *w, gpointer data)
{
  pause_thing = GPOINTER_TO_INT(data);
}

void time_fmt_type_set(GtkWidget *w, gpointer data)
{
  time_fmt_thing = GPOINTER_TO_INT(data);
}

void load_type_set(GtkWidget *w, gpointer data)
{
  always_load_thing = GPOINTER_TO_INT(data);
}

void create_gkrellmms_config(GtkWidget *tab)
{
  GtkWidget *laptop,
            *frame,
            *vbox,
            *hbox,
            *zbox,
            *label,
            *info_label,
            *text,
            *eject_entry,
            *time_draw_entry,
            *pause_entry,
            *time_fmt_entry;
  GtkAdjustment *adjust;
  GSList *eject_group = NULL,
         *time_draw_group = NULL,
         *pause_group = NULL,
         *time_fmt_group = NULL,
         *always_load_group = NULL;
  gchar *gkrellmms_info_text = NULL;
  gint i;
  static gchar *gkrellmms_help_text[] =
  {
    N_("GKrellMMS is a GKrellM XMMS-plugin which allows you to control \n" \
    "XMMS from within GKrellM. It features some cool things, such as: \n" \
    "\n" \
    "- A scrolling title. \n" \
    "- A Krell which indicates where you are in a song. \n" \
    "- Themeable buttons for controlling XMMS. \n" \
    "- A playlist editor. \n" \
    "- A gtk-popup-menu with misc. XMMS-functions. \n" \
    "\n"),

    N_("<b>How to use GKrellMMS: \n"),
    N_("\n" \
    "You can do some cool stuff with the XMMS-Krell, by using your mouse. \n" \
    "\n"),

    N_("<b>Mouse actions: \n" \
    "\n\tLeft mouse-button: "),
    N_("Jump through song. \n"),

    N_("<b>\tMiddle mouse-button: "),
    N_("Pause/stop/play XMMS (configurable), \n" \
    "\t  or launch XMMS if it's not running. \n"),

    N_("<b>\tRight mouse-button: "),
    N_("Popup-menu. \n" \
    "\n" \
    "The led indicator on the Krell indicates several things: \n" \
    "\n"),

    N_("<b>\tConstant red: "),
    N_("XMMS is turned off. \n"),

    N_("<b>\tConstant green: "),
    N_("XMMS is playing. \n"),

    N_("<b>\tRed, blinking green: "),
    N_("XMMS is stopped. \n"),

    N_("<b>\tGreen, blinking red: "),
    N_("XMMS is paused. \n" \
    "\n"),

    N_("<b>Configurabilities:\n"),
    N_("\n" \
    "GKrellM has some configurabilities for if you like to \n" \
    "configure some stuff:\n" \
    "\n"),

    N_("<b>Configs tab: \n" \
    "\n" \
    "\tXMMS Executable: \n"),

    N_("\tHow the XMMS-executable (+ eventually path) \n" \
    "\tis called on your computer. Default is xmms\n" \
    "\n"),

    N_("<b>\tFiles Directory: \n"),
    N_("\tThe directory where your mp3's/xm's/whatever \n" \
    "\tare stored in. When starting XMMS from GKrellM, it will go to this \n" \
    "\tdirectory when ejecting. \n" \
    "\n"),
  
    N_("<b>\tPlaylist Directory: \n"),
    N_("\tThe directory where your m3u-files are stored in. \n" \
    "\n"),

    N_("<b>\tKrell label: \n"),
    N_("\tThe text-label you want in the krell when xmms isn't running/playing. \n" \
    "\n"),
  
    N_("<b>\tScroll separator: \n"),
    N_("\tThe little text that will be appended at the end of the scrolled text. \n" \
    "\tIt defaults to '   ***   ' (that's 3 spaces, 3 *'s and 3 spaces). \n" \
    "\n"),

    N_("<b>\tXMMS Session to use: \n"),
    N_("\tThe XMMS-session you want to use with GKrellMMS. \n" \
    "\tUse 0 if you only have 1 XMMS running. \n" \
    "\n"),
  
    N_("<b>Toggles tab: \n" \
    "\n" \
    "\tDraw minus (-) with remaining time: \n"),
  
    N_("\tDraw a minus (-) before the remaining time, when you have \n" \
    "\tthe output-time displaying remaining time. \n\n"),

    N_("<b>\tXMMS Auto Launch: \n"),
    N_("\tAuto launch XMMS when starting GKrellMMS. \n" \
    "\n"),
  
    N_("<b>\tAuto Mainwindow Close: \n"),
    N_("\tAutomatically close the XMMS-mainwindow \n" \
    "\twhen GKrellMMS starts, and XMMS is already running, or when \n" \
    "\tlaunching XMMS while GKrellMMS runs. This option also enables the \n" \
    "\tmainwindow back when you quit gkrellm (some people really do). \n" \
    "\n"),

    N_("<b>\tAuto hide all XMMS windows: \n"),
    N_("\tAutomatically hide all XMMS windows when GKrellMMS starts. \n" \
     "\n"),

    N_("<b>\tAuto start playing: \n"),
    N_("\tAutomatically start playing when launching XMMS. \n\n"),

    N_("<b>\tEnable scrolling title: \n"),
    N_("\tEnable/disable the scrolling title-panel. \n" \
    "\n"),
  
    N_("<b>Switches tab: \n" \
    "\n" \
    "\tEject opens: \n"),

    N_("\tCheck whether the eject-button on the button-bar opens a \n" \
    "\tplaylist or an other XMMS-file. \n" \
    "\n"),
  
    N_("<b>\tMMB on krell click: \n"),
    N_("\tCheck whether GKrellMMS should pause/continue or \n" \
    "\tstop/play the current song on a MMB-click on the krell. MMB Click will \n" \
    "\talways start playing the song if XMMS isn't playing. \n" \
    "\n"),

    N_("<b>\tLoad file-info: \n"),
    N_("\tCheck if GKrellMMS should load all the playlist-info at \n" \
    "\tplaylist-load (Always), or at file-load (On File play only). \n" \
    "\tThis last option is very useful if you don't use the local \n" \
    "\tplaylist editor, or are playing on a slow network/cdrom. \n\n"),

    N_("<b>\tDraw in time bar: \n"),
    N_("\tCheck whether to draw the output time or 'xmms' in \n" \
    "\tthe time-krell panel. \n" \
    "\n"),

    N_("<b>\tOutput time format: \n"),
    N_("\tDecide whether to draw the elapsed time, or the remaining time \n" \
    "\tas output-time in the Krell. \n")
  };

  laptop = gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(laptop), GTK_POS_TOP);
  gtk_box_pack_start(GTK_BOX(tab), laptop, TRUE, TRUE, 0);

  /* Configs-tab */

  frame = gtk_frame_new(NULL);
  gtk_container_border_width(GTK_CONTAINER(frame), 3);
  
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(vbox), 3);
  
  hbox = gtk_hbox_new(FALSE, 5);

  zbox = gtk_vbox_new(FALSE, 0);
  label = gtk_label_new(_("XMMS Executable:"));
  gtk_box_pack_start(GTK_BOX(zbox), label, TRUE, FALSE, 0);
  label = gtk_label_new(_("Files Directory:"));
  gtk_box_pack_start(GTK_BOX(zbox), label, TRUE, FALSE, 0);
  label = gtk_label_new(_("Playlist Directory:"));
  gtk_box_pack_start(GTK_BOX(zbox), label, TRUE, FALSE, 0);
  label = gtk_label_new(_("Krell label:"));
  gtk_box_pack_start(GTK_BOX(zbox), label, TRUE, FALSE, 0);
  label = gtk_label_new(_("Scroll separator:"));
  gtk_box_pack_start(GTK_BOX(zbox), label, TRUE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), zbox, FALSE, FALSE, 0);
  
  zbox = gtk_vbox_new(FALSE, 0);
  exec_entry = gtk_entry_new_with_max_length(255);
  gtk_entry_set_text(GTK_ENTRY(exec_entry), xmms_exec_command);
  gtk_entry_set_editable(GTK_ENTRY(exec_entry), TRUE);
  gtk_box_pack_start(GTK_BOX(zbox), exec_entry, TRUE, FALSE, 0);

  files_entry = gtk_entry_new_with_max_length(255);
  gtk_entry_set_text(GTK_ENTRY(files_entry), files_directory);
  gtk_entry_set_editable(GTK_ENTRY(files_entry), TRUE);
  gtk_box_pack_start(GTK_BOX(zbox), files_entry, TRUE, FALSE, 0);

  playlist_dir_entry = gtk_entry_new_with_max_length(255);
  gtk_entry_set_text(GTK_ENTRY(playlist_dir_entry), playlist_dir);
  gtk_entry_set_editable(GTK_ENTRY(playlist_dir_entry), TRUE);
  gtk_box_pack_start(GTK_BOX(zbox), playlist_dir_entry, TRUE, FALSE, 0);

  label_entry = gtk_entry_new_with_max_length(16);
  gtk_entry_set_text(GTK_ENTRY(label_entry), gkrellmms_label);
  gtk_entry_set_editable(GTK_ENTRY(label_entry), TRUE);
  gtk_box_pack_start(GTK_BOX(zbox), label_entry, TRUE, FALSE, 0);

  separator_entry = gtk_entry_new_with_max_length(MAXLEN_SCROLL_SEPARATOR);
  gtk_entry_set_text(GTK_ENTRY(separator_entry), scroll_separator);
  gtk_entry_set_editable(GTK_ENTRY(separator_entry), TRUE);
  gtk_box_pack_start(GTK_BOX(zbox), separator_entry, TRUE, FALSE, 0);
  
  gtk_box_pack_start(GTK_BOX(hbox), zbox, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);

  hbox = gtk_hbox_new(FALSE, 5);
  adjust = (GtkAdjustment *) gtk_adjustment_new((gfloat) xmms_session, 0.0,
                             100.0, 1.0, 5.0, 0.0);
  session_entry = gtk_spin_button_new(adjust, 1.0, 1);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(session_entry), (guint) 0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(session_entry), xmms_session);
  gtk_box_pack_start(GTK_BOX(hbox), session_entry, FALSE, FALSE, 0);
  label = gtk_label_new(_("XMMS Session to use"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);

  label = gtk_label_new(_("Configs"));
  gtk_container_add(GTK_CONTAINER(frame), vbox);
  gtk_notebook_append_page(GTK_NOTEBOOK(laptop), frame, label);

  /* Toggles */
  frame = gtk_frame_new(NULL);
  gtk_container_border_width(GTK_CONTAINER(frame), 3);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(vbox), 3);

  draw_minus_entry = gtk_check_button_new_with_label(_("Draw minus (-) when displaying remaining time"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(draw_minus_entry), draw_minus);  
  gtk_container_add(GTK_CONTAINER(vbox), draw_minus_entry);

  xmms_start_entry = gtk_check_button_new_with_label(_("Auto launch XMMS on GKrellMMS startup"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(xmms_start_entry), xmms_autostart);
  gtk_container_add(GTK_CONTAINER(vbox), xmms_start_entry);

  main_close_entry = gtk_check_button_new_with_label(_("Auto close (and open) XMMS Mainwin"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(main_close_entry), auto_main_close);
  gtk_container_add(GTK_CONTAINER(vbox), main_close_entry);

  hide_all_entry = gtk_check_button_new_with_label(_("Auto hide all XMMS windows on XMMS startup"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hide_all_entry), auto_hide_all);
  gtk_container_add(GTK_CONTAINER(vbox), hide_all_entry);

  auto_play_entry = gtk_check_button_new_with_label(_("Auto start playing on XMMS launch"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(auto_play_entry), auto_play_start);
  gtk_container_add(GTK_CONTAINER(vbox), auto_play_entry);

  scroll_enable_entry = gtk_check_button_new_with_label(_("Enable scrolling title panel"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scroll_enable_entry), scroll_enable);
  gtk_container_add(GTK_CONTAINER(vbox), scroll_enable_entry);


  label = gtk_label_new(_("Toggles"));
  gtk_container_add(GTK_CONTAINER(frame), vbox);
  gtk_notebook_append_page(GTK_NOTEBOOK(laptop), frame, label);


  /* Switches */
  frame = gtk_frame_new(NULL);
  gtk_container_border_width(GTK_CONTAINER(frame), 3);
  
  hbox = gtk_hbox_new(FALSE, 5);
  gtk_container_border_width(GTK_CONTAINER(hbox), 0);

  vbox = gtk_vbox_new(FALSE, 0);
  zbox = gtk_vbox_new(FALSE, 0);

  gtk_container_border_width(GTK_CONTAINER(zbox), 10);

  label = gtk_label_new(_("Eject-button opens:"));
  gtk_box_pack_start(GTK_BOX(zbox), label, FALSE, FALSE, 0);

  eject_thing = eject_opens_playlist;
  
  eject_entry = gtk_radio_button_new_with_label(NULL, _("Playlist"));
  eject_group = gtk_radio_button_group(GTK_RADIO_BUTTON(eject_entry));
  gtk_box_pack_start(GTK_BOX(zbox), eject_entry, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(eject_entry), eject_opens_playlist);
  gtk_signal_connect(GTK_OBJECT(eject_entry), "pressed",
                     (GtkSignalFunc) eject_type_set, GINT_TO_POINTER(1));
  
  eject_entry = gtk_radio_button_new_with_label(eject_group, _("File(s)"));
  gtk_box_pack_start(GTK_BOX(zbox), eject_entry, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(eject_entry), !eject_opens_playlist);
  gtk_signal_connect(GTK_OBJECT(eject_entry), "pressed",
                     (GtkSignalFunc) eject_type_set, GINT_TO_POINTER(0));

  gtk_box_pack_start(GTK_BOX(vbox), zbox, FALSE, FALSE, 0);

  zbox = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), zbox, FALSE, FALSE, 0);

  zbox = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(zbox), 10);

  label = gtk_label_new(_("Draw in time bar:"));
  gtk_box_pack_start(GTK_BOX(zbox), label, FALSE, FALSE, 0);

  time_thing = draw_time;
  
  time_draw_entry = gtk_radio_button_new_with_label(NULL, _("Output time"));
  time_draw_group = gtk_radio_button_group(GTK_RADIO_BUTTON(time_draw_entry));
  gtk_box_pack_start(GTK_BOX(zbox), time_draw_entry, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(time_draw_entry), draw_time);
  gtk_signal_connect(GTK_OBJECT(time_draw_entry), "pressed",
                     (GtkSignalFunc) time_type_set, GINT_TO_POINTER(1));

  time_draw_entry = gtk_radio_button_new_with_label(time_draw_group, _("XMMS-text"));
  gtk_box_pack_start(GTK_BOX(zbox), time_draw_entry, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(time_draw_entry), !draw_time);
  gtk_signal_connect(GTK_OBJECT(time_draw_entry), "pressed",
                     (GtkSignalFunc) time_type_set, GINT_TO_POINTER(0));

  gtk_box_pack_start(GTK_BOX(vbox), zbox, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

  zbox = gtk_vseparator_new();
  gtk_box_pack_start(GTK_BOX(hbox), zbox, FALSE, FALSE, 0);

  vbox = gtk_vbox_new(FALSE, 0);

  zbox = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(zbox), 10);

  label = gtk_label_new(_("Middle mouse click on krell will:"));
  gtk_box_pack_start(GTK_BOX(zbox), label, FALSE, FALSE, 0);

  pause_thing = krell_mmb_pause;
  
  pause_entry = gtk_radio_button_new_with_label(NULL, _("Pause/Continue song"));
  pause_group = gtk_radio_button_group(GTK_RADIO_BUTTON(pause_entry));
  gtk_box_pack_start(GTK_BOX(zbox), pause_entry, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pause_entry), krell_mmb_pause);
  gtk_signal_connect(GTK_OBJECT(pause_entry), "pressed",
                     (GtkSignalFunc) pause_type_set, GINT_TO_POINTER(1));

  pause_entry = gtk_radio_button_new_with_label(pause_group, _("Stop/Play song"));
  gtk_box_pack_start(GTK_BOX(zbox), pause_entry, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pause_entry), !krell_mmb_pause);
  gtk_signal_connect(GTK_OBJECT(pause_entry), "pressed",
                     (GtkSignalFunc) pause_type_set, GINT_TO_POINTER(0));

  gtk_box_pack_start(GTK_BOX(vbox), zbox, FALSE, FALSE, 0);

  zbox = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), zbox, FALSE, FALSE, 0);

  zbox = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(zbox), 10);

  label = gtk_label_new(_("Output time format:"));
  gtk_box_pack_start(GTK_BOX(zbox), label, FALSE, FALSE, 0);

  time_fmt_thing = time_format;
  
  time_fmt_entry = gtk_radio_button_new_with_label(NULL, _("Elapsed time"));
  time_fmt_group = gtk_radio_button_group(GTK_RADIO_BUTTON(time_fmt_entry));
  gtk_box_pack_start(GTK_BOX(zbox), time_fmt_entry, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(time_fmt_entry), time_format);
  gtk_signal_connect(GTK_OBJECT(time_fmt_entry), "pressed",
                     (GtkSignalFunc) time_fmt_type_set, GINT_TO_POINTER(1));

  time_fmt_entry = gtk_radio_button_new_with_label(time_fmt_group, _("Remaining time"));
  gtk_box_pack_start(GTK_BOX(zbox), time_fmt_entry, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(time_fmt_entry), !time_format);
  gtk_signal_connect(GTK_OBJECT(time_fmt_entry), "pressed",
                     (GtkSignalFunc) time_fmt_type_set, GINT_TO_POINTER(0));

  gtk_box_pack_start(GTK_BOX(vbox), zbox, FALSE, FALSE, 0);

  gtk_container_add(GTK_CONTAINER(hbox), vbox);

  vbox = gtk_vseparator_new();
  gtk_container_add(GTK_CONTAINER(hbox), vbox);
  
  vbox = gtk_vbox_new(FALSE, 0);

  zbox = gtk_vbox_new(FALSE, 0);
  gtk_container_border_width(GTK_CONTAINER(zbox), 10);

  label = gtk_label_new(_("Load file-info:"));
  gtk_box_pack_start(GTK_BOX(zbox), label, FALSE, FALSE, 0);

  always_load_thing = always_load_info;
  
  always_load_entry = gtk_radio_button_new_with_label(NULL, _("Always"));
  always_load_group = gtk_radio_button_group(GTK_RADIO_BUTTON(always_load_entry));
  gtk_box_pack_start(GTK_BOX(zbox), always_load_entry, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(always_load_entry), always_load_info);
  gtk_signal_connect(GTK_OBJECT(always_load_entry), "pressed",
                     (GtkSignalFunc) load_type_set, GINT_TO_POINTER(1));

  always_load_entry = gtk_radio_button_new_with_label(always_load_group, _("On File play only"));
  gtk_box_pack_start(GTK_BOX(zbox), always_load_entry, FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(always_load_entry), !always_load_info);
  gtk_signal_connect(GTK_OBJECT(always_load_entry), "pressed",
                     (GtkSignalFunc) load_type_set, GINT_TO_POINTER(0));

  gtk_box_pack_start(GTK_BOX(vbox), zbox, FALSE, FALSE, 0);

  zbox = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), zbox, FALSE, FALSE, 0);
  
  zbox = gtk_vbox_new(FALSE, 0);
  
  gtk_box_pack_start(GTK_BOX(vbox), zbox, FALSE, FALSE, 0);

  gtk_container_add(GTK_CONTAINER(hbox), vbox);
  
  label = gtk_label_new(_("Switches"));
  gtk_container_add(GTK_CONTAINER(frame), hbox);
  gtk_notebook_append_page(GTK_NOTEBOOK(laptop), frame, label);
  
  
  /* Help */
  vbox = gkrellm_gtk_framed_notebook_page(laptop,_("Help"));
  text = gkrellm_gtk_scrolled_text_view(vbox,NULL,
      GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  for (i=0; i < sizeof(gkrellmms_help_text)/sizeof(gchar*); ++i)
      gkrellm_gtk_text_view_append(text,_(gkrellmms_help_text[i]));
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);

  /* About */
  gkrellmms_info_text = g_strdup_printf(
    _("GKrellMMS %d.%d.%d\n" \
    "GKrellM XMMS Plugin\n" \
    "\n" \
    "Copyright (C) 2000-2002 Sander Klein Lebbink <sander@cerberus.demon.nl>\n"\
    "Current Maintainer: Sjoerd Simons <sjoerd@luon.net>\n" \
    "http://gkrellm.luon.net/\n" \
    "\n" \
    "Released under the GNU Public License\n"),
    GKRELLMMS_VERSION_MAJOR, GKRELLMMS_VERSION_MINOR, GKRELLMMS_VERSION_REV);
  info_label = gtk_label_new(gkrellmms_info_text);
  g_free(gkrellmms_info_text);
  label = gtk_label_new(_("About"));
  gtk_notebook_append_page(GTK_NOTEBOOK(laptop), info_label, label);
}
