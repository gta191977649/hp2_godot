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

## Race entry

The default project entry runs the Godot frontend. The explicit `--race-track=12`
entry requests the donor's single-player Quick Race and starts it in the
existing skip-start-sequence mode. The donor services original assets,
input, rules, audio, physics and AI. Its state-only update returns before
Ecstasy draw capture and submission.

Entity State Bridge copies the committed vehicle and wheel poses, speed,
visibility and primary camera. It identifies each publication by World
generation and step and each car by its World-issued handle. Godot creates
track meshes from decoded authored placements and car meshes from decoded
model assets. The shared mesh converter uploads authored RGB555 vertex
colors for the track, sky, vehicles and frontend previews. Godot's unshaded
standard material multiplies the vertex color by the albedo texture.
These reads finish synchronously before the next World step.
The scene tree owns the nodes. Godot resource references retain meshes and
textures. Godot applies the camera and vehicle transforms from the same
snapshot and releases nodes when their handles or generation retire.

The donor Player and RaceCoordinator provide the HUD and race screens.
Unsupported entries and invalid integration data emit `HP2_MVP_ASSERT`
with the stage, track and reason and stop the run with exit code 1.

[The implementation record](../tasks/active/TASK-0124-modular-godot-presentation.md)
documents the Forward+ drive and open presentation work.


## Frontend

The donor menu, profile, Player and RaceCoordinator owners provide state.
Flailer resolves authored glyphs, kerning, clipping, bitmap UVs and layer
order, including the event tree's dynamic elements and the race HUD.
Godot synchronously consumes the CPU layout and requests decoded pixels
from game_core's shared resource decoder. Godot CanvasItems own 2D drawing,
ImageTextures hold the uploaded images, and SubViewports present previews.
A Godot Tween handles screen fades and the selected item's echo, using
the authored start pose, scale ratio, alpha and period from Flailer.
Some authored animation values still come from Flailer and Player clocks.
The same canvas survives race entry, pause, and the return to the frontend.
The donor selects the event's track and checks the player count before
loading; the Godot adapter supplies the list of tracks it can present
(12 and 13). A traffic car can change its skin while keeping its World
handle. Godot replaces its visual node when the committed appearance changes.
The donor supplies the minimap geometry and homogeneous texture coordinates.
The Godot canvas shader interpolates those coordinates before sampling the
road texture so it shares the marker projection in tilted modes.
Godot fits the authored 640×480 menu and HUD canvas uniformly inside the
live viewport. It tiles the authored grid at the same scale beyond the
canvas and fits video at 4:3. The race camera retains vertical framing
and renders the scene across the full viewport.
Frontend 3D previews render at the fitted canvas's pixel dimensions
and composite into their authored 4:3 rectangle. Clipped grid fragments
retain their original geometry.
Select Car uses the same unshaded texture and vertex color material
as race vehicles, including donor-resolved lamp texture bindings.

The title movie owner publishes a resource path and request ID. Godot's
VideoStreamPlayback backend gives FFmpeg a seekable view of the original AVI
inside the game resource. FFmpeg decodes MV30 as playback advances. Godot
presents the decoded planes and acknowledges the request at EOF. The
existing PS2 ADPCM decoder handles title audio mislabeled as PCM, and
the playback backend submits it to the Godot audio mixer.
