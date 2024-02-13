CREATE FUNCTION slot_recovery() returns text
AS 'MODULE_PATHNAME', 'slot_recovery'
LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION archived_wal_files() returns text[]
AS 'MODULE_PATHNAME', 'archived_wal_files'
LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION wal_files() returns text[]
AS 'MODULE_PATHNAME', 'wal_files'
    LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION is_file_restored(file_name text) returns bool
AS 'MODULE_PATHNAME', 'is_file_restored'
    LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION restart_lsn() returns pg_lsn
AS 'MODULE_PATHNAME', 'get_restart_lsn'
    LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION last_restored_file() returns text
AS 'MODULE_PATHNAME', 'get_last_restored_file'
    LANGUAGE C PARALLEL SAFE;