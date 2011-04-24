#ifndef __LYRIC_FUNC_H
#define __LYRIC_FUNC_H

#include <glib.h>
#include <gio/gio.h>
#include <glib/gi18n.h>

typedef struct {
	gchar *artist;
	gchar *title;
	gchar *album;
	gchar *uri;
	gint   no;
}LyricId;

typedef GSList* (*ParserFunc)(const LyricId *id,const gchar *data);
typedef gchar *(*LyricGetEngineUriFunc)(const LyricId *id);
typedef struct _LyricSearchEngine LyricSearchEngine;

struct _LyricSearchEngine
{
	const gchar *description;
	////get engine's uri so we can search lyric list
	LyricGetEngineUriFunc get_engine_uri;
	ParserFunc parser;
};

gboolean
lyric_func_save_lyric(const char *uri,const gchar *filename,GError **error);

gboolean
lyric_func_save_data(const gchar *filename,const gchar *data,gsize length,GError **error);

char*
lyric_func_get_contents(const char *uri,gsize *length, GError **error);

void
lyric_func_free_lyricid_list(GSList *list);

void
lyric_func_lyricid_list(GSList *l);

#endif ///__LYRIC_FUNC_H
