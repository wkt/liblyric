//      lyricread.h
//      
//      Copyright 2010 wkt <weikting@gmail.com>
//      

#ifndef LYRIC_LINE_H

#define LYRIC_LINE_H

#include <glib.h>

typedef struct _LyricLine LyricLine;

struct _LyricLine
{
    gint64 time;
    gchar  *line;
};

typedef struct _LyricInfo LyricInfo;

struct _LyricInfo
{
  gchar   *title;
  gchar   *artist;
  gchar   *album;
  gchar   *author;
  goffset offset;
  gpointer  content;
  void (*content_free)(gpointer);
};

void
lyric_line_free(LyricLine *ll);

LyricInfo *
lyric_read(const gchar *filename);

void
lyric_info_free(LyricInfo *info);

const gchar *
lyric_info_get_line(LyricInfo *info,gsize n);

#endif ///LYRIC_LINE_H
