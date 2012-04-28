SUBDIRS = src
if BUILD_DOCUMENTATION
SUBDIRS += doc
endif

EXTRA_DIST = autogen.sh version-gen.sh
CLEANFILES = version

version: ./version-gen.sh
	@./version-gen.sh > /dev/null

