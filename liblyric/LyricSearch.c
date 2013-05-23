#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#include <stdio.h>
#include <gtk/gtk.h>


#include "LyricSearch.h"
#include "LyricTtSearch.h"
#include "LyricSogouSearch.h"
#include "LyricDownloader.h"

#ifdef GETTEXT_PACKAGE
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

static const gchar *default_search_paths[]={"~/Lyric",".",NULL};
static const gchar *default_lyric_format_array[]={"%a-%t","%n","%t",NULL};
static const gchar *default_lyric_dir="~/Lyric";
static const gchar *default_lyric_name_format = "%n";

struct _LyricSearch
{
	GObject parent;


	gchar *artist;
	gchar *title;
	gchar *album;
	gchar *mrl;             ///media locations

	gchar *media_name;        ///basename
	gchar *mediadir;        ///dirname

	gchar **search_paths;           ///Search paths
	gchar **lyric_format_array;     ///lyric name format
    gchar *lyric_dir;
    gchar *lyric_name_format;

	gchar *lyricfile;               ///path to load lyric
    gchar *lyricfile_w;

	GtkDialog   *mainwin;
	GtkComboBox *engine_box;

	GtkEntry  *artist_entry;
	GtkEntry  *title_entry;
	GtkEntry  *lyric_entry;

	GtkTreeView *lyricview;

	GtkButton *search_button;
	GtkButton *download_button;
	GtkButton *close_button;

	GtkLabel *info_label;

	LyricSearchEngine *engine;
	gchar *default_engine;
	LyricSearchPrivate *priv;
};


#define LYRIC_SEARCH_GET_PRIVATE(o)       (G_TYPE_INSTANCE_GET_PRIVATE ((o), LYRIC_SEARCH_TYPE, LyricSearchPrivate))

struct _LyricSearchPrivate
{
	GSList *engine;
    
    GtkWidget *auto_get_lyric;
    GtkWidget *close_after_download;

	LyricSearchType type;
	LyricDownloader *downloader;

	gchar    *config;
	LyricSearchStatus lss;
    gboolean    auto_get_lyric_mode;
    gboolean    auto_close_download_window;
};

typedef struct 
{
	LyricSearch *lys;
	LyricId id;
}SearchThreadData;

typedef struct 
{
	LyricSearch *lys;
	gchar *uri;
	const gchar *lyricfile;
}DownloadThreadData;

enum
{
	LYRIC_VIEW_ID,
	LYRIC_VIEW_ARTIST,
	LYRIC_VIEW_TITLE,
	LYRIC_VIEW_ALBUM,
	LYRIC_VIEW_URI,
	LYRIC_VIEW_LAST
};

enum{
	STATUS_CHANGED,
	LYRIC_UPDATED,
	SIGNAL_LAST
};

enum
{
	ENGINE_BOX_DESCRIPTION,
	ENGINE_BOX_ENGINE_POINTER,
	ENGINE_BOX_LAST
};

enum
{
    PROP_0,
    PROP_AUTO_GET_lYRIC,
    PROP_AUTO_CLOSE_DOWNLOAD_WINDOW,
};

static GObject*
lyric_search_constructor(GType type,
                         guint n_construct_properties,
                         GObjectConstructParam *construct_properties);

static void
lyric_search_finalize(GObject *object);

static void
lyric_search_set_property(GObject        *object,
                guint               property_id,
                const GValue        *value,
                GParamSpec          *pspec);

static void
lyric_search_get_property(GObject        *object,
                guint               property_id,
                GValue        *value,
                GParamSpec          *pspec);

static void
lyric_search_set_status(LyricSearch *lys,LyricSearchStatus lss);

static void
lyric_search_make_lyricfile(LyricSearch *lys);

static void
on_downloader_error(LyricDownloader *ldl,const gchar *message,LyricSearch *lys);

static void
on_downloader_done(LyricDownloader *ldl,const GString *data,LyricSearch *lys);

static void
lyric_search_entry_activate(GtkEntry *entry,LyricSearch *lys);

static void
lyric_search_lyricview_row_activated(GtkTreeView       *tree_view,
                        GtkTreePath       *path,
                        GtkTreeViewColumn *column,
                        LyricSearch *lys);

static void
lyric_search_lyricview_selection_changed(GtkTreeSelection *treeselection,
                                         LyricSearch *lys);

static void
lyric_search_search_button_clicked(GtkButton *button,LyricSearch *lys);

static void
lyric_search_download_button_clicked(GtkButton *button,LyricSearch *lys);

static void
lyric_search_close_button_clicked(GtkButton *button,LyricSearch *lys);

static gboolean
lyric_search_widget_delete_event(GtkWidget *widget,GdkEvent  *event,LyricSearch *lys);

static void
lyric_search_lyricview_config(LyricSearch *lys);

static void
lyric_search_engine_box_config(LyricSearch *lys);

static void
on_toggl_button_toggled(GtkToggleButton *button,LyricSearch *lys);

static guint LYRIC_SEARCH_SIGNALS[SIGNAL_LAST]={0};


G_DEFINE_TYPE (LyricSearch, lyric_search, G_TYPE_OBJECT)

static void
lyric_search_init(LyricSearch *lys)
{
	lys->engine = lyric_search_get_tt_engine();
	lys->priv = LYRIC_SEARCH_GET_PRIVATE(lys);
}

static void
lyric_search_class_init(LyricSearchClass *class)
{
	GObjectClass *objclass = G_OBJECT_CLASS(class);
	objclass->finalize = lyric_search_finalize;
	objclass->constructor = lyric_search_constructor;

    objclass->set_property = lyric_search_set_property;
    objclass->get_property = lyric_search_get_property;

	LYRIC_SEARCH_SIGNALS[STATUS_CHANGED] =
				g_signal_new ("status-changed",
							G_OBJECT_CLASS_TYPE(class),
							G_SIGNAL_RUN_FIRST,
							G_STRUCT_OFFSET(LyricSearchClass,status_changed),
							NULL,NULL,
							g_cclosure_marshal_VOID__VOID,
							G_TYPE_NONE,0);

	LYRIC_SEARCH_SIGNALS[LYRIC_UPDATED] = 
				g_signal_new ("lyric-updated",
							G_OBJECT_CLASS_TYPE(class),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(LyricSearchClass,lyric_updated),
							NULL,NULL,
							g_cclosure_marshal_VOID__STRING,
							G_TYPE_NONE,1,
							G_TYPE_STRING);

    g_object_class_install_property(objclass,
                                    PROP_AUTO_GET_lYRIC,
                                    g_param_spec_boolean("auto-get-lyric",
                                                         "auto-get-lyric",
                                                         "auto-get-lyric",
                                                         FALSE,
                                                         G_PARAM_READWRITE));
                            

    g_object_class_install_property(objclass,
                                    PROP_AUTO_CLOSE_DOWNLOAD_WINDOW,
                                    g_param_spec_boolean("auto-close-download-window",
                                                         "auto-close-download-window",
                                                         "auto-close-download-window",
                                                         TRUE,
                                                         G_PARAM_CONSTRUCT| 
                                                         G_PARAM_READWRITE));

	g_type_class_add_private (objclass, sizeof (LyricSearchPrivate));
}

static void
lyric_search_set_status(LyricSearch *lys,LyricSearchStatus lss)
{
	if(lys->priv->lss != lss){
		lys->priv->lss = lss;
	}
	switch(lss){
		case LYRIC_SEARCH_STATUS_PREPARING:
		
		break;
		case LYRIC_SEARCH_STATUS_SEARCHING:
			if(lys->priv->type == LYRIC_MANUAL_SEARCH){
				gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(lys->lyricview)));
				gtk_widget_set_sensitive(GTK_WIDGET(lys->search_button),FALSE);
				gtk_widget_set_sensitive(GTK_WIDGET(lys->download_button),FALSE);
			}
		break;
		case LYRIC_SEARCH_STATUS_SEARCHING_FAILED:
		case LYRIC_SEARCH_STATUS_SEARCHING_GET_DATA:
			gtk_widget_set_sensitive(GTK_WIDGET(lys->search_button),TRUE);
		break;
		case LYRIC_SEARCH_STATUS_LOCAL_LYRIC_YES:
		case LYRIC_SEARCH_STATUS_LYRIC_UPDATED:
			g_signal_emit(lys,LYRIC_SEARCH_SIGNALS[LYRIC_UPDATED],0,lys->lyricfile);
		break;
		default:
		break;
	}
//    g_warning("status_changed:%d",lss);
    g_signal_emit(lys,LYRIC_SEARCH_SIGNALS[STATUS_CHANGED],0);
}

LyricSearchStatus
lyric_search_get_status(LyricSearch *lys)
{
	return lys->priv->lss;
}

static GObject*
lyric_search_constructor(GType type,
                         guint n_construct_properties,
                         GObjectConstructParam *construct_properties)
{
	GObject *object;
	const gchar *ui_xml;
	LyricSearch *lys;
	GtkBuilder *build;
	GtkTreeSelection *selection;
	gsize n = 0;

	ui_xml = "/home/wkt/projects/lyricsearch/data/download.glade";

	if(!g_file_test(ui_xml,G_FILE_TEST_EXISTS))
		ui_xml = UIDIR "/download.glae";

	///g_debug("%s:coming",__func__);

	object = G_OBJECT_CLASS(lyric_search_parent_class)->constructor(
						type,
						n_construct_properties,
						construct_properties);

	lys = LYRIC_SEARCH(object);

	lys->priv->engine = g_slist_insert(lys->priv->engine,lyric_search_get_tt_engine(),-1);
///	lys->priv->engine = g_slist_insert(lys->priv->engine,lyric_search_get_sogou_engine(),-1);

	build = gtk_builder_new();
    gtk_builder_set_translation_domain(build,GETTEXT_PACKAGE);
	gtk_builder_add_from_file(build,ui_xml,NULL);

	lys->mainwin = GTK_DIALOG(gtk_builder_get_object(build,"dialog"));

	lys->engine_box = GTK_COMBO_BOX(gtk_builder_get_object(build,"engine_box"));
	lyric_search_engine_box_config(lys);

	lys->artist_entry = GTK_ENTRY(gtk_builder_get_object(build,"artist_entry"));
	lys->title_entry = GTK_ENTRY(gtk_builder_get_object(build,"title_entry"));
	lys->lyric_entry = GTK_ENTRY(gtk_builder_get_object(build,"lyric_entry"));

	g_signal_connect(lys->title_entry,"activate",G_CALLBACK(lyric_search_entry_activate),lys);
	g_signal_connect(lys->artist_entry,"activate",G_CALLBACK(lyric_search_entry_activate),lys);

	lys->lyricview = GTK_TREE_VIEW(gtk_builder_get_object(build,"lyricview"));
	lyric_search_lyricview_config(lys);
	selection = gtk_tree_view_get_selection(lys->lyricview);

	g_signal_connect(lys->lyricview,"row-activated",G_CALLBACK(lyric_search_lyricview_row_activated),lys);
	g_signal_connect(selection,"changed",G_CALLBACK(lyric_search_lyricview_selection_changed),lys);

	lys->search_button = GTK_BUTTON(gtk_builder_get_object(build,"search_button"));
	lys->download_button = GTK_BUTTON(gtk_builder_get_object(build,"download_button"));
	lys->close_button = GTK_BUTTON(gtk_builder_get_object(build,"close_button"));

	lys->info_label = GTK_LABEL(gtk_builder_get_object(build,"info_label"));

	gtk_window_set_focus(GTK_WINDOW(lys->mainwin),lys->search_button);

    lys->priv->auto_get_lyric = GTK_WIDGET(gtk_builder_get_object(build,"auto_get_lyric"));
    lys->priv->close_after_download = GTK_WIDGET(gtk_builder_get_object(build,"close_after_download"));

    g_signal_connect(lys->priv->auto_get_lyric,"toggled",G_CALLBACK(on_toggl_button_toggled),lys);
    g_signal_connect(lys->priv->close_after_download,"toggled",G_CALLBACK(on_toggl_button_toggled),lys);

	g_signal_connect(lys->search_button,"clicked",G_CALLBACK(lyric_search_search_button_clicked),lys);
	g_signal_connect(lys->download_button,"clicked",G_CALLBACK(lyric_search_download_button_clicked),lys);
	g_signal_connect(lys->close_button,"clicked",G_CALLBACK(lyric_search_close_button_clicked),lys);

	g_signal_connect(lys->mainwin,"delete-event",G_CALLBACK(lyric_search_widget_delete_event),lys);

	g_object_unref(G_OBJECT(build));

	return object;
}

static void
lyric_search_finalize(GObject *object)
{
	LyricSearch *lys;
	lys = LYRIC_SEARCH(object);

    if(lys->priv->downloader && G_IS_OBJECT(lys->priv->downloader))
    {
        lyric_down_loader_cancel(lys->priv->downloader);
        g_object_unref(lys->priv->downloader);
    }

	g_free(lys->mrl);
    g_free(lys->artist);
    g_free(lys->title);
    g_free(lys->album);

    if(lys->media_name)
        g_free(lys->media_name);

    if(lys->mediadir)
        g_free(lys->mediadir);

	if(lys->lyric_format_array){
		g_strfreev(lys->lyric_format_array);
	}
    
    if(lys->lyric_name_format)
        g_free(lys->lyric_name_format);

    if(lys->lyric_dir)
        g_free(lys->lyric_dir);

	if(lys->search_paths)
		g_strfreev(lys->search_paths);

    if(lys->lyricfile){
        g_free(lys->lyricfile);
    }

    if(lys->lyricfile_w){
        g_free(lys->lyricfile_w);
    }

	if(lys->priv->engine)
		g_slist_free(lys->priv->engine);

	if(lys->default_engine)
		g_free(lys->default_engine);

	G_OBJECT_CLASS(lyric_search_parent_class)->finalize(object);
}


static void
lyric_search_set_property(GObject        *object,
                guint               property_id,
                const GValue        *value,
                GParamSpec          *pspec)
{
    LyricSearch *lys;
    lys = LYRIC_SEARCH(object);
    switch(property_id)
    {
        case PROP_AUTO_GET_lYRIC:
        {
            gboolean v = g_value_get_boolean(value);
            if(lys->priv->auto_get_lyric_mode != v)
            {
                lys->priv->auto_get_lyric_mode = v;
                g_object_notify(object,"auto-get-lyric");
            }
            if(lys->priv->auto_get_lyric)
            {
                if(v != 
                    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lys->priv->auto_get_lyric)))
                {
                    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lys->priv->auto_get_lyric),
                                                v);
                }
            }
        }
        break;
        case PROP_AUTO_CLOSE_DOWNLOAD_WINDOW:
        {
            gboolean v = g_value_get_boolean(value);
            if(lys->priv->auto_close_download_window != v)
            {
                lys->priv->auto_close_download_window = v;
                g_object_notify(object,"auto-close-download-window");
            }
            if(lys->priv->close_after_download)
            {
                if(v != 
                    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lys->priv->close_after_download)))
                {
                    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lys->priv->close_after_download),
                                                v);
                }
            }
        }
        break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
lyric_search_get_property(GObject        *object,
                guint               property_id,
                GValue        *value,
                GParamSpec          *pspec)
{
    LyricSearch *lys;
    lys = LYRIC_SEARCH(object);
    switch(property_id)
    {
        case PROP_AUTO_GET_lYRIC:
            g_value_set_boolean(value,lys->priv->auto_get_lyric_mode);
        break;
        case PROP_AUTO_CLOSE_DOWNLOAD_WINDOW:
            g_value_set_boolean(value,lys->priv->auto_close_download_window);
        break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

gboolean
lyric_search_set_artist(LyricSearch *lys,const gchar *artist)
{
    gboolean    is_changed = FALSE;
	if(artist != lys->artist){
		g_free(lys->artist);
		if(artist){
			lys->artist = g_strdup(artist);
            is_changed = TRUE;
		}else{
			lys->artist = NULL;
		}
	}
    return is_changed;
}

gboolean
lyric_search_set_title(LyricSearch *lys,const gchar *title)
{
    gboolean    is_changed = FALSE;
	if(title != lys->title){
		g_free(lys->title);
		if(title){
			lys->title = g_strdup(title);
            is_changed = TRUE;
		}else{
			lys->title = NULL;
		}
	}
    return is_changed;
}

gboolean
lyric_search_set_album(LyricSearch *lys,const gchar *album)
{
	gboolean    is_changed = FALSE;
    if(album != lys->album){
		g_free(lys->album);
		if(album){
			lys->album = g_strdup(album);
            is_changed = TRUE;
		}else{
			lys->album = NULL;
		}
	}
    return is_changed;
}

gboolean
lyric_search_set_info(LyricSearch *lys,const gchar *artist,const gchar *title,const gchar *album)
{
    gboolean    is_changed = 
        lyric_search_set_artist(lys,artist) && 
        lyric_search_set_title(lys,title)   &&
        lyric_search_set_album(lys,album);
    return is_changed;
}

void
lyric_search_show_info(LyricSearch *lys)
{
	fprintf(stdout,"artist:%s\n",lys->artist);
	fprintf(stdout,"title :%s\n",lys->title);
    fprintf(stdout,"album :%s\n",lys->album);
}

void
lyric_search_set_mrl(LyricSearch *lys,const gchar *mrl)
{
	gint i =0;
    gchar *path = NULL;
    gchar *name_pt;

	g_free(lys->mrl);
	g_free(lys->media_name);
	g_free(lys->mediadir);

	lys->mrl = NULL;
	lys->media_name = NULL;
	lys->mediadir = NULL;

	if(mrl){
		lys->mrl = g_strdup(mrl);
        if(!g_path_is_absolute(mrl))
        {
            path = g_filename_from_uri(mrl,NULL,NULL);
        }else{
            path = g_strdup(mrl);
        }
		//g_debug("path:%s",path);
		if(path){
			lys->media_name = g_path_get_basename(path);
			lys->mediadir = g_path_get_dirname(path);
			g_free(path);
		}
	}

    ///去除后缀
    name_pt = lys->media_name;
    if(name_pt)
    {
        gint len = strlen(name_pt);
        gint i = len-1;
        for(;i > 0 && len -i < 6;i--)
        {
            if(name_pt[i] == '.')
            {
                name_pt[i] = 0;
                break;
            }
        }
    }

	if(lys->mediadir == NULL){
		lys->mediadir = g_build_filename(g_get_home_dir(),"Lyric",NULL);
	}

}

/***
 * %t title
 * %a artist
 * %b album
 * %n filename
 * %d media directory
 * ~  user home
 * . same as %d
 * */
static gchar *
lyric_search_fmt_string(LyricSearch *lys,const gchar *fmt)
{
	GString *str;
	gchar *pt = (gchar*)fmt;
	if(!pt)
		return NULL;

	str = g_string_new("");
	for(;*pt;pt++){
		if(*pt == '%'){
			switch(*(pt+1)){
			case 't'://title
				str = g_string_append(str,lys->title);
				pt++;
			break;
			case 'a'://artist
				str = g_string_append(str,lys->artist);
				pt++;
			break;
			case 'b'://album
				str = g_string_append(str,lys->album);
				pt++;
			break;
			case 'n'://filename
				str = g_string_append(str,lys->media_name);
				pt++;
			break;
			case 'd'://directory media file
				str = g_string_append(str,lys->mediadir);
				pt++;
			break;
			default:
				str = g_string_append_c(str,*pt);
			break;
			}
		}else if(*pt == '~'){
			str = g_string_append(str,g_get_home_dir());
		}else if(*pt == '.'){
            str = g_string_append(str,lys->mediadir);
        }else{
			str = g_string_append_c(str,*pt);
		}
	}
    if(str->len > 0)
    {
        pt = str->str;
        g_string_free(str,FALSE);
    }else{
        pt=NULL;
    }
	return pt;
}

static void
lyric_search_make_lyricfile(LyricSearch *lys)
{
    gchar *lyric_name;
    gchar *lyricfile;
    gboolean diff_w = FALSE;

    g_free(lys->lyricfile);
    g_free(lys->lyricfile_w);
    lys->lyricfile_w = NULL;
    lys->lyricfile = NULL;

    if(lys->lyric_dir == NULL)
    {
        lyric_search_set_lyric_dir(lys,default_lyric_dir);
    }

    if(!lyric_search_is_ready(lys))
        return;

    if(lys->lyric_name_format)
    {
        lyric_name = lyric_search_fmt_string(lys,lys->lyric_name_format);
    }else{
        lyric_name = lyric_search_fmt_string(lys,default_lyric_name_format);
    }
    if(lyric_name == NULL && lys->title)
    {
        lyric_name = g_strdup(lys->title);
    }
    lyricfile = g_strdup_printf("%s%c%s.lrc",lys->lyric_dir,G_DIR_SEPARATOR,lyric_name);
    if(g_file_test(lyricfile,G_FILE_TEST_EXISTS))
    {
        lys->lyricfile = g_strdup(lyricfile);
    }else{
        gchar **search_paths = lys->search_paths;
        gchar **lyric_format_array = lys->lyric_format_array;
        gint i = 0;
        gint j = 0;
        gchar *path;
        gchar *t_lyricfile;
        if(search_paths == NULL||search_paths[0] == NULL)
        {
            search_paths = default_search_paths;
        }
        if(lyric_format_array == NULL||lyric_format_array[0] == NULL)
        {
            lyric_format_array = default_lyric_format_array;
        }
        for(i=0;search_paths[i];i++)
        {
            path = lyric_search_fmt_string(lys,search_paths[i]);
            t_lyricfile = g_strdup_printf("%s%c%s.lrc",path,G_DIR_SEPARATOR,lyric_name);
            if(g_file_test(t_lyricfile,G_FILE_TEST_IS_REGULAR))
            {
                lys->lyricfile = g_strdup(t_lyricfile);
                if(g_access(t_lyricfile,W_OK) == 0)
                {
                    lys->lyricfile_w = g_strdup(t_lyricfile);
                }
                break;
            }
            g_free(t_lyricfile);
            for(j=0;lyric_format_array[j];j++)
            {
                t_lyricfile = g_strdup_printf("%s%c%s.lrc",path,G_DIR_SEPARATOR,lyric_format_array[j]);
                if(g_file_test(t_lyricfile,G_FILE_TEST_IS_REGULAR))
                {
                    lys->lyricfile = g_strdup(t_lyricfile);
                    if(g_access(t_lyricfile,W_OK) == 0)
                    {
                        lys->lyricfile_w = g_strdup(t_lyricfile);
                    }
                    break;
                }
                g_free(t_lyricfile);
            }
            if(lys->lyricfile)
                break;
            g_free(path);
        }
    }
    if(lys->lyricfile_w == NULL)
    {
        lys->lyricfile_w = g_strdup(lyricfile);
    }
    g_free(lyricfile);
    g_free(lyric_name);
}

gboolean
lyric_search_has_local_lyric(LyricSearch *lys)
{
	return (lys->lyricfile != NULL);
}

void
lyric_search_set_search_paths(LyricSearch *lys,gchar **search_paths)
{
    g_strfreev(lys->search_paths);
    lys->search_paths = lyric_func_strv_dup(search_paths);
}

void
lyric_search_set_lyric_foramt_array(LyricSearch *lys,gchar **formats)
{
    g_strfreev(lys->lyric_format_array);
    lys->lyric_format_array = lyric_func_strv_dup(formats);
}

void
lyric_search_set_lyric_name_format(LyricSearch *lys,gchar *lyric_name_format)
{
    g_free(lys->lyric_name_format);
    lys->lyric_name_format = g_strdup(lyric_name_format);
}

gboolean
lyric_search_set_lyric_dir(LyricSearch *lys,const gchar *lyric_dir)
{
    gboolean res;
    g_free(lys->lyric_dir);
    lys->lyric_dir = lyric_search_fmt_string(lys,lyric_dir);
    res = g_mkdir_with_parents(lys->lyric_dir,0744) == 0;
    if(!res && g_strcmp0(default_lyric_dir,lyric_dir) !=0)
    {
        lyric_search_set_lyric_dir(lys,default_lyric_dir);
    }
    return res;
}

gboolean
lyric_search_is_ready(LyricSearch *lys)
{
    return (lys->title && lys->title[0]);
}

void
lyric_search_set_engine(LyricSearch *lys,LyricSearchEngine* engine)
{
	lys->engine = engine;
}

static void
lyric_search_start_search_lyircid(LyricSearch *lys,LyricId *id)
{
	LyricId orig_id={
		.artist = lys->artist,
		.title = lys->title,
		.album = lys->album,
	};
	if(id == NULL){
		id = &orig_id;
	}
    if((id->artist &&id->artist[0]) || (id->title&&id->title[0]))
    {
        gchar *engine_uri;
        lyric_down_loader_cancel(lys->priv->downloader);
        engine_uri = lys->engine->get_engine_uri(id);
        lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_SEARCHING);
        lyric_down_loader_load(lys->priv->downloader,engine_uri);
        g_free(engine_uri);
    }
}

static GSList*
lyric_search_search_result_parser(LyricSearch *lys,const gchar *data)
{
	LyricId id={
		.artist = lys->artist,
		.title = lys->title,
		.album = lys->album,
	};
	return lys->engine->parser(&id,data);
}

static void
lyric_search_auto_get_lyric(LyricSearch *lys)
{

	lys->priv->type = LYRIC_AUTO_SEARCH;
	lyric_down_loader_cancel(lys->priv->downloader);

    lyric_search_start_search_lyircid(lys,NULL);

}

static void
lyric_search_reset_downloader(LyricSearch *lys)
{
	gpointer tmp = lys->priv->downloader;

    lys->priv->downloader = lyric_down_loader_new();
	g_signal_connect(lys->priv->downloader,"error",G_CALLBACK(on_downloader_error),lys);
	g_signal_connect(lys->priv->downloader,"done",G_CALLBACK(on_downloader_done),lys);
    if(tmp)
    {
        lyric_down_loader_cancel(LYRIC_DOWNLOADER(tmp));
    }
}

static void
lyric_search_present_dialog(LyricSearch *lys,gboolean   download_sensitive)
{
	gtk_entry_set_text(lys->artist_entry,lys->artist?lys->artist:"");
	gtk_entry_set_text(lys->title_entry,lys->title?lys->title:"");
	gtk_entry_set_text(lys->lyric_entry,lys->lyricfile_w);
	gtk_widget_set_sensitive(GTK_WIDGET(lys->download_button),download_sensitive);

    if(lys->priv->auto_get_lyric)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lys->priv->auto_get_lyric),lys->priv->auto_get_lyric_mode);
    }

    if(lys->priv->close_after_download)
    {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lys->priv->close_after_download),
                                        lys->priv->auto_close_download_window);
    }

	gtk_window_present(lys->mainwin);
}

static void
lyric_search_search_button_clicked(GtkButton *button,LyricSearch *lys)
{
	LyricId id={NULL};

	id.artist = (gchar*)gtk_entry_get_text(lys->artist_entry);
	id.title = (gchar*)gtk_entry_get_text(lys->title_entry);

	lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_SEARCHING);
	lyric_search_start_search_lyircid(lys,&id);
}

static void
lyric_search_entry_activate(GtkEntry *entry,LyricSearch *lys)
{
	gtk_button_clicked(lys->search_button);
}


static void
lyric_search_lyricview_row_activated(GtkTreeView       *tree_view,
                        GtkTreePath       *path,
                        GtkTreeViewColumn *column,
                        LyricSearch *lys)
{
	GtkTreeModel *model;
	GtkTreeIter  iter;
	gchar *uri = NULL;

	model = gtk_tree_view_get_model(tree_view);

	gtk_tree_model_get_iter(model,&iter,path);
	gtk_tree_model_get(model,&iter,
						LYRIC_VIEW_URI,&uri,
						-1);
	g_debug("Uri:%s",uri);
	lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_DOWNLOADING);
	lyric_down_loader_load(lys->priv->downloader,uri);
	g_free(uri);
}

static void
lyric_search_lyricview_selection_changed(GtkTreeSelection *treeselection,
                                         LyricSearch *lys)
{
	gboolean selected = gtk_tree_selection_get_selected(treeselection,NULL,NULL);
	gtk_widget_set_sensitive(GTK_WIDGET(lys->download_button),selected);
}


static void
lyric_search_download_button_clicked(GtkButton *button,LyricSearch *lys)
{
	GtkTreeSelection *select;
	GtkTreeIter iter;
	GtkTreeModel *model;

	select = gtk_tree_view_get_selection(lys->lyricview);
	if(gtk_tree_selection_get_selected(select,&model,&iter)){
		gchar *uri = NULL;
		gtk_tree_model_get(model,&iter,
						LYRIC_VIEW_URI,&uri,
						-1);
		lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_DOWNLOADING);
		lyric_down_loader_load(lys->priv->downloader,uri);
		g_free(uri);
	}
}

static void
lyric_search_close_button_clicked(GtkButton *button,LyricSearch *lys)
{
	gtk_widget_hide(GTK_WIDGET(lys->mainwin));
///    gtk_window_iconify(GTK_WINDOW(lys->mainwin));

}

static void
lyric_search_lyricview_config(LyricSearch *lys)
{
	GtkListStore *store;
	GtkTreeViewColumn *column;
	GtkCellRenderer  *cell;

	if(!lys->lyricview || !GTK_IS_TREE_VIEW(lys->lyricview))
		return;

	store = gtk_list_store_new(LYRIC_VIEW_LAST,
								G_TYPE_INT, ///id
								G_TYPE_STRING,///artist
								G_TYPE_STRING,///title
								G_TYPE_STRING,///album
								G_TYPE_STRING///uri
								);

	gtk_tree_view_set_model(lys->lyricview,GTK_TREE_MODEL(store));

	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column(lys->lyricview,column);
	gtk_tree_view_column_set_title(column,_("ID"));
	cell = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column,cell,FALSE);
	gtk_tree_view_column_set_attributes(column,cell,"text",LYRIC_VIEW_ID,NULL);

	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column(lys->lyricview,column);
	gtk_tree_view_column_set_title(column,_("Artist"));
	cell = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column,cell,FALSE);
	gtk_tree_view_column_set_attributes(column,cell,"text",LYRIC_VIEW_ARTIST,NULL);

	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column(lys->lyricview,column);
	gtk_tree_view_column_set_title(column,_("Title"));
	cell = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column,cell,FALSE);
	gtk_tree_view_column_set_attributes(column,cell,"text",LYRIC_VIEW_TITLE,NULL);

}

static void
lyric_search_lyricview_update(LyricSearch *lys,GSList *search_result)
{
	GtkListStore *store;
	GtkTreeIter iter;
	LyricId *id;
	GSList *l;

	if(!lys->lyricview || !GTK_IS_TREE_VIEW(lys->lyricview))
		return;

	store = GTK_LIST_STORE(gtk_tree_view_get_model(lys->lyricview));
	gtk_list_store_clear(store);

	for(l=search_result;l;l=l->next){
		id = (LyricId*)l->data;
		gtk_list_store_append(store,&iter);
		gtk_list_store_set(store,&iter,
						LYRIC_VIEW_ID,id->no,
						LYRIC_VIEW_ARTIST,id->artist,
						LYRIC_VIEW_TITLE,id->title,
						LYRIC_VIEW_ALBUM,id->album,
						LYRIC_VIEW_URI,id->uri,
						-1);
	}
}

static gboolean
lyric_search_widget_delete_event(GtkWidget *widget,GdkEvent  *event,LyricSearch *lys)
{
	gtk_widget_hide(widget);
///    gtk_window_iconify(GTK_WINDOW(widget));
	return TRUE;
}

static void
on_toggl_button_toggled(GtkToggleButton *button,LyricSearch *lys)
{
    gboolean    active = gtk_toggle_button_get_active(button);
    if(button == lys->priv->auto_get_lyric)
    {
        g_object_set(lys,"auto-get-lyric",active,NULL);
    }else if(button == lys->priv->close_after_download)
    {
        g_object_set(lys,"auto-close-download-window",active,NULL);
    }
}

static void
lyric_search_engine_box_change(GtkComboBox *engine_box,LyricSearch *lys)
{
	GtkTreeModel *model;
	GtkTreeIter  iter;
	LyricSearchEngine *engine = NULL;

	if(gtk_combo_box_get_active_iter(engine_box,&iter)){
		model = gtk_combo_box_get_model(engine_box);
		gtk_tree_model_get(model,&iter,
					ENGINE_BOX_ENGINE_POINTER,&engine,
					-1);
		if(engine){
			lyric_search_set_engine(lys,engine);
		}
	}
}

static void
lyric_search_engine_box_config(LyricSearch *lys)
{
	GtkListStore *store;
	GtkTreeIter iter;
	GtkCellRenderer *cell;
	LyricSearchEngine *engine;
	GSList *l;
	gsize n =0;

	store = gtk_list_store_new(ENGINE_BOX_LAST,
								G_TYPE_STRING,///description
								G_TYPE_POINTER///engine pointer
								);

	gtk_combo_box_set_model(lys->engine_box,GTK_TREE_MODEL(store));

	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (lys->engine_box), cell, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (lys->engine_box), cell,
								"text", ENGINE_BOX_DESCRIPTION, 
								NULL);

	g_signal_connect(lys->engine_box,"changed",G_CALLBACK(lyric_search_engine_box_change),lys);
	
	for(l = lys->priv->engine;l;l=l->next){
		engine = l->data;
		gtk_list_store_append(store,&iter);
		gtk_list_store_set(store,&iter,
						ENGINE_BOX_DESCRIPTION,engine->description,
						ENGINE_BOX_ENGINE_POINTER,engine,
						-1);
		if(g_strcmp0(engine->description,lys->default_engine) == 0){
			gtk_combo_box_set_active_iter(lys->engine_box,&iter);
		}
	}
	if(gtk_combo_box_get_active(lys->engine_box)<0){
		gtk_combo_box_set_active(lys->engine_box,0);
	}
}

static void
on_downloader_error(LyricDownloader *ldl,const gchar *message,LyricSearch *lys)
{
	if(lys->priv->downloader != ldl)
        return;
    LyricSearchStatus lss =  lyric_search_get_status(lys);
	switch(lss){
		case LYRIC_SEARCH_STATUS_SEARCHING:
			//to do
			lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_SEARCHING_FAILED);
		break;
		case LYRIC_SEARCH_STATUS_DOWNLOADING:
			lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_DOWNLOADING_FAILED);
		break;
		default:
			g_warning("%s:should not come(%d) ...",__FUNCTION__,lss);
		break;
	}
}

static void
on_downloader_done(LyricDownloader *ldl,const GString *data,LyricSearch *lys)
{
	if(lys->priv->downloader != ldl)
    {
        g_object_unref(ldl);
        return;
    }

	LyricSearchStatus lss = lyric_search_get_status(lys);

	switch(lss){
		case LYRIC_SEARCH_STATUS_SEARCHING:
			if(data == NULL){
                lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_SEARCHING_FAILED);
				return;
			}
			lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_SEARCHING_GET_DATA);
			GSList *l = lyric_search_search_result_parser(lys,data->str);
///			lyric_func_lyricid_list(l);
            if(!lys->priv->auto_get_lyric_mode && lys->priv->type == LYRIC_AUTO_SEARCH)
            {
                lys->priv->type = LYRIC_MANUAL_SEARCH;
                if(l)
                    lyric_search_present_dialog(lys,FALSE);
            }
			switch(lys->priv->type){
				case LYRIC_AUTO_SEARCH:
					if(l){
						lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_DOWNLOADING);
						lyric_down_loader_load(lys->priv->downloader,((LyricId*)l->data)->uri);
					}else{
						lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_SEARCHING_FAILED);
					}
				break;
				case LYRIC_MANUAL_SEARCH:
					if(l){
						lyric_search_lyricview_update(lys,l);
					}else{
						lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_SEARCHING_FAILED);
					}
				break;
				default:
					///g_warning("%s:should not come to this type ...",__FUNCTION__);
				break;
			}
			lyric_func_free_lyricid_list(l);
		break;
		case LYRIC_SEARCH_STATUS_DOWNLOADING:
			if(data == NULL){
                lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_DOWNLOADING_FAILED);
				return;
			}
			lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_DOWNLOADING_GET_DATA);
			if(lyric_func_save_data(lys->lyricfile_w,data->str,data->len,NULL)){
				g_free(lys->lyricfile);
				lys->lyricfile = g_strdup(lys->lyricfile_w);
				lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_LYRIC_UPDATED);
                if(lys->priv->auto_close_download_window)
                {
                    gtk_widget_hide(lys->mainwin);
                }
			}else{
				lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_SAVEING_DATA_FAILED);
			}
		break;
		default:
			///g_warning("%s:should not come to this status(%d):%s ...",__FUNCTION__,lss,data?data->str:"");
		break;
	}
}

gboolean
lyric_search_manual_get_lyric(LyricSearch *lys)
{
	lys->priv->type = LYRIC_MANUAL_SEARCH;
	lyric_search_reset_downloader(lys);
	if(!lys->mainwin || !GTK_IS_WINDOW(lys->mainwin)){
		GtkDialog *msg;
		msg = GTK_DIALOG(gtk_message_dialog_new(NULL,
						GTK_DIALOG_MODAL,
						GTK_MESSAGE_ERROR,
						GTK_BUTTONS_CLOSE,
						_("Can't get a window")));
		gtk_dialog_run(msg);
		gtk_widget_destroy(msg);
		return FALSE;
	}
    lyric_search_lyricview_update(lys,NULL);
    lyric_search_start_search_lyircid(lys,NULL);
    lyric_search_present_dialog(lys,FALSE);
	return TRUE;
}

void
lyric_search_find_lyric(LyricSearch *lys)
{
    lyric_search_make_lyricfile(lys);
    lys->priv->type = LYRIC_SEARCH_NONE;
///    lyric_search_show_info(lys);
	if(lyric_search_has_local_lyric(lys)){
		lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_LOCAL_LYRIC_YES);
	}else{
        lyric_search_reset_downloader(lys);
        lyric_search_auto_get_lyric(lys);
    }
}

void
lyric_search_hide(LyricSearch *lys)
{
	gtk_widget_hide(GTK_WIDGET(lys->mainwin));
}

const gchar*
lyric_search_get_lyricfile(LyricSearch* lys)
{
	return (const gchar*)lys->lyricfile;
}

LyricSearch*
lyric_search_new(void)
{
	return LYRIC_SEARCH(g_object_new(LYRIC_SEARCH_TYPE,NULL));
}



#ifdef test_lyric_search

#include "LyricShow.h"
#include "LyricShowTreeView.h"


#if 0
int main(int argc,char **argv)
{
	GtkWidget *win;
	GtkWidget *lsw;
	GtkWidget *sw;
	
	gtk_init(&argc,&argv);
	lsw = lyric_show_tree_view_new();
	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	sw = gtk_scrolled_window_new(NULL,NULL);
	gtk_scrolled_window_set_policy(sw,GTK_POLICY_NEVER,GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(win),sw);
	gtk_container_add(GTK_CONTAINER(sw),lsw);
	lyric_show_set_lyric(LYRIC_SHOW(lsw),argv[1]);
	lyric_show_set_time(LYRIC_SHOW(lsw),13034);
	gtk_window_resize(win,250,300);
	gtk_window_set_position(win,GTK_WIN_POS_CENTER_ALWAYS);
	gtk_window_set_opacity(win,0);
	gtk_widget_show_all(win);
	g_signal_connect(win,"destroy",G_CALLBACK(gtk_main_quit),NULL);
	gtk_main();
	return 0;
}

#else
int main(int argc,char **argv)
{
	LyricSearch *lys;
	GSList *l;
	gtk_init(&argc,&argv);
	g_thread_init(NULL);
	lys = lyric_search_new();
	lyric_search_set_info(lys,"S.H.E",argv[1],NULL);
	////lyric_search_auto_get_lyric(lys);
	lyric_search_find_lyric(lys);
    lyric_search_find_lyric(lys);
    lyric_search_find_lyric(lys);
    g_signal_connect(lys->mainwin,"delete-event",G_CALLBACK(gtk_main_quit),NULL);
	gtk_main();
	return 0;
}
#endif

#endif ///test_lyric_search
