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

#include "lyricshowviewport.h"
#include "lyricread.h"
#include "LyricShow.h"
#include "lyriclinewidget.h"

#ifdef GETTEXT_PACKAGE
#include <glib/gi18n-lib.h>
#else
#include <glib/gi18n.h>
#endif

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
    gint        pressed_pos;
    gint        pressed_y;
    gboolean    is_pressed;
    GtkWidget   *current_widget;
    GtkWidget   *pre_widget;
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
lyric_show_viewport_get_preferred_width(GtkWidget *widget,
                                  gint      *minimum_size,
                                  gint      *natural_size);
static void
lyric_show_viewport_get_preferred_height(GtkWidget *widget,
                                  gint      *minimum_size,
                                  gint      *natural_size);

static void
lyric_show_viewport_size_allocate(GtkWidget *widget,GtkAllocation *allocation);

static gboolean
lyric_show_viewport_expose(GtkWidget    *widget,GdkEventExpose *event);

static gboolean
lyric_show_viewport_draw(GtkWidget  *widget,cairo_t *cr);

static gboolean
lyric_show_viewport_button_press(GtkWidget    *widget,GdkEventButton *event);

static gboolean
lyric_show_viewport_button_release(GtkWidget    *widget,GdkEventButton *event);

static gboolean
lyric_show_viewport_motion_notify(GtkWidget *widget,GdkEventMotion *event);

static void
lyric_show_viewport_update_cursor(LyricShowViewport *lsv);

static void
lyric_show_viewport_update_current_widget(LyricShowViewport *lsv);

static void
lyric_show_viewport_set_time(LyricShow *iface,guint64 time);

static guint64
lyric_show_viewport_get_requested_time(LyricShowViewport *lsv);

static void
lyric_show_viewport_iface_interface_init(LyricShowIface *iface);


///copy from gtk_widget_get_pointer in gtk+3.0
inline static void
gtk_widget_get_mouse_position(GtkWidget *widget,GdkEvent *event,gint *x,gint *y)
{
    GdkDevice *dev = NULL;
    if (x)
        *x = -1;
    if (y)
        *y = -1;
    if(event)
    {
        dev = gdk_event_get_device(event);
    }
    if(dev == NULL)
    {
        dev = gdk_device_manager_get_client_pointer (
                                        gdk_display_get_device_manager (
                                          gtk_widget_get_display (widget)));
    }

    if (gtk_widget_get_realized (widget))
    {
        gdk_window_get_device_position (gtk_widget_get_window(widget),
                                      dev,
                                      x, y, NULL);

        if (!gtk_widget_get_has_window (widget))
        {
            GtkAllocation alc = {0};
            gtk_widget_get_allocation(widget,&alc);
            if (x)
                *x -= alc.x;
            if (y)
                *y -= alc.y;
        }
    }
}

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

#if GTK_CHECK_VERSION(3,2,0)
    widget_class->get_preferred_width = lyric_show_viewport_get_preferred_width;
    widget_class->get_preferred_height = lyric_show_viewport_get_preferred_height;
#else
    widget_class->size_request = lyric_show_viewport_size_request;
#endif


    widget_class->size_allocate = lyric_show_viewport_size_allocate;

#if GTK_CHECK_VERSION(3,2,0)
    widget_class->draw = lyric_show_viewport_draw;
#else
    widget_class->expose_event = lyric_show_viewport_expose;
#endif

    widget_class->button_release_event = lyric_show_viewport_button_release;
    widget_class->button_press_event = lyric_show_viewport_button_press;

    widget_class->motion_notify_event   = lyric_show_viewport_motion_notify;

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
    gtk_widget_add_events(GTK_WIDGET(lsv),
                            GDK_BUTTON_PRESS_MASK|
                            GDK_BUTTON_RELEASE_MASK|
                            GDK_BUTTON1_MOTION_MASK);
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
    lsv->priv->lyricbox = 
#if GTK_CHECK_VERSION(3,2,0)
    gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_box_set_homogeneous(GTK_BOX(lsv->priv->lyricbox),TRUE);
#else
    gtk_vbox_new(TRUE,2);
#endif

    hbox =
#if GTK_CHECK_VERSION(3,2,0)
    gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
#else
    gtk_hbox_new(FALSE,0);
#endif

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

#if GTK_CHECK_VERSION(3,2,0)
static void
lyric_show_viewport_get_preferred_width(GtkWidget *widget,
                                  gint      *minimum_size,
                                  gint      *natural_size)
{
    LyricShowViewport *lsv;
    lsv = LYRIC_SHOW_VIEWPORT(widget);

    GTK_WIDGET_CLASS(lyric_show_viewport_parent_class)->get_preferred_width(widget,minimum_size,natural_size);

    *minimum_size += 8;
    *natural_size += 8;
}

static void
lyric_show_viewport_get_preferred_height(GtkWidget *widget,
                                  gint      *minimum_size,
                                  gint      *natural_size)
{
    LyricShowViewport *lsv;
    lsv = LYRIC_SHOW_VIEWPORT(widget);

    GTK_WIDGET_CLASS(lyric_show_viewport_parent_class)->get_preferred_height(widget,minimum_size,natural_size);
    *minimum_size = 200;
///    g_warning("height(%d,%d)",*minimum_size,*natural_size);
}
#else
static void
lyric_show_viewport_size_request(GtkWidget *widget,GtkRequisition *requisition)
{
    LyricShowViewport *lsv;
    lsv = LYRIC_SHOW_VIEWPORT(widget);

    GTK_WIDGET_CLASS(lyric_show_viewport_parent_class)->size_request(widget,requisition);
    requisition->height = 200;//requisition->height/4.0;
    requisition->width += 8;
}
#endif

static void
lyric_show_viewport_size_allocate(GtkWidget *widget,GtkAllocation *allocation)
{
    LyricShowViewport *lsv;
    lsv = LYRIC_SHOW_VIEWPORT(widget);

    GtkAdjustment *adj = 

#if GTK_CHECK_VERSION(3,2,0)
    gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(widget));
#else
    gtk_viewport_get_vadjustment(GTK_VIEWPORT(lsv));
#endif

    if(gtk_widget_get_realized(widget))
    {
        gdk_window_invalidate_rect(gtk_widget_get_window(widget),NULL,TRUE);
    }

    GTK_WIDGET_CLASS(lyric_show_viewport_parent_class)->size_allocate(widget,allocation);

#if 0
    g_warning("x:%5d,y:%5d,width:%6d,heigth:%d",
            allocation->x,allocation->y,
            allocation->width,allocation->height);
#endif

    gtk_adjustment_set_lower(adj,-allocation->height);
    gtk_adjustment_set_value(adj,(lsv->priv->pos+lsv->priv->pressed_pos)-allocation->height/2.0);
}

#if GTK_CHECK_VERSION(3,2,0)
static gboolean
lyric_show_viewport_draw(GtkWidget  *widget,cairo_t *cr)
{
    LyricShowViewport *lsv;
    lsv = LYRIC_SHOW_VIEWPORT(widget);
    GtkStyleContext *context;

    GdkWindow   *bin = gtk_viewport_get_bin_window(GTK_VIEWPORT(widget));
    context = gtk_widget_get_style_context(widget);

    GTK_WIDGET_CLASS(lyric_show_viewport_parent_class)->draw(widget,cr);
    if(lsv->priv->is_pressed && gtk_cairo_should_draw_window(cr,bin))
    {
        gint y0,y1;
        y0=y1 = gtk_widget_get_allocated_height(widget)/2.0;///lsv->priv->pos+lsv->priv->pressed_pos;
        gtk_render_line(context,cr,0,y0,
                gtk_widget_get_allocated_width(widget),y1);
                                   
    }
    return FALSE;
}
#else
static gboolean
lyric_show_viewport_expose(GtkWidget    *widget,GdkEventExpose *event)
{
    LyricShowViewport *lsv;
    lsv = LYRIC_SHOW_VIEWPORT(widget);
    GdkWindow   *view = gtk_viewport_get_view_window(GTK_VIEWPORT(widget));

#if 1 ///必须启用否则widget大小变化时view上的东西不会被自动清除，界面混乱
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

    if(lsv->priv->is_pressed)
    {
        gtk_paint_hline(widget->style,
                        gtk_viewport_get_bin_window(GTK_VIEWPORT(widget)),
                        GTK_STATE_NORMAL,
                        NULL,
                        widget,
                        NULL,
                        0,widget->allocation.width,
                        lsv->priv->pos+lsv->priv->pressed_pos);//widget->allocation.height/2.0);
    }
    return FALSE;
}
#endif ///GTK_CHECK_VERSION(3,2,0)

static gboolean
lyric_show_viewport_button_press(GtkWidget    *widget,GdkEventButton *event)
{
    LyricShowViewport *lsv;
    lsv = LYRIC_SHOW_VIEWPORT(widget);
    gint x = 0;
    gint y = 0;

#if GTK_CHECK_VERSION(3,2,0)
    gtk_widget_get_mouse_position(widget,(GdkEvent*) event,&x,&y);
#else
    gtk_widget_get_pointer(widget,&x,&y);
#endif

    lsv->priv->is_pressed = TRUE;
    lsv->priv->pressed_y = y;

    lyric_show_viewport_update_cursor(lsv);
    gtk_widget_queue_resize(widget);
///    GTK_WIDGET_CLASS(lyric_show_viewport_parent_class)->button_press_event(widget,event);
    return FALSE;
}

static gboolean
lyric_show_viewport_button_release(GtkWidget    *widget,GdkEventButton *event)
{
    LyricShowViewport *lsv;
    lsv = LYRIC_SHOW_VIEWPORT(widget);

    guint64 t=0;
    gint x = 0;
    gint y = 0;

#if GTK_CHECK_VERSION(3,2,0)
    gtk_widget_get_mouse_position(widget,(GdkEvent*)event,&x,&y);
#else
    gtk_widget_get_pointer(widget,&x,&y);
#endif

    lsv->priv->is_pressed = FALSE;

    t = lyric_show_viewport_get_requested_time(lsv);
    ///lsv->priv->pos += lsv->priv->pressed_pos ;
    lsv->priv->pressed_pos = 0;

    lyric_show_viewport_update_cursor(lsv);
    lyric_show_time_request(LYRIC_SHOW(lsv),t);
///    lyric_show_viewport_update_current_widget(lsv);

///    GTK_WIDGET_CLASS(lyric_show_viewport_parent_class)->button_release_event(widget,event);
    return FALSE;
}

static gboolean
lyric_show_viewport_motion_notify(GtkWidget *widget,GdkEventMotion *event)
{
    LyricShowViewport *lsv;
    lsv = LYRIC_SHOW_VIEWPORT(widget);
    gint x = 0;
    gint y = 0;
    gint tmp_pos = 0;
    GtkAllocation alc = {0};

#if GTK_CHECK_VERSION(3,2,0)
    gtk_widget_get_mouse_position(widget,(GdkEvent*)event,&x,&y);
#else
    gtk_widget_get_pointer(widget,&x,&y);
#endif

    tmp_pos = lsv->priv->pressed_y - y;
    gtk_widget_get_allocation(lsv->priv->lyricbox,&alc);
    if(tmp_pos + lsv->priv->pos > 0)
    {
        if(tmp_pos + lsv->priv->pos >= alc.height)
            tmp_pos = alc.height-lsv->priv->pos-1;
    }else{
        tmp_pos = -lsv->priv->pos;
    }
    lsv->priv->pressed_pos = tmp_pos;
    if(GTK_WIDGET_CLASS(lyric_show_viewport_parent_class)->motion_notify_event)
    {
        GTK_WIDGET_CLASS(lyric_show_viewport_parent_class)->motion_notify_event(widget,event);
    }
    gtk_widget_queue_resize(widget);
    return FALSE;
}

static void
lyric_show_viewport_update_cursor(LyricShowViewport *lsv)
{
    GdkWindow *win = gtk_widget_get_window(GTK_WIDGET(lsv));
    if(lsv->priv->is_pressed)
    {
        GdkCursor *cur = gdk_cursor_new(GDK_FLEUR);
        gdk_window_set_cursor(win,cur);
#if GTK_CHECK_VERSION(3,2,0)
        g_object_unref(cur);
#else
        gdk_cursor_unref(cur);
#endif
    }else{
        gdk_window_set_cursor(win,NULL);
    }
}

static void
lyric_show_viewport_update_current_widget(LyricShowViewport *lsv)
{
    if(!lsv->priv->is_pressed && lsv->priv->current_widget)
    {
        GtkAllocation alc0,alc1;
        const gchar *color_string = "blue";

#if GTK_CHECK_VERSION(3,2,0)
        GdkRGBA color = {0};
        gdk_rgba_parse(&color,color_string);
#else
        GdkColor color = {0};
        gdk_color_parse(color_string,&color);
#endif

        if(lsv->priv->pre_widget &&
            GTK_IS_WIDGET(lsv->priv->pre_widget) &&
            lsv->priv->current_widget !=lsv->priv->pre_widget)
        {
#if GTK_CHECK_VERSION(3,2,0)
            gtk_widget_override_color(lsv->priv->pre_widget,GTK_STATE_FLAG_ACTIVE,NULL);
            gtk_widget_set_state_flags(lsv->priv->pre_widget,GTK_STATE_FLAG_ACTIVE,FALSE);
#else
            gtk_widget_modify_fg(lsv->priv->pre_widget,GTK_STATE_ACTIVE,NULL);
            gtk_widget_set_state(lsv->priv->pre_widget,GTK_STATE_NORMAL);
#endif
        }

#if GTK_CHECK_VERSION(3,2,0)
        gtk_widget_override_color(lsv->priv->current_widget,GTK_STATE_FLAG_ACTIVE,&color);
        gtk_widget_set_state_flags(lsv->priv->current_widget,GTK_STATE_FLAG_ACTIVE,FALSE);
#else
        gtk_widget_modify_fg(lsv->priv->current_widget,GTK_STATE_ACTIVE,&color);
        gtk_widget_set_state(lsv->priv->current_widget,GTK_STATE_ACTIVE);
#endif

        gtk_widget_get_allocation(lsv->priv->current_widget,&alc0);
        gtk_widget_get_allocation(lsv->priv->lyricbox,&alc1);
        lsv->priv->pos = (alc0.y - alc1.y)+alc0.height/2.0;
        gtk_widget_queue_resize(lsv->priv->current_widget);
    }
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
        pre_llw = GTK_WIDGET(llw);
    }
    g_list_free(list);
    if(pre_llw != lsv->priv->current_widget)
    {
        lsv->priv->pre_widget = lsv->priv->current_widget;
        lsv->priv->current_widget = pre_llw;
        lyric_show_viewport_update_current_widget(lsv);
    }
}

static void
lyric_show_viewport_set_lyric_visible(LyricShowViewport *lsv,gboolean visible)
{
    gtk_widget_set_visible(lsv->priv->msg,!visible);
    gtk_widget_set_visible(lsv->priv->lyricbox,visible);
}

static guint64
lyric_show_viewport_get_requested_time(LyricShowViewport *lsv)
{
    guint64 res = 0;
    GList *l,*list;
    gint current_y = 0;
    GtkWidget *llw = NULL;
    GtkAllocation   alc = {0};
    list = gtk_container_get_children(GTK_CONTAINER(lsv->priv->lyricbox));
    gtk_widget_get_allocation(lsv->priv->lyricbox,&alc);
    current_y = alc.y+lsv->priv->pressed_pos + lsv->priv->pos;
    for(l=list;l;l=l->next)
    {
        llw = GTK_WIDGET(l->data);
        gtk_widget_get_allocation(llw,&alc);
        if(llw && current_y >= alc.y && current_y <= alc.y+alc.height)
        {
            res = lyric_line_widget_get_time(LYRIC_LINE_WIDGET(llw));
            break;
        }
    }
    g_list_free(list);
    return res;
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
    if(!lsv->priv->is_pressed)
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

#ifdef on_test_lyric_show_view_port
#define timeout_value 100

static guint64 line_time = 0;

static gboolean
idle_timeout(LyricShowViewport *lsv)
{

    lyric_show_set_time(LYRIC_SHOW(lsv),line_time);
    line_time += timeout_value;
    return TRUE;
}

static void
time_request(LyricShow *lsw,guint64 t)
{
    line_time = t;
    g_warning("%s:%lu",__FUNCTION__,t);
}

int main(int argc,char**argv)
{
    GtkWidget *window;
    GtkWidget *lsv;

#if 0 ///defined(ENABLE_NLS) && ENABLE_NLS == 1
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
#endif

///    gtk_set_locale();
    gtk_init(&argc,&argv);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    lsv = lyric_show_viewport_new();
    lyric_show_set_lyric(lsv,argv[1]);
    gtk_container_add(GTK_CONTAINER(window),lsv);
    gtk_window_resize(GTK_WINDOW(window),350,450);
    gtk_window_set_position(GTK_WINDOW(window),GTK_WIN_POS_CENTER);
    gtk_widget_show_all(GTK_WIDGET(window));
    g_timeout_add(timeout_value,idle_timeout,lsv);
///    lyric_show_set_time(LYRIC_SHOW(lsv),1234);
    g_signal_connect(window,"destroy",G_CALLBACK(gtk_main_quit),NULL);
    g_signal_connect(lsv,"time-request",G_CALLBACK(time_request),NULL);
    gtk_main();
    return 0;
}
#endif

