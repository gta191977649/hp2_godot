# Architecture

The Godot MVP uses one game implementation, imported from the Windows source
distribution at `D:/dev/hp2_ps2_rev`.

| Path | Owner |
| --- | --- |
| `src/game_core` | Donor C/C++ application, input scanner, race, World, collisions, assets, audio, replay and save state. |
| `src/mvp/bridge` | Value snapshots of the donor World after native update. |
| `godot_mvp` | Godot project and explicit race entry. |

The game core owns simulation data and gameplay. Godot owns the lifetime
of scene nodes and presentation resources. The bridge carries committed
state and identifies vehicle lifetimes with World generation and source ID.

## Track 12 race entry

The default project entry runs the Godot frontend. The explicit `--race-track=12`
entry requests the donor's single-player Quick Race and starts it in the
existing skip-start-sequence mode. The donor services original assets,
input, rules, audio, physics and AI. Its state-only update returns before
Ecstasy draw capture and submission.

Entity State Bridge copies the committed vehicle and wheel poses, speed,
visibility and primary camera. It identifies each publication by World
generation and step and each car by its World-issued handle. Godot creates
track meshes from decoded authored placements and car meshes from decoded
model assets. These reads finish synchronously before the next World step.
The scene tree owns the nodes. Godot resource references retain meshes and
textures. Godot applies the camera and vehicle transforms from the same
snapshot and releases nodes when their handles or generation retire.

The race uses a Godot label for speed and controls. Unsupported entries
and invalid integration data emit `HP2_MVP_ASSERT` with the stage, track
and reason and stop the run with exit code 1.

[The implementation record](../tasks/active/TASK-0124-modular-godot-presentation.md)
documents the Forward+ drive and open presentation work.


## Frontend

The frontend starts the donor menu and profile owners. It reads authored
object state, localized text, event entries and decoded preview assets.
Godot creates CanvasItems, controls and preview SubViewports, and uses
Tweens for screen fades and selection feedback. The donor validates the
chosen event and requests the normal GameFlow transition. The port checks
the requested track and player count before native track loading.

The title movie owner publishes a resource path and request ID. FFmpeg
decodes the AVI to a temporary Ogg Theora stream. The existing PS2 ADPCM
decoder handles the title audio payload mislabeled as PCM in the AVI.
Godot plays the resulting stream and acknowledges the request at EOF.
