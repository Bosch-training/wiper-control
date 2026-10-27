---
name: unit-test-spec-generator
description: 'Generate a unit test specification as a CSV file for logical units (pure logic, no hardware) from a story design document, with positive, negative and boundary cases and mocked external dependencies. Use when asked for unit test spec, unit test cases, test specification CSV, test case table, positive negative boundary tests, or mock-based unit tests for a wiper control story (US-nnn) under docs/design. Includes scripts to create and validate the CSV.'
argument-hint: 'Story ID (e.g. US-010) or path to a design doc under docs/design'
---

# Unit Test Spec Generator

Produces one CSV per story at `docs/verification/unit-test-specs/<US-nnn>-<kebab-title>-unit-test-spec.csv`. It specifies tests only; it does not write test code and does not claim any test was run.

## When to Use
- A design document exists in `docs/design/` and the user wants unit test cases for it.
- Not for timing/WCET, HIL, integration, static analysis, or security fuzzing tests (see [test design rules](./references/test-design-rules.md)).

## Procedure
1. **Locate inputs.** Read the design document for the story (if only an ID is given, find `docs/design/<US-nnn>-*.md`; if missing, ask the user to run the story design first). Read the PRD requirements it cites and [AGENTS.md](../../../AGENTS.md) for constraints. Do not read `.env*` files.
2. **Identify logical units.** From Interfaces, Behavior, and Error handling, list each unit and function that is pure host-testable logic. Exclude everything out of scope per the [rules](./references/test-design-rules.md) and note it for the summary.
3. **Identify dependencies.** For each function list external dependencies (HAL, drivers, clock, NVM, network, other modules) and define the mock behavior from the design interface.
4. **Derive cases.** For every function, write Positive, Negative, and Boundary cases following the [rules](./references/test-design-rules.md). Cover every acceptance criterion and every failure path in the design. Use only sourced values; label chosen values `TEST-ONLY`; never invent limits, ASIL, MCU, or timings.
5. **Write the cases JSON** following [csv-schema.md](./references/csv-schema.md) (see [example](./assets/cases-example.json)). Save it outside the deliverable, for example `build/<US-nnn>-cases.json`, or pipe via stdin.
6. **Create the CSV with the script**, never by hand:
   ```
   python3 .github/skills/unit-test-spec-generator/scripts/create_spec_csv.py \
     --story US-010 --title driver-input-filtering-and-debounce --cases build/US-010-cases.json
   ```
   Use `--append` to add cases to an existing file; IDs continue the sequence.
7. **Validate** (resolve every finding, then re-run until PASS):
   ```
   python3 .github/skills/unit-test-spec-generator/scripts/validate_spec_csv.py \
     docs/verification/unit-test-specs/US-010-driver-input-filtering-and-debounce-unit-test-spec.csv \
     --refs docs/wiper_control_system_product_requirements.md docs/design/US-010-driver-input-filtering-and-debounce.md --strict
   ```
   Fix the cases JSON and regenerate (delete the draft CSV first, or use another `--out`); do not hand-edit the CSV.
8. **Report** briefly: file path, case counts per function and category, units or tests deliberately not generated (and why), open questions, and the validator result. State that the spec is a Draft pending human review.

## Completion checks
- Validator prints PASS with `--refs` and `--strict`.
- Every acceptance criterion in the design maps to at least one row.
- Every function has Positive, Negative, and Boundary rows.
- Every external dependency is named in `Mocked_Dependencies`.

## Maintaining this skill
Run `python3 .github/skills/unit-test-spec-generator/scripts/check_skill.py` after changing the scripts, schema, or assets. It verifies required files, front matter, link targets, and that [the template CSV](./assets/unit-test-spec-template.csv) passes the validator.
