SUBDIRS =           \
        po \
        liblyric    \
        totem-plugin \
        data \
        $(NULL)


DISTCLEANFILES = \
  intltool-extract \
  intltool-merge \
  intltool-update

pkgconfigdir = $(libdir)/pkgconfig


if USE_GTK2
pkgconfig_DATA = liblyric-gtk3.pc liblyric-gtk2.pc
else
pkgconfig_DATA= liblyric-gtk3.pc
endif

EXTRA_DIST= intltool-extract.in         \
            intltool-merge.in           \
            intltool-update.in          \
            configure.in                \
            autogen.sh                  \
            liblyric-gtk3.pc.in			\
            liblyric-gtk2.pc.in			\
            $(NULL)

potfile:
	ls liblyric/*.c data/*.glade totem-plugin/*.c totem-plugin/*.plugin.in >po/POTFILES.in

update-po:
	make -C po update-po
