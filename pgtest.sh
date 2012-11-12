#! /bin/bash
#
# PostRR test setup helper script
#
# Copyright (C) 2012 Sebastian 'tokkee' Harl <sh@tokkee.org>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

if test -z "$PG_CONFIG"; then
	PG_CONFIG=`which pg_config`
fi
if test -z "$PG_CONFIG"; then
	echo "pg_config not found!" >&2
	exit 1
fi

BIN_DIR=`$PG_CONFIG --bindir`
if test -z "$TARGET"; then
	TARGET=`pwd`/target
fi

PWD_esc="$( echo "$PWD" | sed -e 's/\//\\\//g' )"
TARGET_esc="$( echo "$TARGET" | sed -e 's/\//\\\//g' )"

set -e

case "$1" in
	setup)
		if test $# -ne 1; then
			echo "Too many arguments!" >&2
			echo "Usage: $0 setup" >&2
			exit 1
		fi
		mkdir -p $TARGET/var/lib/postgresql/main
		mkdir -p $TARGET/var/run/postgresql
		mkdir -p $TARGET/var/log/postgresql/main
		$BIN_DIR/initdb -D $TARGET/var/lib/postgresql/main
		sed -r -i -e 's/^#port = 5432/port = 2345/' \
			$TARGET/var/lib/postgresql/main/postgresql.conf
		sed -r -i -e "s/^#dynamic_library_path = '\\\$libdir'/dynamic_library_path = '\$libdir:$PWD_esc\/src'/" $TARGET/var/lib/postgresql/main/postgresql.conf
		sed -r -i -e "s/^#unix_socket_directory = ''/unix_socket_directory = '$TARGET_esc\/var\/run\/postgresql'/" $TARGET/var/lib/postgresql/main/postgresql.conf
		$0 start -B
		$BIN_DIR/createdb -e -h $TARGET/var/run/postgresql/ -p 2345 "$( id -un )"
		$0 stop
		;;
	client)
		libedit=$( ldd $BIN_DIR/psql 2> /dev/null \
			| grep -Eo '=> [^ ]+\/libedit\.so\..? ' | cut -d' ' -f2 )
		if test -n "$libedit"; then
			libdir=${libedit/libedit.*/}
			libreadline=$( ls $libdir/libreadline.so* 2> /dev/null | head -n1 )
			if test -n "$libreadline"; then
				export LD_PRELOAD="$LD_PRELOAD:$libreadline"
			fi
		fi
		shift
		$BIN_DIR/psql -h $TARGET/var/run/postgresql/ -p 2345 "$@"
		;;
	stop)
		if test $# -ne 1; then
			echo "Too many arguments!" >&2
			echo "Usage: $0 stop" >&2
			exit 1
		fi
		$BIN_DIR/pg_ctl -D $TARGET/var/lib/postgresql/main stop
		;;
	start)
		shift
		if test "$1" = "-B"; then
			shift
			$BIN_DIR/pg_ctl -D $TARGET/var/lib/postgresql/main -l logfile -w start "$@"
		else
			$BIN_DIR/postgres -D $TARGET/var/lib/postgresql/main "$@"
		fi
		;;
	restart)
		$0 stop && $0 start -B
		;;
	dump)
		shift
		$BIN_DIR/pg_dump -h $TARGET/var/run/postgresql/ -p 2345 "$@"
		;;
	restore)
		shift
		$BIN_DIR/pg_restore -h $TARGET/var/run/postgresql/ -p 2345 "$@"
		;;
	*)
		echo "Usage: $0 setup|client|stop|start" >&2
		echo ""
		echo "  - setup"
		echo "    Set up a new PostgreSQL server listening on port 2345."
		echo "  - client [<psql args>]"
		echo "    Start a PostgreSQL interactive terminal connected to the"
		echo "    PostgreSQL server on port 2345."
		echo "  - start [-B [<postgres args>]]"
		echo "    Start the PostgreSQL server."
		echo "  - stop"
		echo "    Stop the PostgreSQL server."
		echo "  - restart"
		echo "    Restart a background PostgreSQL server process."
		echo ""
		echo "Environment variables:"
		echo "  - TARGET"
		echo "    Target directory of the PostgreSQL test setup."
		if test "$1" = "help"; then
			exit 0
		fi
		exit 1
		;;
esac

# vim: set tw=0 :

