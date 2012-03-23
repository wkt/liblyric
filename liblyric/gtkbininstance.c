/**
* *gtkbininstance.c
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

#include "gtkbininstance.h"

enum
{
    PROP_0,
    PROP_INCREMENT_WIDTH,
    PROP_INCREMENT_HEIGHT,
};

struct _GtkBinInstancePrivate
{
    gint increment_width;
    gint increment_height;
};

static void
gtk_bin_instance_set_property(GObject        *object,
                guint               property_id,
                const GValue        *value,
                GParamSpec          *pspec);

static void
gtk_bin_instance_get_property(GObject        *object,
                guint               property_id,
                GValue        *value,
                GParamSpec          *pspec);

static void
gtk_bin_instance_finalize(GObject        *object);

static GObject*
gtk_bin_instance_constructor(GType                  type,
                guint                  n_construct_properties,
                GObjectConstructParam *construct_properties);

static void
gtk_bin_instance_size_allocate(GtkWidget *widget,GtkAllocation *allocation);

#if GTK_CHECK_VERSION(3,2,0)
static void
gtk_bin_instance_get_preferred_height(GtkWidget       *widget,
                                             gint            *minimum,
                                             gint            *natural);

static void
gtk_bin_instance_get_preferred_width_for_height(GtkWidget       *widget,
                                             gint            height,
                                             gint            *minimum,
                                             gint            *natural);

static void
gtk_bin_instance_get_preferred_width(GtkWidget       *widget,
                                             gint            *minimum,
                                             gint            *natural);

static void
gtk_bin_instance_get_preferred_height_for_width(GtkWidget       *widget,
                                             gint            width,
                                             gint            *minimum,
                                             gint            *natural);
#else
static void
gtk_bin_instance_size_request(GtkWidget *widget,GtkRequisition *requisition);
#endif

#define GTK_BIN_INSTANCE_GET_PRIVATE(o)                  (G_TYPE_INSTANCE_GET_PRIVATE((o),GTK_BIN_INSTANCE_TYPE,GtkBinInstancePrivate))


G_DEFINE_TYPE(GtkBinInstance,gtk_bin_instance,GTK_TYPE_BIN)


static void
gtk_bin_instance_class_init(GtkBinInstanceClass *klass)
{
    GObjectClass *object_class= G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    object_class->constructor = gtk_bin_instance_constructor;

    object_class->set_property = gtk_bin_instance_set_property;
    object_class->get_property = gtk_bin_instance_get_property;
    object_class->finalize = gtk_bin_instance_finalize;

#if GTK_CHECK_VERSION(3,2,0)
    widget_class->get_preferred_height = gtk_bin_instance_get_preferred_height;

    widget_class->get_preferred_width_for_height = gtk_bin_instance_get_preferred_width_for_height;

    widget_class->get_preferred_width = gtk_bin_instance_get_preferred_width;

    widget_class->get_preferred_height_for_width = gtk_bin_instance_get_preferred_height_for_width;
#else
    widget_class->size_request  = gtk_bin_instance_size_request;
#endif

    widget_class->size_allocate = gtk_bin_instance_size_allocate;

    g_object_class_install_property(object_class,
                                    PROP_INCREMENT_WIDTH,
                                    g_param_spec_int("increment-width",
                                                     "increment-width",
                                                     "increment-width",
                                                     G_MININT,
                                                     G_MAXINT,
                                                     0,
                                                     G_PARAM_READWRITE));

    g_object_class_install_property(object_class,
                                    PROP_INCREMENT_HEIGHT,
                                    g_param_spec_int("increment-height",
                                                     "increment-height",
                                                     "increment-height",
                                                     G_MININT,
                                                     G_MAXINT,
                                                     0,
                                                     G_PARAM_READWRITE));

    g_type_class_add_private (klass, sizeof (GtkBinInstancePrivate));

}


static void
gtk_bin_instance_init(GtkBinInstance *bin)
{
    bin->priv = GTK_BIN_INSTANCE_GET_PRIVATE(bin);
}


static GObject*
gtk_bin_instance_constructor(GType                  type,
                guint                  n_construct_properties,
                GObjectConstructParam *construct_properties)
{
    GObject *object;
    GtkBinInstance *bin;
    object = G_OBJECT_CLASS(gtk_bin_instance_parent_class)->constructor(
                                    type,
                                    n_construct_properties,
                                    construct_properties);
    bin = GTK_BIN_INSTANCE(object);
    return object;
}


static void
gtk_bin_instance_set_property(GObject        *object,
                guint               property_id,
                const GValue        *value,
                GParamSpec          *pspec)
{
    GtkBinInstance *bin;
    bin = GTK_BIN_INSTANCE(object);
    switch(property_id)
    {
        case PROP_INCREMENT_HEIGHT:
            gtk_bin_instance_set_increment_height(bin,g_value_get_int(value));
        break;
        case PROP_INCREMENT_WIDTH:
            gtk_bin_instance_set_increment_width(bin,g_value_get_int(value));
        break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
gtk_bin_instance_get_property(GObject        *object,
                guint               property_id,
                GValue        *value,
                GParamSpec          *pspec)
{
    GtkBinInstance *bin;
    bin = GTK_BIN_INSTANCE(object);
    switch(property_id)
    {
        case PROP_INCREMENT_HEIGHT:
            g_value_set_int(value,bin->priv->increment_height);
        break;
        case PROP_INCREMENT_WIDTH:
            g_value_set_int(value,bin->priv->increment_width);
        break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}



static void
gtk_bin_instance_finalize(GObject        *object)
{
    GtkBinInstance *bin;
    bin = GTK_BIN_INSTANCE(object);
    if(G_OBJECT_CLASS(gtk_bin_instance_parent_class)->finalize)
    {
       G_OBJECT_CLASS(gtk_bin_instance_parent_class)->finalize(object);
    }
}


static void
gtk_bin_instance_size_allocate(GtkWidget *widget,GtkAllocation *allocation)
{
    GtkBinInstance *bin;
    bin = GTK_BIN_INSTANCE(widget);
    GtkWidget   *child;
    child = gtk_bin_get_child(GTK_BIN(widget));
    if(child)
    {
        gint        width,height;
        GtkAllocation   alc = {0};
        width = (allocation->width-bin->priv->increment_width);
        height = (allocation->height-bin->priv->increment_height);
        alc.x = allocation->x + bin->priv->increment_width/2.0;
        alc.y = allocation->y + bin->priv->increment_height/2.0;
        alc.width = width;
        alc.height = height;
        gtk_widget_size_allocate(child,&alc);
    }
    gtk_widget_set_allocation(widget,allocation);
//    GTK_WIDGET_CLASS(gtk_bin_instance_parent_class)->size_allocate(widget,allocation);
}

#if GTK_CHECK_VERSION(3,2,0)
static void
gtk_bin_instance_get_preferred_height(GtkWidget       *widget,
                                             gint            *minimum,
                                             gint            *natural)
{
    GtkBinInstance *bin;
    bin = GTK_BIN_INSTANCE(widget);

    GtkWidget   *child;
    child = gtk_bin_get_child(GTK_BIN(widget));

    if(minimum)
        *minimum = 0;
    if(natural)
        *natural = 0;

    if(child && gtk_widget_get_visible(child))
    {
        gtk_widget_get_preferred_height(child,minimum,natural);
    }
    if(minimum)
    {
        *minimum = *minimum + bin->priv->increment_height;
    }
    if(natural)
    {
        *natural = *natural + bin->priv->increment_height;
    }
///    GTK_WIDGET_CLASS(gtk_bin_instance_parent_class)->get_preferred_height(widget,minimum,natural);
}

static void
gtk_bin_instance_get_preferred_width_for_height(GtkWidget       *widget,
                                             gint            height,
                                             gint            *minimum,
                                             gint            *natural)
{
    GtkBinInstance *bin;
    bin = GTK_BIN_INSTANCE(widget);

    GTK_WIDGET_CLASS(gtk_bin_instance_parent_class)->get_preferred_width_for_height(widget,height,minimum,natural);
}

static void
gtk_bin_instance_get_preferred_width(GtkWidget       *widget,
                                             gint            *minimum,
                                             gint            *natural)
{
    GtkBinInstance *bin;
    bin = GTK_BIN_INSTANCE(widget);
    GtkWidget   *child;
    child = gtk_bin_get_child(GTK_BIN(widget));

    if(minimum)
        *minimum = 0;
    if(natural)
        *natural = 0;

    if(child && gtk_widget_get_visible(child))
    {
        gtk_widget_get_preferred_width(child,minimum,natural);
    }
    if(minimum)
    {
        *minimum = *minimum + bin->priv->increment_width;
    }
    if(natural)
    {
        *natural = *natural + bin->priv->increment_width;
    }
///    GTK_WIDGET_CLASS(gtk_bin_instance_parent_class)->get_preferred_width(widget,minimum,natural);
}

static void
gtk_bin_instance_get_preferred_height_for_width(GtkWidget       *widget,
                                             gint            width,
                                             gint            *minimum,
                                             gint            *natural)
{
    GtkBinInstance *bin;
    bin = GTK_BIN_INSTANCE(widget);

    GTK_WIDGET_CLASS(gtk_bin_instance_parent_class)->get_preferred_height_for_width(widget,width,minimum,natural);
}

#else

static void
gtk_bin_instance_size_request(GtkWidget *widget,GtkRequisition *requisition)
{
    GtkBinInstance *bin;
    bin = GTK_BIN_INSTANCE(widget);

    GtkWidget   *child;
    child = gtk_bin_get_child(GTK_BIN(widget));
    if(child && gtk_widget_get_visible(widget))
    {
        gtk_widget_size_request(child,requisition);
        requisition->width += bin->priv->increment_width;
        requisition->height+= bin->priv->increment_height;
    }else{
        GTK_WIDGET_CLASS(gtk_bin_instance_parent_class)->size_request(widget,requisition);
    }
}

#endif

void
gtk_bin_instance_set_increment_height(GtkBinInstance *bin,gint increment_height)
{
    if(bin->priv->increment_height != increment_height)
    {
        bin->priv->increment_height = increment_height;
        g_object_notify(G_OBJECT(bin),"increment-height");
        gtk_widget_queue_resize(GTK_WIDGET(bin));
    }
}

void
gtk_bin_instance_set_increment_width(GtkBinInstance *bin,gint increment_width)
{
    if(bin->priv->increment_width != increment_width)
    {
        bin->priv->increment_width = increment_width;
        g_object_notify(G_OBJECT(bin),"increment-width");
        gtk_widget_queue_resize(GTK_WIDGET(bin));
    }
}

GtkWidget*
gtk_bin_instance_new(void)
{
    return GTK_WIDGET(g_object_new(GTK_BIN_INSTANCE_TYPE,NULL));
}

#ifdef on_test
int main(int argc,char**argv)
{
    GtkWidget *window;
    GtkWidget *bin;
    GtkWidget *hbox;

    gtk_init(&argc,&argv);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    bin = gtk_bin_instance_new();
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,FALSE);
    gtk_box_pack_start(hbox,gtk_label_new("HelloWorld4"),TRUE,TRUE,0);
    gtk_box_pack_start(hbox,gtk_label_new("HelloWorld0"),TRUE,TRUE,0);
    gtk_box_pack_start(hbox,gtk_label_new("HelloWorld1"),TRUE,TRUE,0);
    gtk_container_add(GTK_CONTAINER(bin),hbox);
    gtk_container_add(GTK_CONTAINER(window),bin);
    gtk_window_resize(GTK_WINDOW(window),350,250);
    gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);
    gtk_widget_show_all(GTK_WIDGET(window));
    g_signal_connect(window,"destroy",G_CALLBACK(gtk_main_quit),NULL);
    gtk_main();
    return 0;
}
#endif

