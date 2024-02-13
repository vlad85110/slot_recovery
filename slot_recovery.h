//
// Created by vlad on 30.10.23.
//

#ifndef SLOT_RECOVERY_H
#define SLOT_RECOVERY_H

#include "access/xlogreader.h"
#include "replication/slot.h"

#define private static

typedef enum RecoveryMode
{
    SINGLE,
    FULL, // as much as place is available
} RecoveryMode;

typedef struct {
     int slot_recovery_mode;
     bool auto_recovery;
     char *archive_dir;
} SRConfig;
extern SRConfig config;

typedef struct {
    slock_t mutex;
    bool can_start_recovery;

    XLogSegNo last_restored_segno;
    XLogRecPtr apply_ptr;
    XLogRecPtr restart_lsn;

    //todo нормально сделать
    TimeLineID tli;
    int wal_seg_size;
} SRSharedData;
extern SRSharedData* data;

/* Hooks */
void file_not_found_cb(XLogReaderState *state, ReplicationSlot *slot, TimeLineID tli,
                       XLogSegNo logSegNo, int wal_segsz_bytes);
void walFileOpened(XLogReaderState *state);
void walFileClosed(XLogReaderState *state);
bool check_delete_xlog_file(XLogSegNo segNo);
void get_stat(XLogRecPtr writePtr, XLogRecPtr flushPtr, XLogRecPtr applyPtr);
void set_restart_lsn(XLogRecPtr restart_lsn);


#endif
