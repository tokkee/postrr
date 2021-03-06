dnl Process this file with autoconf to produce a configure script.
dnl
dnl This is the PostRR configure script.
dnl
dnl Copyright (C) 2012 Sebastian 'tokkee' Harl <sh@tokkee.org>
dnl All rights reserved.
dnl
dnl Redistribution and use in source and binary forms, with or without
dnl modification, are permitted provided that the following conditions
dnl are met:
dnl 1. Redistributions of source code must retain the above copyright
dnl    notice, this list of conditions and the following disclaimer.
dnl 2. Redistributions in binary form must reproduce the above copyright
dnl    notice, this list of conditions and the following disclaimer in the
dnl    documentation and/or other materials provided with the distribution.
dnl
dnl THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
dnl ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
dnl TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
dnl PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
dnl CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
dnl EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
dnl PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
dnl OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
dnl WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
dnl OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
dnl ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

AC_INIT([PostgreSQL Round-Robin Extension],
		[m4_esyscmd(./version-gen.sh)],
		[sh@tokkee.org],
		[postrr],
		[http://git.tokkee.org/?p=postrr.git])
PACKAGE_MAINTAINER="Sebastian 'tokkee' Harl <sh@tokkee.org>"
AC_DEFINE_UNQUOTED([PACKAGE_MAINTAINER], ["$PACKAGE_MAINTAINER"],
		[Define to the name of the maintainer of this package.])
AC_CONFIG_SRCDIR([src/base.c])
AC_CONFIG_HEADERS([src/postrr_config.h])
AC_PREFIX_DEFAULT([/opt/postrr])

AM_INIT_AUTOMAKE([foreign -Wall])

AC_LANG(C)

AC_SYS_LARGEFILE

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AM_PROG_CC_C_O

m4_ifdef([LT_INIT],
	[
	 LT_INIT
	],
	# else
	# (older libtools)
	[
	 AC_PROG_LIBTOOL
	]
)

test_cc_flags() {
	AC_LANG_CONFTEST([AC_LANG_PROGRAM([[ ]], [[ ]])])
	$CC -c conftest.c $CFLAGS $@ > /dev/null 2> /dev/null
	ret=$?
	rm -f conftest.o
	return $ret
}

m4_divert_once([HELP_ENABLE], [
Build options:])

dnl Optionally stick to standard C99 and POSIX:2001 as close as possible.
AC_ARG_ENABLE([standards],
		AS_HELP_STRING([--enable-standards],
				[C99 / POSIX standards compliance mode @<:@default=no@:>@]),
		[enable_standards="$enableval"],
		[enable_standards="no"])

if test "x$enable_standards" = "xyes"; then
	AC_DEFINE([_ISOC99_SOURCE], 1,
			[Define to enforce ISO/IEC 9899:1999 (C99) compliance.])
	AC_DEFINE([_POSIX_C_SOURCE], 200112L,
			[Define to enforce IEEE 1003.1-2001 (POSIX:2001) compliance.])
	AC_DEFINE([_XOPEN_SOURCE], 600,
			[Define to enforce X/Open 6 (XSI) compliance.])
	AC_DEFINE([_REENTRANT], 1,
			[Define to enable reentrant interfaces.])
	AC_DEFINE([_THREAD_SAFE], 1,
			[Define to enable reentrant interfaces.])

	for flag in -std=c99 -pedantic; do
		AC_MSG_CHECKING([whether $CC accepts $flag])

		if test_cc_flags $flag; then
			CFLAGS="$CFLAGS $flag"
			AC_MSG_RESULT([yes])
		else
			AC_MSG_RESULT([no])
		fi
	done
fi

dnl Hardening (see e.g. http://wiki.debian.org/Hardening for a motivation).
AC_DEFINE([_FORTIFY_SOURCE], 2,
		[Define to enable protection against static sized buffer overflows.])
AC_ARG_ENABLE([hardening],
		AS_HELP_STRING([--disable-hardening],
				[hardening options @<:@default=yes@:>@]),
		[enable_hardening="$enableval"],
		[enable_hardening="yes"])

if test "x$enable_hardening" = "xyes"; then
	hardening=0
	hardening_tests=0
	for flag in -Wformat -Wformat-security; do
		hardening_tests=$(($hardening_tests + 1))
		AC_MSG_CHECKING([whether $CC accepts $flag])

		if test_cc_flags $flag; then
			CFLAGS="$CFLAGS $flag"
			hardening=$(($hardening + 1))
			AC_MSG_RESULT([yes])
		else
			AC_MSG_RESULT([no])
		fi
	done
	if test $hardening -ne $hardening_tests; then
		AC_MSG_WARN(
				[Some hardening options are not supported by your compiler!])
	fi
fi

dnl Strict checking for potential problems.
AC_ARG_ENABLE([strict-checks],
		AS_HELP_STRING([--disable-strict-checks],
				[strict compiler checks @<:@default=yes@:>@]),
		[enable_strict_checks="$enableval"],
		[enable_strict_checks="yes"])

STRICT_CPPFLAGS=""
for flag in -Wall -Werror; do
	AC_MSG_CHECKING([whether $CC accepts $flag])

	if test_cc_flags $flag; then
		STRICT_CPPFLAGS="$STRICT_CPPFLAGS $flag"
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
	fi
done

if test "x$enable_strict_checks" = "xyes"; then
	for flag in -Wextra \
			-Wbad-function-cast \
			-Wcast-align \
			-Wcast-qual \
			-Wconversion \
			-Wdeclaration-after-statement \
			-Wmissing-prototypes \
			-Wpointer-arith \
			-Wshadow \
			-Wstrict-prototypes \
			-Wunreachable-code; do
		AC_MSG_CHECKING([whether $CC accepts $flag])

		if test_cc_flags $flag; then
			STRICT_CPPFLAGS="$STRICT_CPPFLAGS $flag"
			AC_MSG_RESULT([yes])
		else
			AC_MSG_RESULT([no])
		fi
	done
fi
AC_SUBST([STRICT_CPPFLAGS])

AC_CHECK_HEADERS(libgen.h)

dnl Check for dependencies.
build_documentation="yes"

have_xsltproc="yes"
AC_PATH_PROG([XSLTPROC], [xsltproc])
if test "x$XSLTPROC" = "x"; then
	have_xsltproc="no"
	build_documentation="no (missing xsltproc)"
fi

have_a2x="yes"
AC_PATH_PROG([A2X], [a2x])
if test "x$A2X" = "x"; then
	have_a2x="no"
	build_documentation="no (missing a2x)"
fi
AC_SUBST([A2X])

dnl pg_config
AC_ARG_WITH([pgconfig],
		[AS_HELP_STRING([--with-pgconfig=FILE], [path to pg_config],)],
		[PG_CONFIG="$withval"], [PG_CONFIG=""])

if test "x$PG_CONFIG" = "x"; then
	AC_PATH_PROG([PG_CONFIG], [pg_config])

	if test "x$PG_CONFIG" = "x"; then
		AC_MSG_ERROR([could not find pg_config in \$PATH; rerun configure with the --with-pgconfig parameter.])
	fi
else
	AC_MSG_CHECKING([pg_config usability])
	if test "x$PG_CONFIG" = "xyes" -o "x$PG_CONFIG" = "xno"; then
		AC_MSG_ERROR([please specify a parameter for --with-pgconfig])
	else
		if ! test -f "$PG_CONFIG"; then
			AC_MSG_ERROR([the specified path to pg_config does not exist: $PG_CONFIG])
		fi
	fi
	AC_MSG_RESULT([yes])
fi
AC_SUBST([PG_CONFIG])

AC_MSG_CHECKING([for pgxs.mk])
PGXS=`$PG_CONFIG --pgxs`
if ! test -f "$PGXS"; then
	AC_MSG_ERROR([could not find PGXS Makefile: '$PGXS'])
fi
AC_SUBST([PGXS])
AC_MSG_RESULT($PGXS)

PG_VERSION=`$PG_CONFIG --version`

AC_DEFINE_UNQUOTED([PG_VERSION], ["$PG_VERSION"],
		[Define to the version of PostgreSQL the package was built against.])

POSTRR_MAJOR_VERSION=`cat version | grep VERSION_MAJOR | cut -d= -f2`
POSTRR_MINOR_VERSION=`cat version | grep VERSION_MINOR | cut -d= -f2`
POSTRR_PATCH_VERSION=`cat version | grep VERSION_PATCH | cut -d= -f2`

AC_SUBST([POSTRR_MAJOR_VERSION])
AC_SUBST([POSTRR_MINOR_VERSION])
AC_SUBST([POSTRR_PATCH_VERSION])

AM_CONDITIONAL([BUILD_DOCUMENTATION], test "x$build_documentation" = "xyes")

AC_CONFIG_FILES([Makefile doc/Makefile
		src/Makefile src/Makefile.pgxs
		src/postrr.sql src/postrr.control])
AC_OUTPUT

AC_MSG_RESULT()
AC_MSG_RESULT([$PACKAGE_NAME has been configured successfully.])
AC_MSG_RESULT()
AC_MSG_RESULT([Run 'make' to compile the software and use 'make install' to])
AC_MSG_RESULT([install the package into $prefix.])
AC_MSG_RESULT()
AC_MSG_RESULT([Configuration summary:])
AC_MSG_RESULT()
AC_MSG_RESULT([  package version: $PACKAGE_VERSION])
AC_MSG_RESULT()
AC_MSG_RESULT([  Tools:])
AC_MSG_RESULT([    AsciiDoc (a2x): . . . . . . $have_a2x])
AC_MSG_RESULT([    xsltproc: . . . . . . . . . $have_xsltproc])
AC_MSG_RESULT()
AC_MSG_RESULT([  PostgreSQL tools and helpers:])
AC_MSG_RESULT([    pg_config:  . . . . . . . . $PG_CONFIG])
AC_MSG_RESULT([    PGXS Makefile:  . . . . . . $PGXS])
AC_MSG_RESULT([    Version:  . . . . . . . . . $PG_VERSION])
AC_MSG_RESULT()
AC_MSG_RESULT([  Features:])
AC_MSG_RESULT([    documentation:  . . . . . . $build_documentation])
AC_MSG_RESULT()
AC_MSG_RESULT([This package is maintained by $PACKAGE_MAINTAINER.])
AC_MSG_RESULT([Please report bugs to $PACKAGE_BUGREPORT.])
AC_MSG_RESULT()

