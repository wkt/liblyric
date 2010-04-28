INCLUDES =						\
	-DDATADIR=\"$(datadir)\"			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	$(WARN_CFLAGS)                                  \
    $(NULL)
###	-Werror                                         

LIB_LYRIC_DIR=$(prefix)/lib

lib_lyricdir=$(LIB_LYRIC_DIR)

lib_lyric_LTLIBRARIES=liblyric.la

liblyric_la_SOURCES =                       \
                LyricFunc.c                 \
                LyricFunc.h                 \
                LyricSearch.c               \
                LyricSearch.h               \
                LyricShowIface.c            \
                LyricShowIface.h            \
                lyricshowtreeview.c         \
                lyricshowtreeview.h         \
                LyricSogouSearch.c          \
                LyricSogouSearch.h          \
                LyricTtSearch.c             \
                LyricTtSearch.h             \
                $(NULL)

#                LyricSearchCurl.c           
#                LyricSearchCurl.h           


liblyric_la_LDFLAGS = -module -avoid-version $(CURL_LIBS) $(GTK_LIBS) $(LIBXML2_LIBS)
liblyric_la_CFLAGS = $(CURL_CFLAGS) $(GTK_CFLAGS) $(LIBXML2_CFLAGS)
##liblyric_la_LIBADD = 
