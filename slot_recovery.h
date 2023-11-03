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
} SRConfig;
extern SRConfig config;

typedef struct {
    bool can_start_recovery;
} SRSharedData;
extern SRSharedData* data;

/* Hooks */
void file_not_found_cb(XLogReaderState *state, ReplicationSlot *slot, TimeLineID tli,
                       XLogSegNo logSegNo, int wal_segsz_bytes);
void walFileOpened(XLogReaderState *state);
void walFileClosed(XLogReaderState *state);
bool check_delete_xlog_file(XLogSegNo segNo);

#endif
