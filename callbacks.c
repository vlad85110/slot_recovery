//
// Created by vlad on 30.10.23.
//

#include "postgres.h"

#include <unistd.h>

#include "access/relation.h"
#include "storage/bufmgr.h"
#include "replication/slot.h"
#include "access/xlogarchive.h"
#include "access/xlog_internal.h"

#include "slot_recovery.h"

private int in_slot_recovery = false;
private XLogSegNo last_removed_segno;

private void start_recovery();
private void stop_recovery();

private void restore_single_file(XLogReaderState *state, ReplicationSlot *slot,
                                 TimeLineID tli, XLogSegNo logSegNo, int wal_segsz_bytes);
//private void restore_all_files(char *start_xlogfilename);

private void start_recovery() {
    in_slot_recovery = true;
    last_removed_segno = XLogGetLastRemovedSegno();
    elog(LOG, "starting recovery");
}

private void stop_recovery() {
    in_slot_recovery = false;
    elog(LOG, "stopping recovery");
    reset_callbacks();
}

private void restore_single_file(XLogReaderState *state, ReplicationSlot *slot,
                                 TimeLineID tli, XLogSegNo logSegNo, int wal_segsz_bytes) {
    char path[MAXPGPATH];
    char name[MAXPGPATH];

    XLogFileName(name, tli, logSegNo, wal_segsz_bytes);
    if (!RestoreArchivedFile(path, name, name,
                             wal_segment_size, false))
    {
        //todo detect no space left
        reset_callbacks();
        elog(ERROR, "cant restore wal");
    }
}

void
file_not_found_cb(XLogReaderState *state, ReplicationSlot *slot,
                  TimeLineID tli, XLogSegNo logSegNo, int wal_segsz_bytes) {
    if (!in_slot_recovery)
    {
        start_recovery();
    }

    switch (slot_recovery_mode) {
        case SINGLE:
            restore_single_file(state, slot, tli, logSegNo, wal_segsz_bytes);
            break;
        case FULL:
        default:
            break;
    }
}

void walFileOpened(XLogReaderState *state) {
    if (!in_slot_recovery)
        return;
}

void walFileClosed(XLogReaderState *state) {
    char		path[MAXPGPATH];

    if (!in_slot_recovery)
        return;

    XLogFilePath(path, state->seg.ws_tli, state->seg.ws_segno, wal_segment_size);
    unlink(path);

    if (state->seg.ws_segno == last_removed_segno) {
        stop_recovery();
    }
}

bool check_delete_xlog_file(XLogSegNo segNo) {
    if (!in_slot_recovery)
        return true;

    return segNo > last_removed_segno;
}

