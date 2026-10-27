#!/usr/bin/env python3
"""Write a unit test spec CSV from a JSON array of test cases.

Usage:
  create_spec_csv.py --story US-010 --title driver-input-filtering-and-debounce \
      --cases cases.json            # or --cases - to read stdin
  create_spec_csv.py ... --append   # continue Test_ID numbering in an existing file
  create_spec_csv.py --story US-010 --title x --init   # header-only file

Output: docs/verification/unit-test-specs/<story>-<title>-unit-test-spec.csv
(override with --out). Cells are normalised, all fields quoted, UTF-8, LF.
"""
import argparse
import csv
import json
import re
import sys
from pathlib import Path

from spec_schema import (
    CATEGORIES, COLUMNS, DEFAULT_STATUS, INPUT_COLUMNS, STORY_RE, TEST_ID_RE,
    split_requirements, story_prefix,
)

DEFAULT_DIR = Path("docs/verification/unit-test-specs")


def clean(value) -> str:
    return re.sub(r"\s+", " ", str(value if value is not None else "")).strip()


def normalise_case(raw: dict, index: int, story: str) -> dict:
    by_lower = {k.lower(): v for k, v in raw.items()}
    row = {c: clean(by_lower.get(c.lower(), "")) for c in INPUT_COLUMNS}
    row["Story_ID"] = row["Story_ID"] or story
    cat = row["Category"].capitalize()
    row["Category"] = cat if cat in CATEGORIES else row["Category"]
    row["Requirement_IDs"] = "; ".join(t.upper() for t in split_requirements(row["Requirement_IDs"]))
    missing = [c for c in INPUT_COLUMNS if not row[c]]
    if missing:
        raise ValueError(f"case #{index}: empty required field(s): {', '.join(missing)}")
    row["Status"] = clean(by_lower.get("status", "")) or DEFAULT_STATUS
    return row


def next_number(path: Path, prefix: str) -> int:
    highest = 0
    with path.open(newline="", encoding="utf-8") as fh:
        for rec in csv.DictReader(fh):
            m = TEST_ID_RE.match(rec.get("Test_ID", ""))
            if m and m.group(1) == prefix:
                highest = max(highest, int(m.group(2)))
    return highest + 1


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--story", required=True, help="Story ID, e.g. US-010")
    ap.add_argument("--title", required=True, help="kebab-case story title for the file name")
    ap.add_argument("--cases", help="JSON file with an array of cases, or - for stdin")
    ap.add_argument("--out", help="output path (default under docs/verification/unit-test-specs/)")
    ap.add_argument("--append", action="store_true", help="append to an existing file")
    ap.add_argument("--init", action="store_true", help="write header only")
    args = ap.parse_args()

    if not STORY_RE.match(args.story):
        print(f"ERROR: story must look like US-010, got {args.story!r}", file=sys.stderr)
        return 2
    if not re.match(r"^[a-z0-9]+(?:-[a-z0-9]+)*$", args.title):
        print("ERROR: --title must be kebab-case (lowercase letters, digits, hyphens)", file=sys.stderr)
        return 2

    out = Path(args.out) if args.out else DEFAULT_DIR / f"{args.story}-{args.title}-unit-test-spec.csv"
    prefix = story_prefix(args.story)

    cases = []
    if not args.init:
        if not args.cases:
            print("ERROR: --cases is required unless --init is used", file=sys.stderr)
            return 2
        text = sys.stdin.read() if args.cases == "-" else Path(args.cases).read_text(encoding="utf-8")
        try:
            data = json.loads(text)
        except json.JSONDecodeError as exc:
            print(f"ERROR: cases JSON is invalid: {exc}", file=sys.stderr)
            return 2
        if not isinstance(data, list) or not data or not all(isinstance(c, dict) for c in data):
            print("ERROR: cases JSON must be a non-empty array of objects", file=sys.stderr)
            return 2
        try:
            cases = [normalise_case(c, i, args.story) for i, c in enumerate(data, 1)]
        except ValueError as exc:
            print(f"ERROR: {exc}", file=sys.stderr)
            return 2

    if out.exists() and not args.append:
        print(f"ERROR: {out} exists; use --append or choose another --out", file=sys.stderr)
        return 2
    if args.append and not out.exists():
        print(f"ERROR: --append but {out} does not exist", file=sys.stderr)
        return 2

    number = next_number(out, prefix) if args.append else 1
    for case in cases:
        case["Test_ID"] = f"UT-{prefix}-{number:03d}"
        number += 1

    out.parent.mkdir(parents=True, exist_ok=True)
    mode = "a" if args.append else "w"
    with out.open(mode, newline="", encoding="utf-8") as fh:
        writer = csv.DictWriter(fh, fieldnames=COLUMNS, quoting=csv.QUOTE_ALL, lineterminator="\n")
        if not args.append:
            writer.writeheader()
        writer.writerows(cases)

    print(f"Wrote {len(cases)} case(s) to {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
