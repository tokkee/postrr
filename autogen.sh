#! /bin/sh

libtoolize=libtoolize

if which glibtoolize > /dev/null 2>&1; then
	libtoolize=glibtoolize
fi

set -ex

aclocal --force --warnings=all
$libtoolize --automake --copy --force
aclocal
autoconf --force --warnings=all
autoheader --force --warnings=all
automake --add-missing --copy --foreign --warnings=all

set +x

echo ""
echo "Run './configure' to configure the software. See './configure --help'"
echo "for details on command line options and environment variables affecting"
echo "the build."

