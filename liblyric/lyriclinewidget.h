/**
* *lyriclinewidget.h
* *
* *Copyright (c) 2012 Wei Keting
* *
* *Authors by:Wei Keting <weikting@gmail.com>
* *
* */

#ifndef __LYRIC_LINE_WIDGET_H_
#define __LYRIC_LINE_WIDGET_H_

#include <gtk/gtk.h>
#include <glib.h>
#include <glib-object.h>


G_BEGIN_DECLS

#define LYRIC_LINE_WIDGET_TYPE                  (lyric_line_widget_get_type())
#define LYRIC_LINE_WIDGET(o)                    (G_TYPE_CHECK_INSTANCE_CAST((o),LYRIC_LINE_WIDGET_TYPE,LyricLineWidget))
#define LYRIC_LINE_WIDGET_CLASS(o)              (G_TYPE_CHECK_CLASS_CAST((o),LYRIC_LINE_WIDGET_TYPE,LyricLineWidgetClass))
#define LYRIC_LINE_WIDGET_GET_CLASS(o)          (G_TYPE_INSTANCE_GET_CLASS ((o), LYRIC_LINE_WIDGET_TYPE, LyricLineWidgetClass))
#define IS_LYRIC_LINE_WIDGET(o)                 (G_TYPE_CHECK_INSTANCE_TYPE ((o), LYRIC_LINE_WIDGET_TYPE))
#define IS_LYRIC_LINE_WIDGET_CLASS(o)           (G_TYPE_CHECK_CLASS_TYPE ((o), LYRIC_LINE_WIDGET_TYPE))


typedef struct _LyricLineWidget LyricLineWidget;
typedef struct _LyricLineWidgetClass LyricLineWidgetClass;
typedef struct _LyricLineWidgetPrivate LyricLineWidgetPrivate;

struct _LyricLineWidget
{
    GtkLabel parent;

    LyricLineWidgetPrivate *priv;
};

struct _LyricLineWidgetClass
{
    GtkLabelClass parent_class;

};


GType
lyric_line_widget_get_type(void);

guint64
lyric_line_widget_get_time(LyricLineWidget *llw);

GtkWidget*
lyric_line_widget_new(guint64 time,const gchar *label);

G_END_DECLS


#endif //__LYRIC_LINE_WIDGET_H_
