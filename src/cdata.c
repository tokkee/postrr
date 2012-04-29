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
#include <string.h>

#include <postgres.h>
#include <fmgr.h>

/* Postgres utilities */
#include <catalog/pg_type.h>
#include <utils/array.h>

enum {
	CF_AVG = 0,
	CF_MIN = 1,
	CF_MAX = 2
};

#define CF_TO_STR(cf) \
	(((cf) == CF_AVG) \
		? "AVG" \
		: ((cf) == CF_MIN) \
			? "MIN" \
			: ((cf) == CF_MAX) \
				? "MAX" : "UNKNOWN")

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

Datum
cdata_typmodin(PG_FUNCTION_ARGS)
{
	ArrayType *tm_array;

	Datum *elem_values;
	int    n;
	char  *cf_str;
	int32  typmod = CF_AVG;

	if (PG_NARGS() != 1)
		ereport(ERROR, (
					errmsg("cdata_typmodin() expects one argument"),
					errhint("Usage: cdata_typmodin(array)")
				));

	tm_array = PG_GETARG_ARRAYTYPE_P(0);

	if (ARR_ELEMTYPE(tm_array) != CSTRINGOID)
		ereport(ERROR, (
					errcode(ERRCODE_ARRAY_ELEMENT_ERROR),
					errmsg("typmod array must be type cstring[]")
				));

	if (ARR_NDIM(tm_array) != 1)
		ereport(ERROR, (
					errcode(ERRCODE_ARRAY_SUBSCRIPT_ERROR),
					errmsg("typmod array must be one-dimensional")
				));

	deconstruct_array(tm_array, CSTRINGOID,
			/* elmlen = */ -2, /* elmbyval = */ false, /* elmalign = */ 'c',
			&elem_values, /* nullsp = */ NULL, &n);

	if (n != 1)
		ereport(ERROR, (
					errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("cdata typmod array must have one element")
				));

	cf_str = DatumGetCString(elem_values[0]);
	if (! strcasecmp(cf_str, "AVG"))
		typmod = CF_AVG;
	else if (! strcasecmp(cf_str, "MIN"))
		typmod = CF_MIN;
	else if (! strcasecmp(cf_str, "MAX"))
		typmod = CF_MAX;
	else
		ereport(ERROR, (
					errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("invalid cdata typmod: %s", cf_str)
				));

	PG_RETURN_INT32(typmod);
} /* cdata_typmodin */

Datum
cdata_typmodout(PG_FUNCTION_ARGS)
{
	int32 typmod;
	char  tm_str[32];
	char *result;

	if (PG_NARGS() != 1)
		ereport(ERROR, (
					errmsg("cdata_typmodout() expects one argument"),
					errhint("Usage: cdata_typmodout(typmod)")
				));

	typmod = PG_GETARG_INT32(0);
	snprintf(tm_str, sizeof(tm_str), "('%s')", CF_TO_STR(typmod));
	tm_str[sizeof(tm_str) - 1] = '\0';
	result = pstrdup(tm_str);
	PG_RETURN_CSTRING(result);
} /* cdata_typmodout */

Datum
cdata_to_cdata(PG_FUNCTION_ARGS)
{
	cdata_t *data;
	int32 typmod;

	if (PG_NARGS() != 3)
		ereport(ERROR, (
					errmsg("cdata_to_cdata() "
						"expects three arguments"),
					errhint("Usage: cdata_to_cdata"
						"(cdata, typmod, is_explicit)")
				));

	data   = PG_GETARG_CDATA_P(0);
	typmod = PG_GETARG_INT32(1);

	if ((data->cf >= 0) && (data->cf != typmod) && (data->val_num > 1))
		ereport(ERROR, (
					errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("invalid cast: cannot cast cdata "
						"with different typmod (yet)")
				));

	data->cf = typmod;
	PG_RETURN_CDATA_P(data);
} /* cdata_to_cdata */

/* vim: set tw=78 sw=4 ts=4 noexpandtab : */

