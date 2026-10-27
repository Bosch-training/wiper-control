# CSV Schema

One file per story: `docs/verification/unit-test-specs/<US-nnn>-<kebab-title>-unit-test-spec.csv`. UTF-8, LF, all fields quoted, one header row, one row per test case, no newlines inside cells. Write it only with `scripts/create_spec_csv.py`; the column list lives in `scripts/spec_schema.py`.

| Column | Rule |
|---|---|
| `Test_ID` | Generated: `UT-US010-001`, `UT-US010-002`, ... Prefix is the story ID without the hyphen. Unique. Never reused or renumbered once reviewed; use `--append` to add cases. |
| `Story_ID` | `US-nnn`. One story per file. |
| `Requirement_IDs` | One or more of `PR-nnn`, `SR-nnn`, `SEC-nnn`, `DR-nnn-nn`, `AC-n`, separated by `; `. Each must exist in the PRD or design document (checked with `--refs`). |
| `Unit_Under_Test` | Module or class name (for example `DrvInDebounce`). A logical unit only. |
| `Function_Under_Test` | Function or method name. |
| `Category` | `Positive`, `Negative`, or `Boundary`. |
| `Title` | One short sentence stating the behavior checked. Unique per unit, function, and category. |
| `Preconditions` | Initial state, configuration, and mock setup. Test-only values are labelled `TEST-ONLY`. |
| `Inputs` | Concrete arguments and stimulus sequence, with units. |
| `Mocked_Dependencies` | `None (reason)` when the unit has no external dependency; otherwise `MockName: scripted behavior` entries separated by `; `. Never blank. |
| `Expected_Result` | Observable outcome: return status, output values, state, and what must remain untouched. Concrete, not "works correctly". |
| `Source_Ref` | `<repo-relative path>#<anchor or design test ID>`, for example `docs/design/US-010-driver-input-filtering-and-debounce.md#T-02`. The file must exist. |
| `Status` | `Draft` (default) or `Reviewed`. Set `Reviewed` only after a human review. |

## Cases JSON (input to the create script)

An array of objects using the column names above, without `Test_ID` and `Status` (key case is ignored). `Story_ID` defaults to `--story`. See `assets/cases-example.json`; the generated sample is `assets/unit-test-spec-template.csv`.

## Validator rules

Errors: wrong header, empty cell, bad Test_ID or prefix mismatch, duplicate ID or case, bad Category/Status/requirement syntax, out-of-scope test type in Unit/Function/Title (WCET, HIL, static analysis, fuzzing, integration), a unit function lacking any of the three categories, missing `Source_Ref` file, requirement ID not found in `--refs` files.

Warnings (errors with `--strict`): `Mocked_Dependencies` is `None` while Preconditions or Inputs mention a dependency such as HAL, ADC, PWM, LIN, NVM, or a clock; `TBD` inside Inputs.
