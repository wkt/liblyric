#ifndef __LYIRC_DOWN_LOADER_H

#define __LYIRC_DOWN_LOADER_H

#include <glib.h>
#include <glib/gi18n.h>

#include <glib-object.h>

#define LYRIC_DOWNLOADER_TYPE           (lyric_down_loader_get_type())
#define LYRIC_DOWNLOADER(o)             (G_TYPE_CHECK_INSTANCE_CAST((o),LYRIC_DOWNLOADER_TYPE,LyricDownloader))
#define LRYIC_DOWNLOADER_CLASS(o)       (G_TYPE_CHECK_CLASS_CAST((o),LYRIC_DOWNLOADER_TYPE,LyricDownloaderClass))
#define LYRIC_DOWNLOADER_GET_CLASS(o)   (G_TYPE_INSTANCE_GET_CLASS((o),LYRIC_DOWNLOADER_TYPE,LyricDownloaderClass))

typedef struct _LyricDownloaderPriv LyricDownloaderPriv;
typedef struct _LyricDownloader LyricDownloader;
typedef struct _LyricDownloaderClass LyricDownloaderClass;

enum{
    DONE_OK = 0,
    RUN_CMDMAND_FAILD = 1,
    CREATE_THREAD_FAILED,
    DOWNLODER_FAILED,
    DOWNLODER_UNKNOWN
};

struct _LyricDownloader
{
    GObject parent;
    LyricDownloaderPriv *priv;
};

struct _LyricDownloaderClass
{
    GObjectClass parent_class;
    void (*error)(LyricDownloader *ldl,const gchar *message);
    void (*done)(LyricDownloader *ldl,const GString *data);
    void (*cancel)(LyricDownloader *ldl);
};


GType
lyric_down_loader_get_type(void);

LyricDownloader*
lyric_down_loader_new(void);

void
lyric_down_loader_cancel(LyricDownloader* ldl);

const GString*
lyric_down_loader_get_data(LyricDownloader* ldl);

void
lyric_down_loader_load(LyricDownloader *ldl,const gchar *uri);


#endif ///__LYIRC_DOWN_LOADER_H
