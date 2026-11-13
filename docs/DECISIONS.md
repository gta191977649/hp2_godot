# Design decisions

## TASK-0123 â€” One Windows donor game implementation

The user selected the Windows source distribution at `D:/dev/hp2_ps2_rev`
as the sole implementation. Its native pointers and allocators own the
simulation and resource lifetimes. The single `game_core` target builds the
donor without its standalone Vulkan executable. The Godot MVP uses the
donor's native application host interface and a committed World bridge.
The retired port's Debug and Retail symbol records remain historical evidence.

Godot keeps the earlier ownership of scene instances and the entire rendering
pipeline, including UI presentation and post-processing. Donor presentation code supplies CPU
inputs. The bridge may read those inputs and committed World state.

This supersedes the earlier two-edition build and Debug World ownership. Their recorded history is in
[PROGRESS_LOG.md](PROGRESS_LOG.md) and the task files.

## TASK-0124 — Publish committed World state to Godot

The explicit Track 12 entry uses the donor Quick Race and the state-only
World update. The Godot extension owns the scene and reads decoded model
assets during construction. It receives live transforms and the primary
camera through Entity State Bridge. The donor retains simulation lifetimes
and player scoring. Godot presents the HUD. The earlier draw-command
adapter was retired.


## TASK-0125 - Frontend presentation and media

The donor owns frontend actions, profile settings and race selection.
Godot owns menu nodes, transitions, car preview viewports and movie
playback. The external movie boundary carries request and EOF state.
FFmpeg reads the original MV30 video and writes the encoded stream
consumed by Godot's VideoStreamPlayer.
