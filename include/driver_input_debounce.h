/*
 * US-010 (WIPC-7): Driver input filtering and debounce.
 *
 * Design doc: docs/design/US-010-driver-input-filtering-and-debounce.md,
 * section 4.2 (interfaces copied verbatim). Pure C logic, no HAL, no
 * dynamic allocation, no recursion, O(1) per call (DR-010-01, DR-010-09).
 *
 * Traces: PR-007, DR-010-01 to DR-010-10.
 */
#ifndef DRIVER_INPUT_DEBOUNCE_H
#define DRIVER_INPUT_DEBOUNCE_H

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t DrvIn_Value_t; /* opaque encoded position; encoding TBD (PRD OQ-6) */

typedef enum {
    DRVIN_DEB_OK = 0,
    DRVIN_DEB_ERR_NULL_PTR,
    DRVIN_DEB_ERR_BAD_CONFIG,
    DRVIN_DEB_ERR_NOT_INIT,
    DRVIN_DEB_ERR_STATE
} DrvInDebounce_Status_t;

typedef struct {
    DrvIn_Value_t stable_value;    /* current debounced output */
    DrvIn_Value_t candidate_value; /* value being counted */
    uint16_t      count_ticks;     /* consecutive ticks of candidate, 0..threshold_ticks */
    uint16_t      threshold_ticks; /* N, 1..UINT16_MAX */
    bool          initialised;
} DrvInDebounce_t;

/*
 * Initialises inst with threshold_ticks = ceil(debounce_ms / tick_period_ms)
 * and stable/candidate values set to safe_default (DR-010-02, DR-010-06).
 * Returns DRVIN_DEB_ERR_BAD_CONFIG, leaving inst uninitialised, if
 * debounce_ms or tick_period_ms is zero, or if the computed ticks would
 * be zero, exceed UINT16_MAX, or require an overflowing computation
 * (DR-010-07). Returns DRVIN_DEB_ERR_NULL_PTR if inst is NULL.
 */
DrvInDebounce_Status_t DrvInDebounce_Init(DrvInDebounce_t *inst,
                                          DrvIn_Value_t safe_default,
                                          uint32_t debounce_ms,
                                          uint32_t tick_period_ms);

/*
 * Samples raw_value for one tick and reports the debounced output
 * (DR-010-03 to DR-010-05). Returns an error status and writes no
 * outputs for NULL pointers (DRVIN_DEB_ERR_NULL_PTR), an uninitialised
 * instance (DRVIN_DEB_ERR_NOT_INIT), or a corrupted instance
 * (DRVIN_DEB_ERR_STATE, DR-010-08).
 */
DrvInDebounce_Status_t DrvInDebounce_Update(DrvInDebounce_t *inst,
                                            DrvIn_Value_t raw_value,
                                            DrvIn_Value_t *stable_out,
                                            bool *changed_out);

#endif /* DRIVER_INPUT_DEBOUNCE_H */
