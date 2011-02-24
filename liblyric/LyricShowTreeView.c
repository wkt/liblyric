
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>
#include "LyricShowTreeView.h"

#include "LyricShow.h"
#include "lyricread.h"

enum{
    LYRIC_STORE_ID=0,
    LYRIC_STORE_TIME,
    LYRIC_STORE_LINE,
    LYRIC_STORE_LAST
};

struct _LSTreeViewPrivate
{
    const gchar   *label;
    guint64     time;
    gchar       *lyric;
    LyricInfo   *lyricinfo;
};

#define LYIRC_SHOW_TREE_VIEW_GET_PRIVATE(o)       (G_TYPE_INSTANCE_GET_PRIVATE ((o),LYRIC_SHOW_TREE_VIEW_TYPE,LSTreeViewPrivate))

static void
lyric_show_iface_interface_init(LyricShowIface *iface);

static GObject*
lyric_show_tree_view_constructor(GType                  type,
                                 guint                  n_construct_properties,
                                 GObjectConstructParam *construct_properties);

G_DEFINE_TYPE_WITH_CODE (LyricShowTreeView, lyric_show_tree_view, GTK_TYPE_TREE_VIEW,
			 G_IMPLEMENT_INTERFACE (LYRIC_TYPE_SHOW,
						lyric_show_iface_interface_init))

static void
lyric_show_tree_view_init(LyricShowTreeView *lstv)
{
    LSTreeViewPrivate *priv = LYIRC_SHOW_TREE_VIEW_GET_PRIVATE(lstv);
    priv->label = _("SeeSee");
    lstv->priv = priv;
}

static void
lyric_show_tree_view_class_init(LyricShowTreeViewClass *class)
{
    GObjectClass *objclass = G_OBJECT_CLASS(class);

    objclass->constructor = lyric_show_tree_view_constructor;

    g_type_class_add_private(class,sizeof(LSTreeViewPrivate));
}

static GObject*
lyric_show_tree_view_constructor(GType                  type,
                                 guint                  n_construct_properties,
                                 GObjectConstructParam *construct_properties)
{
    GObject *object;
    LyricShowTreeView *lstv;
    GtkTreeViewColumn *column;
    GtkTreeSelection  *selection;

    object = G_OBJECT_CLASS(lyric_show_tree_view_parent_class)->constructor(
                                type,
                                n_construct_properties,
                                construct_properties);

    lstv = LYRIC_SHOW_TREE_VIEW(object);

    lstv->store = gtk_list_store_new(LYRIC_STORE_LAST,
                            G_TYPE_INT,    ///ID
                            G_TYPE_INT64,  ///time
                            G_TYPE_STRING  ///lyric line
                            );
    column = gtk_tree_view_column_new();
    lstv->cell = gtk_cell_renderer_text_new();
    g_object_set(lstv->cell,"xalign",0.5,NULL);
    ///gtk_rc_parse_string("style \"d\"{\nGtkTreeView::vertical-separator=0\n}\nclass \"GtkTreeView\" style \"d\"");
    gtk_tree_view_column_pack_start(column,lstv->cell,TRUE);
    gtk_tree_view_column_set_attributes(column,lstv->cell,"markup",LYRIC_STORE_LINE,NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(lstv),column);
    gtk_tree_view_column_set_expand(column,TRUE);

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(lstv));
    gtk_tree_selection_set_mode(selection,GTK_SELECTION_NONE);
    gtk_tree_view_set_model(GTK_TREE_VIEW(lstv),GTK_TREE_MODEL(lstv->store));
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(lstv),FALSE);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW(lstv),FALSE);
    gtk_tree_view_set_hover_selection(GTK_TREE_VIEW(lstv),FALSE);
    gtk_tree_view_set_rubber_banding(GTK_TREE_VIEW(lstv),FALSE);

    return object;
}

static void
lyric_show_tree_view_update(LyricShowTreeView *lstv)
{
    GtkTreeIter iter;
    GtkTreeIter niter;
    GtkTreeModel *model;
    gint64 time = 0;
    gint64 ntime = -1;
    gint id = 0;

    model = GTK_LIST_STORE(lstv->store);
    if(gtk_tree_model_get_iter_first(model,&iter)){
        do{
            gtk_tree_model_get(model,&iter, LYRIC_STORE_ID,&id, LYRIC_STORE_TIME,&time,-1);
            if(id>0 && time >0){
                const gchar *line = lyric_info_get_line(lstv->priv->lyricinfo,id);
                gchar *text = (gchar*)line;
                gchar *markup = NULL;

                if(lstv->priv->time > time){
                    gchar *path_str = gtk_tree_model_get_string_from_iter(model,&iter);
                    gtk_tree_model_get_iter_from_string(model,&niter,path_str);
                    g_free(path_str);
                    if(gtk_tree_model_iter_next(model,&niter)){
                         gtk_tree_model_get(model,&niter,LYRIC_STORE_TIME,&ntime,-1);
                         if(lstv->priv->time < ntime){
                            markup = g_strdup_printf("<span foreground=\"red\"><b>%s</b></span>",line);
                            text = markup;
                            ///g_warning("%s",text);
                         }
                    }
                }
                gtk_list_store_set(lstv->store,&iter,LYRIC_STORE_LINE,text,-1);
                g_free(markup);
                markup = NULL;
            }
        }while(gtk_tree_model_iter_next(model,&iter));
    }
}

GtkWidget*
lyric_show_tree_view_new(void)
{
    return GTK_WIDGET(g_object_new(LYRIC_SHOW_TREE_VIEW_TYPE,NULL));
}

static const gchar*
lyric_show_tree_view_label(LyricShow *iface)
{
    LyricShowTreeView *lstv = LYRIC_SHOW_TREE_VIEW(iface);
    return lstv->priv->label;
}


static void
lyric_show_tree_view_set_time(LyricShow *iface,guint64 time)
{
    LyricShowTreeView *lstv = LYRIC_SHOW_TREE_VIEW(iface);
    lstv->priv->time = time;
    lyric_show_tree_view_update(lstv);
    ///g_printerr("set time : %llu\n",time);
}

static void
lyric_show_tree_view_set_lyric(LyricShow *iface,const gchar *lyric_file)
{
    LyricShowTreeView *lstv = LYRIC_SHOW_TREE_VIEW(iface);
    LyricInfo *info;
    LyricLine *ll;
    gchar *text;
    gint id = 0;
    gint64 time = -1;
    GtkTreeIter iter;
    GList *l = NULL;

    g_free(lstv->priv->lyric);
    lstv->priv->lyric = NULL;
    gtk_list_store_clear(lstv->store);
    if(lyric_file){
        lstv->priv->lyric = g_strdup(lyric_file);
        lstv->priv->lyricinfo = info = lyric_read(lyric_file);

        if(info->title){
            text = g_strdup_printf(_("Title:%s"),info->title);
            gtk_list_store_append(lstv->store,&iter);
            gtk_list_store_set(lstv->store,&iter,
                            LYRIC_STORE_ID,-1,
                            LYRIC_STORE_TIME,time,
                            LYRIC_STORE_LINE,text,
                            -1);
            g_free(text);
        }
        if(info->artist){
            text = g_strdup_printf(_("Artist:%s"),info->artist);
            gtk_list_store_append(lstv->store,&iter);
            gtk_list_store_set(lstv->store,&iter,
                            LYRIC_STORE_ID,-1,
                            LYRIC_STORE_TIME,time,
                            LYRIC_STORE_LINE,text,
                            -1);
            g_free(text);
        }
        if(info->album){
            text = g_strdup_printf(_("Album:%s"),info->album);
            gtk_list_store_append(lstv->store,&iter);
            gtk_list_store_set(lstv->store,&iter,
                            LYRIC_STORE_ID,-1,
                            LYRIC_STORE_TIME,time,
                            LYRIC_STORE_LINE,text,
                            -1);
            g_free(text);
        }
        if(info->author){
            text = g_strdup_printf(_("Author:%s"),info->author);
            gtk_list_store_append(lstv->store,&iter);
            gtk_list_store_set(lstv->store,&iter,
                            LYRIC_STORE_ID,-1,
                            LYRIC_STORE_TIME,time,
                            LYRIC_STORE_LINE,text,
                            -1);
            g_free(text);
        }
        gtk_list_store_append(lstv->store,&iter);
        id = 0;
        for(l = info->content;l;l = l->next){
            ll = l->data;
            time = ll->time;
            text = g_strdup(ll->line);
            gtk_list_store_append(lstv->store,&iter);
            gtk_list_store_set(lstv->store,&iter,
                            LYRIC_STORE_ID,id,
                            LYRIC_STORE_TIME,time,
                            LYRIC_STORE_LINE,text,
                            -1);
            if(!g_utf8_validate(ll->line,-1,NULL)){
                fprintf(stderr,"fuck:%s\n",ll->line);
            }
            id++;
        }
    }else{
        g_free(lstv->priv->lyric);
        lstv->priv->lyric = NULL;
        lyric_info_free(lstv->priv->lyricinfo);
        lstv->priv->lyricinfo = NULL;
    }
}

static void
lyric_show_iface_interface_init(LyricShowIface *iface)
{
    
    iface->label = lyric_show_tree_view_label;
    iface->set_time = lyric_show_tree_view_set_time;
    iface->set_lyric = lyric_show_tree_view_set_lyric;
}
