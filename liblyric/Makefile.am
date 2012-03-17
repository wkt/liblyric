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
  -export-dynamic $(no_undefined)

if USE_GTK2
lib_LTLIBRARIES=liblyric-gtk2.la
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
                $(NULL)


liblyric_la_LDFLAGS = -module $(CURL_LIBS) $(GTK_LIBS) $(LIBXML2_LIBS) $(libtool_opts)
liblyric_la_CFLAGS = $(CURL_CFLAGS) $(GTK_CFLAGS) $(LIBXML2_CFLAGS)
##liblyric_la_LIBADD = 

liblyric_gtk2_la_SOURCES = $(liblyric_la_SOURCES)
liblyric_gtk2_la_CFLAGS = $(liblyric_la_CFLAGS)
liblyric_gtk2_la_LDFLAGS = $(liblyric_la_LDFLAGS)

liblyric_gtk3_la_SOURCES = $(liblyric_la_SOURCES)
liblyric_gtk3_la_CFLAGS = $(liblyric_la_CFLAGS)
liblyric_gtk3_la_LDFLAGS = $(liblyric_la_LDFLAGS)

lyricincdir=$(includedir)/liblyric
lyricinc_HEADERS =  LyricSearch.h       \
                    lyricread.h         \
                    LyricShow.h         \
                    LyricFunc.h         \
                    LibLyric.h          \
					lyricshowviewport.h			\
                    $(NULL)
if TEST
noinst_PROGRAMS = test_lyric test_lyric_show_viewport
else
noinst_PROGRAMS = 
endif

test_lyric_SOURCES = $(liblyric_la_SOURCES)
test_lyric_CFLAGS = $(liblyric_la_CFLAGS) -Dtest_lyric_search
test_lyric_LDFLAGS = $(liblyric_la_LDFLAGS)

test_lyric_show_viewport_SOURCES = $(liblyric_la_SOURCES)
test_lyric_show_viewport_CFLAGS = $(liblyric_la_CFLAGS) -Don_test_lyric_show_view_port
test_lyric_show_viewport_LDFLAGS = $(liblyric_la_LDFLAGS)
