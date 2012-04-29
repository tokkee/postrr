-- PostRR - src/uninstall_postrr.sql
-- Copyright (C) 2012 Sebastian 'tokkee' Harl <sh@tokkee.org>
-- All rights reserved.
--
-- Redistribution and use in source and binary forms, with or without
-- modification, are permitted provided that the following conditions
-- are met:
-- 1. Redistributions of source code must retain the above copyright
--    notice, this list of conditions and the following disclaimer.
-- 2. Redistributions in binary form must reproduce the above copyright
--    notice, this list of conditions and the following disclaimer in the
--    documentation and/or other materials provided with the distribution.
--
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
-- ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
-- TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
-- PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
-- CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
-- EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
-- PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
-- OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
-- WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
-- OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
-- ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

--
-- PostRR - PostgreSQL Round-Robin Extension
--

SET client_min_messages TO WARNING;

DROP FUNCTION IF EXISTS RRTimeslice_validate(integer);

DROP CAST IF EXISTS (rrtimeslice AS rrtimeslice);
DROP FUNCTION IF EXISTS RRTimeslice(rrtimeslice, integer, boolean);
DROP TYPE RRTimeslice CASCADE;
DROP FUNCTION IF EXISTS RRTimeslice_typmodin(cstring[]);
DROP FUNCTION IF EXISTS RRTimeslice_typmodout(integer);

DROP FUNCTION IF EXISTS CData_validate(integer);

DROP CAST IF EXISTS (cdata AS cdata);
DROP FUNCTION IF EXISTS CData(cdata, integer, boolean);
DROP TYPE CData CASCADE;
DROP FUNCTION IF EXISTS CData_typmodin(cstring[]);
DROP FUNCTION IF EXISTS CData_typmodout(integer);

DROP FUNCTION IF EXISTS PostRR_Version();

DROP TABLE postrr.rrtimeslices;
DROP SEQUENCE postrr.tsid;

DROP SCHEMA postrr;

SET client_min_messages TO DEFAULT;

-- vim: set tw=78 sw=4 ts=4 noexpandtab :

