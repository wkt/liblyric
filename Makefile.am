SUBDIRS =           \
        liblyric    \
        totem-plugin \
        po \
        data \
        $(NULL)


DISTCLEANFILES = \
  intltool-extract \
  intltool-merge \
  intltool-update

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA= liblyric.pc

EXTRA_DIST= intltool-extract.in         \
            intltool-merge.in           \
            intltool-update.in          \
            configure.in                \
            autogen.sh                  \
            $(NULL)

potfile:
	ls liblyric/*.c data/*.glade >po/POTFILES.in

update-po:
	make -C po update-po
