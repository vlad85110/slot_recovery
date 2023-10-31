/*-------------------------------------------------------------------------
 *
 * slot_recovery.c
 *		  prewarming utilities
 *
 * Copyright (c) 2010-2023, PostgreSQL Global Development Group
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

#include "slot_recovery.h"


PG_MODULE_MAGIC;
PG_FUNCTION_INFO_V1(slot_recovery);

static const struct config_enum_entry variable_conflict_options[] = {
        {"single", SINGLE, false},
        {"full_available", FULL, false},
};

int	slot_recovery_mode = SINGLE;

void
_PG_init(void)
{
    init_callbacks();
    ArchiveRecoveryRequested = true;

    DefineCustomEnumVariable("slot_recovery.mode",
                             gettext_noop(""),
                             NULL,
                             &slot_recovery_mode,
                             SINGLE,
                             variable_conflict_options,
                             PGC_SUSET, 0,
                             NULL, NULL, NULL);
}

void _PG_fini()
{
    reset_callbacks();
    ArchiveRecoveryRequested = false;
}

Datum
slot_recovery(PG_FUNCTION_ARGS)
{
    PG_RETURN_VOID();
}

void init_callbacks(void)
{
    InstallCallback(recoveryCb, file_not_found_cb)
    InstallCallback(openCb, walFileOpened)
    InstallCallback(closeCb, walFileClosed)
    InstallCallback(check_delete_xlog_file_cb, check_delete_xlog_file)
}

void reset_callbacks(void)
{
    ResetCallback(recoveryCb, file_not_found_cb)
    ResetCallback(openCb, walFileOpened)
    ResetCallback(closeCb, walFileClosed)
    ResetCallback(check_delete_xlog_file_cb, check_delete_xlog_file)
 // problem
}