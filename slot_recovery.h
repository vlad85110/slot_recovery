//
// Created by vlad on 30.10.23.
//

#ifndef SLOT_RECOVERY_H
#define SLOT_RECOVERY_H

#include "access/xlogreader.h"
#include "replication/slot.h"

#define InstallCallback(cb, fun) if(!cb) cb = fun;
#define ResetCallback(cb, fun) if(cb == fun) cb = NULL;
#define private static

typedef enum RecoveryMode
{
    SINGLE,
    FULL, // as much as place is available
} RecoveryMode;


extern int slot_recovery_mode;
//extern int in_slot_recovery;
//extern XLogSegNo last_removed_segno;

void file_not_found_cb(XLogReaderState *state, ReplicationSlot *slot, TimeLineID tli,
                       XLogSegNo logSegNo, int wal_segsz_bytes);
void walFileOpened(XLogReaderState *state);
void walFileClosed(XLogReaderState *state);
bool check_delete_xlog_file(XLogSegNo segNo);

void init_callbacks(void);
void reset_callbacks(void);

#endif
