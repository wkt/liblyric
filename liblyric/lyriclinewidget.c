/**
* *lyriclinewidget.c
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

#include "lyriclinewidget.h"

enum
{
    PROP_0,
    PROP_TIME,
};

struct _LyricLineWidgetPrivate
{
    guint64 time;
};

static void
lyric_line_widget_set_property(GObject        *object,
                guint               property_id,
                const GValue        *value,
                GParamSpec          *pspec);

static void
lyric_line_widget_get_property(GObject        *object,
                guint               property_id,
                GValue        *value,
                GParamSpec          *pspec);

static void
lyric_line_widget_finalize(GObject        *object);

static GObject*
lyric_line_widget_constructor(GType                  type,
                guint                  n_construct_properties,
                GObjectConstructParam *construct_properties);



#define LYRIC_LINE_WIDGET_GET_PRIVATE(o)                  (G_TYPE_INSTANCE_GET_PRIVATE((o),LYRIC_LINE_WIDGET_TYPE,LyricLineWidgetPrivate))


G_DEFINE_TYPE(LyricLineWidget,lyric_line_widget,GTK_TYPE_LABEL)


static void
lyric_line_widget_class_init(LyricLineWidgetClass *klass)
{
    GObjectClass *object_class= G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    object_class->constructor = lyric_line_widget_constructor;

    object_class->set_property = lyric_line_widget_set_property;
    object_class->get_property = lyric_line_widget_get_property;
    object_class->finalize = lyric_line_widget_finalize;

    g_object_class_install_property(object_class,
                                    PROP_TIME,
                                    g_param_spec_uint64("time",
                                                       "time",
                                                       "time",
                                                       0,
                                                       G_MAXINT64,
                                                       0,
                                                       G_PARAM_CONSTRUCT|G_PARAM_READWRITE)
                                    );

    g_type_class_add_private (klass, sizeof (LyricLineWidgetPrivate));

}


static void
lyric_line_widget_init(LyricLineWidget *llw)
{
    llw->priv = LYRIC_LINE_WIDGET_GET_PRIVATE(llw);
}


static GObject*
lyric_line_widget_constructor(GType                  type,
                guint                  n_construct_properties,
                GObjectConstructParam *construct_properties)
{
    GObject *object;
    LyricLineWidget *llw;
    object = G_OBJECT_CLASS(lyric_line_widget_parent_class)->constructor(
                                    type,
                                    n_construct_properties,
                                    construct_properties);
    llw = LYRIC_LINE_WIDGET(object);
    return object;
}


static void
lyric_line_widget_set_property(GObject        *object,
                guint               property_id,
                const GValue        *value,
                GParamSpec          *pspec)
{
    LyricLineWidget *llw;
    llw = LYRIC_LINE_WIDGET(object);
    switch(property_id)
    {
        case PROP_TIME:
            llw->priv->time = g_value_get_uint64(value);
        break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
lyric_line_widget_get_property(GObject        *object,
                guint               property_id,
                GValue        *value,
                GParamSpec          *pspec)
{
    LyricLineWidget *llw;
    llw = LYRIC_LINE_WIDGET(object);
    switch(property_id)
    {
        case PROP_TIME:
            g_value_set_uint64(value,llw->priv->time);
        break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}



static void
lyric_line_widget_finalize(GObject        *object)
{
    LyricLineWidget *llw;
    llw = LYRIC_LINE_WIDGET(object);
    if(G_OBJECT_CLASS(lyric_line_widget_parent_class)->finalize)
    {
       G_OBJECT_CLASS(lyric_line_widget_parent_class)->finalize(object);
    }
}

guint64
lyric_line_widget_get_time(LyricLineWidget *llw)
{
    return llw->priv->time;
}

GtkWidget*
lyric_line_widget_new(guint64 time,const gchar *label)
{
    return GTK_WIDGET(g_object_new(LYRIC_LINE_WIDGET_TYPE,
                                  "time",time,
                                  "label",label,
                                  NULL));
}

#ifdef on_test_line
int main(int argc,char**argv)
{
    GtkWidget *window;
    GtkWidget *llw;

    gtk_init(&argc,&argv);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    llw = lyric_line_widget_new(333,"test lines");
    gtk_container_add(GTK_CONTAINER(window),llw);
    gtk_window_resize(GTK_WINDOW(window),350,250);
    gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);
    gtk_widget_show_all(GTK_WIDGET(window));
    g_signal_connect(window,"destroy",G_CALLBACK(gtk_main_quit),NULL);
    gtk_main();
    return 0;
}
#endif

