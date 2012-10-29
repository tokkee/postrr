/*
 * PostRR - src/base.c
 * Copyright (C) 2012 Sebastian 'tokkee' Harl <sh@tokkee.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * The base PostRR components.
 */

#include "postrr.h"

#include <stdio.h>
#include <string.h>

#include <postgres.h>
#include <pg_config.h>
#include <fmgr.h>

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

/*
 * prototypes for PostgreSQL functions
 */

PG_FUNCTION_INFO_V1(postrr_version);

/*
 * public API
 */

Datum
postrr_version(PG_FUNCTION_ARGS)
{
	char  version[1024];
	char *result;

	if (PG_NARGS() != 0)
		ereport(NOTICE, (errmsg("PostRR_Version() "
						"does not accept any arguments")));

	snprintf(version, sizeof(version),
			"PostgreSQL Round-Robin Extension, version %s\n"
			"Built against PostgreSQL version "PG_VERSION,
			POSTRR_VERSION_STRING POSTRR_VERSION_EXTRA);
	version[sizeof(version) - 1] = '\0';

	result = pstrdup(version);
	PG_RETURN_CSTRING(result);
} /* postrr_version */

/* vim: set tw=78 sw=4 ts=4 noexpandtab : */

