/**
* *lyricshowmenu.c
* *
* *Copyright (c) 2012 Wei Keting
* *
* *Authors by:Wei Keting <weikting@gmail.com>
* *
* */

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#include <glib.h>

#ifdef GETTEXT_PACKAGE
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

#include <gtk/gtk.h>
#include "LyricShow.h"

static const gchar *ui_xml=
"<ui>"
"   <popup name='menu'>"
"       <menuitem action='search_action' />"
///"       <menuitem action='about_action' />"
"   </popup>"
"</ui>";

static void
about_action_callback(GtkAction *action,LyricShow *lsw);

static void
search_action_callback(GtkAction *action,LyricShow *lsw);

static  GtkActionEntry ui_menu_entry[]=
{
  {"about_action",                      ////name
   GTK_STOCK_ABOUT,                     ////STOCK ID
   N_("_About us"),                     ////label
   NULL,                                ////accelerator
   N_("About us"),                      ////tooltip
   G_CALLBACK(about_action_callback)   ////callback
  },
  {"search_action",                      ////name
   GTK_STOCK_ABOUT,                     ////STOCK ID
   N_("Search Lyric"),                     ////label
   NULL,                                ////accelerator
   N_("Search Lyric"),                      ////tooltip
   G_CALLBACK(search_action_callback)   ////callback
  }
};

static guint n_ui_menu_entry = G_N_ELEMENTS (ui_menu_entry);



static void
about_action_callback(GtkAction *action,LyricShow *lsw)
{

}

static void
search_action_callback(GtkAction *action,LyricShow *lsw)
{
    lyric_show_search_request(lsw);
}

GtkWidget*
lyric_show_menu_get_for(LyricShow *lsw)
{
    GtkUIManager *ui;
    GtkActionGroup *actiongroup;
    GtkWidget   *menu = NULL;

    g_return_if_fail(lsw != NULL && G_IS_OBJECT(lsw));

    menu = g_object_get_data(lsw,"MENU");

    if(menu == NULL)
    {
        ui = gtk_ui_manager_new();

        gtk_ui_manager_add_ui_from_string(ui,ui_xml,-1,NULL);
        actiongroup = gtk_action_group_new("menu");

#ifdef GETTEXT_PACKAGE
        gtk_action_group_set_translation_domain(actiongroup,GETTEXT_PACKAGE);
#endif

        gtk_action_group_add_actions(actiongroup,ui_menu_entry,n_ui_menu_entry,lsw);

        gtk_ui_manager_insert_action_group(ui,actiongroup,-1);

        g_object_unref(actiongroup);
        menu = gtk_ui_manager_get_widget(ui,"/menu");
        g_signal_connect_swapped(lsw,"destroy",G_CALLBACK(gtk_widget_destroy),menu);
        g_object_set_data(lsw,"MENU",menu);
        g_object_ref(menu);
        g_object_unref(ui);
    }
    return menu;
}
