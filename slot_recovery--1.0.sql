CREATE FUNCTION slot_recovery() returns text
AS 'MODULE_PATHNAME', 'slot_recovery'
LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION archived_wal_files() returns text[]
AS 'MODULE_PATHNAME', 'archived_wal_files'
LANGUAGE C PARALLEL SAFE;

CREATE FUNCTION wal_files() returns text[]
AS 'MODULE_PATHNAME', 'wal_files'
    LANGUAGE C PARALLEL SAFE;