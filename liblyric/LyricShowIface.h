#ifndef LYRIC_SHOW_IFACE_H
#define LYRIC_SHOW_IFACE_H


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
    const gchar *(*label)(LyricShow *lsw);
    void (*set_time)(LyricShow *lsw,guint64 time);
    void (*set_lyric)(LyricShow *lsw,const gchar *lyric_file);
};

GType
lyric_show_get_type(void);

const gchar *lyric_show_get_label(LyricShow *lsw);

void lyric_show_set_time(LyricShow *lsw,guint64 time);

void lyric_show_set_lyric(LyricShow *lsw,const gchar *lyric_file);

#endif ///LYRIC_SHOW_IFACE_H
