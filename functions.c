//
// Created by vlad on 03.11.23.
//

#include "postgres.h"
#include "slot_recovery.h"
#include "utils/builtins.h"

PG_FUNCTION_INFO_V1(slot_recovery);

Datum
slot_recovery(PG_FUNCTION_ARGS) {
    if (config.auto_recovery) {
        PG_RETURN_TEXT_P(cstring_to_text("can't start - auto recovery is enabled"));
    } else {
//        ReplicationSlot *slot = MyReplicationSlot;
//
//        if (slot->data.invalidated != RS_INVAL_WAL_REMOVED) {
//            PG_RETURN_TEXT_P("can't start - slot is not invalidated");
//        } else {
//            data->can_start_recovery = true;
//            PG_RETURN_TEXT_P(cstring_to_text("starting recovery"));
//        }

        data->can_start_recovery = true;
        PG_RETURN_TEXT_P(cstring_to_text("starting recovery"));
    }
}
