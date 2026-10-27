# Test Design Rules

## 1. What counts as a logical unit

Test only deterministic logic that runs on the host with no hardware: pure C functions and modules, state machines, calculations, validation, arbitration, mapping and filtering, status-code decisions.

Do not generate rows for these (list them as "not generated" in the summary instead):
- Timing on target: WCET, jitter, overrun, scheduler behavior.
- Hardware-in-the-loop, integration or SIL scenarios spanning several modules.
- Static analysis, MISRA or coding-rule checks, code review items.
- Fuzzing, tamper, and other security tests of external interfaces.
- Register-level HAL or driver code that has no logic of its own.

Source for the unit list: the Interfaces, Behavior, and Error handling sections of the design document, plus its Verification plan rows of type Unit, Boundary, or Fault injection.

## 2. Mocking

Every external dependency of the unit is replaced, never exercised for real: HAL and drivers, ADC, PWM, GPIO, clock or tick sources, NVM or flash, vehicle network (LIN) and diagnostics, watchdog, other modules owned by other stories.

- Name each in `Mocked_Dependencies` as `MockName: scripted behavior`, for example `MockAdc: returns 512 then 0; MockClock: advances 2 ms per call`.
- Script the mock to produce the stimulus the case needs, including its fault returns (timeout, error status, stale value).
- If the unit takes plain values and ticks as inputs and has no dependency, write `None (pure logic, no external dependency)`.
- Do not mock the unit under test or its own private helpers.
- Mock behavior must come from the design interface. If the interface is undefined, record an open question rather than inventing it.

## 3. Case categories

Per function, produce at least one case of each category. Add more where the design has several behaviors.

| Category | Derive from | Examples |
|---|---|---|
| Positive | Each acceptance criterion and each documented normal behavior | Valid input produces the specified output, state, or status OK |
| Negative | Each failure path in the Error handling section | NULL pointer, uninitialised use, invalid configuration, out-of-range or corrupted state, dependency mock returning an error; outputs untouched, error status returned |
| Boundary | Every numeric limit, threshold, size, and state edge | Minimum, minimum-1, maximum, maximum+1; threshold N-1, N, N+1; empty and full buffers; counter saturation; exact multiple versus non-multiple in conversions |

For each boundary, include the values on both sides of the edge: either one row per value or one row with a clearly stepped input sequence.

## 4. No invented values

- Use values from the PRD, story, or design. Where a value is `TBD` there, choose a test value, label it `TEST-ONLY` in Preconditions or Inputs, and never present it as a requirement.
- Never infer motor limits, thresholds, timings, ASIL, MCU, or vehicle interface. Demo defaults from AGENTS.md keep their "demo default" label.
- If the expected result cannot be stated from the sources, do not guess; leave the case out and report it as an open question.

## 5. Traceability and quality

- Every case lists at least one requirement ID; every acceptance criterion in the story is covered by at least one case.
- Add the matching design test ID to `Source_Ref` when one exists.
- Expected results name concrete observable outputs (status codes, values, flags) and what must stay unchanged on failure.
- Safety-related units (motor limits, timing, watchdog, fault handling) keep `Status` as `Draft` and are flagged for review.
- Do not claim the tests were run or that compliance is achieved.
