/**
* *lyricshowviewport.h
* *
* *Copyright (c) 2012 Wei Keting
* *
* *Authors by:Wei Keting <weikting@gmail.com>
* *
* */

#ifndef __LYRIC_SHOW_VIEWPORT_H_
#define __LYRIC_SHOW_VIEWPORT_H_

#include <gtk/gtk.h>
#include <glib.h>
#include <glib-object.h>


G_BEGIN_DECLS

#define LYRIC_SHOW_VIEWPORT_TYPE                  (lyric_show_viewport_get_type())
#define LYRIC_SHOW_VIEWPORT(o)                    (G_TYPE_CHECK_INSTANCE_CAST((o),LYRIC_SHOW_VIEWPORT_TYPE,LyricShowViewport))
#define LYRIC_SHOW_VIEWPORT_CLASS(o)              (G_TYPE_CHECK_CLASS_CAST((o),LYRIC_SHOW_VIEWPORT_TYPE,LyricShowViewportClass))
#define LYRIC_SHOW_VIEWPORT_GET_CLASS(o)          (G_TYPE_INSTANCE_GET_CLASS ((o), LYRIC_SHOW_VIEWPORT_TYPE, LyricShowViewportClass))
#define IS_LYRIC_SHOW_VIEWPORT(o)                 (G_TYPE_CHECK_INSTANCE_TYPE ((o), LYRIC_SHOW_VIEWPORT_TYPE))
#define IS_LYRIC_SHOW_VIEWPORT_CLASS(o)           (G_TYPE_CHECK_CLASS_TYPE ((o), LYRIC_SHOW_VIEWPORT_TYPE))


typedef struct _LyricShowViewport LyricShowViewport;
typedef struct _LyricShowViewportClass LyricShowViewportClass;
typedef struct _LyricShowViewportPrivate LyricShowViewportPrivate;

struct _LyricShowViewport
{
    GtkViewport parent;

    LyricShowViewportPrivate *priv;
};

struct _LyricShowViewportClass
{
    GtkViewportClass parent_class;

};


GType
lyric_show_viewport_get_type(void);

GtkWidget*
lyric_show_viewport_new(void);

G_END_DECLS


#endif //__LYRIC_SHOW_VIEWPORT_H_
