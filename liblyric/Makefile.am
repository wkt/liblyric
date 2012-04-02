INCLUDES =						\
	-DDATADIR=\"$(datadir)\"			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DUIDIR=\"$(UIDIR)\"			\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	$(WARN_CFLAGS)                                  \
    $(NULL)
###	-Werror                                         

libtool_opts =							\
	-version-info $(LT_VERSION_INFO)				\
	-export-dynamic $(no_undefined)				\
	-module 										\
	$(NULL)

if USE_GTK2
lib_LTLIBRARIES=liblyric-gtk2.la liblyric-gtk3.la
else
lib_LTLIBRARIES=liblyric-gtk3.la
endif

common-glue.c:common.marshal Makefile.am common-glue.h
	glib-genmarshal --body  --prefix=lyric_show_common_marshal $< >$@

common-glue.h:common.marshal Makefile.am
	glib-genmarshal --header --prefix=lyric_show_common_marshal $< >$@


liblyric_la_SOURCES =                       \
				common-glue.c				\
				common-glue.h				\
                LyricFunc.c                 \
                LyricSearch.c               \
                LyricShow.c            \
                LyricShowTreeView.c         \
                LyricShowTreeView.h         \
                LyricTtSearch.c             \
                LyricTtSearch.h             \
                lyricread.c                 \
                LyricDownloader.c           \
                LyricDownloader.h           \
                lyriclinewidget.c  			\
                lyriclinewidget.h			\
                lyricshowviewport.c  		\
                gtkbininstance.c			\
                gtkbininstance.h			\
                lyricshowmenu.c				\
                lyricshowmenu.h				\
                $(NULL)


liblyric_la_LDFLAGS = $(CURL_LIBS) $(GLIB_LIBS) $(LIBXML2_LIBS) $(libtool_opts)
liblyric_la_CFLAGS = $(CURL_CFLAGS) $(GLIB_CFLAGS) $(LIBXML2_CFLAGS)
##liblyric_la_LIBADD = 

liblyric_gtk2_la_SOURCES = $(liblyric_la_SOURCES) 
liblyric_gtk2_la_CFLAGS = $(liblyric_la_CFLAGS)  $(GTK2_CFLAGS)
liblyric_gtk2_la_LDFLAGS = $(liblyric_la_LDFLAGS) $(GTK2_LIBS)

liblyric_gtk3_la_SOURCES = $(liblyric_la_SOURCES)
liblyric_gtk3_la_CFLAGS = $(liblyric_la_CFLAGS) $(GTK3_CFLAGS)
liblyric_gtk3_la_LDFLAGS = $(liblyric_la_LDFLAGS) $(GTK3_LIBS)

lyricincdir=$(includedir)/liblyric
lyricinc_HEADERS =  LyricSearch.h       \
                    lyricread.h         \
                    LyricShow.h         \
                    LyricFunc.h         \
                    LibLyric.h          \
					lyricshowviewport.h			\
                    $(NULL)
if TEST
noinst_PROGRAMS =  lyric_show_viewport
else
noinst_PROGRAMS = 
endif

lyric_show_viewport_SOURCES = $(liblyric_la_SOURCES)
lyric_show_viewport_CFLAGS = $(liblyric_la_CFLAGS) -Don_lyric_show_viewport $(GTK3_CFLAGS)
lyric_show_viewport_LDFLAGS = $(liblyric_la_LDFLAGS) $(GTK3_LIBS)
