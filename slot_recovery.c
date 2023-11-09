/*-------------------------------------------------------------------------
 *
 * slot_recovery.c
 *		  replication recovery after slot's overflow
 *
 * IDENTIFICATION
 *		  contrib/slot_recovery/slot_recovery.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <unistd.h>

#include "access/relation.h"
#include "fmgr.h"
#include "storage/bufmgr.h"
#include "replication/slot.h"
#include "access/xlog_internal.h"
#include "access/xlogutils.h"
#include "utils/guc.h"
#include "miscadmin.h"
#include "storage/ipc.h"

#include "slot_recovery.h"


PG_MODULE_MAGIC;

static const struct config_enum_entry variable_conflict_options[] = {
        {"single", SINGLE, false},
        {"full", FULL, false},
};

SRConfig config;
SRSharedData *data = NULL;

static shmem_request_hook_type prev_shmem_request_hook = NULL;
static shmem_startup_hook_type prev_shmem_startup_hook = NULL;

private void init_callbacks(void);

private void pgss_shmem_request(void);
private void pgss_shmem_startup(void);
private void init_shared_data(void);

// on_shmem_exit

void pgss_shmem_startup(void) {
    bool found;

    if (prev_shmem_startup_hook)
        prev_shmem_startup_hook();

    data = (SRSharedData *)ShmemInitStruct("shared data", sizeof(SRSharedData), &found);
    //todo where free memory

//    if (!found) {
//        //todo cant access shared memory - only auto mode can be used for recovery
//        return;
//    }

    init_shared_data();
}

void pgss_shmem_request(void) {
    if (prev_shmem_request_hook)
        prev_shmem_request_hook();

    RequestAddinShmemSpace(sizeof(SRSharedData));
}

void init_shared_data(void) {
    //todo free
    SpinLockInit(&data->mutex);
    data->can_start_recovery = config.auto_recovery;
    elog(LOG, "init - %d", data->can_start_recovery);
}

void
_PG_init(void)
{
    init_callbacks();
    ArchiveRecoveryRequested = true;

    config.slot_recovery_mode = SINGLE;
    config.auto_recovery = true;

    DefineCustomEnumVariable("slot_recovery.mode",
                             gettext_noop(""),
                             NULL,
                             &config.slot_recovery_mode,
                             SINGLE,
                             variable_conflict_options,
                             PGC_BACKEND, 0,
                             NULL, NULL, NULL);

    DefineCustomBoolVariable("slot_recovery.auto_recovery",
                             gettext_noop(""),
                             NULL,
                             &config.auto_recovery,
                             true,
                             PGC_BACKEND, 0,
                             NULL, NULL, NULL);

    prev_shmem_request_hook = shmem_request_hook;
    shmem_request_hook = pgss_shmem_request;
    prev_shmem_startup_hook = shmem_startup_hook;
    shmem_startup_hook = pgss_shmem_startup;
}

void init_callbacks(void)
{
    recoveryCb = file_not_found_cb;
    openCb = walFileOpened;
    closeCb = walFileClosed;
    check_delete_xlog_file_cb = check_delete_xlog_file;
}