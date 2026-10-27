#!/usr/bin/env python3
"""Validate a unit test spec CSV.

Usage:
  validate_spec_csv.py SPEC.csv [--refs FILE ...] [--root DIR] [--skip-source-paths]
                       [--strict] [--warn-coverage]

Errors (exit 1): header/columns, empty cells, Test_ID format/uniqueness/story match,
Category/Status values, requirement ID syntax, duplicates, out-of-scope test types,
missing Positive/Negative/Boundary per function, Source_Ref file missing under --root,
requirement IDs absent from every --refs file.
Warnings: possible un-mocked external dependency, TBD in inputs. --strict makes them errors.
"""
import argparse
import csv
import io
import re
import sys
from collections import defaultdict
from pathlib import Path

from spec_schema import (
    CATEGORIES, COLUMNS, NONE_PREFIX, REQ_TOKEN_RE, STATUSES, STORY_RE, TEST_ID_RE,
    split_requirements, story_prefix,
)

OUT_OF_SCOPE = re.compile(
    r"\b(wcet|jitter|hil|hardware[- ]in[- ]the[- ]loop|on[- ]target|static analysis|cppcheck|"
    r"clang-tidy|fuzz\w*|integration|sil)\b", re.I)
EXTERNAL_DEP = re.compile(
    r"\b(hal|adc|pwm|gpio|uart|spi|i2c|lin|can|nvm|eeprom|flash|register|watchdog|"
    r"clock|timer|sensor|motor driver|network|diagnostic)\b", re.I)
# Source_Ref is "<relative path>" optionally followed by "#<anchor or test id>".
SOURCE_PATH_RE = re.compile(r"^([^#\s]+)")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("spec")
    ap.add_argument("--refs", nargs="*", default=[], help="files that must contain every requirement ID used")
    ap.add_argument("--root", default=".", help="repo root for resolving Source_Ref paths")
    ap.add_argument("--skip-source-paths", action="store_true", help="do not check that Source_Ref files exist")
    ap.add_argument("--strict", action="store_true", help="treat warnings as errors")
    ap.add_argument("--warn-coverage", action="store_true", help="downgrade missing-category errors to warnings")
    args = ap.parse_args()

    errors, warnings = [], []
    path = Path(args.spec)
    if not path.is_file():
        print(f"ERROR: {path} not found", file=sys.stderr)
        return 1

    try:
        text = path.read_bytes().decode("utf-8")
    except UnicodeDecodeError:
        print("ERROR: file is not valid UTF-8", file=sys.stderr)
        return 1
    if "\r" in text:
        errors.append("file contains CR characters; use LF line endings")

    rows = list(csv.reader(io.StringIO(text, newline="")))
    if not rows or rows[0] != COLUMNS:
        print("ERROR: header does not match the required columns:\n  " + ",".join(COLUMNS), file=sys.stderr)
        return 1
    if len(rows) == 1:
        errors.append("no test cases")

    ref_text = ""
    for ref in args.refs:
        rp = Path(ref)
        if not rp.is_file():
            errors.append(f"--refs file not found: {ref}")
        else:
            ref_text += rp.read_text(encoding="utf-8") + "\n"

    root = Path(args.root)
    seen_ids, seen_keys, stories = set(), set(), set()
    coverage = defaultdict(set)

    for lineno, row in enumerate(rows[1:], start=2):
        where = f"line {lineno}"
        if len(row) != len(COLUMNS):
            errors.append(f"{where}: expected {len(COLUMNS)} columns, found {len(row)}")
            continue
        rec = dict(zip(COLUMNS, row))
        tid = rec["Test_ID"]
        where = f"line {lineno} ({tid or 'no id'})"

        for col in COLUMNS:
            if not rec[col].strip():
                errors.append(f"{where}: empty {col}")
            elif rec[col] != rec[col].strip() or "\n" in rec[col]:
                errors.append(f"{where}: {col} has leading/trailing space or newline")

        m = TEST_ID_RE.match(tid)
        if not m:
            errors.append(f"{where}: Test_ID must match UT-US###-### (e.g. UT-US010-001)")
        if tid in seen_ids:
            errors.append(f"{where}: duplicate Test_ID")
        seen_ids.add(tid)

        story = rec["Story_ID"]
        stories.add(story)
        if not STORY_RE.match(story):
            errors.append(f"{where}: Story_ID must look like US-010")
        elif m and m.group(1) != story_prefix(story):
            errors.append(f"{where}: Test_ID prefix does not match Story_ID {story}")

        if rec["Category"] not in CATEGORIES:
            errors.append(f"{where}: Category must be one of {', '.join(CATEGORIES)}")
        if rec["Status"] not in STATUSES:
            errors.append(f"{where}: Status must be one of {', '.join(STATUSES)}")

        tokens = split_requirements(rec["Requirement_IDs"])
        for tok in tokens:
            if not REQ_TOKEN_RE.match(tok):
                errors.append(f"{where}: bad requirement ID {tok!r} (use PR-007, SR-001, SEC-001, DR-010-01, AC-2)")
            elif ref_text and not re.search(rf"(?<![A-Za-z0-9-]){re.escape(tok)}(?![A-Za-z0-9-])", ref_text):
                errors.append(f"{where}: {tok} not found in any --refs file")

        if not args.skip_source_paths:
            sm = SOURCE_PATH_RE.match(rec["Source_Ref"])
            if not sm or not (root / sm.group(1)).is_file():
                errors.append(f"{where}: Source_Ref path does not exist under --root: {rec['Source_Ref']}")

        scope_text = " ".join((rec["Unit_Under_Test"], rec["Function_Under_Test"], rec["Title"]))
        hit = OUT_OF_SCOPE.search(scope_text)
        if hit:
            errors.append(f"{where}: out of scope for a logical unit test ({hit.group(0)!r}); remove or move to another test level")

        mocks = rec["Mocked_Dependencies"].strip().lower()
        if mocks.startswith(NONE_PREFIX):
            dep = EXTERNAL_DEP.search(rec["Preconditions"] + " " + rec["Inputs"])
            if dep:
                warnings.append(f"{where}: mentions {dep.group(0)!r} but Mocked_Dependencies is None; mock it or reword")
        if re.search(r"\bTBD\b", rec["Inputs"]):
            warnings.append(f"{where}: Inputs contain TBD; use a value labelled TEST-ONLY")

        key = (rec["Unit_Under_Test"], rec["Function_Under_Test"], rec["Category"], rec["Title"].lower())
        if key in seen_keys:
            errors.append(f"{where}: duplicate case (same unit, function, category, title)")
        seen_keys.add(key)
        coverage[(rec["Unit_Under_Test"], rec["Function_Under_Test"])].add(rec["Category"])

    if len(stories) > 1:
        errors.append(f"multiple Story_IDs in one file: {', '.join(sorted(stories))}")

    for (unit, func), cats in sorted(coverage.items()):
        missing = [c for c in CATEGORIES if c not in cats]
        if missing:
            msg = f"{unit}::{func} missing {', '.join(missing)} case(s)"
            (warnings if args.warn_coverage else errors).append(msg)

    for w in warnings:
        print(f"WARN:  {w}")
    for e in errors:
        print(f"ERROR: {e}")
    failed = bool(errors) or (args.strict and bool(warnings))
    count = len(rows) - 1
    print(f"{'FAIL' if failed else 'PASS'}: {count} case(s), {len(errors)} error(s), {len(warnings)} warning(s)")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
