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

enum {
	PG_SPI_OK = 0,
	PG_SPI_ERROR_EXEC_QUERY,
	PG_SPI_ERROR_NO_VALUES,
};

#define PG_SPI_GET_STATUS(rc) ((rc) & 0xF)
#define PG_SPI_GET_SPI_STATUS(rc) ((rc) >> 4)
#define PG_SPI_MAKE_RC(status, spi_status) \
	(((spi_status) << 4) | ((PG_SPI_ ## status) & 0xF))

int
pg_spi_get_int(const char *query, int num_vals, ...);

int
pg_spi_ereport(int elevel, const char *context, int rc);

/* vim: set tw=78 sw=4 ts=4 noexpandtab : */

