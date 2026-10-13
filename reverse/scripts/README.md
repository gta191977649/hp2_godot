# Analysis scripts

Future Ghidra exports belong here. Scripts should record tool/specification
versions and binary hashes, avoid destructive analysis changes by default,
and export stable CSV/Markdown references. The initial "no analysis script run" statement
is historical; recovery tooling and evidence now live here.

TASK-0028 class templates: `py -3 reverse/scripts/generate_class_templates.py --write`
appends missing declarations/bodies and refreshes metadata without replacing existing
implementations. `--check` is read-only and is a CTest coverage check. See
`docs/CLASS_TEMPLATES.md` before changing signatures or adding recovered names.
