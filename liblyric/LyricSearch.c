#include <stdio.h>
#include <gtk/gtk.h>
#include "LyricSearch.h"
#include "LyricTtSearch.h"

struct _LyricSearch
{
	GObject parent;
	gchar *artist;
	gchar *title;
	gchar *album;
	gchar *mrl;      ///media locations
	gchar *filename; ///basename
	gchar *mediadir;  ///dirname
	gchar *lyricfile;  ///lyric file's path
	gchar **pathv;   ///Search paths
	gchar **fmtv;    ///lyric name format

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
	FILE *fpw;
	GSList *search_result;
	LyricSearchEngine **engine_array;
	const gchar *config;
	GKeyFile *confkey;

	guint io_watch_tag;
	gboolean is_show;
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
	LYRIC_INFO_CHANGE,
	LYRIC_FILE_CHANGE,
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
lyric_search_file_changed(LyricSearch *lys,const gchar *mrl);

static guint LYRIC_SEARCH_SIGNALS[SIGNAL_LAST]={0};

static gpointer 
lyric_search_search_thread(SearchThreadData* sthd)
{
	LyricSearch *lys = sthd->lys;

	lyric_func_free_lyricid_list(lys->priv->search_result);
	lys->priv->search_result = lys->engine->func(&(sthd->id));
	if(lys->priv->search_result){
		lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_SEARCH_OK);
	}else{
		lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_SEARCH_FAILED);
	}
	g_free(sthd);
	g_debug("%s:exit",__func__);
	return NULL;
}

static void
lyric_search_search_button_clicked(GtkButton *button,LyricSearch *lys)
{
	SearchThreadData *sthd;
	sthd = g_new0(SearchThreadData,1);
	sthd->lys = lys;

	sthd->id.artist = (gchar*)gtk_entry_get_text(lys->artist_entry);
	sthd->id.title = (gchar*)gtk_entry_get_text(lys->title_entry);

	lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_SEARCHING);
	if(g_thread_create((GThreadFunc)lyric_search_search_thread,sthd,FALSE,NULL) == NULL){
		lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_SEARCH_FAILED);
		g_free(sthd);
	}
}

static void
lyric_search_entry_activate(GtkEntry *entry,LyricSearch *lys)
{
	gtk_button_clicked(lys->search_button);
}

static gpointer 
lyric_search_get_and_save_thread(DownloadThreadData *dthd)
{
	LyricSearch *lys = dthd->lys;
	if(lyric_func_save_lyric(dthd->uri,dthd->lyricfile,NULL)){
		lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_DOWNLOAD_OK);
	}else{
		lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_DOWNLOAD_FAILED);
	}
	g_free(dthd->uri);
	g_free(dthd);
	g_debug("%s:exit",__func__);
	return NULL;
}

static void
lyric_search_lyricview_row_activated(GtkTreeView       *tree_view,
                        GtkTreePath       *path,
                        GtkTreeViewColumn *column,
                        LyricSearch *lys)
{
	GtkTreeModel *model;
	GtkTreeIter  iter;
	DownloadThreadData *dthd = g_new0(DownloadThreadData,1);
	dthd->lys = lys;

	model = gtk_tree_view_get_model(tree_view);
	dthd->lyricfile = gtk_entry_get_text(lys->lyric_entry);

	gtk_tree_model_get_iter(model,&iter,path);
	gtk_tree_model_get(model,&iter,
						LYRIC_VIEW_URI,&(dthd->uri),
						-1);
	g_debug("Uri:%s",dthd->uri);
	lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_DOWNLOADING);
	if(g_thread_create((GThreadFunc)lyric_search_get_and_save_thread,dthd,FALSE,NULL) == NULL){
		lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_DOWNLOAD_FAILED);
		g_free(dthd->uri);
		g_free(dthd);
	}
}

static void
lyric_search_download_button_clicked(GtkButton *button,LyricSearch *lys)
{
	GtkTreeSelection *select;
	GtkTreeIter iter;
	GtkTreeModel *model;

	select = gtk_tree_view_get_selection(lys->lyricview);
	if(gtk_tree_selection_get_selected(select,&model,&iter)){
		DownloadThreadData *dthd = g_new0(DownloadThreadData,1);
		dthd->lys = lys;
		gtk_tree_model_get(model,&iter,
						LYRIC_VIEW_URI,&(dthd->uri),
						-1);
		lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_DOWNLOADING);
		if(g_thread_create((GThreadFunc)lyric_search_get_and_save_thread,dthd,FALSE,NULL) == NULL){
			lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_DOWNLOAD_FAILED);
			g_free(dthd->uri);
			g_free(dthd);
		}
	}
}

static void
lyric_search_close_button_clicked(GtkButton *button,LyricSearch *lys)
{
	gtk_widget_hide(GTK_WIDGET(lys->mainwin));
	lys->priv->is_show = FALSE;
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
lyric_search_lyricview_update(LyricSearch *lys)
{
	GtkListStore *store;
	GtkTreeIter iter;
	LyricId *id;
	GSList *l;

	if(!lys->lyricview || !GTK_IS_TREE_VIEW(lys->lyricview))
		return;

	store = GTK_LIST_STORE(gtk_tree_view_get_model(lys->lyricview));
	gtk_list_store_clear(store);

	for(l=lys->priv->search_result;l;l=l->next){
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
lyric_search_mainwin_hide(GtkWidget *widget,LyricSearch *lys)
{
	lys->priv->is_show = FALSE;
	g_warning("main window hide");
}

static void
lyric_search_mainwin_show(GtkWidget *widget,LyricSearch *lys)
{
	lys->priv->is_show = TRUE;
	g_warning("main window show");
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
	LyricSearchEngine **engine_array;
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
	engine_array = lys->priv->engine_array;
	for(n=0;engine_array[n];n++){
		gtk_list_store_append(store,&iter);
		gtk_list_store_set(store,&iter,
						ENGINE_BOX_DESCRIPTION,engine_array[n]->description,
						ENGINE_BOX_ENGINE_POINTER,engine_array[n],
						-1);
		if(g_strcmp0(engine_array[n]->description,lys->default_engine) == 0){
			gtk_combo_box_set_active_iter(lys->engine_box,&iter);
		}
	}
	if(gtk_combo_box_get_active(lys->engine_box)<0){
		gtk_combo_box_set_active(lys->engine_box,0);
	}
}

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
	class->media_file_changed = lyric_search_file_changed;

	LYRIC_SEARCH_SIGNALS[LYRIC_INFO_CHANGE] = 
				g_signal_new ("lyric-info-changed",
							G_TYPE_FROM_CLASS(class),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(LyricSearchClass,lyric_info_changed),
							NULL,NULL,
							g_cclosure_marshal_VOID__VOID,
							G_TYPE_NONE,0);

	LYRIC_SEARCH_SIGNALS[LYRIC_FILE_CHANGE] = 
				g_signal_new ("media-file-changed",
							G_OBJECT_CLASS_TYPE(class),
							G_SIGNAL_RUN_LAST,
							G_STRUCT_OFFSET(LyricSearchClass,media_file_changed),
							NULL,NULL,
							g_cclosure_marshal_VOID__STRING,
							G_TYPE_NONE,1,
							G_TYPE_STRING);


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
	fprintf(lys->priv->fpw,"%d\n",lss);
	fflush(lys->priv->fpw);
}

static gboolean
lyric_search_io_watch (GIOChannel *source,
                       GIOCondition condition,
                       LyricSearch *lys)
{
	gchar *line = NULL;
	gint64 st_id = -1;
	gsize n = 0;
	GIOStatus  ios;
	gint can_sensitive = 0;//not change

	ios = g_io_channel_read_line(source,&line,&n,NULL,NULL);

	if(!lys->priv->is_show){
		can_sensitive = 2;
		goto ext;
	}

	if(line){
		st_id = g_ascii_strtoll(line,NULL,0);
	}
	gtk_label_set_text(lys->info_label,"");
	switch(st_id){
	case LYRIC_SEARCH_STATUS_NONE:
	case LYRIC_SEARCH_STATUS_LOCAL_LYRIC_YES:
	case LYRIC_SEARCH_STATUS_AUTO_GET_LYRIC_FAIL:
	case LYRIC_SEARCH_STATUS_AUTO_GET_LYRIC_OK:
	break;
	case LYRIC_SEARCH_STATUS_SEARCHING:
		can_sensitive = 1; //insensitive
	break;
	case LYRIC_SEARCH_STATUS_SEARCH_OK:
		can_sensitive = 2; //sensitive
		lyric_search_lyricview_update(lys);
	break;
	case LYRIC_SEARCH_STATUS_SEARCH_FAILED:
		lyric_search_lyricview_update(lys);
		gtk_label_set_markup(lys->info_label,_("<b>Can't find lyric</b>"));
		can_sensitive = 2; //sensitive
	break;
	case LYRIC_SEARCH_STATUS_DOWNLOADING:
		can_sensitive = 1; //insensitive
	break;
	case LYRIC_SEARCH_STATUS_DOWNLOAD_OK:
		gtk_label_set_text(lys->info_label,_("lyric updated"));
		can_sensitive = 2; //sensitive
		g_signal_emit(lys,LYRIC_SEARCH_SIGNALS[LYRIC_UPDATED],0,lys->lyricfile);
	break;
	case LYRIC_SEARCH_STATUS_DOWNLOAD_FAILED:
		gtk_label_set_markup(lys->info_label,_("<b>Failed to update lyric</b>"));
		can_sensitive = 2; //sensitive
	break;
	case LYRIC_SEARCH_STATUS_LAST:
	default:
		st_id = LYRIC_SEARCH_STATUS_LAST;
		g_debug("totally wrong");
	}

	lys->priv->lss = (LyricSearchStatus)st_id;

	ext:
	if(can_sensitive >0){
		can_sensitive --;
		gtk_widget_set_sensitive(lys->download_button,can_sensitive);
		gtk_widget_set_sensitive(lys->lyricview,can_sensitive);
		gtk_widget_set_sensitive(lys->search_button,can_sensitive);
	}
	g_free(line);
	return TRUE;
}

static GObject*
lyric_search_constructor(GType type,
                         guint n_construct_properties,
                         GObjectConstructParam *construct_properties)
{
	GObject *object;
	int pipefd[2];
	const gchar *ui_xml;
	LyricSearch *lys;
	GtkBuilder *build;
	gsize n = 0;
	LyricSearchEngine **engine_array;
	LyricSearchEngine *enginev[]={lyric_search_get_tt_engine(),NULL};
	n = G_N_ELEMENTS(enginev);

	ui_xml = "/home/wkt/projects/lyricsearch/data/download.glade";

	if(!g_file_test(ui_xml,G_FILE_TEST_EXISTS))
		ui_xml = UIDIR"/download.glae";

	g_debug("%s:coming",__func__);

	object = G_OBJECT_CLASS(lyric_search_parent_class)->constructor(
						type,
						n_construct_properties,
						construct_properties);

	pipe(pipefd);
	lys = LYRIC_SEARCH(object);
	lys->priv->fpw = fdopen(pipefd[1],"w");

	GIOChannel *gio = g_io_channel_unix_new(pipefd[0]);
	lys->priv->io_watch_tag = g_io_add_watch(gio,G_IO_IN|G_IO_PRI|G_IO_HUP,lyric_search_io_watch,lys);
	g_io_channel_unref (gio);

	build = gtk_builder_new();
	gtk_builder_add_from_file(build,ui_xml,NULL);

	lys->mainwin = GTK_DIALOG(gtk_builder_get_object(build,"dialog"));

	g_signal_connect(lys->mainwin,"delete-event",G_CALLBACK(lyric_search_widget_delete_event),lys);
	g_signal_connect(lys->mainwin,"hide",G_CALLBACK(lyric_search_mainwin_hide),lys);
	g_signal_connect(lys->mainwin,"show",G_CALLBACK(lyric_search_mainwin_show),lys);

	engine_array = g_new0(LyricSearchEngine*,n);
	memcpy(engine_array,enginev,sizeof(enginev));
	lys->priv->engine_array = engine_array;

	lys->engine_box = GTK_COMBO_BOX(gtk_builder_get_object(build,"engine_box"));
	lyric_search_engine_box_config(lys);

	lys->artist_entry = GTK_ENTRY(gtk_builder_get_object(build,"artist_entry"));
	lys->title_entry = GTK_ENTRY(gtk_builder_get_object(build,"title_entry"));
	lys->lyric_entry = GTK_ENTRY(gtk_builder_get_object(build,"lyric_entry"));

	g_signal_connect(lys->title_entry,"activate",G_CALLBACK(lyric_search_entry_activate),lys);
	g_signal_connect(lys->artist_entry,"activate",G_CALLBACK(lyric_search_entry_activate),lys);

	lys->lyricview = GTK_TREE_VIEW(gtk_builder_get_object(build,"lyricview"));
	lyric_search_lyricview_config(lys);

	g_signal_connect(lys->lyricview,"row-activated",G_CALLBACK(lyric_search_lyricview_row_activated),lys);

	lys->search_button = GTK_BUTTON(gtk_builder_get_object(build,"search_button"));
	lys->download_button = GTK_BUTTON(gtk_builder_get_object(build,"download_button"));
	lys->close_button = GTK_BUTTON(gtk_builder_get_object(build,"close_button"));

	lys->info_label = GTK_LABEL(gtk_builder_get_object(build,"info_label"));

	gtk_window_set_focus(GTK_WINDOW(lys->mainwin),lys->search_button);

	g_signal_connect(lys->search_button,"clicked",G_CALLBACK(lyric_search_search_button_clicked),lys);
	g_signal_connect(lys->download_button,"clicked",G_CALLBACK(lyric_search_download_button_clicked),lys);
	g_signal_connect(lys->close_button,"clicked",G_CALLBACK(lyric_search_close_button_clicked),lys);

	g_object_unref(G_OBJECT(build));

	return object;
}

static void
lyric_search_finalize(GObject *object)
{
	LyricSearch *lys;
	lys = LYRIC_SEARCH(object);

	if(lys->priv->fpw)
		fclose(lys->priv->fpw);

	if(lys->priv->io_watch_tag>0)
		g_source_remove(lys->priv->io_watch_tag);

	if(lys->priv->search_result)
		lyric_func_free_lyricid_list(lys->priv->search_result);

	lyric_search_save_config(lys);

	if(lys->default_engine)
		g_free(lys->default_engine);

	if(lys->fmtv){
		g_strfreev(lys->fmtv);
	}

	if(lys->pathv)
		g_strfreev(lys->pathv);

	G_OBJECT_CLASS(lyric_search_parent_class)->finalize(object);
}

static void
lyric_search_file_changed(LyricSearch *lys,const gchar *mrl)
{
	if(lys->mainwin && GTK_IS_WINDOW(lys->mainwin))
		gtk_widget_hide(GTK_WIDGET(lys->mainwin));
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
	g_signal_emit(lys,LYRIC_SEARCH_SIGNALS[LYRIC_UPDATED],0);
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
		lys->mrl = g_strdup(lys->mrl);
		path = g_filename_from_uri(mrl,NULL,NULL);
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
	g_signal_emit(lys,LYRIC_SEARCH_SIGNALS[LYRIC_FILE_CHANGE],0,lys->mrl);
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

static gboolean
lyric_search_find_local_lrc(LyricSearch *lys)
{
	gboolean ret_bl = FALSE;
	gint i,j;
	gchar *lyricdir;
	gchar *lyricname;
	gchar *lyricfile;
	gchar *pathvbackup[] = {"~/Lyric",lys->mediadir,NULL};
	gchar *fmtvbackup[] = {"%a-%t.lrc","%f.lrc",NULL};
	gchar **pathv;
	gchar **fmtv;

	pathv = lys->pathv;
	fmtv = lys->fmtv;

	g_free(lys->lyricfile);
	lys->lyricfile = NULL;

	if(pathv == NULL)
		pathv = pathvbackup;

	if(fmtv == NULL)
		fmtv = fmtvbackup;

	for(i=0;!ret_bl && pathv[i];i++){
		lyricdir = lyric_search_fmt_string(lys,pathv[i]);
		for(j=0;fmtv[j];j++){
			lyricname = lyric_search_fmt_string(lys,fmtv[j]);
			lyricfile = g_build_filename(lyricdir,lyricname,NULL);
			g_free(lyricname);
			if(g_file_test(lyricfile,G_FILE_TEST_EXISTS)){
				ret_bl = TRUE;
				lys->lyricfile = lyricfile;
				break;
			}
			g_free(lyricfile);
		}
		g_free(lyricdir);
	}
	return ret_bl;
}

void
lyric_search_lyric_path_for_save(LyricSearch *lys)
{
	gint   i;
	gchar *lyricdir;
	gchar *lyricname;
	gchar *lyricfile;
	gchar *pathvbackup[] = {"~/Lyric",lys->mediadir,NULL};
	gchar *fmtvbackup[] = {"%a-%t.lrc","%f.lrc",NULL};
	gchar **pathv;
	gchar **fmtv;

	pathv = lys->pathv;
	fmtv = lys->fmtv;

	if(pathv == NULL)
		pathv = pathvbackup;

	if(fmtv == NULL)
		fmtv = fmtvbackup;

	if(lys->lyricfile && *lys->lyricfile == '/')
		return;

	g_free(lys->lyricfile);
	lys->lyricfile = NULL;

	lyricfile = NULL;

	for(i=0;pathv[i];i++){
		lyricdir = lyric_search_fmt_string(lys,pathv[i]);
		g_mkdir_with_parents(lyricdir,0744);
		if(g_file_test(lyricdir,G_FILE_TEST_IS_DIR|G_FILE_TEST_IS_EXECUTABLE)){
			lyricname = lyric_search_fmt_string(lys,fmtv[0]);
			lyricfile = g_build_filename(lyricdir,lyricname,NULL);
			g_free(lyricname);
		}
		g_free(lyricdir);
		if(lyricfile){
			lys->lyricfile = lyricfile;
			break;
		}
	}
	fprintf(stdout,"lyric save to :%s\n",lys->lyricfile);
}

void
lyric_search_set_engine(LyricSearch *lys,LyricSearchEngine* engine)
{
	lys->engine = engine;
}

static GSList*
lyric_search_find_from_remote(LyricSearch *lys)
{
	LyricId id={
	.artist = lys->artist,
	.title = lys->title,
	.album = lys->album,
	};

	lyric_search_lyric_path_for_save(lys);

	GSList *l = lys->engine->func(&id);
	return l;
}

gboolean
lyric_search_save_lyric(LyricSearch *lys,const gchar *uri)
{
	return lyric_func_save_lyric(uri,lys->lyricfile,NULL);
}

gboolean
lyric_search_auto_get_lyric(LyricSearch *lys)
{
	gboolean ret_bl = TRUE;
	if(lyric_search_find_local_lrc(lys)){
		lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_LOCAL_LYRIC_YES);
		fprintf(stderr,"find lyric : %s\n",lys->lyricfile);
	}else{
		GSList *l;
		l = lyric_search_find_from_remote(lys);
		ret_bl = FALSE;
		if(l){
			lyric_func_lyricid_list(l);
			ret_bl = lyric_func_save_lyric(((LyricId*)l->data)->uri,lys->lyricfile,NULL);
			lyric_func_free_lyricid_list(l);
		}else{
			lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_AUTO_GET_LYRIC_FAIL);
		}
	}
	if(ret_bl){
		lyric_search_set_status(lys,LYRIC_SEARCH_STATUS_AUTO_GET_LYRIC_OK);
	}
	return ret_bl;
}

gboolean
lyric_search_manual_lyric(LyricSearch *lys)
{
	lyric_search_lyric_path_for_save(lys);
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
	if(!lys->priv->is_show){
		gtk_entry_set_text(lys->artist_entry,lys->artist);
		gtk_entry_set_text(lys->title_entry,lys->title);
		gtk_entry_set_text(lys->lyric_entry,lys->lyricfile);
	}
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

	lys->fmtv = g_key_file_get_string_list(key,
			property_names[PROPERTY_LYRIC_SEARCH_GROUP],
			property_names[PROPERTY_LYRIC_NAME_FMT],
			NULL,NULL);

	lys->pathv = g_key_file_get_string_list(key,
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
			lys->fmtv,-1);

	g_key_file_set_string_list(key,
			property_names[PROPERTY_LYRIC_SEARCH_GROUP],
			property_names[PROPERTY_LYRIC_PATH_LIST],
			lys->pathv,-1);

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

LyricSearch*
lyric_search_new(void)
{
	return LYRIC_SEARCH(g_object_new(LYRIC_SEARCH_TYPE,NULL));
}

#ifdef test_lyric_search

#include "LyricShowIface.h"
#include "LyricShowTreeView.h"

void
lyric_search_show_info(LyricSearch *lys)
{
	fprintf(stdout,"artist:%s\n",lys->artist);
	fprintf(stdout,"title :%s\n",lys->title);
}

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

#if 0
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
	lyric_search_manual_lyric(lys);
	gtk_main();
	return 0;
}
#endif

#endif ///test_lyric_search
