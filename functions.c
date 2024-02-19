//
// Created by vlad on 03.11.23.
//

#include <dirent.h>
#include "postgres.h"
#include "slot_recovery.h"
#include "utils/builtins.h"
#include "access/xlog_internal.h"
#include "storage/fd.h"
#include "funcapi.h"
#include "utils/array.h"
#include "utils/pg_lsn.h"

PG_FUNCTION_INFO_V1(slot_recovery);

PG_FUNCTION_INFO_V1(archived_wal_files);

PG_FUNCTION_INFO_V1(wal_files);

PG_FUNCTION_INFO_V1(retcomposite);

PG_FUNCTION_INFO_V1(is_file_restored);

PG_FUNCTION_INFO_V1(get_restart_lsn);

PG_FUNCTION_INFO_V1(get_last_restored_file);

private ArrayType *read_dir(const char *dirname);

Datum
slot_recovery(PG_FUNCTION_ARGS) {
    if (config.auto_recovery) {
        PG_RETURN_TEXT_P(cstring_to_text("auto recovery is enabled"));
    } else {
        int slot_num;
        ReplicationSlot *slot;
        bool invalidated = false;

        LWLockAcquire(ReplicationSlotControlLock, LW_SHARED);
        for (slot_num = 0; slot_num < max_replication_slots; slot_num++) {
            slot = &ReplicationSlotCtl->replication_slots[slot_num];
            SpinLockAcquire(&slot->mutex);

            if (slot->data.invalidated == RS_INVAL_WAL_REMOVED)
                invalidated = true;

            SpinLockRelease(&slot->mutex);
        }
        LWLockRelease(ReplicationSlotControlLock);

        if (!invalidated) {
            PG_RETURN_TEXT_P(cstring_to_text("there are no invalidated slots"));
        } else {
            SpinLockAcquire(&data->mutex);
            data->can_start_recovery = true;
            SpinLockRelease(&data->mutex);

            PG_RETURN_TEXT_P(cstring_to_text("starting recovery"));
        }
    }
}

Datum
archived_wal_files(PG_FUNCTION_ARGS) {
    PG_RETURN_ARRAYTYPE_P(read_dir(config.archive_dir));
}

Datum
wal_files(PG_FUNCTION_ARGS) {
    PG_RETURN_ARRAYTYPE_P(read_dir(XLOGDIR));
}

ArrayType *
read_dir(const char *dirname) {
    ArrayType *result;
    Datum *elements;
    int num_elements;
    DIR *xldir;
    struct dirent *xlde;
    List *list = NIL;
    int i;

    xldir = AllocateDir(dirname);
    while ((xlde = ReadDir(xldir, dirname)) != NULL) {
        if (!IsXLogFileName(xlde->d_name) &&
            !IsPartialXLogFileName(xlde->d_name))
            continue;

        list = lappend(list, (void *) CStringGetTextDatum(xlde->d_name));
    }
    FreeDir(xldir);

    num_elements = list_length(list);
    elements = (Datum *) palloc(list_length(list) * sizeof(Datum));
    for (i = 0; i < num_elements; ++i) {
        Datum elem = (Datum) list_nth(list, i);
        elements[i] = elem;
    }
    result = construct_array(elements, num_elements, TEXTOID, -1, false, 'i');
    pfree(elements);

    return result;
}


Datum get_restart_lsn(PG_FUNCTION_ARGS) {
    ReplicationSlot *slot;
    XLogRecPtr restart_lsn;

    LWLockAcquire(ReplicationSlotControlLock, LW_SHARED);
    slot = &ReplicationSlotCtl->replication_slots[0];
    SpinLockAcquire(&slot->mutex);
    restart_lsn = slot->data.restart_lsn;
    SpinLockRelease(&slot->mutex);
    LWLockRelease(ReplicationSlotControlLock);

    if (restart_lsn == InvalidXLogRecPtr) {
        restart_lsn = data->restart_lsn;
    }

    PG_RETURN_DATUM(LSNGetDatum(restart_lsn));
}

Datum get_last_restored_file(PG_FUNCTION_ARGS) {
    XLogSegNo last_restored_segno;

    SpinLockAcquire(&data->mutex);
    last_restored_segno = data->last_restored_segno;
    SpinLockRelease(&data->mutex);

    if (last_restored_segno == 0) {
        PG_RETURN_TEXT_P(cstring_to_text("null"));
    } else {
        char name[MAXPGPATH];
        XLogFileName(name, data->tli, last_restored_segno, data->wal_seg_size);
        PG_RETURN_TEXT_P(cstring_to_text(name));
    }
}



