#ifndef __LYIRC_SEARCH_H_
#define __LYIRC_SEARCH_H_

#include <glib.h>
#include <glib/gi18n.h>

#include <glib-object.h>
#include <unistd.h>
#include "LyricFunc.h"

G_BEGIN_DECLS


#define LYRIC_SEARCH_TYPE                 (lyric_search_get_type ())
#define LYRIC_SEARCH(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), LYRIC_SEARCH_TYPE, LyricSearch))
#define LYRIC_SEARCH_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), LYRIC_SEARCH_TYPE, LyricSearchClass))
#define IS_LYRIC_SEARCH(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LYRIC_SEARCH_TYPE))
#define IS_LYRIC_SEARCH_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), LYRIC_SEARCH_TYPE))
#define LYRIC_SEARCH_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), LYRIC_SEARCH_TYPE, LyricSearchClass))

typedef struct _LyricSearch LyricSearch;
typedef struct _LyricSearchClass LyricSearchClass;
typedef struct _LyricSearchPrivate LyricSearchPrivate;

typedef enum{
	LYRIC_SEARCH_STATUS_NONE,
	LYRIC_SEARCH_STATUS_PREPARING,
	LYRIC_SEARCH_STATUS_LOCAL_LYRIC_YES,
	LYRIC_SEARCH_STATUS_AUTO_GET_LYRIC_FAIL,
	LYRIC_SEARCH_STATUS_AUTO_GET_LYRIC_OK,
	LYRIC_SEARCH_STATUS_SEARCHING,
	LYRIC_SEARCH_STATUS_SEARCHING_GET_DATA,
	LYRIC_SEARCH_STATUS_SEARCHING_OK,
	LYRIC_SEARCH_STATUS_SEARCHING_FAILED,
	LYRIC_SEARCH_STATUS_DOWNLOADING,
	LYRIC_SEARCH_STATUS_DOWNLOADING_GET_DATA,
	LYRIC_SEARCH_STATUS_DOWNLOADING_OK,
	LYRIC_SEARCH_STATUS_DOWNLOADING_FAILED,
	LYRIC_SEARCH_STATUS_SAVEING_DATA_FAILED,
	LYRIC_SEARCH_STATUS_LYRIC_UPDATED,
	LYRIC_SEARCH_STATUS_LAST
}LyricSearchStatus;

typedef enum{
	LYRIC_SEARCH_NONE,
	LYRIC_AUTO_SEARCH,
	LYRIC_MANUAL_SEARCH,
	LYRIC_SEARCH_TYPE_LAST
}LyricSearchType;

struct _LyricSearchClass
{
	GObjectClass parent_class;
	void (*start_search)(LyricSearch *lys);
	void (*lyric_listed)(LyricSearch *lys);
	void (*finished_search)(LyricSearch *lys);
	void (*status_changed)(LyricSearch *lys);
	void (*lyric_updated)(LyricSearch *lys,const gchar *lrcpath);
};

GType lyric_search_get_type(void);

gboolean
lyric_search_save_lyric(LyricSearch *lys,const gchar *uri);

void
lyric_search_set_mrl(LyricSearch *lys,const gchar *mrl);

const gchar *
lyric_search_get_lyricfile(LyricSearch *lys);


void
lyric_search_set_info(LyricSearch *lys,const gchar *artist,const gchar *title,const gchar *album);

void
lyric_search_auto_get_lyric(LyricSearch *lys);

gboolean
lyric_search_manual_lyric(LyricSearch *lys);

LyricSearchStatus
lyric_search_get_status(LyricSearch *lys);


LyricSearch*
lyric_search_new(void);

G_END_DECLS
#endif ///__LYIRC_SEARCH_H_
