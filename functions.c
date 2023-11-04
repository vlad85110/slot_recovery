//
// Created by vlad on 03.11.23.
//

#include "postgres.h"
#include "slot_recovery.h"
#include "utils/builtins.h"
//#include "ut"

PG_FUNCTION_INFO_V1(slot_recovery);

Datum
slot_recovery(PG_FUNCTION_ARGS) {
    if (config.auto_recovery) {
        PG_RETURN_TEXT_P(cstring_to_text("auto recovery is enabled"));
    } else {
        int	slot_num;
        ReplicationSlot *slot;
        bool invalidated = false;

        LWLockAcquire(ReplicationSlotControlLock, LW_SHARED);
        for (slot_num = 0; slot_num < max_replication_slots; slot_num++)
        {
            slot = &ReplicationSlotCtl->replication_slots[0];
            SpinLockAcquire(&slot->mutex);

            if (slot->data.invalidated == RS_INVAL_WAL_REMOVED)
                invalidated = true;

            SpinLockRelease(&slot->mutex);
        }

        if (!invalidated) {
            PG_RETURN_TEXT_P(cstring_to_text("there are no invalidated slots"));
        } else {
            SpinLockAcquire(&data->mutex);
            data->can_start_recovery = true;
            SpinLockRelease(&data->mutex);

            PG_RETURN_TEXT_P(cstring_to_text("starting recovery"));
        }

//todo несколько слотов и  принимать в параметрах имя хранить в хешсете каком нибудь
// и проверять при колбеке что для этого слота можно восстанавливать
    }
}
