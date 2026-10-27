#!/usr/bin/env python3
"""Check the skill itself: layout, front matter, link targets, and that the template CSV validates.

Usage: check_skill.py   (run from anywhere)
"""
import re
import subprocess
import sys
from pathlib import Path

SKILL_DIR = Path(__file__).resolve().parent.parent
REQUIRED = [
    "SKILL.md",
    "scripts/spec_schema.py",
    "scripts/create_spec_csv.py",
    "scripts/validate_spec_csv.py",
    "assets/unit-test-spec-template.csv",
    "assets/cases-example.json",
    "references/test-design-rules.md",
    "references/csv-schema.md",
]


def main() -> int:
    errors = []

    for rel in REQUIRED:
        if not (SKILL_DIR / rel).is_file():
            errors.append(f"missing file: {rel}")

    skill_md = SKILL_DIR / "SKILL.md"
    if skill_md.is_file():
        text = skill_md.read_text(encoding="utf-8")
        fm = re.match(r"^---\n(.*?)\n---\n", text, re.S)
        if not fm:
            errors.append("SKILL.md: front matter missing")
        else:
            name = re.search(r"^name:\s*(\S+)\s*$", fm.group(1), re.M)
            if not name or name.group(1) != SKILL_DIR.name:
                errors.append(f"SKILL.md: name must equal folder name {SKILL_DIR.name!r}")
            desc = re.search(r"^description:\s*(.+)$", fm.group(1), re.M)
            if not desc:
                errors.append("SKILL.md: description missing")
            elif len(desc.group(1).strip("'\"")) > 1024:
                errors.append("SKILL.md: description exceeds 1024 characters")
        for target in re.findall(r"\]\((\./[^)#\s]+)", text):
            if not (SKILL_DIR / target).exists():
                errors.append(f"SKILL.md: broken link {target}")

    template = SKILL_DIR / "assets" / "unit-test-spec-template.csv"
    if template.is_file():
        proc = subprocess.run(
            [sys.executable, str(SKILL_DIR / "scripts" / "validate_spec_csv.py"),
             str(template), "--skip-source-paths", "--strict"],
            capture_output=True, text=True)
        if proc.returncode != 0:
            errors.append("template CSV failed validation:\n" + proc.stdout + proc.stderr)

    for e in errors:
        print(f"ERROR: {e}")
    print("PASS: skill assets and references are consistent" if not errors else f"FAIL: {len(errors)} problem(s)")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
