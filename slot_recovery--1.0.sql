CREATE FUNCTION slot_recovery() returns text
AS 'MODULE_PATHNAME', 'slot_recovery'
LANGUAGE C PARALLEL SAFE;