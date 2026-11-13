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
The Godot VideoStreamPlayback backend uses libavformat and libavcodec
on the original AVI bytes. It publishes decoded texture planes to Godot
and mixes the original title audio in memory. The temporary Ogg path
has been retired and its generated files removed.

## TASK-0126 - Shared authored UI canvas

The Godot frontend consumes Flailer CPU layout across menu and race states.
The donor owns resource selection, text layout, Player HUD data and
GameFlow. Godot owns CanvasItems, texture uploads, video and screen fades.
The port reads layout synchronously before the next logical step. This
path does not create an Ecstasy draw frame. Remaining donor animation
producers are tracked in TASK-0126.

## TASK-0128 - Display refresh pacing

The Godot window uses VSync and a zero software FPS limit for the
frontend and race. The donor clock remains independent of presentation.
External movie playback reports completion without borrowing writable
pixels from the original texture pack. The Godot texture cache can keep
reusing the published read-only textures when the menu movie is active.

## TASK-0129 - First Ultimate Racer event in Godot

The donor GameFlow continues to choose the authored event and its track.
The Godot race adapter supplies the supported track policy and presents
the committed Track 13 scene. Traffic appearance changes replace the
Godot presentation for the same World handle. The external smoke driver
uses read-only pose telemetry and normal input; it is not part of the
game or the donor core.

## TASK-0137 - Pursuit rendering

Godot batches repeated static opaque placements by authored section and
mesh. The entity bridge retains snapshot storage across publications, and
the Godot HUD retains minimap meshes while submitted geometry is identical.

## TASK-0138 - Busted cinematic

The donor provides the original replay timeline, camera, vehicle overrides
and actor pose matrices through the synchronous presentation boundary.
Godot owns the skinned meshes, props and monochrome scene compositing.

## TASK-0139 - Godot speech transport

The donor request queue and voice manager retain ordering, completion and
controls. The MVP can route original speech to Godot's resampled stream
player. One worker reads and decodes bounded ADPCM blocks. The mixer reads
PCM from a preallocated ring. The Godot route is opt-in while the other
game audio categories and the matched performance comparison remain open.

## TASK-0126 - Godot pointer input and renderer choice

Godot owns hit testing and fallback controls in the centered frontend canvas.
The application runtime validates screen generations and dispatches pointer
commands through the original UI callbacks on its frontend tick. Flailer
publishes the geometry and identities needed for mouse targets. Vulkan remains
the default Forward+ driver. The Windows launcher can also select D3D12 with
SDR output so Windows can decide whether to apply Auto HDR.

## TASK-0140 - Race effects ownership

The source keeps the AcidFX emitter scheduler, random streams and lifetime
ledger. Godot consumes ordered effects through the race scene and shares GPU
particle nodes by effect definition. It renders tyre marks from original
trail data and performs lamp and sun composition using Godot camera depth.
