INCLUDES =						\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	$(WARN_CFLAGS)                                  \
    $(NULL)

modules_flags = -export_dynamic -avoid-version -module -no-undefined

plugindir=$(PLUGINDIR)/totem-lyric

%.plugin: %.plugin.in
	$(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_in_files = totem-lyric.plugin.in
plugin_DATA = $(plugin_in_files:.plugin.in=.plugin)

CLEANFILES = $(plugin_DATA)
DISTCLEANFILES = $(plugin_DATA)
EXTRA_DIST = $(plugin_in_files)


if ENABLE_TOTEM_PLUGIN

plugin_LTLIBRARIES = libtotem-lyric.la

else
plugin_LTLIBRARIES = 
endif

libtotem_lyric_la_SOURCES = totemlyricplugin.c
libtotem_lyric_la_CFLAGS   = $(TOTEM_CFLAGS)
libtotem_lyric_la_LDFLAGS = $(modules_flags)        \
                            $(TOTEM_LIBS)           \
                            ../liblyric/liblyric-gtk3.la \
                            $(NULL)

libtotem_lyric_la_LIBADD =
