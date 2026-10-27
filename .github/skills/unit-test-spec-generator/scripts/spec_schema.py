"""Shared schema for unit test spec CSV files (single source of truth)."""
import re

COLUMNS = [
    "Test_ID",
    "Story_ID",
    "Requirement_IDs",
    "Unit_Under_Test",
    "Function_Under_Test",
    "Category",
    "Title",
    "Preconditions",
    "Inputs",
    "Mocked_Dependencies",
    "Expected_Result",
    "Source_Ref",
    "Status",
]

# Columns the author supplies; Test_ID is generated, Status defaults to Draft.
INPUT_COLUMNS = [c for c in COLUMNS if c not in ("Test_ID", "Status")]

CATEGORIES = ("Positive", "Negative", "Boundary")
STATUSES = ("Draft", "Reviewed")
DEFAULT_STATUS = "Draft"

STORY_RE = re.compile(r"^US-\d{3}$")
TEST_ID_RE = re.compile(r"^UT-(US\d{3})-(\d{3})$")
REQ_TOKEN_RE = re.compile(r"^(?:PR|SR|SEC|DR|AC)-\d{1,3}(?:-\d{1,2})?$")

NONE_PREFIX = "none"  # Mocked_Dependencies must start with "None" when nothing is mocked


def story_prefix(story_id: str) -> str:
    return story_id.replace("-", "")


def split_requirements(value: str):
    return [t for t in re.split(r"[;,\s]+", value.strip()) if t]
