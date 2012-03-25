#ifndef LYRIC_SHOW_IFACE_H
#define LYRIC_SHOW_IFACE_H

#include <glib.h>

G_BEGIN_DECLS

#define LYRIC_TYPE_SHOW           (lyric_show_get_type ())
#define LYRIC_SHOW(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), LYRIC_TYPE_SHOW, LyricShow))
#define LYRIC_IS_SHOW(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LYRIC_TYPE_SHOW))
#define LYRIC_SHOW_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), LYRIC_TYPE_SHOW, LyricShowIface))

typedef struct _LyricShow LyricShow;
typedef struct _LyricShowIface LyricShowIface;

struct _LyricShowIface
{
    GTypeInterface g_iface;

    /*  signal  */
    void (*time_request)(LyricShow *lsw,gint64);
    void (*search_request)(LyricShow *lsw);

    /*  function */
    const gchar *(*get_name)(LyricShow *lsw);
    void (*set_time)(LyricShow *lsw,gint64 time);
    void (*set_lyric)(LyricShow *lsw,const gchar *lyric_file);
    void (*set_text)(LyricShow *lsw,const gchar *text);
};

GType
lyric_show_get_type(void);

const gchar *
lyric_show_get_name(LyricShow *lsw);

void
lyric_show_set_time(LyricShow *lsw,gint64 time);

void
lyric_show_set_lyric(LyricShow *lsw,const gchar *lyric_file);

void
lyric_show_set_text(LyricShow *lsw,const gchar *text);

void
lyric_show_time_request(LyricShow *lsw,gint64 t);

void
lyric_show_search_request(LyricShow *lsw);

#endif ///LYRIC_SHOW_IFACE_H
