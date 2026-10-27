/*
 * US-010 (WIPC-7): Driver input filtering and debounce.
 *
 * Implements the state machine from design doc section 4.4. Treated as
 * safety-related per AGENTS.md Section 8 pending HARA (no ASIL assumed);
 * request independent review before production use.
 */
#include "driver_input_debounce.h"

#include <stddef.h>

DrvInDebounce_Status_t DrvInDebounce_Init(DrvInDebounce_t *inst,
                                          DrvIn_Value_t safe_default,
                                          uint32_t debounce_ms,
                                          uint32_t tick_period_ms)
{
    DrvInDebounce_Status_t status;
    uint32_t numerator;
    uint32_t ticks;

    /* Single point of exit (misra-c2012-15.5): status is set on every path. */
    if (inst == NULL) {
        status = DRVIN_DEB_ERR_NULL_PTR;
    } else if ((debounce_ms == 0U) || (tick_period_ms == 0U)) {
        inst->initialised = false;
        status = DRVIN_DEB_ERR_BAD_CONFIG;
    } else if (debounce_ms > (UINT32_MAX - (tick_period_ms - 1U))) {
        /* ceil(debounce_ms / tick_period_ms) without unsigned overflow (DR-010-07). */
        inst->initialised = false;
        status = DRVIN_DEB_ERR_BAD_CONFIG;
    } else {
        numerator = debounce_ms + (tick_period_ms - 1U);
        ticks = numerator / tick_period_ms;

        if ((ticks < 1U) || (ticks > (uint32_t)UINT16_MAX)) {
            inst->initialised = false;
            status = DRVIN_DEB_ERR_BAD_CONFIG;
        } else {
            inst->stable_value    = safe_default;
            inst->candidate_value = safe_default;
            inst->count_ticks     = 0U;
            inst->threshold_ticks = (uint16_t)ticks;
            inst->initialised     = true;
            status = DRVIN_DEB_OK;
        }
    }

    return status;
}

DrvInDebounce_Status_t DrvInDebounce_Update(DrvInDebounce_t *inst,
                                            DrvIn_Value_t raw_value,
                                            DrvIn_Value_t *stable_out,
                                            bool *changed_out)
{
    DrvInDebounce_Status_t status;
    bool changed;

    /* Single point of exit (misra-c2012-15.5): status is set on every path. */
    if ((inst == NULL) || (stable_out == NULL) || (changed_out == NULL)) {
        status = DRVIN_DEB_ERR_NULL_PTR;
    } else if (!inst->initialised) {
        status = DRVIN_DEB_ERR_NOT_INIT;
    } else if ((inst->threshold_ticks == 0U) || (inst->count_ticks > inst->threshold_ticks)) {
        status = DRVIN_DEB_ERR_STATE;
    } else {
        if (raw_value == inst->stable_value) {
            inst->candidate_value = inst->stable_value;
            inst->count_ticks     = 0U;
        } else if ((raw_value == inst->candidate_value) && (inst->count_ticks > 0U)) {
            if (inst->count_ticks < inst->threshold_ticks) {
                inst->count_ticks++;
            }
        } else {
            inst->candidate_value = raw_value;
            inst->count_ticks     = 1U;
        }

        if (inst->count_ticks >= inst->threshold_ticks) {
            inst->stable_value = inst->candidate_value;
            inst->count_ticks  = 0U;
            changed            = true;
        } else {
            changed = false;
        }

        *stable_out  = inst->stable_value;
        *changed_out = changed;

        status = DRVIN_DEB_OK;
    }

    return status;
}
