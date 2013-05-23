/**
* *lyricshowmenu.c
* *
* *Copyright (c) 2012 Wei Keting
* *
* *Authors by:Wei Keting <weikting@gmail.com>
* *
* */

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#ifndef ___LYRIC_SHOW_MENU_H__
#define ___LYRIC_SHOW_MENU_H__

#include <glib.h>

#include "LyricShow.h"

GtkWidget*
lyric_show_menu_get_for(LyricShow *lsw);

#endif ///___LYRIC_SHOW_MENU_H__
