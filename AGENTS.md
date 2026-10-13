# Session contract

This repository initializes a PS2-to-PC native port. No actual game functions
have been recovered. Repository files are the durable project memory.

See **[AGENT.md](AGENT.md)** for the binding naming/evidence rules (no guesswork; identifiers
must be recovered-verbatim, address-anchored, or explicitly port-side; unimplemented paths throw
`hp2::NotImplemented`). AGENT.md is stricter than this file wherever they overlap.

## Mandatory rules

1. Read project documentation before making changes.
2. Read the current task file before implementation.
3. Work on one clearly scoped task at a time; record its ID in progress notes.
4. Never invent unverified PS2 behavior. Unknown means unknown.
5. Preserve original function addresses and provenance, including ELF SHA-256 and version.
6. Separate confirmed findings, hypotheses, and unresolved questions.
7. Keep recovered C++ logic independent of Godot and its headers/types.
8. Avoid unnecessary modifications to unrelated modules and preserve user files.
9. Require tests for reconstructed behavior whenever practical. Explain omissions.
10. Update task status and documentation after completing work.
11. Never mark unverified code as Verified. PC tests alone do not establish equivalence.
12. Ensure the next session can continue from repository files without chat history.
13. Follow AGENT.md identifier provenance: recovered names verbatim, otherwise address-anchored
    (`Sub_<ADDR>`), never an invented descriptive name for recovered code.
14. An unimplemented recovered path throws `hp2::NotImplemented`; never a silent default.

Do not replace recovered physics with Godot physics without an explicit recorded
decision. Never add proprietary ELF/ISO files, extracted assets, secrets, or Ghidra
databases to Git. Infrastructure examples have no original-game provenance.

## Startup protocol

1. Inspect `git status --short`, active tasks, and local instructions before edits.
2. Read `README.md`, `docs/PROGRESS.md` (the tracker; `docs/PROGRESS_LOG.md` holds the
   session narrative behind it), `docs/ARCHITECTURE.md`,
   `docs/DECISIONS.md`, `docs/DEVELOPMENT.md`, and `docs/REVERSE_NOTES.md`.
3. Read the selected task. If no task is active, select the earliest unblocked
   backlog task matching the user's request and move it to active before work.
   A new request may require its own scoped task; do not silently widen another.
4. Read relevant function/structure records and evidence. Check dependency pins
   and tool availability; do not assume the last machine's environment.
5. Record scope, acceptance checks, and blockers. Continue independent work when
   blocked; do not manufacture evidence or advance verification status.

## Completion protocol

1. Run relevant builds/tests and record exact commands, results, tool versions,
   and limitations. Distinguish passed, failed, and blocked checks.
2. Update knowledge records and symbols index when findings change. Keep evidence
   identifiers stable; record counterevidence and unresolved assumptions.
3. Update the task's findings, acceptance criteria, next steps, and related commits.
   Move to completed only when every criterion has evidence; blocked/incomplete
   tasks stay active or return to backlog with an explicit explanation.
4. Update `docs/PROGRESS.md` with active work, blockers, validation, and exact next
   action. Its tables are generated -- run `py -3 reverse/scripts/generate_progress.py`
   rather than editing between the `generated:` markers; the `progress_tables` test
   fails when they are stale. Session narrative goes in `docs/PROGRESS_LOG.md`, not in
   the tracker. Record architecture changes in `docs/DECISIONS.md`.
5. Inspect the diff and Git status for unrelated edits and restricted material.
   Never stage everything blindly. Do not claim a commit exists until made.
6. Report outcome and remaining limitations. Leave a reproducible continuation
   prompt or command, including the task ID and any missing prerequisites.
