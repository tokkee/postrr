/*
 * PostRR - src/utils/pg_spi.c
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
 * Helpers for using the PostgreSQL Server Programming Interface (SPI).
 */

#include "utils/pg_spi.h"

#include <postgres.h>
#include <executor/spi.h>

#include <stdarg.h>

/*
 * public API
 */

int
pg_spi_get_int(const char *query, int num_vals, ...)
{
	int spi_rc;
	int i;

	va_list ap;

	spi_rc = SPI_exec(query, /* max num rows = */ 1);
	if (spi_rc != SPI_OK_SELECT) {
		errmsg("failed to execute query: %s",
			SPI_result_code_string(spi_rc));
		return PG_SPI_MAKE_RC(ERROR_EXEC_QUERY, spi_rc);
	}

	if (SPI_processed <= 0)
		return PG_SPI_MAKE_RC(ERROR_NO_VALUES, 0);

	va_start(ap, num_vals);
	/* column count starts at 1 */
	for (i = 1; i <= num_vals; ++i) {
		char  *tmp;
		int32 *valp;

		valp = va_arg(ap, int32 *);

		tmp = SPI_getvalue(SPI_tuptable->vals[0],
				SPI_tuptable->tupdesc, /* col = */ i);

		*valp = atoi(tmp);
	}
	va_end(ap);
	return PG_SPI_OK;
} /* pg_spi_get_int */

int
pg_spi_ereport(int elevel, const char *context, int rc)
{
	int status     = PG_SPI_GET_STATUS(rc);
	int spi_status = PG_SPI_GET_SPI_STATUS(rc);

	switch (status) {
		case PG_SPI_OK:
			return 0;
		case PG_SPI_ERROR_EXEC_QUERY:
			ereport(elevel, (
						errmsg("failed to %s: failed to execute query: %s",
							context, SPI_result_code_string(spi_status))
					));
		case PG_SPI_ERROR_NO_VALUES:
			ereport(elevel, (
						errmsg("failed to %s: "
							"query did not return any values",
							context)
					));
	}
	return 0;
} /* pg_spi_ereport */

/* vim: set tw=78 sw=4 ts=4 noexpandtab : */

