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

###LIB_LYRIC_DIR=$(pkglibdir)

lib_lyricdir=$(LIB_LYRIC_DIR)

lib_lyric_LTLIBRARIES=liblyric.la

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

#                LyricSogouSearch.c          \
#                LyricSogouSearch.h          \
#                LyricSearchCurl.c           
#                LyricSearchCurl.h           

lyricincdir=$(includedir)/liblyric
lyricinc_HEADERS =  LyricSearch.h       \
                    lyricread.h         \
                    LyricShow.h         \
                    LyricFunc.h         \
                    LibLyric.h          \
					lyricshowviewport.h			\
                    $(NULL)

liblyric_la_LDFLAGS = -module $(CURL_LIBS) $(GTK_LIBS) $(LIBXML2_LIBS) $(libtool_opts)
liblyric_la_CFLAGS = $(CURL_CFLAGS) $(GTK_CFLAGS) $(LIBXML2_CFLAGS)
##liblyric_la_LIBADD = 

noinst_PROGRAMS = test_lyric test_lyric_show_viewport

test_lyric_SOURCES = $(liblyric_la_SOURCES)
test_lyric_CFLAGS = $(liblyric_la_CFLAGS) -Dtest_lyric_search
test_lyric_LDFLAGS = $(liblyric_la_LDFLAGS)

test_lyric_show_viewport_SOURCES = $(liblyric_la_SOURCES)
test_lyric_show_viewport_CFLAGS = $(liblyric_la_CFLAGS) -Don_test_lyric_show_view_port
test_lyric_show_viewport_LDFLAGS = $(liblyric_la_LDFLAGS)
