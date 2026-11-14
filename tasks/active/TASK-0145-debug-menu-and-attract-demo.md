# TASK-0145 - Debug menu and attract demo

- Status: active
- Scope: HP2 frontend Debug submenu, existing Track Viewer entry, Car Viewer placeholder, and original attract demo in Godot.
- Acceptance: Debug and Release builds, frontend smoke, menu input and popup, manual and idle demo, completion and interruption, and isolated profile checks.

The Windows donor implements the PAL `FrontendDemoMode` launch policy and
`RaceCoordinator` demo states. Its exact source ELF hash is unresolved as
documented in `docs/REVERSE_NOTES.md`. Debug frontend entries are port code.
