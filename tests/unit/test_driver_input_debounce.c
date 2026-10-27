/*
 * Unit tests for US-010 (WIPC-7): driver input filtering and debounce.
 *
 * Source spec: docs/verification/unit-test-specs/
 *   US-010-driver-input-filtering-and-debounce-unit-test-spec.csv
 * Design doc: docs/design/US-010-driver-input-filtering-and-debounce.md
 *
 * Production code (include/driver_input_debounce.h,
 * src/drivers/driver_input_debounce.c) does not exist yet. Per the
 * design doc's Interfaces section (4.2), the types, status codes and
 * function prototypes below are copied verbatim as `extern` declarations
 * so this file compiles; DrvInDebounce_Init/_Update are declared, never
 * defined, here. Linking this test is expected to fail (undefined
 * reference) until src/ provides real definitions (TDD Red).
 *
 * All Mocked_Dependencies entries in the CSV are "None (pure logic, no
 * external dependency)", so no mocks are required or created for this
 * unit.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "test_support.h"

/* ---- Stand-in for the not-yet-existing include/driver_input_debounce.h,
 * copied verbatim from design doc section 4.2. ---- */

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

extern DrvInDebounce_Status_t DrvInDebounce_Init(DrvInDebounce_t *inst,
                                                  DrvIn_Value_t safe_default,
                                                  uint32_t debounce_ms,
                                                  uint32_t tick_period_ms);

extern DrvInDebounce_Status_t DrvInDebounce_Update(DrvInDebounce_t *inst,
                                                    DrvIn_Value_t raw_value,
                                                    DrvIn_Value_t *stable_out,
                                                    bool *changed_out);

/* ---- Test-only sentinels, distinguishable from any real debounced value. ---- */

static const DrvIn_Value_t SENTINEL_VALUE   = 0xAAU;
static const bool          SENTINEL_CHANGED = true;

/*
 * Builds an instance matching an Update-test precondition such as
 * "Instance initialised with threshold_ticks=N; stable_value=V" directly,
 * independent of any particular (debounce_ms, tick_period_ms) pair, since
 * the CSV's Update preconditions specify threshold_ticks directly and not
 * Init's millisecond inputs.
 */
static void init_instance_direct(DrvInDebounce_t *inst, DrvIn_Value_t stable_value, uint16_t threshold_ticks)
{
    (void)memset(inst, 0, sizeof(*inst));
    inst->stable_value    = stable_value;
    inst->candidate_value = stable_value;
    inst->count_ticks      = 0U;
    inst->threshold_ticks  = threshold_ticks;
    inst->initialised      = true;
}

/* ===================== DrvInDebounce_Init: Positive ===================== */

static void test_UT_US010_001(void)
{
    DrvInDebounce_t inst;
    (void)memset(&inst, 0, sizeof(inst));

    DrvInDebounce_Status_t status = DrvInDebounce_Init(&inst, 0x00U, 10UL, 2UL);

    TEST_CHECK(status == DRVIN_DEB_OK, "status == DRVIN_DEB_OK");
    TEST_CHECK(inst.threshold_ticks == 5U, "threshold_ticks == 5 for an exact-multiple debounce time");
    TEST_CHECK(inst.initialised == true, "initialised == true");
    TEST_CHECK(inst.stable_value == 0x00U, "stable_value == safe_default");
}

static void test_UT_US010_002(void)
{
    DrvInDebounce_t inst;
    (void)memset(&inst, 0, sizeof(inst));

    DrvInDebounce_Status_t status = DrvInDebounce_Init(&inst, 0x00U, 11UL, 2UL);

    TEST_CHECK(status == DRVIN_DEB_OK, "status == DRVIN_DEB_OK");
    TEST_CHECK(inst.threshold_ticks == 6U, "threshold_ticks == 6 (ceil(11/2)) for a non-multiple debounce time");
    TEST_CHECK(inst.initialised == true, "initialised == true");
    TEST_CHECK(inst.stable_value == 0x00U, "stable_value == safe_default");
}

static void test_UT_US010_003(void)
{
    DrvInDebounce_t inst;
    (void)memset(&inst, 0, sizeof(inst));

    DrvInDebounce_Status_t status = DrvInDebounce_Init(&inst, 0x00U, 2UL, 2UL);

    TEST_CHECK(status == DRVIN_DEB_OK, "status == DRVIN_DEB_OK");
    TEST_CHECK(inst.threshold_ticks == 1U, "threshold_ticks == 1 when debounce_ms equals tick_period_ms");
    TEST_CHECK(inst.initialised == true, "initialised == true");
}

/* ===================== DrvInDebounce_Init: Boundary ===================== */

static void test_UT_US010_004(void)
{
    DrvInDebounce_t inst;
    (void)memset(&inst, 0, sizeof(inst));

    DrvInDebounce_Status_t status = DrvInDebounce_Init(&inst, 0x00U, 1UL, 2UL);

    TEST_CHECK(status == DRVIN_DEB_OK, "status == DRVIN_DEB_OK");
    TEST_CHECK(inst.threshold_ticks == 1U, "threshold_ticks == 1 for the minimum sub-tick debounce time");
}

static void test_UT_US010_005(void)
{
    DrvInDebounce_t inst;
    (void)memset(&inst, 0, sizeof(inst));

    DrvInDebounce_Status_t status = DrvInDebounce_Init(&inst, 0x00U, 131070UL, 2UL);

    TEST_CHECK(status == DRVIN_DEB_OK, "status == DRVIN_DEB_OK");
    TEST_CHECK(inst.threshold_ticks == 65535U, "threshold_ticks == 65535 (the maximum representable value)");
}

static void test_UT_US010_006(void)
{
    DrvInDebounce_t inst;
    (void)memset(&inst, 0, sizeof(inst));

    DrvInDebounce_Status_t status = DrvInDebounce_Init(&inst, 0x00U, 131071UL, 2UL);

    TEST_CHECK(status == DRVIN_DEB_ERR_BAD_CONFIG, "status == DRVIN_DEB_ERR_BAD_CONFIG one tick above the maximum threshold");
    TEST_CHECK(inst.initialised == false, "initialised == false on rejected configuration");
}

/* ===================== DrvInDebounce_Init: Negative ===================== */

static void test_UT_US010_007(void)
{
    DrvInDebounce_t inst;
    (void)memset(&inst, 0, sizeof(inst));

    DrvInDebounce_Status_t status = DrvInDebounce_Init(&inst, 0x00U, 0UL, 2UL);

    TEST_CHECK(status == DRVIN_DEB_ERR_BAD_CONFIG, "status == DRVIN_DEB_ERR_BAD_CONFIG for a zero debounce time");
    TEST_CHECK(inst.initialised == false, "initialised == false on rejected configuration");
}

static void test_UT_US010_008(void)
{
    DrvInDebounce_t inst;
    (void)memset(&inst, 0, sizeof(inst));

    DrvInDebounce_Status_t status = DrvInDebounce_Init(&inst, 0x00U, 10UL, 0UL);

    TEST_CHECK(status == DRVIN_DEB_ERR_BAD_CONFIG, "status == DRVIN_DEB_ERR_BAD_CONFIG for a zero tick period");
    TEST_CHECK(inst.initialised == false, "initialised == false on rejected configuration");
}

static void test_UT_US010_009(void)
{
    DrvInDebounce_t inst;
    (void)memset(&inst, 0, sizeof(inst));

    DrvInDebounce_Status_t status = DrvInDebounce_Init(&inst, 0x00U, UINT32_MAX, 1UL);

    TEST_CHECK(status == DRVIN_DEB_ERR_BAD_CONFIG, "status == DRVIN_DEB_ERR_BAD_CONFIG when the ms-to-tick conversion would overflow");
    TEST_CHECK(inst.initialised == false, "initialised == false on rejected configuration");
}

/* ===================== DrvInDebounce_Update: Positive ===================== */

static void test_UT_US010_010(void)
{
    DrvInDebounce_t inst;
    init_instance_direct(&inst, 0x00U, 5U);

    DrvIn_Value_t stable_out = SENTINEL_VALUE;
    bool          changed_out = false;

    for (uint16_t call = 1U; call <= 5U; call++) {
        DrvInDebounce_Status_t status = DrvInDebounce_Update(&inst, 0x01U, &stable_out, &changed_out);
        TEST_CHECK(status == DRVIN_DEB_OK, "status == DRVIN_DEB_OK");
        if (call < 5U) {
            TEST_CHECK(changed_out == false, "changed_out == false before the Nth call");
            TEST_CHECK(stable_out == 0x00U, "stable_out == 0x00 before the Nth call");
        } else {
            TEST_CHECK(changed_out == true, "changed_out == true exactly on the Nth call");
            TEST_CHECK(stable_out == 0x01U, "stable_out == 0x01 on the Nth call");
        }
    }
}

static void test_UT_US010_011(void)
{
    DrvInDebounce_t inst;
    init_instance_direct(&inst, 0x00U, 5U);

    DrvIn_Value_t stable_out = SENTINEL_VALUE;
    bool          changed_out = false;
    unsigned int  changed_count = 0U;

    for (uint16_t call = 1U; call <= 3U; call++) {
        (void)DrvInDebounce_Update(&inst, 0x01U, &stable_out, &changed_out);
        TEST_CHECK(stable_out != 0x01U, "stable_out never equals the short-lived candidate 0x01");
        if (changed_out) {
            changed_count++;
        }
    }
    for (uint16_t call = 1U; call <= 5U; call++) {
        (void)DrvInDebounce_Update(&inst, 0x02U, &stable_out, &changed_out);
        TEST_CHECK(stable_out != 0x01U, "stable_out never equals the short-lived candidate 0x01");
        if (changed_out) {
            changed_count++;
        }
        if (call == 5U) {
            TEST_CHECK(changed_out == true, "changed_out == true on the 5th call of the held-C run");
            TEST_CHECK(stable_out == 0x02U, "stable_out moves directly from 0x00 to 0x02");
        } else {
            TEST_CHECK(changed_out == false, "changed_out == false before the 5th call of the held-C run");
        }
    }
    TEST_CHECK(changed_count == 1U, "changed_out is true on exactly one call across the whole sequence");
}

static void test_UT_US010_012(void)
{
    DrvInDebounce_t inst;
    init_instance_direct(&inst, 0x00U, 5U);

    DrvIn_Value_t stable_out = SENTINEL_VALUE;
    bool          changed_out = false;

    for (uint16_t call = 1U; call <= 2U; call++) {
        (void)DrvInDebounce_Update(&inst, 0x00U, &stable_out, &changed_out);
        TEST_CHECK(stable_out == 0x00U, "stable_out == safe default while raw equals the safe default");
        TEST_CHECK(changed_out == false, "changed_out == false while raw equals the safe default");
    }
    for (uint16_t call = 1U; call <= 5U; call++) {
        (void)DrvInDebounce_Update(&inst, 0x01U, &stable_out, &changed_out);
        if (call < 5U) {
            TEST_CHECK(stable_out == 0x00U, "stable_out holds the safe default before the Nth call");
            TEST_CHECK(changed_out == false, "changed_out == false before the Nth call");
        } else {
            TEST_CHECK(changed_out == true, "changed_out == true on the Nth call");
            TEST_CHECK(stable_out == 0x01U, "stable_out becomes the confirmed value on the Nth call");
        }
    }
}

static void test_UT_US010_013(void)
{
    DrvInDebounce_t inst;
    init_instance_direct(&inst, 0x00U, 5U);

    DrvIn_Value_t stable_out = SENTINEL_VALUE;
    bool          changed_out = false;

    for (uint16_t call = 1U; call <= 10U; call++) {
        (void)DrvInDebounce_Update(&inst, 0x01U, &stable_out, &changed_out);
        if (call == 5U) {
            TEST_CHECK(changed_out == true, "changed_out == true only on call 5");
        } else {
            TEST_CHECK(changed_out == false, "changed_out == false on calls other than 5");
        }
        if (call >= 5U) {
            TEST_CHECK(stable_out == 0x01U, "stable_out == 0x01 from call 5 onward");
        }
    }
}

static void test_UT_US010_014(void)
{
    DrvInDebounce_t inst;
    init_instance_direct(&inst, 0x00U, 5U);

    DrvIn_Value_t       stable_out = SENTINEL_VALUE;
    bool                changed_out = false;
    const DrvIn_Value_t codes[3] = { 0x01U, 0x80U, 0xFFU };

    for (unsigned int code_idx = 0U; code_idx < 3U; code_idx++) {
        for (uint16_t call = 1U; call <= 5U; call++) {
            DrvInDebounce_Status_t status = DrvInDebounce_Update(&inst, codes[code_idx], &stable_out, &changed_out);
            TEST_CHECK(status == DRVIN_DEB_OK, "status == DRVIN_DEB_OK for an arbitrary opaque byte code");
            if (call == 5U) {
                TEST_CHECK(changed_out == true, "changed_out == true on the code's own 5th call");
                TEST_CHECK(stable_out == codes[code_idx], "stable_out equals the held code, unmodified");
            } else {
                TEST_CHECK(changed_out == false, "changed_out == false before the code's own 5th call");
            }
        }
    }
}

static void test_UT_US010_015(void)
{
    DrvInDebounce_t inst;
    init_instance_direct(&inst, 0x00U, 5U);

    DrvIn_Value_t stable_out = SENTINEL_VALUE;
    bool          changed_out = false;
    uint16_t      first_changed_call = 0U;

    for (uint16_t call = 1U; call <= 5U; call++) {
        (void)DrvInDebounce_Update(&inst, 0x01U, &stable_out, &changed_out);
        if ((changed_out == true) && (first_changed_call == 0U)) {
            first_changed_call = call;
        }
    }
    TEST_CHECK(first_changed_call == 5U, "changed_out first becomes true exactly threshold_ticks calls after the differing sample began");
}

static void test_UT_US010_016(void)
{
    DrvInDebounce_t inst1;
    DrvInDebounce_t inst2;
    init_instance_direct(&inst1, 0x00U, 5U);
    init_instance_direct(&inst2, 0x01U, 5U);

    DrvIn_Value_t stable_out1 = SENTINEL_VALUE;
    DrvIn_Value_t stable_out2 = SENTINEL_VALUE;
    bool          changed_out1 = false;
    bool          changed_out2 = false;

    for (uint16_t call = 1U; call <= 5U; call++) {
        (void)DrvInDebounce_Update(&inst1, 0x01U, &stable_out1, &changed_out1);
        (void)DrvInDebounce_Update(&inst2, 0x00U, &stable_out2, &changed_out2);

        if (call < 5U) {
            TEST_CHECK(stable_out1 == 0x00U, "instance 1 is unaffected before its own 5th call");
            TEST_CHECK(stable_out2 == 0x01U, "instance 2 is unaffected before its own 5th call");
        } else {
            TEST_CHECK(stable_out1 == 0x01U, "instance 1 stable_out becomes 0x01 on its 5th call");
            TEST_CHECK(stable_out2 == 0x00U, "instance 2 stable_out becomes 0x00 on its 5th call");
        }
    }
}

/* ===================== DrvInDebounce_Update: Boundary ===================== */

static void test_UT_US010_017(void)
{
    DrvInDebounce_t inst;
    init_instance_direct(&inst, 0x00U, 5U);

    DrvIn_Value_t stable_out = SENTINEL_VALUE;
    bool          changed_out = false;

    (void)DrvInDebounce_Update(&inst, 0x01U, &stable_out, &changed_out);
    TEST_CHECK(changed_out == false, "changed_out == false on a one-tick bounce");
    TEST_CHECK(stable_out == 0x00U, "stable_out == 0x00 on a one-tick bounce");

    (void)DrvInDebounce_Update(&inst, 0x00U, &stable_out, &changed_out);
    TEST_CHECK(changed_out == false, "changed_out == false after returning to the stable value");
    TEST_CHECK(stable_out == 0x00U, "stable_out == 0x00 after returning to the stable value");
    TEST_CHECK(inst.count_ticks == 0U, "count_ticks == 0 after the second call");
}

static void test_UT_US010_018(void)
{
    DrvInDebounce_t inst;
    init_instance_direct(&inst, 0x00U, 5U);

    DrvIn_Value_t stable_out = SENTINEL_VALUE;
    bool          changed_out = false;

    for (uint16_t call = 1U; call <= 4U; call++) {
        (void)DrvInDebounce_Update(&inst, 0x01U, &stable_out, &changed_out);
        TEST_CHECK(changed_out == false, "changed_out == false one tick short of the threshold");
        TEST_CHECK(stable_out == 0x00U, "stable_out == 0x00 one tick short of the threshold");
    }
    (void)DrvInDebounce_Update(&inst, 0x00U, &stable_out, &changed_out);
    TEST_CHECK(changed_out == false, "changed_out == false on the final call");
    TEST_CHECK(stable_out == 0x00U, "stable_out == 0x00 on the final call");
    TEST_CHECK(inst.count_ticks == 0U, "count_ticks == 0 after the final call");
}

static void test_UT_US010_019(void)
{
    DrvInDebounce_t inst;
    init_instance_direct(&inst, 0x00U, 5U);

    DrvIn_Value_t stable_out = SENTINEL_VALUE;
    bool          changed_out = false;

    for (uint16_t call = 1U; call <= 20U; call++) {
        DrvIn_Value_t raw = ((call % 2U) == 1U) ? (DrvIn_Value_t)0x01U : (DrvIn_Value_t)0x00U;
        (void)DrvInDebounce_Update(&inst, raw, &stable_out, &changed_out);
        TEST_CHECK(changed_out == false, "changed_out == false under alternating input");
        TEST_CHECK(stable_out == 0x00U, "stable_out == 0x00 under alternating input");
    }
}

static void test_UT_US010_020(void)
{
    DrvInDebounce_t inst;
    init_instance_direct(&inst, 0x00U, 5U);

    DrvIn_Value_t stable_out = SENTINEL_VALUE;
    bool          changed_out = false;

    for (uint16_t call = 1U; call <= 4U; call++) {
        (void)DrvInDebounce_Update(&inst, 0x01U, &stable_out, &changed_out);
        TEST_CHECK(changed_out == false, "changed_out == false during the first (cancelled) candidate run");
    }
    (void)DrvInDebounce_Update(&inst, 0x00U, &stable_out, &changed_out);
    TEST_CHECK(changed_out == false, "changed_out == false on the cancelling call");

    for (uint16_t call = 1U; call <= 5U; call++) {
        (void)DrvInDebounce_Update(&inst, 0x01U, &stable_out, &changed_out);
        if (call == 4U) {
            TEST_CHECK(changed_out == false, "changed_out == false on the 4th call of the second run (not yet 5 after reset)");
        } else if (call == 5U) {
            TEST_CHECK(changed_out == true, "changed_out == true on the 5th call of the second run, five calls after the reset");
            TEST_CHECK(stable_out == 0x01U, "stable_out == 0x01 once the full threshold is reached after the reset");
        } else {
            TEST_CHECK(changed_out == false, "changed_out == false before the 5th call of the second run");
        }
    }
}

static void test_UT_US010_021(void)
{
    DrvInDebounce_t inst;
    init_instance_direct(&inst, 0x00U, 65535U);

    DrvIn_Value_t stable_out = SENTINEL_VALUE;
    bool          changed_out = false;
    uint32_t      changed_count = 0U;
    uint32_t      first_changed_call = 0U;

    for (uint32_t call = 1U; call <= 70000U; call++) {
        (void)DrvInDebounce_Update(&inst, 0x01U, &stable_out, &changed_out);
        TEST_CHECK(inst.count_ticks <= 65535U, "count_ticks never exceeds the maximum representable threshold");
        if (changed_out) {
            changed_count++;
            if (first_changed_call == 0U) {
                first_changed_call = call;
            }
        }
        if (call >= 65535U) {
            TEST_CHECK(stable_out == 0x01U, "stable_out == 0x01 from call 65535 onward");
            TEST_CHECK(inst.count_ticks == 0U, "count_ticks reads 0 on every call after the change");
        }
    }
    TEST_CHECK(changed_count == 1U, "changed_out is true exactly once across the whole 70000-call run");
    TEST_CHECK(first_changed_call == 65535U, "the single change is reported on call 65535, never exceeding or wrapping the threshold");
}

static void test_UT_US010_022(void)
{
    DrvInDebounce_t inst;
    init_instance_direct(&inst, 0x00U, 1U);

    DrvIn_Value_t stable_out = SENTINEL_VALUE;
    bool          changed_out = false;

    (void)DrvInDebounce_Update(&inst, 0x01U, &stable_out, &changed_out);
    TEST_CHECK(changed_out == true, "changed_out == true immediately with a one-tick threshold");
    TEST_CHECK(stable_out == 0x01U, "stable_out == 0x01 immediately with a one-tick threshold");
}

/* ===================== DrvInDebounce_Update: Negative ===================== */

static void test_UT_US010_023(void)
{
    DrvIn_Value_t stable_out = SENTINEL_VALUE;
    bool          changed_out = SENTINEL_CHANGED;

    DrvInDebounce_Status_t status = DrvInDebounce_Update(NULL, 0x01U, &stable_out, &changed_out);

    TEST_CHECK(status == DRVIN_DEB_ERR_NULL_PTR, "status == DRVIN_DEB_ERR_NULL_PTR for a NULL instance pointer");
    TEST_CHECK(stable_out == SENTINEL_VALUE, "stable_out keeps its sentinel value");
    TEST_CHECK(changed_out == SENTINEL_CHANGED, "changed_out keeps its sentinel value");
}

static void test_UT_US010_024(void)
{
    DrvInDebounce_t inst;
    init_instance_direct(&inst, 0x00U, 5U);
    DrvInDebounce_t inst_before = inst;

    bool changed_out = SENTINEL_CHANGED;

    DrvInDebounce_Status_t status = DrvInDebounce_Update(&inst, 0x01U, NULL, &changed_out);

    TEST_CHECK(status == DRVIN_DEB_ERR_NULL_PTR, "status == DRVIN_DEB_ERR_NULL_PTR for a NULL stable-output pointer");
    TEST_CHECK(changed_out == SENTINEL_CHANGED, "changed_out keeps its sentinel value");
    TEST_CHECK(memcmp(&inst, &inst_before, sizeof(inst)) == 0, "instance state is unchanged");
}

static void test_UT_US010_025(void)
{
    DrvInDebounce_t inst;
    init_instance_direct(&inst, 0x00U, 5U);
    DrvInDebounce_t inst_before = inst;

    DrvIn_Value_t stable_out = SENTINEL_VALUE;

    DrvInDebounce_Status_t status = DrvInDebounce_Update(&inst, 0x01U, &stable_out, NULL);

    TEST_CHECK(status == DRVIN_DEB_ERR_NULL_PTR, "status == DRVIN_DEB_ERR_NULL_PTR for a NULL changed-output pointer");
    TEST_CHECK(stable_out == SENTINEL_VALUE, "stable_out keeps its sentinel value");
    TEST_CHECK(memcmp(&inst, &inst_before, sizeof(inst)) == 0, "instance state is unchanged");
}

static void test_UT_US010_026(void)
{
    DrvInDebounce_t inst;
    (void)memset(&inst, 0, sizeof(inst)); /* zero-filled; Init never called */

    DrvIn_Value_t stable_out = SENTINEL_VALUE;
    bool          changed_out = SENTINEL_CHANGED;

    DrvInDebounce_Status_t status = DrvInDebounce_Update(&inst, 0x01U, &stable_out, &changed_out);

    TEST_CHECK(status == DRVIN_DEB_ERR_NOT_INIT, "status == DRVIN_DEB_ERR_NOT_INIT before a successful Init");
    TEST_CHECK(stable_out == SENTINEL_VALUE, "stable_out is left untouched");
    TEST_CHECK(changed_out == SENTINEL_CHANGED, "changed_out is left untouched");
}

static void test_UT_US010_027(void)
{
    DrvInDebounce_t inst;
    init_instance_direct(&inst, 0x00U, 5U);
    inst.threshold_ticks = 0U; /* TEST-ONLY corruption simulating damaged state */
    DrvIn_Value_t stable_value_before = inst.stable_value;

    DrvIn_Value_t stable_out = SENTINEL_VALUE;
    bool          changed_out = SENTINEL_CHANGED;

    DrvInDebounce_Status_t status = DrvInDebounce_Update(&inst, 0x01U, &stable_out, &changed_out);

    TEST_CHECK(status == DRVIN_DEB_ERR_STATE, "status == DRVIN_DEB_ERR_STATE for a corrupted zero threshold");
    TEST_CHECK(stable_out == SENTINEL_VALUE, "stable_out is left untouched");
    TEST_CHECK(changed_out == SENTINEL_CHANGED, "changed_out is left untouched");
    TEST_CHECK(inst.stable_value == stable_value_before, "stable_value field is unchanged");
}

static void test_UT_US010_028(void)
{
    DrvInDebounce_t inst;
    init_instance_direct(&inst, 0x00U, 5U);
    inst.count_ticks = 6U; /* TEST-ONLY corruption: count greater than threshold */

    DrvIn_Value_t stable_out = SENTINEL_VALUE;
    bool          changed_out = SENTINEL_CHANGED;

    DrvInDebounce_Status_t status = DrvInDebounce_Update(&inst, 0x01U, &stable_out, &changed_out);

    TEST_CHECK(status == DRVIN_DEB_ERR_STATE, "status == DRVIN_DEB_ERR_STATE when count exceeds threshold");
    TEST_CHECK(stable_out == SENTINEL_VALUE, "stable_out is left untouched");
    TEST_CHECK(changed_out == SENTINEL_CHANGED, "changed_out is left untouched");
}

int main(void)
{
    (void)printf("=== DrvInDebounce unit tests (US-010 / WIPC-7) ===\n");

    (void)printf("-- DrvInDebounce_Init: Positive --\n");
    TEST_RUN(test_UT_US010_001);
    TEST_RUN(test_UT_US010_002);
    TEST_RUN(test_UT_US010_003);

    (void)printf("-- DrvInDebounce_Init: Boundary --\n");
    TEST_RUN(test_UT_US010_004);
    TEST_RUN(test_UT_US010_005);
    TEST_RUN(test_UT_US010_006);

    (void)printf("-- DrvInDebounce_Init: Negative --\n");
    TEST_RUN(test_UT_US010_007);
    TEST_RUN(test_UT_US010_008);
    TEST_RUN(test_UT_US010_009);

    (void)printf("-- DrvInDebounce_Update: Positive --\n");
    TEST_RUN(test_UT_US010_010);
    TEST_RUN(test_UT_US010_011);
    TEST_RUN(test_UT_US010_012);
    TEST_RUN(test_UT_US010_013);
    TEST_RUN(test_UT_US010_014);
    TEST_RUN(test_UT_US010_015);
    TEST_RUN(test_UT_US010_016);

    (void)printf("-- DrvInDebounce_Update: Boundary --\n");
    TEST_RUN(test_UT_US010_017);
    TEST_RUN(test_UT_US010_018);
    TEST_RUN(test_UT_US010_019);
    TEST_RUN(test_UT_US010_020);
    TEST_RUN(test_UT_US010_021);
    TEST_RUN(test_UT_US010_022);

    (void)printf("-- DrvInDebounce_Update: Negative --\n");
    TEST_RUN(test_UT_US010_023);
    TEST_RUN(test_UT_US010_024);
    TEST_RUN(test_UT_US010_025);
    TEST_RUN(test_UT_US010_026);
    TEST_RUN(test_UT_US010_027);
    TEST_RUN(test_UT_US010_028);

    (void)printf("=== %u passed, %u failed ===\n", g_test_pass_count, g_test_fail_count);
    return (g_test_fail_count == 0U) ? 0 : 1;
}
