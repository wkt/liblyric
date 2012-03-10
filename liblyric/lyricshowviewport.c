/**
* *lyricshowviewport.c
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

#include "lyricshowviewport.h"
#include "lyricread.h"
#include "LyricShow.h"
#include "lyriclinewidget.h"

enum
{
    PROP_0,
    PROP_TIME,
};


struct _LyricShowViewportPrivate
{
    LyricInfo   *info;
    guint64     current_time;
    GtkWidget   *lyricbox;
    GtkWidget   *msg;
};

static void
lyric_show_viewport_set_property(GObject        *object,
                guint               property_id,
                const GValue        *value,
                GParamSpec          *pspec);

static void
lyric_show_viewport_get_property(GObject        *object,
                guint               property_id,
                GValue              *value,
                GParamSpec          *pspec);

static void
lyric_show_viewport_finalize(GObject        *object);

static void
lyric_show_viewport_size_request(GtkWidget *widget,GtkRequisition *requisition);

static void
lyric_show_viewport_size_allocate(GtkWidget *widget,GtkAllocation *allocation);

static GObject*
lyric_show_viewport_constructor(GType                  type,
                guint                  n_construct_properties,
                GObjectConstructParam *construct_properties);

static void
lyric_show_viewport_set_time(LyricShow *iface,guint64 time);

static void
lyric_show_viewport_iface_interface_init(LyricShowIface *iface);

#define LYRIC_SHOW_VIEWPORT_GET_PRIVATE(o)                  (G_TYPE_INSTANCE_GET_PRIVATE((o),LYRIC_SHOW_VIEWPORT_TYPE,LyricShowViewportPrivate))


G_DEFINE_TYPE_WITH_CODE(LyricShowViewport,lyric_show_viewport,GTK_TYPE_VIEWPORT,
                        G_IMPLEMENT_INTERFACE (LYRIC_TYPE_SHOW,
                        lyric_show_viewport_iface_interface_init))

static void
lyric_show_viewport_class_init(LyricShowViewportClass *klass)
{
    GObjectClass *object_class= G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    object_class->constructor = lyric_show_viewport_constructor;

    object_class->set_property = lyric_show_viewport_set_property;
    object_class->get_property = lyric_show_viewport_get_property;
    object_class->finalize = lyric_show_viewport_finalize;

    widget_class->size_request = lyric_show_viewport_size_request;
    widget_class->size_allocate = lyric_show_viewport_size_allocate;

    g_object_class_install_property(object_class,
                                    PROP_TIME,
                                    g_param_spec_uint64("time",
                                                        "time",
                                                        "time",
                                                        0,
                                                        G_MAXUINT64,
                                                        0,
                                                        G_PARAM_CONSTRUCT|G_PARAM_READWRITE)
                                    );

    g_type_class_add_private (klass, sizeof (LyricShowViewportPrivate));

}


static void
lyric_show_viewport_init(LyricShowViewport *lsv)
{
    lsv->priv = LYRIC_SHOW_VIEWPORT_GET_PRIVATE(lsv);
}


static GObject*
lyric_show_viewport_constructor(GType                  type,
                guint                  n_construct_properties,
                GObjectConstructParam *construct_properties)
{
    GObject *object;
    LyricShowViewport *lsv;
    GtkWidget *hbox;
    object = G_OBJECT_CLASS(lyric_show_viewport_parent_class)->constructor(
                                    type,
                                    n_construct_properties,
                                    construct_properties);
    lsv = LYRIC_SHOW_VIEWPORT(object);
    lsv->priv->msg = gtk_label_new("");
    lsv->priv->lyricbox = gtk_vbox_new(TRUE,2);

    hbox = gtk_hbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(hbox),lsv->priv->msg,TRUE,TRUE,0);
    gtk_box_pack_start(GTK_BOX(hbox),lsv->priv->lyricbox,TRUE,TRUE,0);
    gtk_widget_set_no_show_all(lsv->priv->lyricbox,TRUE);
    gtk_widget_set_no_show_all(lsv->priv->msg,TRUE);
    gtk_widget_show_all(hbox);
    gtk_container_add(GTK_CONTAINER(lsv),hbox);
    return object;
}


static void
lyric_show_viewport_set_property(GObject        *object,
                guint               property_id,
                const GValue        *value,
                GParamSpec          *pspec)
{
    LyricShowViewport *lsv;
    lsv = LYRIC_SHOW_VIEWPORT(object);
    switch(property_id)
    {
        case PROP_TIME:
            lsv->priv->current_time = g_value_get_uint64(value);
        break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
lyric_show_viewport_get_property(GObject        *object,
                guint               property_id,
                GValue        *value,
                GParamSpec          *pspec)
{
    LyricShowViewport *lsv;
    lsv = LYRIC_SHOW_VIEWPORT(object);
    switch(property_id)
    {
        case PROP_TIME:
            g_value_set_uint64(value,lsv->priv->current_time);
        break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}



static void
lyric_show_viewport_finalize(GObject        *object)
{
    LyricShowViewport *lsv;
    lsv = LYRIC_SHOW_VIEWPORT(object);
    if(G_OBJECT_CLASS(lyric_show_viewport_parent_class)->finalize)
    {
       G_OBJECT_CLASS(lyric_show_viewport_parent_class)->finalize(object);
    }
}

static void
lyric_show_viewport_size_request(GtkWidget *widget,GtkRequisition *requisition)
{
    LyricShowViewport *lsv;
    lsv = LYRIC_SHOW_VIEWPORT(widget);

    GTK_WIDGET_CLASS(lyric_show_viewport_parent_class)->size_request(widget,requisition);
}

static void
lyric_show_viewport_size_allocate(GtkWidget *widget,GtkAllocation *allocation)
{
    LyricShowViewport *lsv;
    lsv = LYRIC_SHOW_VIEWPORT(widget);
    GTK_WIDGET_CLASS(lyric_show_viewport_parent_class)->size_allocate(widget,allocation);
}

static void
lyric_show_viewport_set_lyric_visible(LyricShowViewport *lsv,gboolean visible)
{
    gtk_widget_set_visible(lsv->priv->msg,!visible);
    gtk_widget_set_visible(lsv->priv->lyricbox,visible);
}

static const gchar*
lyric_show_viewport_get_name(LyricShow *iface)
{
    return "LyricShowViewport";
}

static void
lyric_show_viewport_set_time(LyricShow *iface,guint64 time)
{
    LyricShowViewport *lsv;
    lsv = LYRIC_SHOW_VIEWPORT(iface);
    if(lsv->priv->current_time != time)
    {
        lsv->priv->current_time = time;
        g_object_notify(G_OBJECT(lsv),"time");
    }
}

static void
lyric_show_viewport_set_lyric(LyricShow *iface,const gchar *lyricfile)
{
    gint i = 0;
    LyricLine *ll = NULL;
    LyricInfo *info;
    LyricShowViewport *lsv;
    LyricLineWidget   *llw;
    lsv = LYRIC_SHOW_VIEWPORT(iface);
    lyric_info_free(lsv->priv->info);
    lsv->priv->info = lyric_read(lyricfile);
    if(lsv->priv->info)
    {
        gchar *text = NULL;
        info = lsv->priv->info;
        if(info->title)
        {
            text = g_strdup_printf("Title:%s",info->title);
            llw = lyric_line_widget_new(0,text);
            gtk_widget_show(llw);
            g_free(text);
            text = NULL;
            gtk_box_pack_start(GTK_BOX(lsv->priv->lyricbox),llw,FALSE,FALSE,0);
        }
        if(info->artist)
        {
            text = g_strdup_printf("Artist:%s",info->artist);
            llw = lyric_line_widget_new(0,text);
            gtk_widget_show(llw);
            g_free(text);
            text = NULL;
            gtk_box_pack_start(GTK_BOX(lsv->priv->lyricbox),llw,FALSE,FALSE,0);
        }
        if(info->album)
        {
            text = g_strdup_printf("Album:%s",info->album);
            llw = lyric_line_widget_new(0,text);
            gtk_widget_show(llw);
            g_free(text);
            text = NULL;
            gtk_box_pack_start(GTK_BOX(lsv->priv->lyricbox),llw,FALSE,FALSE,0);
        }
        i= 0;
        ll = lyric_info_get_line(info,i);
        while(ll)
        {
            llw = lyric_line_widget_new(ll->time,ll->line);
            gtk_widget_show(llw);
            gtk_box_pack_start(GTK_BOX(lsv->priv->lyricbox),llw,FALSE,FALSE,0);
            i++;
            ll = lyric_info_get_line(info,i);
        }
        gtk_widget_show_all(lsv->priv->lyricbox);
        lyric_show_viewport_set_lyric_visible(lsv,TRUE);
    }
}

static void
lyric_show_viewport_set_text(LyricShow *iface,const gchar *text)
{
    LyricShowViewport *lsv;
    lsv = LYRIC_SHOW_VIEWPORT(iface);
    gtk_label_set_label(GTK_LABEL(lsv->priv->msg),text);
    lyric_show_viewport_set_lyric_visible(lsv,FALSE);
}

static void
lyric_show_viewport_iface_interface_init(LyricShowIface *iface)
{
    iface->get_name =   lyric_show_viewport_get_name;
    iface->set_time =   lyric_show_viewport_set_time;
    iface->set_lyric =  lyric_show_viewport_set_lyric;
    iface->set_text =   lyric_show_viewport_set_text;
}

GtkWidget*
lyric_show_viewport_new(void)
{
    return GTK_WIDGET(g_object_new(LYRIC_SHOW_VIEWPORT_TYPE,NULL));
}

#ifdef on_test
int main(int argc,char**argv)
{
    GtkWidget *window;
    GtkWidget *lsv;

    gtk_init(&argc,&argv);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    lsv = lyric_show_viewport_new();
    lyric_show_set_lyric(lsv,argv[1]);
    gtk_container_add(GTK_CONTAINER(window),lsv);
    gtk_window_resize(GTK_WINDOW(window),350,250);
    gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);
    gtk_widget_show_all(GTK_WIDGET(window));
    g_signal_connect(window,"destroy",G_CALLBACK(gtk_main_quit),NULL);
    gtk_main();
    return 0;
}
#endif

