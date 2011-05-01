#include <stdio.h>
#include <gtk/gtk.h>
#include "LyricSearch.h"
#include "LyricTtSearch.h"
#include "LyricSogouSearch.h"
#include "LyricDownloader.h"

struct _LyricSearch
{
	GObject parent;
	gchar *artist;
	gchar *title;
	gchar *album;
	gchar *mrl;      ///media locations
	gchar *filename; ///basename
	gchar *mediadir;  ///dirname
	gchar **search_paths;   ///Search paths
	gchar **lyric_format_array;    ///lyric name format
	gchar *lyricfile;  ///path to load lyric
	gchar *lyric_save_to;

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

	LyricSearchType type;
	LyricDownloader *downloader;

	GKeyFile *confkey;
	gchar    *config;
	LyricSearchStatus lss;
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

enum{
	PROPERTY_LYRIC_NAME_FMT,
	PROPERTY_LYRIC_PATH_LIST,
	PROPERTY_DEFAULT_ENGINE,
	PROPERTY_LYRIC_SEARCH_GROUP,
	PROPERTY_LAST_ALL
};

static const gchar *property_names[PROPERTY_LAST_ALL+1]=
{
	[PROPERTY_LYRIC_NAME_FMT]="lyric_fmtv",
	[PROPERTY_LYRIC_PATH_LIST]="search_pathv",
	[PROPERTY_DEFAULT_ENGINE]="default_engine",
	[PROPERTY_LYRIC_SEARCH_GROUP]="lyric_search",
	[PROPERTY_LAST_ALL] = NULL
};

static GObject*
lyric_search_constructor(GType type,
                         guint n_construct_properties,
                         GObjectConstructParam *construct_properties);

static void
lyric_search_finalize(GObject *object);

static void
lyric_search_set_status(LyricSearch *lys,LyricSearchStatus lss);

static void
lyric_search_save_config(LyricSearch *lys);

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
		ui_xml = UIDIR"/download.glae";

	///g_debug("%s:coming",__func__);

	object = G_OBJECT_CLASS(lyric_search_parent_class)->constructor(
						type,
						n_construct_properties,
						construct_properties);

	lys = LYRIC_SEARCH(object);

	lys->priv->engine = g_slist_insert(lys->priv->engine,lyric_search_get_tt_engine(),-1);
///	lys->priv->engine = g_slist_insert(lys->priv->engine,lyric_search_get_sogou_engine(),-1);

	lys->priv->downloader = lyric_down_loader_new();


	build = gtk_builder_new();
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

	g_signal_connect(lys->priv->downloader,"error",G_CALLBACK(on_downloader_error),lys);
	g_signal_connect(lys->priv->downloader,"done",G_CALLBACK(on_downloader_done),lys);

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

	lyric_search_save_config(lys);

	if(lys->default_engine)
		g_free(lys->default_engine);

	if(lys->lyric_format_array){
		g_strfreev(lys->lyric_format_array);
	}

	if(lys->search_paths)
		g_strfreev(lys->search_paths);

	if(lys->priv->engine)
		g_slist_free(lys->priv->engine);

    if(lys->lyricfile){
        g_free(lys->lyricfile);
    }

    if(lys->lyric_save_to){
        g_free(lys->lyric_save_to);
    }

	G_OBJECT_CLASS(lyric_search_parent_class)->finalize(object);
}


void
lyric_search_set_info(LyricSearch *lys,const gchar *artist,const gchar *title,const gchar *album)
{
	if(artist != lys->artist){
		g_free(lys->artist);
		if(artist){
			lys->artist = g_strdup(artist);
		}else{
			lys->artist = NULL;
		}
	}

	if(title != lys->title){
		g_free(lys->title);
		if(title){
			lys->title = g_strdup(title);
		}else{
			lys->title = NULL;
		}
	}

	if(album != lys->album){
		g_free(lys->album);
		if(album){
			lys->album = g_strdup(album);
		}else{
			lys->album = NULL;
		}
	}
	lyric_search_lyric_set_lyric_paths(lys);
}

void
lyric_search_set_mrl(LyricSearch *lys,const gchar *mrl)
{
	gchar *path = NULL;
	gint i =0;

	g_free(lys->mrl);
	g_free(lys->filename);
	g_free(lys->mediadir);

	lys->mrl = NULL;
	lys->filename = NULL;
	lys->mediadir = NULL;

	if(mrl){
		lys->mrl = g_strdup(mrl);
		path = g_filename_from_uri(mrl,NULL,NULL);
		//g_debug("path:%s",path);
		if(path){
			lys->filename = g_path_get_basename(path);
			lys->mediadir = g_path_get_dirname(path);
			g_free(path);
		}
	}

	path = lys->filename;
	if(path){
		gsize len = 0;
		len = strlen(path);
		for(i=len;len-i<5 && len-i>1;i--){
			if(path[i] == '.'){
				path[i] = 0;
				break;
			}
		}
	}

	if(lys->mediadir == NULL){
		lys->mediadir = g_build_filename(g_get_home_dir(),"Lyric",NULL);
	}
	lyric_search_lyric_set_lyric_paths(lys);
}

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
			case 'f'://filename
				str = g_string_append(str,lys->filename);
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
		}else{
			str = g_string_append_c(str,*pt);
		}
	}
	pt = str->str;
	g_string_free(str,FALSE);
	return pt;
}

gboolean
lyric_search_has_local_lyric(LyricSearch *lys)
{
	return (lys->lyricfile != NULL);
}

void
lyric_search_lyric_set_lyric_paths(LyricSearch *lys)
{
	gboolean has_lyric = FALSE;
	gboolean has_lyric_save = FALSE;
	gint i,j;
	gchar *lyricdir = NULL;
	gchar *lyricname = NULL;
	gchar *lyricfile = NULL;
	gchar *pathvbackup[] = {"~/Lyric",lys->mediadir,NULL};
	gchar *fmtvbackup[] = {"%a-%t.lrc","%f.lrc",NULL};
	gchar **pathv;
	gchar **fmtv;

	pathv = lys->search_paths;
	fmtv = lys->lyric_format_array;

	g_free(lys->lyricfile);
	lys->lyricfile = NULL;

	g_free(lys->lyric_save_to);
	lys->lyric_save_to = NULL;

	if(pathv == NULL)
		pathv = pathvbackup;

	if(fmtv == NULL)
		fmtv = fmtvbackup;

	for(i=0;!has_lyric && pathv[i];i++){
		lyricdir = lyric_search_fmt_string(lys,pathv[i]);
		for(j=0;fmtv[j];j++){
			lyricname = lyric_search_fmt_string(lys,fmtv[j]);
			lyricfile = g_build_filename(lyricdir,lyricname,NULL);
			g_free(lyricname);
			///fprintf(stderr,"%s--lyric file :%s\n",__func__,lyricfile);
			if(g_file_test(lyricfile,G_FILE_TEST_EXISTS)){
				has_lyric = TRUE;
				lys->lyricfile = lyricfile;
				break;
			}
			g_free(lyricfile);
		}
		g_free(lyricdir);
		lyricdir = NULL;
	}

	if(!has_lyric){
		for(i=0;!has_lyric_save && pathv[i];i++){
			lyricdir = lyric_search_fmt_string(lys,pathv[i]);
			g_mkdir_with_parents(lyricdir,0744);
			if(g_file_test(lyricdir,G_FILE_TEST_IS_DIR|G_FILE_TEST_IS_EXECUTABLE)){
				lyricname = lyric_search_fmt_string(lys,fmtv[0]);
				lys->lyric_save_to = g_build_filename(lyricdir,lyricname,NULL);
				g_free(lyricname);
				has_lyric_save = TRUE;
			}
			g_free(lyricdir);
			lyricdir = NULL;
		}
	}else{
		lys->lyric_save_to = g_strdup(lys->lyricfile);
	}
	///g_debug("lyricfile:%s,lyric_save_to:%s",lys->lyricfile ,lys->lyric_save_to);
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
	gchar *engine_uri;
	lyric_down_loader_cancel(lys->priv->downloader);
	engine_uri = lys->engine->get_engine_uri(id);
	lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_SEARCHING);
	lyric_down_loader_load(lys->priv->downloader,engine_uri);
	g_free(engine_uri);
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

void
lyric_search_auto_get_lyric(LyricSearch *lys)
{
	gboolean ret_bl = TRUE;

	lys->priv->type = LYRIC_AUTO_SEARCH;
	lyric_down_loader_cancel(lys->priv->downloader);

	if(lyric_search_has_local_lyric(lys)){
		lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_LOCAL_LYRIC_YES);
	}else{
		lyric_search_start_search_lyircid(lys,NULL);
	}
}

gboolean
lyric_search_manual_get_lyric(LyricSearch *lys)
{
	lys->priv->type = LYRIC_MANUAL_SEARCH;
	lyric_down_loader_cancel(lys->priv->downloader);
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
	gtk_entry_set_text(lys->artist_entry,lys->artist?lys->artist:"");
	gtk_entry_set_text(lys->title_entry,lys->title?lys->title:"");
	gtk_entry_set_text(lys->lyric_entry,lys->lyricfile);
	gtk_widget_set_sensitive(GTK_WIDGET(lys->download_button),FALSE);

	gtk_window_present(lys->mainwin);
	return TRUE;
}

void
lyric_search_hide(LyricSearch *lys)
{
	gtk_widget_hide(GTK_WIDGET(lys->mainwin));
}

static void
lyric_search_load_config(LyricSearch *lys,const gchar *conffile)
{
	GKeyFile *key;
	GKeyFileFlags flags = 0;

	if(!conffile || !(*conffile)){
		g_warning("config file can not be skybook");
		return ;
	}

	if(g_strcmp0(lys->priv->config,conffile) != 0){
		g_free((gpointer)lys->priv->config);
		lys->priv->config = g_strdup(conffile);
	}

	if(lys->priv->confkey == NULL)
		lys->priv->confkey = g_key_file_new();

	key = lys->priv->confkey;
	flags = ~flags;
	g_key_file_load_from_file(key,conffile,flags,NULL);

	lys->lyric_format_array = g_key_file_get_string_list(key,
			property_names[PROPERTY_LYRIC_SEARCH_GROUP],
			property_names[PROPERTY_LYRIC_NAME_FMT],
			NULL,NULL);

	lys->search_paths = g_key_file_get_string_list(key,
			property_names[PROPERTY_LYRIC_SEARCH_GROUP],
			property_names[PROPERTY_LYRIC_PATH_LIST],
			NULL,NULL);

	lys->default_engine = g_key_file_get_string(key,
			property_names[PROPERTY_LYRIC_SEARCH_GROUP],
			property_names[PROPERTY_DEFAULT_ENGINE],
			NULL);
}

static void
lyric_search_save_config(LyricSearch *lys)
{
	FILE *fp;
	gchar *data;
	gsize n = 0;
	GKeyFile *key;

	if(!lys->priv->config || !lys->priv->confkey){
		g_warning("Thing is NULL??You crazy ?");
		return ;
	}

	key = lys->priv->confkey;

	fp = fopen(lys->priv->config,"w");
	if(fp == NULL){
		g_warning("fopen(%s):%m",lys->priv->config);
	}

	g_key_file_set_string_list(key,
			property_names[PROPERTY_LYRIC_SEARCH_GROUP],
			property_names[PROPERTY_LYRIC_NAME_FMT],
			lys->lyric_format_array,-1);

	g_key_file_set_string_list(key,
			property_names[PROPERTY_LYRIC_SEARCH_GROUP],
			property_names[PROPERTY_LYRIC_PATH_LIST],
			lys->search_paths,-1);

	g_key_file_set_string(key,
			property_names[PROPERTY_LYRIC_SEARCH_GROUP],
			property_names[PROPERTY_LYRIC_NAME_FMT],
			lys->default_engine);

	data = g_key_file_to_data(lys->priv->confkey,&n,NULL);
	if(data){
		if(fwrite(data,1,n,fp) != n){
			g_warning("fwrite(%s):%m",lys->priv->config);
		}
		g_free(data);
	}
	fclose(fp);
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
	return TRUE;
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
	LyricSearchStatus lss = lyric_search_get_status(lys);

	switch(lss){
		case LYRIC_SEARCH_STATUS_SEARCHING:
			if(data == NULL){
                lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_SEARCHING_FAILED);
				return;
			}
			lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_SEARCHING_GET_DATA);
			GSList *l = lyric_search_search_result_parser(lys,data->str);
			lyric_func_lyricid_list(l);
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
					g_warning("%s:should not come to this type ...",__FUNCTION__);
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
			if(lyric_func_save_data(lys->lyric_save_to,data->str,data->len,NULL)){
				g_free(lys->lyricfile);
				lys->lyricfile = g_strdup(lys->lyric_save_to);
				lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_LYRIC_UPDATED);
			}else{
				lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_SAVEING_DATA_FAILED);
			}
		break;
		default:
			g_warning("%s:should not come to this status(%d) ...",__FUNCTION__,lss);
		break;
	}
}

#ifdef test_lyric_search

#include "LyricShow.h"
#include "LyricShowTreeView.h"

void
lyric_search_show_info(LyricSearch *lys)
{
	fprintf(stdout,"artist:%s\n",lys->artist);
	fprintf(stdout,"title :%s\n",lys->title);
}

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
	lyric_search_load_config(lys,"/tmp/lll");
	lyric_search_set_info(lys,"S.H.E",argv[1],NULL);
	///lyric_search_auto_get_lyric(lys);
	lyric_search_manual_get_lyric(lys);
	gtk_main();
	return 0;
}
#endif

#endif ///test_lyric_search
