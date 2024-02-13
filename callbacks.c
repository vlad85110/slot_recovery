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

private bool in_slot_recovery;
private XLogSegNo last_removed_segno;
private int files_restored;

private void stop_recovery(void);
private void start_recovery(XLogReaderState *state, ReplicationSlot *slot,
                            TimeLineID tli, XLogSegNo logSegNo, int wal_segsz_bytes);
private void restore_single_file(XLogReaderState *state, ReplicationSlot *slot,
                                 TimeLineID tli, XLogSegNo logSegNo, int wal_segsz_bytes);
private void restore_all_files(XLogReaderState *state, ReplicationSlot *slot,
                               TimeLineID tli, XLogSegNo logSegNo, int wal_segsz_bytes);

//todo если в процессе чекпоинтер снес более поздние, посмотреть возможно ли такое

void start_recovery(XLogReaderState *state, ReplicationSlot *slot,
                    TimeLineID tli, XLogSegNo logSegNo, int wal_segsz_bytes) {
    char start_file[MAXPGPATH];
    char end_file[MAXPGPATH];

    files_restored = 0;
    in_slot_recovery = true;

    SpinLockAcquire(&data->mutex);
    data->last_restored_segno = 0;
    SpinLockRelease(&data->mutex);

    //todo работает только после контрольной точки
    //todo select last_archived_wal from pg_stat_archiver;

    //todo обновлять после каждого восстановленного файла
    last_removed_segno = XLogGetLastRemovedSegno();

    XLogFileName(start_file, tli, logSegNo, wal_segsz_bytes);
    XLogFileName(end_file, tli, last_removed_segno, wal_segsz_bytes);

    elog(LOG, "starting recovery: oldest requested file - %s; last removed requested file - %s",
         start_file, end_file);
}

private void stop_recovery() {
    in_slot_recovery = false;

    SpinLockAcquire(&data->mutex);
    data->can_start_recovery = config.auto_recovery;
    SpinLockRelease(&data->mutex);

    elog(LOG, "recovery complete: restore %d files", files_restored);
}

private void restore_single_file(XLogReaderState *state, ReplicationSlot *slot,
                                 TimeLineID tli, XLogSegNo logSegNo, int wal_segsz_bytes) {
    char path[MAXPGPATH];
    char name[MAXPGPATH];

    //todo mb tli callback

    XLogFileName(name, tli, logSegNo, wal_segsz_bytes);
    if (!RestoreArchivedFile(path, name, name,
                             wal_segment_size, false))
    {
        elog(ERROR, "cant restore wal");
    } else {
        ++files_restored;
        data->last_restored_segno = logSegNo;
    }

    sleep(1);
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
//    elog(LOG, "can start recovery - %d", data->can_start_recovery);
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
// todo при остановке реплики становится lost
bool check_delete_xlog_file(XLogSegNo segNo) {
    if (!in_slot_recovery)
        return true;

    return segNo > last_removed_segno;
}

void get_stat(XLogRecPtr writePtr, XLogRecPtr flushPtr, XLogRecPtr applyPtr) {
    data->apply_ptr = applyPtr;
}

void set_restart_lsn(XLogRecPtr restart_lsn) {
    data->restart_lsn = restart_lsn;
}

// todo посмотреть другие варианты решения
// todo мониторинг - можно добавить функцию статистики - визуализация lsn слота визуализацию

