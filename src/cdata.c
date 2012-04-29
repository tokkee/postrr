/*
 * PostRR - src/cdata.c
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
 * A PostgreSQL data-type providing consolidated data points.
 */

#include "postrr.h"

#include <errno.h>

#include <postgres.h>
#include <fmgr.h>

/* Postgres utilities */
#include <utils/array.h>

/*
 * data type
 */

struct cdata {
	double value;
	int32 undef_num;
	int32 val_num;
	int32 cf;
};

/*
 * prototypes for PostgreSQL functions
 */

PG_FUNCTION_INFO_V1(cdata_validate);

PG_FUNCTION_INFO_V1(cdata_in);
PG_FUNCTION_INFO_V1(cdata_out);
PG_FUNCTION_INFO_V1(cdata_typmodin);
PG_FUNCTION_INFO_V1(cdata_typmodout);

PG_FUNCTION_INFO_V1(cdata_to_cdata);

/*
 * public API
 */

Datum
cdata_validate(PG_FUNCTION_ARGS)
{
	char   type_info[1024];
	char  *result;
	size_t req_len;
	size_t len;

	if (PG_NARGS() != 1)
		ereport(ERROR, (
					errmsg("cdata_validate() expect one argument"),
					errhint("Usage cdata_validate(expected_size)")
				));

	req_len = (size_t)PG_GETARG_UINT32(0);
	len = sizeof(cdata_t);

	if (req_len != len)
		ereport(ERROR, (
					errmsg("length of the cdata type "
						"does not match the expected length"),
					errhint("Please report a bug against PostRR")
				));

	snprintf(type_info, sizeof(type_info),
			"cdata validated successfully; type length = %zu", len);
	type_info[sizeof(type_info) - 1] = '\0';

	result = pstrdup(type_info);
	PG_RETURN_CSTRING(result);
} /* cdata_validate */

Datum
cdata_in(PG_FUNCTION_ARGS)
{
	cdata_t *data;
	int32 typmod;

	char *val_str, *orig;
	char *endptr = NULL;

	if (PG_NARGS() != 3)
		ereport(ERROR, (
					errmsg("cdata_in() expects three arguments"),
					errhint("Usage: cdata_in(col_name, oid, typmod)")
				));

	data = (cdata_t *)palloc0(sizeof(*data));

	val_str = PG_GETARG_CSTRING(0);
	typmod  = PG_GETARG_INT32(2);

	orig = val_str;
	while ((*val_str != '\0') && isspace((int)*val_str))
		++val_str;

	if (*val_str == '\0')
		ereport(ERROR, (
					errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					errmsg("invalid input syntax for cdata: \"%s\"", orig)
				));

	errno = 0;
	data->value   = strtod(val_str, &endptr);
	data->val_num = 1;

	if ((endptr == val_str) || errno)
		ereport(ERROR, (
					errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					errmsg("invalid input syntax for cdata: \"%s\"", orig)
				));

	while ((*endptr != '\0') && isspace((int)*endptr))
		ereport(ERROR, (
					errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					errmsg("invalid input syntax for cdata: \"%s\"", orig)
				));

	if (typmod > 0)
		data->cf = typmod;
	else
		data->cf = 0;

	PG_RETURN_CDATA_P(data);
} /* cdata_in */

Datum
cdata_out(PG_FUNCTION_ARGS)
{
	cdata_t *data;

	char  cd_str[1024];
	char *result;

	if (PG_NARGS() != 1)
		ereport(ERROR, (
					errmsg("cdata_out() expects one argument"),
					errhint("Usage: cdata_out(cdata)")
				));

	data = PG_GETARG_CDATA_P(0);

	snprintf(cd_str, sizeof(cd_str), "%g (U:%i/%i)",
			data->value, data->undef_num, data->val_num);

	result = pstrdup(cd_str);
	PG_RETURN_CSTRING(result);
} /* cdata_out */

/* vim: set tw=78 sw=4 ts=4 noexpandtab : */

