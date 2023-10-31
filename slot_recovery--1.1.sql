/* contrib/slot_recovery/slot_recovery--1.1.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_prewarm" to load this file. \quit

-- Register the function.
CREATE FUNCTION slot_recovery(regclass,
						   mode text default 'buffer',
						   fork text default 'main',
						   first_block int8 default null,
						   last_block int8 default null)
RETURNS int8
AS 'MODULE_PATHNAME', 'slot_recovery'
LANGUAGE C PARALLEL SAFE;
