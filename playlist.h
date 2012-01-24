#ifndef GKRELLMMS_PLAYLIST_H
#define GKRELLMMS_PLAYLIST_H
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

//#include <audacious/auddrct.h>

#include "gkrellmms.h"

/* init the playlist */
void
pl_init();

/* show the playlist window */ 
void pl_show_playlist();

/* let the user open a new playlist */
void pl_open_playlist();

/* check if the playlist is still correct 
 * returns TRUE when there where changes 
 */ 
gint update_playlist(void);

/* gets the time/filename/title of the current song 
 * don't free !
 * */
gchar *pl_get_current_file();
gchar *pl_get_current_title();
int pl_get_current_time();
int pl_get_current_position();

#endif /* GKRELLMMS_PLAYLIST_H  */
