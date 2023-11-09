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

PG_FUNCTION_INFO_V1(slot_recovery);
PG_FUNCTION_INFO_V1(archived_wal_files);
PG_FUNCTION_INFO_V1(wal_files);
PG_FUNCTION_INFO_V1(retcomposite);

private ArrayType *read_dir(const char* dirname);

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
            slot = &ReplicationSlotCtl->replication_slots[0];
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

#define ARCHIVE_DIR "/home/vlad/cluster/archivedir"

Datum
archived_wal_files(PG_FUNCTION_ARGS) {
    PG_RETURN_ARRAYTYPE_P(read_dir(ARCHIVE_DIR));
}

Datum
wal_files(PG_FUNCTION_ARGS) {
    PG_RETURN_ARRAYTYPE_P(read_dir(XLOGDIR));
}

ArrayType *read_dir(const char *dirname) {
    ArrayType *result;
    Datum *elements;
    int num_elements;
    DIR *xldir;
    struct dirent *xlde;
    List * list = NIL;
    int i;

    xldir = AllocateDir(dirname);
    while ((xlde = ReadDir(xldir, dirname)) != NULL) {
        if (!IsXLogFileName(xlde->d_name) &&
            !IsPartialXLogFileName(xlde->d_name))
            continue;

        list = lappend(list, (void *)CStringGetTextDatum(xlde->d_name));
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
