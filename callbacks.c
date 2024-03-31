//
// Created by vlad on 30.10.23.
//

#include "postgres.h"

#include <unistd.h>
#include <time.h>

#include "access/relation.h"
#include "storage/bufmgr.h"
#include "replication/slot.h"
#include "access/xlogarchive.h"
#include "access/xlog_internal.h"

#include "slot_recovery.h"

private bool in_slot_recovery;

//todo перененсти в разделяемую память
private XLogSegNo last_removed_segno;


private int files_restored;
private struct timespec start, end;

private void stop_recovery(void);

private void start_recovery(XLogReaderState *state, ReplicationSlot *slot,
                            TimeLineID tli, XLogSegNo logSegNo, int wal_segsz_bytes);

private void restore_single_file(XLogReaderState *state, ReplicationSlot *slot,
                                 TimeLineID tli, XLogSegNo logSegNo, int wal_segsz_bytes);

private void restore_all_files(XLogReaderState *state, ReplicationSlot *slot,
                               TimeLineID tli, XLogSegNo logSegNo, int wal_segsz_bytes);

void start_recovery(XLogReaderState *state, ReplicationSlot *slot,
                    TimeLineID tli, XLogSegNo logSegNo, int wal_segsz_bytes) {
    char start_file[MAXPGPATH];
    char end_file[MAXPGPATH];

    files_restored = 0;
    in_slot_recovery = true;

    SpinLockAcquire(&data->mutex);
    data->last_restored_segno = 0;
    SpinLockRelease(&data->mutex);

    last_removed_segno = XLogGetLastRemovedSegno();

    XLogFileName(start_file, tli, logSegNo, wal_segsz_bytes);
    XLogFileName(end_file, tli, last_removed_segno, wal_segsz_bytes);

    clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    elog(LOG, "starting recovery: oldest requested file - %s; last removed requested file - %s",
         start_file, end_file);
}

private void stop_recovery() {
    double time_taken;

    in_slot_recovery = false;

    LWLockAcquire(ReplicationSlotControlLock, LW_SHARED);
    MyReplicationSlot->data.invalidated = RS_INVAL_NONE;
    LWLockRelease(ReplicationSlotControlLock);

    SpinLockAcquire(&data->mutex);
    data->can_start_recovery = config.auto_recovery;
    SpinLockRelease(&data->mutex);

    clock_gettime(CLOCK_MONOTONIC_RAW, &end);

    time_taken = (double) (end.tv_sec - start.tv_sec)
                 + 0.000000001 * (double) (end.tv_nsec - start.tv_nsec);

    elog(LOG, "recovery complete: restore %d files, time taken - %.3lf s", files_restored, time_taken);
}

private void restore_single_file(XLogReaderState *state, ReplicationSlot *slot,
                                 TimeLineID tli, XLogSegNo logSegNo, int wal_segsz_bytes) {
    char path[MAXPGPATH];
    char name[MAXPGPATH];

    XLogFileName(name, tli, logSegNo, wal_segsz_bytes);
    if (!RestoreArchivedFile(path, name, name,
                             wal_segment_size, false)) {
        elog(ERROR, "cant restore wal");
    } else {
        ++files_restored;
        data->last_restored_segno = logSegNo;
    }
}

void restore_all_files(XLogReaderState *state, ReplicationSlot *slot, TimeLineID tli, XLogSegNo logSegNo,
                       int wal_segsz_bytes) {
    XLogSegNo seg_no;

    elog(LOG, "restoring all files");

    for (seg_no = logSegNo; seg_no <= last_removed_segno; ++seg_no) {
        restore_single_file(state, slot, tli, seg_no, wal_segsz_bytes);
    }
}

void
file_not_found_cb(XLogReaderState *state, ReplicationSlot *slot,
                  TimeLineID tli, XLogSegNo logSegNo, int wal_segsz_bytes) {
    SpinLockAcquire(&data->mutex);
    data->tli = tli;
    data->wal_seg_size = wal_segsz_bytes;
    SpinLockRelease(&data->mutex);

    if (!data->can_start_recovery) return;

    if (!in_slot_recovery) {
        start_recovery(state, slot, tli, logSegNo, wal_segsz_bytes);
    }

    switch (config.slot_recovery_mode) {
        case SINGLE:
            restore_single_file(state, slot, tli, logSegNo, wal_segsz_bytes);
            break;
        case FULL:
            restore_all_files(state, slot, tli, logSegNo, wal_segsz_bytes);
            break;
        default:
            break;
    }
}

void walFileClosed(XLogReaderState *state) {
    char path[MAXPGPATH];

    if (!in_slot_recovery)
        return;

    if (state->seg.ws_segno <= last_removed_segno)
    {
        XLogFilePath(path, state->seg.ws_tli, state->seg.ws_segno, wal_segment_size);
        unlink(path);
    }

    if (state->seg.ws_segno == last_removed_segno) {
        stop_recovery();
    }
}

bool check_delete_xlog_file(XLogSegNo segNo) {
    if (!in_slot_recovery)
        return true;

    return segNo > last_removed_segno;
}

