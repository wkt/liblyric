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
    gint        pos;
    GtkWidget   *current_widget;
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

static GObject*
lyric_show_viewport_constructor(GType                  type,
                guint                  n_construct_properties,
                GObjectConstructParam *construct_properties);

static void
lyric_show_viewport_size_request(GtkWidget *widget,GtkRequisition *requisition);

static void
lyric_show_viewport_size_allocate(GtkWidget *widget,GtkAllocation *allocation);

static gboolean
lyric_show_viewport_expose(GtkWidget    *widget,GdkEventExpose *event);

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

    widget_class->expose_event = lyric_show_viewport_expose;

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
    gtk_widget_set_redraw_on_allocate(GTK_WIDGET(lsv),TRUE);
    gtk_container_set_resize_mode (GTK_CONTAINER (lsv), GTK_RESIZE_PARENT);
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
    requisition->height = requisition->height/4.0;
    requisition->width += 10;
}

static void
lyric_show_viewport_size_allocate(GtkWidget *widget,GtkAllocation *allocation)
{
    LyricShowViewport *lsv;
    lsv = LYRIC_SHOW_VIEWPORT(widget);
    GtkAdjustment *adj = gtk_viewport_get_vadjustment(GTK_VIEWPORT(lsv));
    if(gtk_widget_get_realized(widget))
    {
        gdk_window_invalidate_rect(widget->window,NULL,TRUE);
    }

    GTK_WIDGET_CLASS(lyric_show_viewport_parent_class)->size_allocate(widget,allocation);
#if 0
    g_warning("x:%5d,y:%5d,width:%6d,heigth:%d",
            allocation->x,allocation->y,
            allocation->width,allocation->height);
#endif
    gtk_adjustment_set_lower(adj,-allocation->height);
    gtk_adjustment_set_value(adj,lsv->priv->pos-allocation->height/2.0);
}

static gboolean
lyric_show_viewport_expose(GtkWidget    *widget,GdkEventExpose *event)
{

#if 1 ///必须启用否则widget大小变化时view上的东西不会被自动清除，界面混乱
    GdkWindow   *view = gtk_viewport_get_view_window(GTK_VIEWPORT(widget));
    gint        view_width,view_height;
    view_width = gdk_window_get_width(view);
    view_height = gdk_window_get_height(view);
    
    gtk_paint_flat_box(widget->style,
                       view,
                       GTK_STATE_NORMAL,
                       GTK_SHADOW_NONE,
                       NULL,
                       widget,
                       NULL,
                       0,0,
                       view_width,
                       view_height);
#endif

    GTK_WIDGET_CLASS(lyric_show_viewport_parent_class)->expose_event(widget,event);
    return FALSE;
}
static void
lyric_show_viewport_sync_time_line_widget(LyricShowViewport *lsv)
{
    GList *l,*list;
    LyricLineWidget  *llw = NULL;
    GtkWidget  *pre_llw = NULL;
    guint64 pre_time = 0;
    guint64 line_time = 0;
    list = gtk_container_get_children(GTK_CONTAINER(lsv->priv->lyricbox));
    for(l=list;l;l=l->next)
    {
        llw = LYRIC_LINE_WIDGET(l->data);
        line_time = lyric_line_widget_get_time(llw);
        if(lsv->priv->current_time <line_time)
        {
            ///g_warning("line_ime:%lu,current_time:%lu",line_time,lsv->priv->current_time);
            break;
        }
        pre_time = line_time;
        pre_llw = llw;
    }
    g_list_free(list);
    if(pre_llw != lsv->priv->current_widget)
    {
        if(lsv->priv->current_widget && GTK_IS_WIDGET(lsv->priv->current_widget))
        {
            gtk_widget_modify_fg(lsv->priv->current_widget,GTK_STATE_ACTIVE,NULL);
            gtk_widget_set_state(lsv->priv->current_widget,GTK_STATE_NORMAL);
        }
        lsv->priv->current_widget = pre_llw;
        if(lsv->priv->current_widget)
        {
            GdkColor color = {0};
            GtkAllocation alc0,alc1;
            gdk_color_parse("blue",&color);
            gtk_widget_modify_fg(lsv->priv->current_widget,GTK_STATE_ACTIVE,&color);
            gtk_widget_set_state(lsv->priv->current_widget,GTK_STATE_ACTIVE);
            gtk_widget_get_allocation(lsv->priv->current_widget,&alc0);
            gtk_widget_get_allocation(lsv->priv->lyricbox,&alc1);
            lsv->priv->pos = (alc0.y - alc1.y);
            gtk_widget_queue_resize(lsv->priv->current_widget);
        
        }
    }
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
    lyric_show_viewport_sync_time_line_widget(lsv);
}

static void
lyric_show_viewport_set_lyric(LyricShow *iface,const gchar *lyricfile)
{
    gint i = 0;
    const LyricLine *ll = NULL;
    LyricInfo *info;
    LyricShowViewport *lsv;
    GtkWidget   *llw;
    lsv = LYRIC_SHOW_VIEWPORT(iface);

    lyric_info_free(lsv->priv->info);
    gtk_container_forall(GTK_CONTAINER(lsv->priv->lyricbox),(GtkCallback)gtk_widget_destroy,NULL);

    lsv->priv->info = lyric_read(lyricfile);
    if(lsv->priv->info)
    {
        gchar *text = NULL;
        info = lsv->priv->info;
        if(info->title)
        {
            text = g_strdup_printf(_("Title:%s"),info->title);
            llw = lyric_line_widget_new(0,text);
            g_free(text);
            text = NULL;
            gtk_box_pack_start(GTK_BOX(lsv->priv->lyricbox),GTK_WIDGET(llw),FALSE,FALSE,0);
        }
        if(info->artist)
        {
            text = g_strdup_printf(_("Artist:%s"),info->artist);
            llw = lyric_line_widget_new(0,text);
            g_free(text);
            text = NULL;
            gtk_box_pack_start(GTK_BOX(lsv->priv->lyricbox),GTK_WIDGET(llw),FALSE,FALSE,0);
        }
        if(info->album)
        {
            text = g_strdup_printf(_("Album:%s"),info->album);
            llw = lyric_line_widget_new(0,text);
            g_free(text);
            text = NULL;
            gtk_box_pack_start(GTK_BOX(lsv->priv->lyricbox),GTK_WIDGET(llw),FALSE,FALSE,0);
        }
        i= 0;
        ll = lyric_info_get_line(info,i);
        while(ll)
        {
            llw = lyric_line_widget_new(ll->time,ll->line);

            gtk_box_pack_start(GTK_BOX(lsv->priv->lyricbox),GTK_WIDGET(llw),FALSE,FALSE,0);
            i++;
            ll = lyric_info_get_line(info,i);
        }
        gtk_container_forall(GTK_CONTAINER(lsv->priv->lyricbox),(GtkCallback) gtk_widget_show,NULL);
        gtk_widget_show(lsv->priv->lyricbox);
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
#define timeout_value 100
static gboolean
idle_timeout(LyricShowViewport *lsv)
{
    static guint64 time = 0;
    lyric_show_set_time(LYRIC_SHOW(lsv),time);
    time += timeout_value;
    return TRUE;
}

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
    g_timeout_add(timeout_value,idle_timeout,lsv);
///    lyric_show_set_time(LYRIC_SHOW(lsv),1234);
    g_signal_connect(window,"destroy",G_CALLBACK(gtk_main_quit),NULL);
    gtk_main();
    return 0;
}
#endif

