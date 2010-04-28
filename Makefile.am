SUBDIRS =           \
        liblyric    \
        po \
        $(NULL)


DISTCLEANFILES = \
  intltool-extract \
  intltool-merge \
  intltool-update

EXTRA_DIST= intltool-extract.in         \
            intltool-merge.in           \
            intltool-update.in          \
            configure.in                \
            autogen.sh                  \
            $(NULL)

potfile:
	ls liblyric/*.c >po/POTFILES.in

update-po:
	make -C po update-po
