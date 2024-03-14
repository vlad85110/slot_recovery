CREATE FUNCTION slot_recovery() returns text
AS 'MODULE_PATHNAME', 'slot_recovery'
LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION archived_wal_files() returns text[]
AS 'MODULE_PATHNAME', 'archived_wal_files'
LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION wal_files() returns text[]
AS 'MODULE_PATHNAME', 'wal_files'
    LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION last_restored_file() returns text
AS 'MODULE_PATHNAME', 'get_last_restored_file'
    LANGUAGE C PARALLEL SAFE;

CREATE OR REPLACE FUNCTION restart_lsn() RETURNS pg_lsn AS
$$
DECLARE
    lsn_value pg_lsn;
BEGIN
    SELECT restart_lsn INTO lsn_value FROM pg_replication_slots LIMIT 1;
    RETURN lsn_value;
END;
$$
    LANGUAGE plpgsql;
