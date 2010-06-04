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


/* utf8->iso-8859-1
 * loacle->utf8
 **/
gchar *
guess_encode_to_utf8(const gchar *str);

/*
 * loacle->utf8
 * */
gchar *
guess_string_to_utf8(const gchar *str);

gchar*
encode_to_utf8(const gchar *str,const gchar *charset);

gchar*
encode_from_utf8(const gchar *str,const gchar *charset);

void
lyric_line_free(LyricLine *ll);

LyricInfo *
lyric_read(const gchar *filename);

void
lyric_info_free(LyricInfo *info);

const gchar *
lyric_info_get_line(LyricInfo *info,gsize n);

#endif ///LYRIC_LINE_H
