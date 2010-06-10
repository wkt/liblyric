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

liblyric_la_SOURCES =                       \
                LyricFunc.c                 \
                LyricFunc.h                 \
                LyricSearch.c               \
                LyricShowIface.c            \
                LyricShowIface.h            \
                LyricShowTreeView.c         \
                LyricShowTreeView.h         \
                LyricSogouSearch.c          \
                LyricSogouSearch.h          \
                LyricTtSearch.c             \
                LyricTtSearch.h             \
                lyricread.c                 \
                $(NULL)

#                LyricSearchCurl.c           
#                LyricSearchCurl.h           

lyricincdir=$(includedir)/liblyric
lyricinc_HEADERS = LyricSearch.h lyricread.h LyricShowIface.h

liblyric_la_LDFLAGS = -module $(CURL_LIBS) $(GTK_LIBS) $(LIBXML2_LIBS) $(libtool_opts)
liblyric_la_CFLAGS = $(CURL_CFLAGS) $(GTK_CFLAGS) $(LIBXML2_CFLAGS)
##liblyric_la_LIBADD = 

noinst_PROGRAMS = test_lyric

test_lyric_SOURCES = $(liblyric_la_SOURCES)
test_lyric_CFLAGS = $(liblyric_la_CFLAGS) -Dtest_lyric_search
test_lyric_LDFLAGS = $(liblyric_la_LDFLAGS)
