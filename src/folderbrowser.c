/* This file is part of gPHPEdit, a GNOME2 PHP Editor.

   Copyright (C) 2003, 2004, 2005 Andy Jeffries <andy at gphpedit.org>
   Copyright (C) 2009 Anoop John <anoop dot john at zyxware.com>
   Copyright (C) 2009 José Rostagno(for vijona.com.ar)
   For more information or to find the latest release, visit our
   website at http://www.gphpedit.org/

   gPHPEdit is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   gPHPEdit is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with gPHPEdit.  If not, see <http://www.gnu.org/licenses/>.

   The GNU General Public License is contained in the file COPYING.
*/
/* ******* FOLDERBROWSER DESIGN ********
Folderbrowser has a treeview to show the directory struct. This treeview has a column for the pixmap
and the name (both shown to the user), but also the mimetype of the file (not shown). 

Folderbrowser has the following features:
-Remember last folder
-Autorefresh when directory contents changes
-Popup menu
-AutoSort (directories go before files, files are sorted by name and at last by extension)
-Double click in treeview open files and expand/collapse treerow for directories
-Keypress capture: press delete will delete current selected file/folder; press enter will open current selected file/folder
-Only displays files which could be opened by the editor
-Drag and drop: if you drop uri into the folderbrowser these files will be copied to current folderbrowser folder
*/
//TODO:Async Update
#include "folderbrowser.h"
#include "tab.h"
//#define DEBUGFOLDERBROWSER
typedef struct {
  gchar *filename;
  gchar *mime;
} POPUPDATA;
POPUPDATA pop;
GFileMonitor *monitor;

enum {
	TARGET_URI_LIST,
	TARGET_STRING
};
void update_folderbrowser (void){
    #ifdef DEBUGFOLDERBROWSER
    g_print("DEBUG::UPDATING FOLDERBROWSER");
    #endif
    GtkTreeIter iter2;
    GtkTreeIter* iter=NULL;
    gtk_tree_store_clear(main_window.pTree);
    gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(main_window.pTree), 1, filebrowser_sort_func, NULL, NULL);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(main_window.pTree), 1, GTK_SORT_ASCENDING);
	if (sChemin && !IS_DEFAULT_DIR(sChemin)){
	    create_tree(GTK_TREE_STORE(main_window.pTree),sChemin,iter,&iter2);
	}
}

/*
 * tree_double_clicked
 *
 *   this function handles double click signal of folderbrowser treeview:
 *   if selected item is a file then open it
 *   if selected file is a directory force expand/collapse treerow
 */
void tree_double_clicked(GtkTreeView *tree_view,GtkTreePath *path,GtkTreeViewColumn *column,gpointer user_data)
 {
 	GtkTreeModel *model;
 	GtkTreeSelection *select;
 	GtkTreeIter iter;
        select = gtk_tree_view_get_selection(tree_view);
  	if(!sChemin)
        sChemin=(gchar*)gtk_button_get_label(GTK_BUTTON(main_window.button_dialog));
  if(gtk_tree_selection_get_selected (select, &model, &iter))
  {
       	gchar *nfile;
        gchar *mime;
     gtk_tree_model_get (model, &iter,1, &nfile,2,&mime, -1);

 	GtkTreeIter* parentiter=(GtkTreeIter*)g_malloc(sizeof(GtkTreeIter));
 	while(gtk_tree_model_iter_parent(model,parentiter,&iter)){
 		gchar *rom;
     	gtk_tree_model_get (model, parentiter, 1, &rom, -1);
 		nfile = g_build_path (G_DIR_SEPARATOR_S, rom, nfile, NULL);
 		iter=*parentiter;
 		parentiter=(GtkTreeIter*)g_malloc(sizeof(GtkTreeIter));
 	}
     gchar* file_name = g_build_path (G_DIR_SEPARATOR_S, sChemin, nfile, NULL);
     if (!MIME_ISDIR(mime))
     	switch_to_file_or_open(file_name,0);
     else
     {
	if(gtk_tree_view_row_expanded (tree_view,path))
     		gtk_tree_view_collapse_row (tree_view,path);
     	else
     		gtk_tree_view_expand_row (tree_view,path,0);
     }  
   }
 }

/*
 * filebrowser_sort_func
 *
 *   this function is the sort function, and has the following  features:
 * - directories go before files
 * - files are first sorted without extension, only equal names are sorted by extension
 *
 */
gint filebrowser_sort_func(GtkTreeModel * model, GtkTreeIter * a, GtkTreeIter * b,
						   gpointer user_data)
{
	gchar *namea, *nameb, *mimea, *mimeb;
	gboolean isdira, isdirb;
	gint retval = 0;
	gtk_tree_model_get((GtkTreeModel *)model, a, 1, &namea,2, &mimea,-1);
	gtk_tree_model_get((GtkTreeModel *)model, b, 1, &nameb,2, &mimeb,-1);
        isdira = (mimea && MIME_ISDIR(mimea));
	isdirb = (mimeb && MIME_ISDIR(mimeb));
         #ifdef DEBUGFOLDERBROWSER
        g_print("DEBUG::UPDATING FOLDERBROWSER\n");
        g_print("isdira=%d, mimea=%s, isdirb=%d, mimeb=%s\n",isdira,mimea,isdirb,mimeb);
        #endif
	
	if (isdira == isdirb) {		/* both files, or both directories */
		if (namea == nameb) {
			retval = 0;			/* both NULL */
		} else if (namea == NULL || nameb == NULL) {
			retval = (namea - nameb);
		} else {				/* sort by name, first without extension */
			gchar *dota, *dotb;
			dota = strrchr(namea, '.');
			dotb = strrchr(nameb, '.');
			if (dota)
				*dota = '\0';
			if (dotb)
				*dotb = '\0';
			retval = strcmp(namea, nameb);
			if (retval == 0) {
				if (dota)
					*dota = '.';
				if (dotb)
					*dotb = '.';
				retval = strcmp(namea, nameb);
			}
		}
	} else {					/* a directory and a file */
		retval = (isdirb - isdira);
	}
	g_free(namea);
	g_free(nameb);
	g_free(mimea);
	g_free(mimeb);
	return retval;
}

/*
 * icon_name_from_icon
 *
 *   this function returns the icon name of a Gicon
 *   for the current gtk default icon theme
 */

static gchar *icon_name_from_icon(GIcon *icon) {
	gchar *icon_name=NULL;
	if (icon && G_IS_THEMED_ICON(icon)) {
		GStrv names;

		g_object_get(icon, "names", &names, NULL);
		if (names && names[0]) {
			GtkIconTheme *icon_theme;
			int i;
			icon_theme = gtk_icon_theme_get_default();
			for (i = 0; i < g_strv_length (names); i++) {
				if (gtk_icon_theme_has_icon(icon_theme, names[i])) {
					icon_name = g_strdup(names[i]);
					break;
				}
			}
			g_strfreev (names);
		}
	} else {
		icon_name = g_strdup("folder");
	}
        if (!icon_name){
           icon_name=g_strdup("folder");
       }
        return icon_name;
}

void add_file(GtkTreeStore *pTree, GFileInfo *info, GtkTreeIter *iter, GtkTreeIter *iter2){
    GdkPixbuf *p_file_image = NULL;
          //don't show hidden files
       if (!g_file_info_get_is_hidden (info)  && !g_file_info_get_is_backup (info)){
            const char *mime;
            mime=g_file_info_get_content_type (info);
            if (IS_TEXT(mime) && !IS_APPLICATION(mime)){
              #ifdef DEBUGFOLDERBROWSER
              g_print("DEBUG::ADD file to folderbrowser; Mime:%s\n",mime);
              #endif
            GIcon *icon= g_file_info_get_icon (info); // get iconname 
            p_file_image =gtk_icon_theme_load_icon (gtk_icon_theme_get_default (), icon_name_from_icon(icon), 16, 0, NULL); // get icon of size 16px
            gtk_tree_store_insert_with_values(GTK_TREE_STORE(pTree), iter2, iter, 0, 0, p_file_image, 1, g_file_info_get_display_name (info),2,mime,-1);
            g_object_unref(info);
            }
       }
 while (gtk_events_pending ())
	  gtk_main_iteration ();

}
void add_folder(GtkTreeStore *pTree, gchar *sChemin,GFileInfo *info, GtkTreeIter *iter, GtkTreeIter *iter2){
   GdkPixbuf *p_file_image = NULL;
   if (g_file_info_get_file_type (info)==G_FILE_TYPE_DIRECTORY){
       if (!g_file_info_get_is_hidden (info)){
       p_file_image =gtk_icon_theme_load_icon (gtk_icon_theme_get_default (), "folder", 16, 0, NULL);
       gtk_tree_store_insert_with_values(GTK_TREE_STORE(pTree), iter2, iter, 0, 0, p_file_image, 1, g_file_info_get_display_name (info),2, DIRMIME,-1);
       GtkTreeIter iter_new;
       gchar *folder=(gchar *)g_file_info_get_display_name (info);
       gchar *next_dir = NULL;
       next_dir = g_build_path (G_DIR_SEPARATOR_S, sChemin, folder, NULL);
       #ifdef DEBUGFOLDERBROWSER
       g_print("DEBUG::folderbrowser next dir:%s\n",next_dir);
       #endif
       //if has dot in name pos 0 don't process
       if(folder[0]!='.'){
       GFile *file;
       file= g_file_new_for_uri (next_dir);
       if (g_file_query_exists (file,NULL)){
            g_object_unref(file);
            create_tree(pTree,next_dir, iter2, &iter_new);
       } else {
           g_object_unref(file);
       }
       }
       while (gtk_events_pending ())
	gtk_main_iteration ();
       }
}
}
/*
 *   create tree
 *
 *   this function fill folderbrowser tree and start folderbrowser autorefresh
 */
void create_tree(GtkTreeStore *pTree, gchar *sChemin, GtkTreeIter *iter, GtkTreeIter *iter2){
    GFile *file;
    file= g_file_new_for_uri (sChemin);
    GFileEnumerator *files;
    GError *error=NULL;
    files=g_file_enumerate_children (file,FOLDER_INFOFLAGS,G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,NULL,&error);
    if (!files){
    g_print(_("Error getting folderbrowser files. GIO Error:%s\n"),error->message);
    gtk_button_set_label(GTK_BUTTON(main_window.button_dialog), DEFAULT_DIR);
    return;
}
GFileInfo *info;
// Get fileinfo
info=g_file_enumerator_next_file (files,NULL,&error);
while (info){
   if (g_file_info_get_file_type (info)==G_FILE_TYPE_DIRECTORY){
      add_folder(pTree, sChemin,info, iter, iter2);
      if (error!=NULL){
       //error=NULL;
       //Start file monitor for folderbrowser autorefresh
       monitor= g_file_monitor_directory (file,G_FILE_MONITOR_NONE,NULL,&error);
	if (!monitor){
	g_print(_("Error initing folderbrowser autorefresh. GIO Error:%s\n"),error->message);
	return;
	}else{
	g_signal_connect(monitor, "changed", (GCallback) update_folderbrowser_signal, NULL);
	}
      }
   } else {
      add_file(pTree, info,iter, iter2);
   }
   // Get fileinfo
   info=g_file_enumerator_next_file (files,NULL,&error);
}
g_object_unref(files);
g_object_unref(file);
}
void update_folderbrowser_signal (GFileMonitor *monitor,GFile *file,GFile *other_file, GFileMonitorEvent event_type, gpointer user_data){
    update_folderbrowser ();
}
void popup_open_file(gchar *filename){
    switch_to_file_or_open(filename, 0);
}

/*
 * popup_delete_file
 *
 *   This function is the delete function of the folderbrowser popup menu, and has the following  features:
 * - Promp before delete the file
 * - Send file to trash if filesystem support that feature
 * - Delete file if filesystem don't support send to trash feature
 *
 */

void popup_delete_file(void){
 GtkWidget *dialog;
 gint button;
 dialog = gtk_message_dialog_new(GTK_WINDOW(main_window.window),GTK_DIALOG_DESTROY_WITH_PARENT,GTK_MESSAGE_INFO,GTK_BUTTONS_YES_NO,"%s",
            _("Are you sure you wish to delete this file?"));
 gtk_window_set_title(GTK_WINDOW(dialog), _("Question"));
 button = gtk_dialog_run (GTK_DIALOG (dialog));
 gtk_widget_destroy(dialog);
 if (button == GTK_RESPONSE_YES){
    GFile *fi;
    GError *error=NULL;
    gchar *filename;
    filename=convert_to_full(pop.filename);
    fi=g_file_new_for_uri (filename);
    if (!g_file_trash (fi,NULL,&error)){
        if (error->code == G_IO_ERROR_NOT_SUPPORTED){
            if (!g_file_delete (fi,NULL,&error)){
            g_print(_("GIO Error deleting file: %s\n"),error->message);
            } else {
            update_folderbrowser();
            }
        } else {
         g_print(_("GIO Error deleting file: %s\n"),error->message);
        }
        } else {
        update_folderbrowser();
        }
    }
}

void popup_rename_file(void){
    GFile *fi;
    GError *error=NULL;
    gchar *filename;
    filename=convert_to_full(pop.filename);
    fi=g_file_new_for_uri (filename);
    GFileInfo *info= g_file_query_info (fi, "standard::display-name",G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL,&error);
    if (!info){
        g_print(_("Error renaming file. GIO Error:%s\n"),error->message);
        return ;
    }
    GtkWidget *window;
    window = gtk_dialog_new_with_buttons(_("Rename File"), GTK_WINDOW(main_window.window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
    GtkWidget *vbox1 = gtk_vbox_new (FALSE, 8);
    gtk_widget_show (vbox1);
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(window)->vbox),vbox1);
    GtkWidget *hbox1 = gtk_hbox_new (FALSE, 8);
    gtk_widget_show (hbox1);
    gtk_container_add (GTK_CONTAINER (vbox1),hbox1);
    GtkWidget *label1 = gtk_label_new (_("New Filename"));
    gtk_widget_show (label1);
    gtk_container_add (GTK_CONTAINER (hbox1),label1);
    GtkWidget *text_filename = gtk_entry_new();
    gtk_entry_set_max_length (GTK_ENTRY(text_filename),20);
    gtk_entry_set_width_chars(GTK_ENTRY(text_filename),21);
    gtk_entry_set_text (GTK_ENTRY(text_filename),(gchar *)g_file_info_get_display_name (info));
    gtk_widget_show (text_filename);
    gtk_container_add (GTK_CONTAINER (hbox1),text_filename);
    gint res=gtk_dialog_run(GTK_DIALOG(window));
    const char *name=gtk_entry_get_text (GTK_ENTRY(text_filename));
    if ( res==GTK_RESPONSE_ACCEPT){
    if (strcmp(name,g_file_info_get_display_name (info))!=0){
    fi=g_file_set_display_name (fi,name,NULL,&error);
    //FIXME: better method to do this??
    update_folderbrowser();
    }
    }
    gtk_widget_destroy(window);
    g_object_unref(fi);
}

void popup_create_dir(void){
    gchar *filename;
    filename=convert_to_full(pop.filename);

    GtkWidget *window;
    window = gtk_dialog_new_with_buttons(_("New Dir"), GTK_WINDOW(main_window.window), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
    GtkWidget *vbox1 = gtk_vbox_new (FALSE, 8);
    gtk_widget_show (vbox1);
    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(window)->vbox),vbox1);
    GtkWidget *hbox1 = gtk_hbox_new (FALSE, 8);
    gtk_widget_show (hbox1);
    gtk_container_add (GTK_CONTAINER (vbox1),hbox1);
    GtkWidget *label1 = gtk_label_new (_("Directory Name"));
    gtk_widget_show (label1);
    gtk_container_add (GTK_CONTAINER (hbox1),label1);
    GtkWidget *text_filename = gtk_entry_new();
    gtk_entry_set_max_length (GTK_ENTRY(text_filename),20);
    gtk_entry_set_width_chars(GTK_ENTRY(text_filename),21);
    gtk_widget_show (text_filename);
    gtk_container_add (GTK_CONTAINER (hbox1),text_filename);
    gint res=gtk_dialog_run(GTK_DIALOG(window));
    const char *name=gtk_entry_get_text (GTK_ENTRY(text_filename));
    if ( res==GTK_RESPONSE_ACCEPT && name){
    GFile *config;
    GError *error;
    error=NULL;

    if (!MIME_ISDIR(pop.mime)){
        config=g_file_new_for_uri (convert_to_full(pop.filename));
        gchar *parent=g_file_get_path (g_file_get_parent(config));
        filename= g_build_path (G_DIR_SEPARATOR_S, parent, name, NULL);
        config= g_file_new_for_path (filename);
    } else {
        filename= g_build_path (G_DIR_SEPARATOR_S, convert_to_full(pop.filename), name, NULL);
        config=g_file_new_for_uri (filename);
    }
    #ifdef DEBUGFOLDERBROWSER
    g_print("DEBUG::New directory:%s",filename);
    #endif
    
    if (!g_file_make_directory (config, NULL, &error)){
            g_print(_("Error creating folder. GIO error:%s\n"), error->message);
            gtk_widget_destroy(window);
            return ;
    }
    //FIXME: better method to do this??
    update_folderbrowser();
    g_object_unref(config);
    gtk_widget_destroy(window);
    }
}
/*
 * view_popup_menu
 *
 *   This function shows a popup menu with the following features:
 * - Open File
 * - Rename File
 * - Delete File
 * - Create New directory
 *
 */

  void
  view_popup_menu (GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
  {
    GtkWidget *menu, *menuopen,*menurename,*menudelete,*menucreate,*sep;

    menu = gtk_menu_new();

    menuopen = gtk_menu_item_new_with_label(_("Open file"));
    if (MIME_ISDIR(pop.mime)){
    gtk_widget_set_state (menuopen,GTK_STATE_INSENSITIVE);
    }else {
    g_signal_connect(menuopen, "activate", (GCallback) popup_open_file, pop.filename);
    }
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuopen);

    menurename = gtk_menu_item_new_with_label(_("Rename file"));
    g_signal_connect(menurename, "activate", (GCallback) popup_rename_file, NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menurename);

    menudelete = gtk_menu_item_new_with_label(_("Delete file"));
    g_signal_connect(menudelete, "activate", (GCallback) popup_delete_file, NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menudelete);

    sep = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep);
  
    menucreate = gtk_menu_item_new_with_label(_("Create New Directory"));
    g_signal_connect(menucreate, "activate", (GCallback) popup_create_dir, NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menucreate);

    gtk_widget_show_all(menu);

    /* Note: event can be NULL here when called from view_onPopupMenu;
     *  gdk_event_get_time() accepts a NULL argument */
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
                   (event != NULL) ? event->button : 0,
                   gdk_event_get_time((GdkEvent*)event));
  }


  gboolean
  view_onButtonPressed (GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
  {
    /* single click with the right mouse button? */
    if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3)
    {
        /* select row if no row is selected */
        GtkTreeSelection *selection;

        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

        /* Note: gtk_tree_selection_count_selected_rows() does not
         *   exist in gtk+-2.0, only in gtk+ >= v2.2 ! */
        if (gtk_tree_selection_count_selected_rows(selection)  <= 1)
        {
           GtkTreePath *path;

           /* Get tree path for row that was clicked */
           if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview),
                                             (gint) event->x,
                                             (gint) event->y,
                                             &path, NULL, NULL, NULL))
           {
             gtk_tree_selection_unselect_all(selection);
             gtk_tree_selection_select_path(selection, path);
             gtk_tree_path_free(path);
           }
        }
     GtkTreeModel *model;
     GtkTreeIter iter;
     if(sChemin==NULL)
     sChemin=(gchar*)gtk_button_get_label(GTK_BUTTON(main_window.button_dialog));
     if(gtk_tree_selection_get_selected (selection, &model, &iter))
        {
 	gchar *nfile;
        gchar *mime;
        gtk_tree_model_get (model, &iter,1, &nfile,2,&mime, -1);

 	GtkTreeIter* parentiter=(GtkTreeIter*)g_malloc(sizeof(GtkTreeIter));
 	while(gtk_tree_model_iter_parent(model,parentiter,&iter)){
 		gchar *rom;
     	gtk_tree_model_get (model, parentiter, 1, &rom, -1);
 		nfile = g_build_path (G_DIR_SEPARATOR_S, rom, nfile, NULL);
 		iter=*parentiter;
 		parentiter=(GtkTreeIter*)g_malloc(sizeof(GtkTreeIter));
 	}
     gchar* file_name = g_build_path (G_DIR_SEPARATOR_S, sChemin, nfile, NULL);
     pop.filename=file_name;
     pop.mime=mime;
     view_popup_menu(treeview, event, userdata);
     }
      return TRUE; /* we handled this */
    }

    return FALSE; /* we did not handle this */
  }


  gboolean
  view_onPopupMenu (GtkWidget *treeview, gpointer userdata)
  {
    view_popup_menu(treeview, NULL, userdata);

    return TRUE; /* we handled this */
  }

  gboolean key_press (GtkWidget   *widget, GdkEventKey *event, gpointer     user_data){
      if (event->keyval==GDK_Delete || event->keyval==GDK_Return){
      	GtkTreeModel *model;
 	GtkTreeSelection *select;
 	GtkTreeIter iter;
        select = gtk_tree_view_get_selection(GTK_TREE_VIEW(widget));
  	if(!sChemin)
        sChemin=(gchar*)gtk_button_get_label(GTK_BUTTON(main_window.button_dialog));
  if(gtk_tree_selection_get_selected (select, &model, &iter))
  {
       	gchar *nfile;
        gchar *mime;
     gtk_tree_model_get (model, &iter,1, &nfile,2,&mime, -1);

 	GtkTreeIter* parentiter=(GtkTreeIter*)g_malloc(sizeof(GtkTreeIter));
 	while(gtk_tree_model_iter_parent(model,parentiter,&iter)){
 		gchar *rom;
     	gtk_tree_model_get (model, parentiter, 1, &rom, -1);
 		nfile = g_build_path (G_DIR_SEPARATOR_S, rom, nfile, NULL);
 		iter=*parentiter;
 		parentiter=(GtkTreeIter*)g_malloc(sizeof(GtkTreeIter));
 	}
     gchar* file_name = g_build_path (G_DIR_SEPARATOR_S, sChemin, nfile, NULL);
     if (event->keyval==GDK_Delete){
         //delete file
         pop.filename=file_name;
         pop.mime=mime;
         popup_delete_file();
     }else {
         //open file
         if (!MIME_ISDIR(mime)){
         switch_to_file_or_open(file_name, 0);
         }
     }
      }
  }
      return TRUE;
  }
/*
 * folderbrowser_create
 *
 *   Create folderbrowser widget and append it to the side bar
 *   Folderbrowser treeview has 3 columns:
 *   --icon column
 *   --filename column
 *   --mimetype column (hide for users)
 */

void folderbrowser_create(MainWindow *main_window)
 {
    	main_window->folder = gtk_vbox_new(FALSE, 0);

 	GtkTreeViewColumn *pColumn;
 	GtkCellRenderer  *pCellRenderer;
 	main_window->pTree = gtk_tree_store_new(3, GDK_TYPE_PIXBUF, G_TYPE_STRING,G_TYPE_STRING);
 	main_window->pListView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(main_window->pTree));

        pCellRenderer = gtk_cell_renderer_pixbuf_new();
        pColumn = gtk_tree_view_column_new_with_attributes("",pCellRenderer,"pixbuf",0,NULL);

 	gtk_tree_view_append_column(GTK_TREE_VIEW(main_window->pListView), pColumn);
 	main_window->pScrollbar = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(main_window->pScrollbar),GTK_POLICY_ALWAYS,GTK_POLICY_ALWAYS);
        gtk_container_add(GTK_CONTAINER(main_window->pScrollbar), main_window->pListView);

 	//renderer for text
 	g_signal_connect(G_OBJECT(main_window->pListView), "row-activated", G_CALLBACK(tree_double_clicked), NULL);
        g_signal_connect(G_OBJECT(main_window->pListView), "button-press-event", (GCallback) view_onButtonPressed, NULL);
        g_signal_connect(G_OBJECT(main_window->pListView), "popup-menu", (GCallback) view_onPopupMenu, NULL);
        g_signal_connect(G_OBJECT(main_window->pListView), "key-press-event", G_CALLBACK(key_press), NULL);
        const GtkTargetEntry drag_dest_types[] = {
		{"text/uri-list", 0, TARGET_URI_LIST},
		{"STRING", 0, TARGET_STRING},
	};
        gtk_drag_dest_set(main_window->pListView, (GTK_DEST_DEFAULT_ALL), drag_dest_types, 2,
					  (GDK_ACTION_DEFAULT | GDK_ACTION_COPY));
	g_signal_connect(G_OBJECT(main_window->pListView), "drag_data_received", G_CALLBACK(fb_file_v_drag_data_received),NULL);

 	pColumn=NULL;
 	pCellRenderer=NULL;
        pCellRenderer = gtk_cell_renderer_text_new();
        pColumn = gtk_tree_view_column_new_with_attributes(_("File"), pCellRenderer, "text",1,NULL);
 	gtk_tree_view_append_column(GTK_TREE_VIEW(main_window->pListView), pColumn);

        pColumn=NULL;
 	pCellRenderer=NULL;
        pCellRenderer = gtk_cell_renderer_text_new();
        pColumn = gtk_tree_view_column_new_with_attributes(_("Mime"), pCellRenderer,"text",2,NULL);
  	gtk_tree_view_append_column(GTK_TREE_VIEW(main_window->pListView), pColumn);
        gtk_tree_view_column_set_visible    (pColumn,FALSE);
 	gtk_widget_show(main_window->folder);
 	GtkWidget *label= gtk_label_new(_("Folder browser"));
        GConfClient *config;
        config=gconf_client_get_default ();
 	if(sChemin!=NULL)
   		main_window->button_dialog = gtk_button_new_with_label (sChemin);
   	else {
            /*load folder from config*/
             GError *error = NULL;
             sChemin= gconf_client_get_string(config,"/gPHPEdit/main_window/folderbrowser/folder",&error);
             if (!sChemin){
                 main_window->button_dialog = gtk_button_new_with_label (DEFAULT_DIR);
             } else {
             main_window->button_dialog = gtk_button_new_with_label (sChemin);
             }
        }
        g_object_unref(config);
 	g_signal_connect(G_OBJECT(main_window->button_dialog), "pressed", G_CALLBACK(pressed_button_file_chooser), NULL);
        gtk_widget_set_size_request (main_window->pListView,80,450);
 	//Close button for the side bar
	GtkWidget *hbox;
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 0);
	main_window->close_image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
	gtk_misc_set_padding(GTK_MISC(main_window->close_image), 0, 0);
	main_window->close_sidebar_button = gtk_button_new();
	gtk_widget_set_tooltip_text(main_window->close_sidebar_button, _("Close class Browser"));
	gtk_button_set_image(GTK_BUTTON(main_window->close_sidebar_button), main_window->close_image);
	gtk_button_set_relief(GTK_BUTTON(main_window->close_sidebar_button), GTK_RELIEF_NONE);
	gtk_button_set_focus_on_click(GTK_BUTTON(main_window->close_sidebar_button), FALSE);
	g_signal_connect(G_OBJECT(main_window->close_sidebar_button), "clicked", G_CALLBACK (classbrowser_show_hide),NULL);
	gtk_widget_show(main_window->close_image);
	gtk_widget_show(main_window->close_sidebar_button);
	gtk_box_pack_end(GTK_BOX(hbox), main_window->close_sidebar_button, FALSE, FALSE, 0);
	gtk_widget_show(hbox);
        gtk_box_pack_start(GTK_BOX(main_window->folder), hbox, FALSE, TRUE, 2);
 	gtk_box_pack_start(GTK_BOX(main_window->folder), main_window->button_dialog, FALSE, FALSE, 2);
 	gtk_box_pack_start(GTK_BOX(main_window->folder), main_window->pScrollbar, TRUE, TRUE, 2);
        gtk_widget_show(main_window->button_dialog);
	gtk_widget_show_all(main_window->folder);
        gint pos;
       	pos=gtk_notebook_insert_page (GTK_NOTEBOOK(main_window->notebook_manager), main_window->folder, label, 1);
        if(sChemin && !IS_DEFAULT_DIR(sChemin)){
                                sChemin=convert_to_full(sChemin); /*necesary for gfile*/
 				GtkTreeIter iter2;
				GtkTreeIter* iter=NULL;
 				gtk_tree_store_clear(main_window->pTree);
                                gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(main_window->pTree), 1,
										filebrowser_sort_func, NULL, NULL);
                                gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(main_window->pTree), 1, GTK_SORT_ASCENDING);
				init_folderbrowser(GTK_TREE_STORE(main_window->pTree),sChemin,iter,&iter2);
   }
}
void init_folderbrowser(GtkTreeStore *pTree, gchar *filename, GtkTreeIter *iter, GtkTreeIter *iter2){
    GFile *file;
    GError *error=NULL;
    file= g_file_new_for_uri (filename);
	//path don't exist?
    if (!g_file_query_exists (file,NULL)){
        gtk_button_set_label(GTK_BUTTON(main_window.button_dialog), DEFAULT_DIR);
	return;
    }
    GFileInfo *info =g_file_query_info (file,G_FILE_ATTRIBUTE_ACCESS_CAN_READ, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,NULL,&error);
    if (!info){
    g_print("ERROR initing folderbrowser:%s\n",error->message);
    gtk_button_set_label(GTK_BUTTON(main_window.button_dialog), DEFAULT_DIR);
    return;
    }
    if (!g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ)){
    g_print("Error Don't have read access for current folderbrowser path.\n");
    gtk_button_set_label(GTK_BUTTON(main_window.button_dialog), DEFAULT_DIR);
    return;
    }
    create_tree(GTK_TREE_STORE(main_window.pTree),sChemin,iter,iter2);
}

void fb_file_v_drag_data_received(GtkWidget * widget, GdkDragContext * context, gint x,  gint y, GtkSelectionData * data, guint info, guint time,gpointer user_data)
{

	gchar *stringdata;
	GFile *destdir = g_file_new_for_path ((gchar*)gtk_button_get_label(GTK_BUTTON(main_window.button_dialog)));
	g_object_ref(destdir);

	g_signal_stop_emission_by_name(widget, "drag_data_received");
	if ((data->length == 0) || (data->format != 8)
		|| ((info != TARGET_STRING) && (info != TARGET_URI_LIST))) {
		gtk_drag_finish(context, FALSE, TRUE, time);
		return;
	}
	stringdata = g_strndup((gchar *) data->data, data->length);
	g_print("fb2_file_v_drag_data_received, stringdata='%s', len=%d\n", stringdata, data->length);
	if (destdir) {
		if (strchr(stringdata, '\n') == NULL) {	/* no newlines, probably a single file */
			GSList *list = NULL;
			GFile *uri;
			uri = g_file_new_for_commandline_arg(stringdata);
			list = g_slist_append(list, uri);
        		copy_uris_async(destdir, list);
			g_slist_free(list);
			g_object_unref(uri);
		} else {
                    /* there are newlines, probably this is a list of uri's */
			copy_files_async(destdir, stringdata);
		}
		g_object_unref(destdir);
		gtk_drag_finish(context, TRUE, TRUE, time);
	} else {
		gtk_drag_finish(context, FALSE, TRUE, time);
	}
	g_free(stringdata);
}
/************************/
/**
 * trunc_on_char:
 * @string: a #gchar * to truncate
 * @which_char: a #gchar with the char to truncate on
 *
 * Returns a pointer to the same string which is truncated at the first
 * occurence of which_char
 *
 * Return value: the same gchar * as passed to the function
 **/
gchar *trunc_on_char(gchar * string, gchar which_char)
{
	gchar *tmpchar = string;
	while(*tmpchar) {
		if (*tmpchar == which_char) {
			*tmpchar = '\0';
			return string;
		}
		tmpchar++;
	}
	return string;
}
/********************/
 typedef struct {
 GSList *sourcelist;
 GFile *destdir;
 GFile *curfile, *curdest;
 } Tcopyfile;

  static gboolean copy_uris_process_queue(Tcopyfile *cf);
  void copy_async_lcb(GObject *source_object,GAsyncResult *res,gpointer user_data) {
  Tcopyfile *cf = user_data;
  gboolean done;
  GError *error=NULL;
 /* fill in the blanks */
  	done = g_file_copy_finish(cf->curfile,res,&error);

  	if (!done) {
  		if (error->code == G_IO_ERROR_EXISTS) {
  			gint retval;
  			gchar *tmpstr, *dispname;
                        GFileInfo *info =g_file_query_info (cf->curfile,"standard::display-name", G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,NULL,NULL);
  			dispname = (gchar *)g_file_info_get_display_name (info);
  			tmpstr = g_strdup_printf(_("%s cannot be copied, it already exists, overwrite?"),dispname);
  			retval = yes_no_dialog (_("Overwrite file?"), tmpstr);
  			g_free(tmpstr);
  			g_free(dispname);
  			if (retval != -8) {
  				g_file_copy_async(cf->curfile,cf->curdest,G_FILE_COPY_OVERWRITE,
  					G_PRIORITY_LOW,NULL,
  					NULL,NULL,
  					copy_async_lcb,cf);
  				return;
  			}
                }else {
                    g_print("ERROR copying file::%s\n",error->message);
                }
  	}
  	g_object_unref(cf->curfile);
  	g_object_unref(cf->curdest);
  	if (!copy_uris_process_queue(cf)) {
  		update_folderbrowser();
  		g_object_unref(cf->destdir);
  		g_free(cf);
  	}
  }

  static gboolean copy_uris_process_queue(Tcopyfile *cf) {
  	if (cf->sourcelist) {
  		GFile *uri, *dest;
  		char *tmp;

  		uri = cf->sourcelist->data;
  		cf->sourcelist = g_slist_remove(cf->sourcelist, uri);
  		tmp = g_file_get_basename(uri);
  		dest = g_file_get_child(cf->destdir,tmp);
  		g_free(tmp);

  		cf->curfile = uri;
  		cf->curdest = dest;
  		g_file_copy_async(uri,dest,G_FILE_COPY_NONE,
  				G_PRIORITY_LOW,NULL,
  				NULL,NULL,
  				copy_async_lcb,cf);

  		return TRUE;
  	}
  	return FALSE;
  }

  void copy_uris_async(GFile *destdir, GSList *sources) {
  	Tcopyfile *cf;
  	GSList *tmplist;
  	cf = g_new0(Tcopyfile,1);
  	//cf->bfwin = bfwin;
  	cf->destdir = destdir;
  	g_object_ref(cf->destdir);
  	cf->sourcelist = g_slist_copy(sources);
  	tmplist = cf->sourcelist;
  	while (tmplist) {
  		g_object_ref(tmplist->data);
  		tmplist = tmplist->next;
  	}
  	copy_uris_process_queue(cf);
  }
void copy_files_async(GFile *destdir, gchar *sources) {
  	Tcopyfile *cf;
  	gchar **splitted, **tmp;
  	cf = g_new0(Tcopyfile,1);
  	//cf->bfwin = bfwin;
  	cf->destdir = destdir;
  	g_object_ref(cf->destdir);
  	/* create the source and destlist ! */
  	tmp = splitted = g_strsplit(sources, "\n",0);
  	while (*tmp) {
  		trunc_on_char(trunc_on_char(*tmp, '\r'), '\n');
  		if (strlen(*tmp) > 1) {
  			GFile *src;
                        src = g_file_new_for_commandline_arg(*tmp);
  			cf->sourcelist = g_slist_append(cf->sourcelist, src);
  		}
  		tmp++;
  	}
  	g_strfreev(splitted);
  	copy_uris_process_queue(cf);
  }
