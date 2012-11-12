AM_CFLAGS = @STRICT_CFLAGS@

BUILT_SOURCES = postrr.h
CLEANFILES = postrr.h

all-local: postrr.h postrr.sql postrr.control Makefile.pgxs ../version-gen.sh
	@echo Building $(PACKAGE_NAME) version $$( cd ..; ./version-gen.sh )
	make -f Makefile.pgxs all

check-local: Makefile.pgxs
	make -f Makefile.pgxs check

# XXX: make it possible to install to the directories passed to configure
install-data-local: Makefile.pgxs
	make -f Makefile.pgxs install

uninstall-local: Makefile.pgxs
	make -f Makefile.pgxs uninstall

installdirs-local: Makefile.pgxs
	make -f Makefile.pgxs installdirs

installcheck-local: Makefile.pgxs
	make -f Makefile.pgxs installcheck

clean-local: Makefile.pgxs
	make -f Makefile.pgxs clean

distclean-local: Makefile.pgxs
	make -f Makefile.pgxs distclean

maintainer-clean-local: Makefile.pgxs
	make -f Makefile.pgxs maintainer-clean

../version:
	(cd ..; make version)

postrr.h: postrr.h.in ../version
	. ../version; sed \
	    -e "s/@POSTRR_VERSION_MAJOR@/$$VERSION_MAJOR/g" \
	    -e "s/@POSTRR_VERSION_MINOR@/$$VERSION_MINOR/g" \
	    -e "s/@POSTRR_VERSION_PATCH@/$$VERSION_PATCH/g" \
	    -e "s/@POSTRR_VERSION_EXTRA@/$$VERSION_EXTRA/g" \
	    -e "s/@POSTRR_VERSION_STRING@/$$VERSION_STRING/g" \
	    postrr.h.in > postrr.h

