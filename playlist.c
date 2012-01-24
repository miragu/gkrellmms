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

#include <errno.h>
#include <string.h>

#include "playlist.h"
#include "gkrellmms.h"

enum { 
       PLAYLIST_POSITION = 0, 
       PLAYLIST_TITLE, 
       PLAYLIST_FILENAME, 
       PLAYLIST_TIME , 
       PLAYLIST_NRCOLUMNS};

/* the store for the playlist */
static GtkListStore *playlist = NULL; 
/* the playlist window */
static GtkWidget *playlist_window = NULL;

/* fast access for the current functions */
static gchar *current_file = NULL;
static gchar *current_title = NULL;
static gint  current_time = 0;
static gint  current_position = 0;
/* playlist lenght is an indicator for changes */
static gint playlist_length = -1;

/* util function to convert local filename to utf-8 
 * The argument string is freed and the new converted version is returned 
 * */
static gchar *
string_to_utf8(gchar *str, gboolean is_filename) {
  gchar *result = NULL;
  GError *error = NULL;
  gsize read = 0, lastread = -1;


  if (str == NULL) 
    return NULL;

  if (g_utf8_validate(str, -1, NULL)) {
    return str;
  }
  if (is_filename) {
    result = g_filename_to_utf8(str, -1, NULL, NULL, NULL);
  }
  if (result == NULL) {
    /* filename to utf8 failed, assume file encoding was local */
    for (;;) {
      result = g_locale_to_utf8(str, -1, &read, NULL, &error);
      if (result != NULL)
        break;

      if (G_CONVERT_ERROR_ILLEGAL_SEQUENCE == error->code) {
        /* ensure we progress through the string */
        if (read == lastread) read++;
        str[read] = '?';
        lastread = read;
      } else {
        /* unknown error, giving up */
        g_error_free(error);
        break;
      }
      g_error_free(error);
      error = NULL;
    }
  }
  g_free(str);
  return result;
}

gchar *pl_get_current_file() {
  return current_file == NULL ? "" : current_file;
}

gchar *pl_get_current_title() {
  return current_title == NULL ? "" : current_title;
}

int pl_get_current_time() {
  return current_time;
}
int pl_get_current_position() {
  return current_position;
}

void update_playlist_position(void) {
  GtkTreeIter iter;

  current_position = xmms_remote_get_playlist_pos(xmms_session) + 1;
  g_free(current_title);
  g_free(current_file);
  if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(playlist),
                                &iter, NULL, current_position -1))  {
    current_file = xmms_remote_get_playlist_file(xmms_session,
                                                     current_position -1);
    current_title = xmms_remote_get_playlist_title(xmms_session,
                                                     current_position -1);
    current_time =  xmms_remote_get_playlist_time(xmms_session,
                                                     current_position -1);

    current_file = string_to_utf8(current_file, TRUE);

    current_title = string_to_utf8(current_title, FALSE);
		if (current_title == NULL && current_file != NULL) { 
			current_title = g_strdup(current_file);
		}
    /* get the info from xmms to be sure it's up to data */
    gtk_list_store_set(playlist,&iter,
                  PLAYLIST_TITLE,current_title == NULL   ? "" : current_title,
                  PLAYLIST_FILENAME,current_file == NULL ? "" : current_file,
                  PLAYLIST_TIME,current_time,
                  -1);
  } else {
    current_title = NULL;
    current_file = NULL;
    current_position = 0;
    current_time = 0;
  }
}

static void
empty_playlist(void) {
  gtk_list_store_clear(playlist);
  update_playlist_position();
}

static void
load_playlist(void) {
  int len,i;
  gint time;
  char *filename, *title, *basename;
  GtkTreeIter iter;

  total_plist_time = 0;

  if (!xmms_remote_is_running(xmms_session)) return;

  len = xmms_remote_get_playlist_length(xmms_session);
  playlist_length = len;

  for (i = 0 ; i < len ; i ++) {
    filename =  xmms_remote_get_playlist_file(xmms_session, i);
    if (filename == NULL) {
      /* error occurred empty playlist and try again */
      empty_playlist();
      return load_playlist();
    }
    filename = string_to_utf8(filename, TRUE);

    while (gtk_events_pending()) 
      gtk_main_iteration();

    gtk_list_store_append(playlist,&iter);
    if (always_load_info) {
      title = xmms_remote_get_playlist_title(xmms_session, i);
      if (title != NULL) { 
        title = string_to_utf8(title, FALSE);
      } 
      time = xmms_remote_get_playlist_time(xmms_session, i);
      gtk_list_store_set(playlist,&iter,
                         PLAYLIST_POSITION,i+ 1,
                         PLAYLIST_TITLE,title == NULL ? "" : title,
                         PLAYLIST_FILENAME,filename,
                         PLAYLIST_TIME,time,
                         -1);
      total_plist_time += time;
      g_free(title);
    } else {
			basename = filename == NULL ? NULL : g_path_get_basename(filename) ;
      gtk_list_store_set(playlist,&iter,
                         PLAYLIST_POSITION,i+ 1,
                         PLAYLIST_TITLE, basename,
                         PLAYLIST_FILENAME,filename,
                         PLAYLIST_TIME,0,
                         -1);
			g_free(basename);
    }
    g_free(filename);
  }
  update_playlist_position();
}

gint
update_playlist(void) {
  char *filename = NULL;

  /* playlist lenght changed, reload */
  if (playlist_length != xmms_remote_get_playlist_length(xmms_session)) {
    empty_playlist();
    load_playlist();
    return TRUE;
  }
  filename = string_to_utf8(
               xmms_remote_get_playlist_file(xmms_session,current_position-1),
               TRUE
             );
  
  if (filename == NULL || strcmp(pl_get_current_file(), filename)) {
    empty_playlist();
    load_playlist();
    g_free(filename);
    return TRUE;
  }
  g_free(filename);

  if (xmms_remote_get_playlist_pos(xmms_session) != current_position + 1) {
    update_playlist_position();
  }
  return TRUE;
}

void
pl_init() {
  playlist = gtk_list_store_new(PLAYLIST_NRCOLUMNS,
                                GTK_TYPE_INT,
                                GTK_TYPE_STRING,
                                GTK_TYPE_STRING,
                                GTK_TYPE_INT);
}

static int
playlist_window_destroy_cb(GtkWidget *w, gpointer data) {
  playlist_window = NULL;
  return TRUE;
}

int
open_playlist_file_choosen(GtkWidget *w, gpointer selector) {
  char *file; 
  FILE *fp;
  gchar *error;
  gchar *path;
  gchar *dirname = NULL;
  gchar buf[MAXPATHLEN+1];
  GList *list = NULL,*tlist;

  file = (char *) gtk_file_selection_get_filename(GTK_FILE_SELECTION(selector));
  dirname = g_path_get_dirname(file);

  fp = fopen(file,"r");
  if (fp == NULL) {
    error = g_strdup_printf("Couldn't open %s \n%s",file,strerror(errno));
    gkrellm_message_window("GKrellMMS Error", error, NULL);
    g_free(error);
    return TRUE;
  }
  while (fgets(buf,MAXPATHLEN+1,fp) != NULL) {
    if (buf[0] != G_DIR_SEPARATOR) {
      path = g_build_filename(dirname,buf,NULL);
    } else {
      path = g_strdup(buf);
    }
    list = g_list_append(list,path);
  }
  if (xmms_remote_is_running(xmms_session)) {
    xmms_remote_playlist_clear(xmms_session);
    xmms_remote_playlist_add(xmms_session, list);
  }

  for (tlist = list; tlist != NULL; tlist = g_list_next(tlist)) {
    g_free(tlist->data);
  }
  g_list_free(list);
  g_free(dirname);

  return TRUE;
}

void
pl_open_playlist(void) {
  GtkWidget *selector;
  gchar *path;
  selector = gtk_file_selection_new(_("Please select a playlist"));
  path = g_strconcat(playlist_dir,"/",NULL);
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(selector),path);
  g_free(path);

  g_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(selector)->ok_button),
      "clicked",
      G_CALLBACK(open_playlist_file_choosen),
      selector);

  g_signal_connect_swapped(GTK_OBJECT(GTK_FILE_SELECTION(selector)->ok_button),
      "clicked",
      G_CALLBACK (gtk_widget_destroy), (gpointer) selector);

  g_signal_connect_swapped(
      GTK_OBJECT(GTK_FILE_SELECTION(selector)->cancel_button),
      "clicked", G_CALLBACK (gtk_widget_destroy), (gpointer) selector);
  gtk_widget_show(selector);
}

static int
playlist_open_clicked_cb(GtkWidget *w, gpointer data) {
  pl_open_playlist();
  return TRUE;
}

int
save_playlist_file_choosen(GtkWidget *w, gpointer selector) {
 gchar *file;
 gchar *error;
 GtkTreeIter iter;
 gboolean valid;
 FILE *fp;
 gchar *fname;
 file = (char *) gtk_file_selection_get_filename(GTK_FILE_SELECTION(selector));

 fp = fopen(file,"w");
 if (fp == NULL) {
   error = g_strdup_printf("Couldn't save playlist to %s:\n %s",
                           file,strerror(errno));
   gkrellm_message_window("GKrellMMS Error", error, NULL);
   g_free(error);
   return TRUE;
 }

 valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(playlist),&iter);
 while (valid) {
   gtk_tree_model_get(GTK_TREE_MODEL(playlist),&iter,
                      PLAYLIST_FILENAME,&fname,
                      -1);
   fprintf(fp,"%s\n",fname);
   g_free(fname);
   valid = gtk_tree_model_iter_next (GTK_TREE_MODEL(playlist), &iter);
 }
 fclose(fp);
 return TRUE;
}

static int
playlist_save_clicked_cb(GtkWidget *w, gpointer data) {
  GtkWidget *selector;
  gchar *path;
  selector = gtk_file_selection_new(
      _("Please select a location to save playlist"));
  path = g_strconcat(playlist_dir,"/",NULL);
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(selector),path);
  g_free(path);

  g_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(selector)->ok_button),
      "clicked",
      G_CALLBACK(save_playlist_file_choosen),
      selector);

  g_signal_connect_swapped(GTK_OBJECT(GTK_FILE_SELECTION(selector)->ok_button),
      "clicked",
      G_CALLBACK (gtk_widget_destroy), (gpointer) selector);

  g_signal_connect_swapped(
      GTK_OBJECT(GTK_FILE_SELECTION(selector)->cancel_button),
      "clicked", G_CALLBACK (gtk_widget_destroy), (gpointer) selector);
  gtk_widget_show(selector);
  return TRUE;
}

int
playlist_row_activated_cb(GtkTreeView *treeview, GtkTreePath *path,
                          GtkTreeViewColumn *col, gpointer user_data) {
  /* play the activated song */
  gint position;
  GtkTreeIter iter;

  gtk_tree_model_get_iter (GTK_TREE_MODEL(playlist), &iter, path);
  gtk_tree_model_get(GTK_TREE_MODEL(playlist),&iter,
                     PLAYLIST_POSITION,&position,
                     -1);
  xmms_remote_set_playlist_pos(xmms_session,position - 1);
  xmms_remote_play(xmms_session);
  return TRUE;
}

void
playlist_time_func(GtkTreeViewColumn *col,
                   GtkCellRenderer   *renderer,
                   GtkTreeModel      *model,
                   GtkTreeIter       *iter,
                   gpointer           user_data) {
  gint time;
  gchar *timestr;
  gtk_tree_model_get(model, iter, PLAYLIST_TIME, &time, -1);
	if (time == 0) { 
		timestr = g_strdup_printf("??");
	} else {
    timestr = g_strdup_printf("%d:%02d",(time/1000)/60,(time/1000)%60);
	}
  g_object_set(renderer, "text", timestr, NULL);
  g_free(timestr);
}

static void
create_playlist_window(void) {
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *scrolled;
  GtkWidget *treeview;
  GtkWidget *widget;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *col;

  playlist_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(playlist_window),
                              400,300);
  gtk_window_set_title(GTK_WINDOW(playlist_window),
                                                _("GKrellMMS Playlist Editor"));
  g_signal_connect(G_OBJECT(playlist_window),"destroy",
                     G_CALLBACK(playlist_window_destroy_cb),NULL);

  treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(playlist));
  gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(treeview),TRUE);


  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(treeview), -1,
            _("#"),renderer,
            "text", PLAYLIST_POSITION,
            NULL);

  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new_with_attributes(_("Title"),renderer,
                                                 "text",PLAYLIST_TITLE,
                                                 NULL);
  gtk_tree_view_column_set_resizable(col,TRUE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col);
  gtk_tree_view_set_expander_column(GTK_TREE_VIEW(treeview),col);

  renderer = gtk_cell_renderer_text_new();
  col = gtk_tree_view_column_new_with_attributes(_("Time"),renderer,NULL);
  gtk_tree_view_column_set_cell_data_func(col, renderer, playlist_time_func, 
                                          NULL, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), col);

  scrolled = gtk_scrolled_window_new(NULL,NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_ALWAYS);
  gtk_container_add(GTK_CONTAINER(scrolled),treeview);

  vbox = gtk_vbox_new(FALSE,3);
  gtk_box_pack_start(GTK_BOX(vbox),scrolled,TRUE,TRUE,3);
  
  hbox = gtk_hbox_new(FALSE,3);
  gtk_box_pack_start(GTK_BOX(vbox),hbox,FALSE,FALSE,3);

  widget = gtk_button_new_from_stock(GTK_STOCK_SAVE);
  g_signal_connect(G_OBJECT(widget),"clicked",
                   G_CALLBACK(playlist_save_clicked_cb),NULL);
  gtk_box_pack_end(GTK_BOX(hbox),widget,FALSE,FALSE,3);

  widget = gtk_button_new_from_stock(GTK_STOCK_OPEN);
  g_signal_connect(G_OBJECT(widget),"clicked",
                   G_CALLBACK(playlist_open_clicked_cb),NULL);

  g_signal_connect(G_OBJECT(treeview),"row-activated",
                   G_CALLBACK(playlist_row_activated_cb),NULL);
  gtk_box_pack_end(GTK_BOX(hbox),widget,FALSE,FALSE,3);

  gtk_container_add(GTK_CONTAINER(playlist_window),vbox);

  gtk_widget_show_all(playlist_window);
}

void pl_show_playlist(void) { 
  if (playlist_window != NULL) {
    gtk_widget_show(GTK_WIDGET(playlist_window));
  } else {
    create_playlist_window();
  }
}
