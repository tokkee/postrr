/*
 * PostRR - src/rrtimeslice.c
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
 * A PostgreSQL data-type providing a timeslice implementing round-robin
 * features.
 */

#include "postrr.h"
#include "utils/pg_spi.h"

#include <string.h>

#include <postgres.h>
#include <fmgr.h>

/* Postgres utilities */
#include <access/hash.h>
#include <executor/spi.h>
#include <utils/array.h>
#include <utils/datetime.h>
#include <utils/timestamp.h>
#include <miscadmin.h> /* DateStyle */

#ifdef HAVE_INT64_TIMESTAMP
#	define TSTAMP_TO_INT64(t) (t)
#	define INT64_TO_TSTAMP(i) (i)
#else /* ! HAVE_INT64_TIMESTAMP */
#	define TSTAMP_TO_INT64(t) (int64)((t) * (double)USECS_PER_SEC)
#	define INT64_TO_TSTAMP(i) ((double)(i) / (double)USECS_PER_SEC)
#endif

/*
 * data type
 */

struct rrtimeslice {
	TimestampTz tstamp;
	int32  tsid;
	uint32 seq;
};

/*
 * internal helper functions
 */

static int32
rrtimeslice_set_spec(int32 len, int32 num)
{
	int spi_rc;

	char  query[256];
	int32 typmod = 0;

	if ((len <= 0) || (num <= 0))
		ereport(ERROR, (
					errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("rrtimeslice(%i, %i) "
						"length/num may not be less than zero",
						len, num)
				));

	if ((spi_rc = SPI_connect()) != SPI_OK_CONNECT)
		ereport(ERROR, (
					errmsg("failed to store rrtimeslice spec: "
						"could not connect to SPI manager: %s",
						SPI_result_code_string(spi_rc))
				));

	snprintf(query, sizeof(query),
			"SELECT tsid FROM postrr.rrtimeslices "
				"WHERE tslen = %d AND tsnum = %d "
				"LIMIT 1", len, num);
	query[sizeof(query) - 1] = '\0';

	spi_rc = pg_spi_get_int(query, 1, &typmod);
	if (spi_rc == PG_SPI_OK) {
		SPI_finish();
		return typmod;
	}
	else if (spi_rc != PG_SPI_ERROR_NO_VALUES)
		pg_spi_ereport(ERROR, "store rrtimeslice spec", spi_rc);

	snprintf(query, sizeof(query),
			"SELECT nextval('postrr.tsid'::regclass)");
	query[sizeof(query) - 1] = '\0';

	spi_rc = pg_spi_get_int(query, 1, &typmod);
	if ((spi_rc != PG_SPI_OK) || (typmod <= 0))
		pg_spi_ereport(ERROR, "retrieve nextval(postrr.tsid)", spi_rc);

	snprintf(query, sizeof(query),
			"INSERT INTO postrr.rrtimeslices(tsid, tslen, tsnum) "
			"VALUES (%d, %d, %d)", typmod, len, num);
	query[sizeof(query) - 1] = '\0';

	spi_rc = SPI_exec(query, /* max num rows = */ 1);
	if (spi_rc != SPI_OK_INSERT)
		ereport(ERROR, (
					errmsg("failed to store rrtimeslice spec: "
						"failed to execute query: %s",
						SPI_result_code_string(spi_rc))
				));

	SPI_finish();
	return typmod;
} /* rrtimeslice_set_spec */

static int
rrtimeslice_get_spec(int32 typmod, int32 *len, int32 *num)
{
	int spi_rc;

	char query[256];

	if (typmod <= 0)
		return -1;

	if ((spi_rc = SPI_connect()) != SPI_OK_CONNECT)
		ereport(ERROR, (
					errmsg("failed to determine rrtimeslice spec: "
						"could not connect to SPI manager: %s",
						SPI_result_code_string(spi_rc))
				));

	snprintf(query, sizeof(query),
			"SELECT tslen, tsnum FROM postrr.rrtimeslices "
				"WHERE tsid = %d", typmod);
	query[sizeof(query) - 1] = '\0';

	spi_rc = pg_spi_get_int(query, 2, len, num);
	if (spi_rc != PG_SPI_OK)
		pg_spi_ereport(ERROR, "determine rrtimeslice spec", spi_rc);

	SPI_finish();
	return 0;
} /* rrtimeslice_get_spec */

static int
rrtimeslice_apply_typmod(rrtimeslice_t *tslice, int32 typmod)
{
	int64 tstamp;
	int64 length;
	int64 seq;

	int32 len = 0;
	int32 num = 0;

	if (rrtimeslice_get_spec(typmod, &len, &num))
		return -1;

	if ((len <= 0) || (num <= 0))
		ereport(ERROR, (
					errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("rrtimeslice(%i, %i) "
						"length/num may not be less than zero",
						len, num)
				));

	tstamp = TSTAMP_TO_INT64(tslice->tstamp);

	length = len * USECS_PER_SEC;
	if (tstamp % length != 0)
		tstamp = tstamp - (tstamp % length) + length;
	seq    = tstamp % (length * num) / length;
	seq    = seq % num;

	tslice->tstamp = INT64_TO_TSTAMP(tstamp);
	tslice->tsid   = typmod;
	tslice->seq    = (uint32)seq;
	return 0;
} /* rrtimeslice_apply_typmod */

/*
 * rrtimeslice_cmp_unify:
 * Unify two RRTimeslices in order to prepare them for comparison. That is, if
 * either one of the arguments does not have any typmod applied, then apply
 * the typmod of the other argument. Throws an error if the typmods don't
 * match.
 *
 * Returns:
 *  - 0 if the arguments could be unified
 *  - 1 if only the first argument is NULL
 *  - 2 if both arguments are NULL
 *  - 3 if only the second argument is NULL
 */
static int
rrtimeslice_cmp_unify(rrtimeslice_t *ts1, rrtimeslice_t *ts2)
{
	if ((! ts1) && (! ts2))
		return 0;
	else if (! ts1)
		return -1;
	else if (! ts2)
		return 1;

	if (ts1->tsid && (! ts2->tsid))
		rrtimeslice_apply_typmod(ts2, ts1->tsid);
	else if ((! ts1->tsid) && ts2->tsid)
		rrtimeslice_apply_typmod(ts1, ts2->tsid);

	if (ts1->tsid != ts2->tsid) /* XXX: compare len/num */
		ereport(ERROR, (
					errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("invalid comparison: cannot compare "
						"rrtimeslices with different typmods (yet)")
				));
	return 0;
} /* rrtimeslice_cmp_unify */

/*
 * prototypes for PostgreSQL functions
 */

PG_FUNCTION_INFO_V1(rrtimeslice_validate);

PG_FUNCTION_INFO_V1(rrtimeslice_in);
PG_FUNCTION_INFO_V1(rrtimeslice_out);
PG_FUNCTION_INFO_V1(rrtimeslice_typmodin);
PG_FUNCTION_INFO_V1(rrtimeslice_typmodout);

PG_FUNCTION_INFO_V1(rrtimeslice_to_rrtimeslice);
PG_FUNCTION_INFO_V1(timestamptz_to_rrtimeslice);
PG_FUNCTION_INFO_V1(rrtimeslice_to_timestamptz);

PG_FUNCTION_INFO_V1(rrtimeslice_cmp);

PG_FUNCTION_INFO_V1(rrtimeslice_seq_eq);
PG_FUNCTION_INFO_V1(rrtimeslice_seq_ne);
PG_FUNCTION_INFO_V1(rrtimeslice_seq_lt);
PG_FUNCTION_INFO_V1(rrtimeslice_seq_gt);
PG_FUNCTION_INFO_V1(rrtimeslice_seq_le);
PG_FUNCTION_INFO_V1(rrtimeslice_seq_ge);
PG_FUNCTION_INFO_V1(rrtimeslice_seq_cmp);
PG_FUNCTION_INFO_V1(rrtimeslice_seq_hash);

/*
 * public API
 */

Datum
rrtimeslice_validate(PG_FUNCTION_ARGS)
{
	char   type_info[1024];
	char  *result;
	size_t req_len;
	size_t len;

	if (PG_NARGS() != 1)
		ereport(ERROR, (
					errmsg("rrtimeslice_validate() expect one argument"),
					errhint("Usage rrtimeslice_validate(expected_size)")
				));

	req_len = (size_t)PG_GETARG_UINT32(0);
	len = sizeof(rrtimeslice_t);

	if (req_len != len)
		ereport(ERROR, (
					errmsg("length of the rrtimeslice type "
						"does not match the expected length"),
					errhint("Please report a bug against PostRR")
				));

	snprintf(type_info, sizeof(type_info),
			"rrtimeslice validated successfully; type length = %zu", len);
	type_info[sizeof(type_info) - 1] = '\0';

	result = pstrdup(type_info);
	PG_RETURN_CSTRING(result);
} /* rrtimeslice_validate */

Datum
rrtimeslice_in(PG_FUNCTION_ARGS)
{
	rrtimeslice_t *tslice;

	TimestampTz tstamp = 0;
	int32 typmod;

	struct pg_tm tm;
	fsec_t fsec = 0;
	int tz = 0;

	char *time_str;
	int   pg_dt_err;
	char  buf[MAXDATELEN + MAXDATEFIELDS];
	char *field[MAXDATEFIELDS];
	int   ftype[MAXDATEFIELDS];
	int   num_fields = 0;
	int   dtype = 0;

	if (PG_NARGS() != 3)
		ereport(ERROR, (
					errmsg("rrtimeslice_in() expects three arguments"),
					errhint("Usage: rrtimeslice_in(col_name, oid, typmod)")
				));

	tslice   = (rrtimeslice_t *)palloc0(sizeof(*tslice));
	time_str = PG_GETARG_CSTRING(0);
	typmod   = PG_GETARG_INT32(2);

	pg_dt_err = ParseDateTime(time_str, buf, sizeof(buf),
			field, ftype, MAXDATEFIELDS, &num_fields);

	if (! pg_dt_err)
		pg_dt_err = DecodeDateTime(field, ftype, num_fields,
				&dtype, &tm, &fsec, &tz);
	if (pg_dt_err)
		DateTimeParseError(pg_dt_err, time_str, "rrtimeslice");

	switch (dtype) {
		case DTK_DATE:
			if (tm2timestamp(&tm, fsec, &tz, &tstamp))
				ereport(ERROR, (
							errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
							errmsg("timestamp out of range: %s", time_str)
						));
			break;

		case DTK_EPOCH:
			tstamp = SetEpochTimestamp();
			break;

		default:
			ereport(ERROR, (
						errmsg("unexpected dtype %d while "
							"parsing rrtimeslice: %s", dtype, time_str)
					));
	}

	tslice->tstamp = tstamp;

	/* most likely, this won't happen … coerce_type
	 * (src/backend/parser/parse_coerce.c) does not pass that information to
	 * the input function but rather lets a length conversion cast do that
	 * work */
	if (typmod > 0)
		rrtimeslice_apply_typmod(tslice, typmod);

	PG_RETURN_RRTIMESLICE_P(tslice);
} /* rrtimeslice_in */

Datum
rrtimeslice_out(PG_FUNCTION_ARGS)
{
	rrtimeslice_t *tslice;

	struct pg_tm tm;
	fsec_t fsec = 0;
	int tz = 0;

	char *tz_str = NULL;

	char  ts_str[MAXDATELEN + 1];
	char  buf_l[MAXDATELEN + 1];
	char  buf_u[MAXDATELEN + 1];
	char *result;

	int32 len = 0;
	int32 num = 0;

	if (PG_NARGS() != 1)
		ereport(ERROR, (
					errmsg("rrtimeslice_out() expects one argument"),
					errhint("Usage: rrtimeslice_out(rrtimeslice)")
				));

	tslice = PG_GETARG_RRTIMESLICE_P(0);

	if (TIMESTAMP_NOT_FINITE(tslice->tstamp)
			|| timestamp2tm(tslice->tstamp, &tz, &tm, &fsec, &tz_str, NULL))
		ereport(ERROR, (
					errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
					errmsg("invalid (non-finite) timestamp")
				));

	EncodeDateTime(&tm, fsec, &tz, &tz_str, DateStyle, buf_u);

	if (! rrtimeslice_get_spec(tslice->tsid, &len, &num)) {
		TimestampTz lower = tslice->tstamp - (len * USECS_PER_SEC);

		if (timestamp2tm(lower, &tz, &tm, &fsec, &tz_str, NULL))
			ereport(ERROR, (
						errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
						errmsg("invalid (non-finite) lower timestamp")
					));

		EncodeDateTime(&tm, fsec, &tz, &tz_str, DateStyle, buf_l);
	}
	else {
		strncpy(buf_l, "ERR", sizeof(buf_l));
		buf_l[sizeof(buf_l) - 1] = '\0';
	}

	snprintf(ts_str, sizeof(ts_str), "(\"%s\", \"%s\"] #%i/%i",
			buf_l, buf_u, tslice->seq, num);

	result = pstrdup(ts_str);
	PG_RETURN_CSTRING(result);
} /* rrtimeslice_out */

Datum
rrtimeslice_typmodin(PG_FUNCTION_ARGS)
{
	ArrayType *tm_array;

	int32 *spec;
	int    spec_elems = 0;
	int32  typmod;

	if (PG_NARGS() != 1)
		ereport(ERROR, (
					errmsg("rrtimeslice_typmodin() expects one argument"),
					errhint("Usage: rrtimeslice_typmodin(array)")
				));

	tm_array = PG_GETARG_ARRAYTYPE_P(0);

	spec = ArrayGetIntegerTypmods(tm_array, &spec_elems);
	if (spec_elems != 2)
		ereport(ERROR, (
					errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("invalid rrtimeslice type modifier"),
					errhint("Usage: rrtimeslice(<slice_len>, <num>)")
				));

	typmod = rrtimeslice_set_spec(spec[0], spec[1]);
	PG_RETURN_INT32(typmod);
} /* rrtimeslice_typmodin */

Datum
rrtimeslice_typmodout(PG_FUNCTION_ARGS)
{
	int32 typmod;
	char  tm_str[1024];
	char *result;

	int32 len = 0;
	int32 num = 0;

	if (PG_NARGS() != 1)
		ereport(ERROR, (
					errmsg("rrtimeslice_typmodout() expects one argument"),
					errhint("Usage: rrtimeslice_typmodout(typmod)")
				));

	typmod = PG_GETARG_INT32(0);
	if (rrtimeslice_get_spec(typmod, &len, &num))
		tm_str[0] = '\0';
	else if ((len <= 0) || (num <= 0))
		snprintf(tm_str, sizeof(tm_str), "(#ERR, #ERR)");
	else
		snprintf(tm_str, sizeof(tm_str), "(%d, %d)", len, num);

	result = pstrdup(tm_str);
	PG_RETURN_CSTRING(result);
} /* rrtimeslice_typmodout */

Datum
rrtimeslice_to_rrtimeslice(PG_FUNCTION_ARGS)
{
	rrtimeslice_t *tslice;
	int32 typmod;

	if (PG_NARGS() != 3)
		ereport(ERROR, (
					errmsg("rrtimeslice_to_rrtimeslice() "
						"expects three arguments"),
					errhint("Usage: rrtimeslice_to_rrtimeslice"
						"(rrtimeslice, typmod, is_explicit)")
				));

	tslice = PG_GETARG_RRTIMESLICE_P(0);
	typmod = PG_GETARG_INT32(1);

	if (typmod > 0) {
		if ((! tslice->tsid) && (! tslice->seq))
			rrtimeslice_apply_typmod(tslice, typmod);
		else
			ereport(ERROR, (
						errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("invalid cast: cannot cast rrtimeslices "
							"with different typmod (yet)")
					));
	}

	PG_RETURN_RRTIMESLICE_P(tslice);
} /* rrtimeslice_to_rrtimeslice */

Datum
timestamptz_to_rrtimeslice(PG_FUNCTION_ARGS)
{
	TimestampTz tstamp;
	int32 typmod;

	rrtimeslice_t *tslice;

	if (PG_NARGS() != 3)
		ereport(ERROR, (
					errmsg("timestamptz_to_rrtimeslice() "
						"expects three arguments"),
					errhint("Usage: timestamptz_to_rrtimeslice"
						"(timestamptz, typmod, is_explicit)")
				));

	tstamp = PG_GETARG_TIMESTAMPTZ(0);
	typmod = PG_GETARG_INT32(1);

	tslice = (rrtimeslice_t *)palloc0(sizeof(*tslice));

	tslice->tstamp = tstamp;
	if (typmod >= 0)
		rrtimeslice_apply_typmod(tslice, typmod);

	PG_RETURN_RRTIMESLICE_P(tslice);
} /* timestamptz_to_rrtimeslice */

Datum
rrtimeslice_to_timestamptz(PG_FUNCTION_ARGS)
{
	rrtimeslice_t *tslice;

	if (PG_NARGS() != 1)
		ereport(ERROR, (
					errmsg("rrtimeslice_to_timestamptz() "
						"expects one argument"),
					errhint("Usage: rrtimeslice_to_timestamptz"
						"(rrtimeslice)")
				));

	tslice = PG_GETARG_RRTIMESLICE_P(0);
	PG_RETURN_TIMESTAMPTZ(tslice->tstamp);
} /* rrtimeslice_to_timestamptz */

int
rrtimeslice_cmp_internal(rrtimeslice_t *ts1, rrtimeslice_t *ts2)
{
	int status;

	status = rrtimeslice_cmp_unify(ts1, ts2);
	if (status) /* [1, 3] -> [-1, 1] */
		return status - 2;

	if (ts1->tstamp == ts2->tstamp)
		return 0;
	else if ((ts1->seq == ts2->seq) && (ts1->tstamp < ts2->tstamp))
		return -1;
	else if (ts1->tstamp < ts2->tstamp)
		return -2;
	else if ((ts1->seq == ts2->seq) && (ts1->tstamp > ts2->tstamp))
		return 1;
	else
		return 2;
} /* rrtimeslice_cmp_internal */

Datum
rrtimeslice_cmp(PG_FUNCTION_ARGS)
{
	rrtimeslice_t *ts1 = PG_GETARG_RRTIMESLICE_P(0);
	rrtimeslice_t *ts2 = PG_GETARG_RRTIMESLICE_P(1);

	PG_RETURN_INT32(rrtimeslice_cmp_internal(ts1, ts2));
} /* rrtimeslice_cmp */

int
rrtimeslice_seq_cmp_internal(rrtimeslice_t *ts1, rrtimeslice_t *ts2)
{
	int status;

	status = rrtimeslice_cmp_unify(ts1, ts2);
	if (status) /* [1, 3] -> [-1, 1] */
		return status - 2;

	if (ts1->seq < ts2->seq)
		return -1;
	else if (ts1->seq == ts2->seq)
		return 0;
	else
		return 1;
} /* rrtimeslice_seq_cmp_internal */

Datum
rrtimeslice_seq_eq(PG_FUNCTION_ARGS)
{
	rrtimeslice_t *ts1 = PG_GETARG_RRTIMESLICE_P(0);
	rrtimeslice_t *ts2 = PG_GETARG_RRTIMESLICE_P(1);

	PG_RETURN_BOOL(rrtimeslice_seq_cmp_internal(ts1, ts2) == 0);
} /* rrtimeslice_seq_eq */

Datum
rrtimeslice_seq_ne(PG_FUNCTION_ARGS)
{
	rrtimeslice_t *ts1 = PG_GETARG_RRTIMESLICE_P(0);
	rrtimeslice_t *ts2 = PG_GETARG_RRTIMESLICE_P(1);

	PG_RETURN_BOOL(rrtimeslice_seq_cmp_internal(ts1, ts2) != 0);
} /* rrtimeslice_seq_ne */

Datum
rrtimeslice_seq_lt(PG_FUNCTION_ARGS)
{
	rrtimeslice_t *ts1 = PG_GETARG_RRTIMESLICE_P(0);
	rrtimeslice_t *ts2 = PG_GETARG_RRTIMESLICE_P(1);

	PG_RETURN_BOOL(rrtimeslice_seq_cmp_internal(ts1, ts2) < 0);
} /* rrtimeslice_seq_lt */

Datum
rrtimeslice_seq_le(PG_FUNCTION_ARGS)
{
	rrtimeslice_t *ts1 = PG_GETARG_RRTIMESLICE_P(0);
	rrtimeslice_t *ts2 = PG_GETARG_RRTIMESLICE_P(1);

	PG_RETURN_BOOL(rrtimeslice_seq_cmp_internal(ts1, ts2) <= 0);
} /* rrtimeslice_seq_le */

Datum
rrtimeslice_seq_gt(PG_FUNCTION_ARGS)
{
	rrtimeslice_t *ts1 = PG_GETARG_RRTIMESLICE_P(0);
	rrtimeslice_t *ts2 = PG_GETARG_RRTIMESLICE_P(1);

	PG_RETURN_BOOL(rrtimeslice_seq_cmp_internal(ts1, ts2) > 0);
} /* rrtimeslice_seq_gt */

Datum
rrtimeslice_seq_ge(PG_FUNCTION_ARGS)
{
	rrtimeslice_t *ts1 = PG_GETARG_RRTIMESLICE_P(0);
	rrtimeslice_t *ts2 = PG_GETARG_RRTIMESLICE_P(1);

	PG_RETURN_BOOL(rrtimeslice_seq_cmp_internal(ts1, ts2) >= 0);
} /* rrtimeslice_seq_ge */

Datum
rrtimeslice_seq_cmp(PG_FUNCTION_ARGS)
{
	rrtimeslice_t *ts1 = PG_GETARG_RRTIMESLICE_P(0);
	rrtimeslice_t *ts2 = PG_GETARG_RRTIMESLICE_P(1);

	PG_RETURN_INT32(rrtimeslice_seq_cmp_internal(ts1, ts2));
} /* rrtimeslice_seq_cmp */

Datum
rrtimeslice_seq_hash(PG_FUNCTION_ARGS)
{
	rrtimeslice_t *ts = PG_GETARG_RRTIMESLICE_P(0);
	return hash_uint32(ts->seq);
} /* rrtimeslice_seq_hash */

/* vim: set tw=78 sw=4 ts=4 noexpandtab : */

