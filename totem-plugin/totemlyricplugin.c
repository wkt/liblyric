/**
* *totemlyricplugin.c
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

#include <totem.h>
#include <totem-plugin.h>

#include "liblyric/LyricShow.h"
#include "liblyric/LyricSearch.h"
#include "liblyric/lyricshowviewport.h"
#include "liblyric/lyricread.h"

#define TOTEM_TYPE_LYRIC_PLUGIN                  (totem_lyric_plugin_get_type())
#define TOTEM_LYRIC_PLUGIN(o)                    (G_TYPE_CHECK_INSTANCE_CAST((o),TOTEM_TYPE_LYRIC_PLUGIN,TotemLyricPlugin))
#define TOTEM_LYRIC_PLUGIN_CLASS(o)              (G_TYPE_CHECK_CLASS_CAST((o),TOTEM_TYPE_LYRIC_PLUGIN,TotemLyricPluginClass))
#define TOTEM_LYRIC_PLUGIN_GET_CLASS(o)          (G_TYPE_INSTANCE_GET_CLASS ((o), TOTEM_TYPE_LYRIC_PLUGIN, TotemLyricPluginClass))
#define TOTEM_IS_LYRIC_PLUGIN(o)                 (G_TYPE_CHECK_INSTANCE_TYPE ((o), TOTEM_TYPE_LYRIC_PLUGIN))
#define TOTEM_IS_LYRIC_PLUGIN_CLASS(o)           (G_TYPE_CHECK_CLASS_TYPE ((o), TOTEM_TYPE_LYRIC_PLUGIN))

struct _TotemLyricPluginPrivate
{
    guint       timeout_id;
    LyricSearch *lys;
    LyricShow    *lsw;
    TotemObject *totem;
    GtkWidget *bvw;
};

typedef struct _TotemLyricPluginPrivate TotemLyricPluginPrivate;

#ifdef TOTEM2_PLUGIN
typedef struct _TotemLyricPlugin TotemLyricPlugin;
typedef struct _TotemLyricPluginClass TotemLyricPluginClass;


struct _TotemLyricPlugin
{
    TotemPlugin parent;

    TotemLyricPluginPrivate *priv;
};

struct _TotemLyricPluginClass
{
    TotemPluginClass parent_class;

};

#else
/// TOTEM_PLUGIN_REGISTER_CONFIGURABLE(TOTEM_TYPE_LYRIC_PLUGIN,TotemLyricPlugin,totem_lyric_plugin)
    TOTEM_PLUGIN_REGISTER(TOTEM_TYPE_LYRIC_PLUGIN,TotemLyricPlugin,totem_lyric_plugin)
#endif ///TOTEM2_PLUGIN

static gboolean
impl_activate_real			(TotemLyricPlugin *pi, TotemObject *totem, GError **error);

static void
impl_deactivate_real			(TotemLyricPlugin *pi, TotemObject *totem);

static GtkWidget *
create_configure_widget_real		(TotemLyricPlugin *pi);

#ifdef TOTEM2_PLUGIN

static void
totem_lyric_plugin_finalize(GObject        *object);

static GObject*
totem_lyric_plugin_constructor(GType                  type,
                guint                  n_construct_properties,
                GObjectConstructParam *construct_properties);

GType
totem_lyric_plugin_get_type(void);

#define TOTEM_LYRIC_PLUGIN_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE((o),TOTEM_TYPE_LYRIC_PLUGIN,TotemLyricPluginPrivate))

G_DEFINE_TYPE(TotemLyricPlugin,totem_lyric_plugin,TOTEM_PLUGIN_TYPE)

static void
totem_lyric_plugin_class_init(TotemLyricPluginClass *klass)
{
    GObjectClass *object_class= G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    TotemPluginClass *plugin_class = TOTEM_PLUGIN_CLASS (klass);
    plugin_class->impl_activate = impl_activate_real;
    plugin_class->impl_deactivate = impl_deactivate_real;

//    plugin_class->create_configure_dialog = create_configure_widget;

    object_class->constructor = totem_lyric_plugin_constructor;
    object_class->finalize = totem_lyric_plugin_finalize;

    g_type_class_add_private (klass, sizeof (TotemLyricPluginPrivate));

}


static void
totem_lyric_plugin_init(TotemLyricPlugin *plugin)
{
    plugin->priv = TOTEM_LYRIC_PLUGIN_GET_PRIVATE(plugin);
}


static GObject*
totem_lyric_plugin_constructor(GType                  type,
                guint                  n_construct_properties,
                GObjectConstructParam *construct_properties)
{
    GObject *object;
    TotemLyricPlugin *plugin;
    object = G_OBJECT_CLASS(totem_lyric_plugin_parent_class)->constructor(
                                    type,
                                    n_construct_properties,
                                    construct_properties);
    plugin = TOTEM_LYRIC_PLUGIN(object);
    return object;
}


static void
totem_lyric_plugin_finalize(GObject        *object)
{
    TotemLyricPlugin *plugin;
    plugin = TOTEM_LYRIC_PLUGIN(object);
    if(G_OBJECT_CLASS(totem_lyric_plugin_parent_class)->finalize)
    {
       G_OBJECT_CLASS(totem_lyric_plugin_parent_class)->finalize(object);
    }
}

#else


static void
impl_activate (PeasActivatable *plugin)
{
    impl_activate_real(TOTEM_LYRIC_PLUGIN(plugin),
                      g_object_get_data (G_OBJECT (plugin), "object"),
                      NULL);
}

static void
impl_deactivate (PeasActivatable *plugin)
{
    TotemLyricPlugin *pi = TOTEM_LYRIC_PLUGIN(plugin);
    impl_deactivate_real(pi,pi->priv->totem);
}


static GtkWidget *
impl_create_configure_widget (PeasGtkConfigurable *configurable)
{
    return create_configure_widget_real(TOTEM_LYRIC_PLUGIN(configurable));
}

#endif //TOTEM2_PLUGIN

static void
file_opened(TotemObject *totem,const gchar *mrl,TotemLyricPlugin *pi)
{
    lyric_search_set_mrl(pi->priv->lys,mrl);
}

static void
file_closed(TotemObject *totem,TotemLyricPlugin *pi)
{
    lyric_search_set_info(pi->priv->lys,NULL,NULL,NULL);
    lyric_show_set_text(pi->priv->lsw,_("Lyric Show"));
}

#define SET_INFO(n) \
    if(n && n[0]) \
    {   \
        gchar *t = NULL;    \
        const gchar *pt = NULL;   \
        t = guess_encode_to_utf8(n);    \
        pt = n;    \
        if(t)   \
        {       \
            pt = t; \
        }           \
        lyric_search_set_##n(pi->priv->lys,pt); \
        g_free(t);          \
    }

static void
metadata_updated(TotemObject *totem,
						 const char *artist,
						 const char *title,
						 const char *album,
						 guint track_num,
                         TotemLyricPlugin *pi)
{
    SET_INFO(artist)
    SET_INFO(title)
    else{
        gchar *t = totem_get_short_title(totem);
        lyric_search_set_title(pi->priv->lys,t);
        g_free(t);
    }
    SET_INFO(album)
    g_warning("track_num:%d",track_num);
    lyric_search_find_lyric(pi->priv->lys);
    g_warning("track_num:%d end",track_num);
}

static void
seekable_notify(TotemObject *totem,
                    GParamSpec *pspec,
                    TotemLyricPlugin *pi)
{
    g_object_set(pi->priv->lsw,"time-requestable",totem_is_seekable(totem),NULL);
    g_warning("seekable:%d",totem_is_seekable(totem));
}

static gboolean
timeout_update(TotemLyricPlugin *pi)
{
    if(totem_is_playing(pi->priv->totem))
    {
        lyric_show_set_time(pi->priv->lsw,totem_get_current_time(pi->priv->totem));
    }
    return TRUE;
}

static void
lyric_updated(LyricSearch *lys,const gchar *lyricfile,TotemLyricPlugin *pi)
{
    lyric_show_set_lyric(pi->priv->lsw,lyricfile);
}

static void
time_request(LyricShow *lsw,guint64 t,TotemLyricPlugin *pi)
{
    totem_action_seek_time(pi->priv->totem,t,FALSE);
    totem_action_play(pi->priv->totem);
}

static gboolean 
impl_activate_real(TotemLyricPlugin *pi, TotemObject *totem, GError **error)
{
    pi->priv->totem = totem;
    pi->priv->lys = lyric_search_new();
    pi->priv->lsw = LYRIC_SHOW(lyric_show_viewport_new());
    pi->priv->bvw = totem_get_video_widget (pi->priv->totem);
    lyric_show_set_text(pi->priv->lsw,_("Lyric show"));

	totem_add_sidebar_page (totem,
				"lyric",
				_("Lyric Show"),
				GTK_WIDGET(pi->priv->lsw));
///	gtk_widget_set_sensitive (pi->priv->lsw, FALSE);

    g_signal_connect(totem,"file-opened",G_CALLBACK(file_opened),pi);
    g_signal_connect(totem,"file-closed",G_CALLBACK(file_closed),pi);
    g_signal_connect(totem,"metadata-updated",G_CALLBACK(metadata_updated),pi);
    g_signal_connect(totem,"notify::seekable",G_CALLBACK(seekable_notify),pi);

    g_signal_connect(pi->priv->lys,"lyric-updated",G_CALLBACK(lyric_updated),pi);

    g_signal_connect(pi->priv->lsw,"time-request",G_CALLBACK(time_request),pi);

    pi->priv->timeout_id = g_timeout_add(200,(GSourceFunc)timeout_update,pi);

    if(totem_is_playing(totem) || totem_is_paused(totem))
    {
        gchar *t = totem_get_current_mrl(totem);
        lyric_search_set_mrl(pi->priv->lys,t);
        g_free(t);
        g_signal_emit_by_name(pi->priv->bvw,"got-metadata");
    }

    return TRUE;
}

static void
impl_deactivate_real(TotemLyricPlugin *pi, TotemObject *totem)
{

    g_source_remove(pi->priv->timeout_id);
    g_signal_handlers_block_by_func(pi->priv->totem,file_opened,pi);
    g_signal_handlers_block_by_func(pi->priv->totem,file_closed,pi);
    g_signal_handlers_block_by_func(pi->priv->totem,metadata_updated,pi);
    g_signal_handlers_block_by_func(pi->priv->totem,seekable_notify,pi);

    totem_remove_sidebar_page (totem, "lyric");

    g_object_unref(pi->priv->lys);
//    g_object_unref(pi->priv->lsw);
}

static GtkWidget *
create_configure_widget_real(TotemLyricPlugin *pi)
{
}

GtkWidget*
totem_lyric_plugin_new(void)
{
    return GTK_WIDGET(g_object_new(TOTEM_TYPE_LYRIC_PLUGIN,NULL));
}

#ifdef on_test
int main(int argc,char**argv)
{
    GtkWidget *window;
    GtkWidget *plugin;

    gtk_init(&argc,&argv);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    plugin = gtk_widget_new(TOTEM_TYPE_LYRIC_PLUGIN,NULL);
    gtk_container_add(GTK_CONTAINER(window),plugin);
    gtk_window_resize(GTK_WINDOW(window),350,250);
    gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);
    gtk_widget_show_all(GTK_WIDGET(window));
    g_signal_connect(window,"destroy",G_CALLBACK(gtk_main_quit),NULL);
    gtk_main();
    return 0;
}
#endif

