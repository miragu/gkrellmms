#ifndef GKRELLMMS_H
#define GKRELLMMS_H
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

#include <stdio.h>
#include <time.h>
#include <gkrellm2/gkrellm.h>

#include <audacious/audctrl.h>
#include <audacious/dbus.h>


#include "playlist.h"
/* You can change these ones, but at your own risk.
 | Changing the maxlen_scroll_separator shouldn't
 | cause any weird things (though the scroll-bar could
 | get the hiccup), but it's not recommended to set the
 | config_keyword to a word that is also defined as
 | config_keyword in any other used gkrellm-plugin.
 */
#define SCROLL_SEPARATOR " ++++ "
#define MAXLEN_SCROLL_SEPARATOR 25
#define CONFIG_KEYWORD "gkrellmms"

/* It's fun to set these ones to other values. ;) */
#define GKRELLMMS_VERSION_MAJOR 2
#define GKRELLMMS_VERSION_MINOR 1
#define GKRELLMMS_VERSION_REV   22

#define DEFAULT_STYLE style_id
#define STYLE_NAME    "gkrellmms"

/* Style to use for GKrellMMS. You can choose from:
 |   DEFAULT_STYLE_ID, MEM_STYLE, SWAP_STYLE, FS_STYLE, MAIL_STYLE
 |   APM_STYLE, UPTIME_STYLE, CLOCK_STYLE, CAL_STYLE, HOST_STYLE
 |   and TIMERBUTTON_STYLE
 |
 |  I didn't test it with all of them, but I think they'll all work. ;)
 */
#define GKRELLMMS_STYLE DEFAULT_STYLE

/* The location of the plugin. You can choose from:
 |   MON_CLOCK, MON_CPU, MON_PROC, MON_DISK,
 |   MON_INET, MON_NET, MON_FS, MON_MAIL,
 |   MON_APM, or MON_UPTIME
 |
 |  The plugin will place itself above GKRELLMMS_PLACE
 |
 |  The gravity defines where the plugin should be placed in comparison
 |  with other plugins with the same placement position.
 |  If you want the plugin higher (but with the same placement), you should
 |  set the gravity a bit lower, and if you want the plugin lower (with
 |  the same placement), you should set the gravity a bit higher.
 |  Min is 0, max is 15.
 |
 */
#ifdef GRAVITY
#define GKRELLMMS_PLACE (MON_APM | GRAVITY(8))
#else
#define GKRELLMMS_PLACE (MON_APM)
#endif

#if !GKRELLM_CHECK_VERSION(2,1,100)
#define gkrellm_gdk_string_width	gdk_string_width
#endif

/* Don't edit stuff yourself under here,
 | or you'll have to be a programmer/gkrellmms hacker!
 */
#define gkrellmms_prev  1
#define gkrellmms_play  2
#define gkrellmms_paus  3
#define gkrellmms_stop  4
#define gkrellmms_next  5
#define gkrellmms_eject 6

#define gkrellmms_mainwin  7
#define gkrellmms_playlist 8   
#define gkrellmms_eq       9
#define gkrellmms_repeat   10
#define gkrellmms_shuffle  11
#define gkrellmms_aot      12
#define gkrellmms_prefs    13


/* Very global vars */
extern DBusGProxy* proxy;

GtkItemFactory *running_factory;
GtkItemFactory *not_running_factory;

GkrellmPanel *time_bar;
gboolean xmms_running;
gchar *xmms_exec_command;
gchar *files_directory;
gchar *playlist_dir;
gchar *playlist_file;
gchar *position_file;
gchar *time_file;
gchar *gkrellmms_label;
gchar *scroll_separator;
gint scroll_enable;
gint enable_buttonbar;
gint xmms_autostart;
gint auto_main_close;
gint auto_hide_all;
gint eject_opens_playlist;
gint draw_time;
gint krell_mmb_pause;
gint time_format;
gint auto_play_start;
gint auto_seek;
gint always_load_info;
gint total_plist_time;
gint xmms_pos;
gint draw_minus;

gboolean pl_window_open;

/* Functions in gkrellmms.c */
void update_gkrellmms_config(gint);
void do_xmms_command(gint);
void gkrellmms_set_scroll_separator_len(void);

/* Functions in options.c */
GtkItemFactory *options_menu_factory(gint);
void options_menu(GdkEventButton *);
void apply_gkrellmms_config(void);
void create_gkrellmms_config(GtkWidget *);
void load_gkrellmms_config(gchar *);
void save_gkrellmms_config(FILE *);
void xmms_start_func(void);

/* Functions in gkrelmms.c */
GkrellmMonitor * gkrellmms_get_monitor(void);
#endif /* GKRELLMMS_H */
