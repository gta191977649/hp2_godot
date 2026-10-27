TASK0098 adds a native SelectionPage ownership binder with explicit host resource
leases, copied car nodes, cached widget groups and shared after-display FrameEffects.
The production host/entry remains unbound. Native Flailer layers/depth and Debug
alpha radix order are preserved for the required shared-depth compositor; the GPU
executor remains open. See reverse/task0098/page-ownership.md and
composition-preparation.md. No full career/race/save completion claim.

# Architecture and ownership

TASK0092/0093 extends the native frontend owner with Jukebox and Audio Options
snapshots, exact song-pair metadata, scalar Hermite state, live volume/output
requests and the successful Autosave host-device path. The Godot script polls
audio-device requests and owns its private output bus; it does not choose songs,
widget content, slider positions or save transitions. Autosave uses original
memory-card popup geometry and capacity wrapping, while platform storage retains
the existing read-input/private-write separation. See FRONTEND_COMPLETION.md and
the TASK0092/0093 evidence records for scope and original-hardware limitations.

TASK0088 separates original POV selection from held lookback: native camera
publishes pose/projection/own-car flags; the host binds V cycle, B hold/release
and temporary mouse orbit. TASK0089 adds raw native timed-wiper state and splash
requests to committed snapshots. The model decodes original rigid animation and
selects discrete matrices; Godot validates/transports snapshots and original sound
samples. DriveOver spray shares the existing native/GPU visual transaction and
does not advance native audio or physics during environment captures. See
reverse/task0088 and reverse/task0089 for exact boundaries and evidence.

TASK0081 adds explicit port-side frame-state slice bindings in frame_lifecycle.hpp,
implemented in the evidence-backed original car.cpp owner. NativeBackend runs them
once around movement/collision retries. Full Car entries remain partial/throwing;
unsupported route recovery fails transactionally at00132FB0. ResetCar's recovered
leaf is shared by fixture initialization and future route recovery. No Godot
physics or replacement reset policy. See reverse/task0081/lifecycle.md.

TASK0077 P0 establishes an export-separated benchmark boundary: bounded Profile
collection stops before snapshot/export, and the benchmark releases export only
after its measured loop. Run manifests bind source/engine/DLL inputs, actual
thread/effect settings and post-run identity checks; A/A deterministic controls
precede candidate diagnostics. This is port-side tooling, not a new RHI or PS2
behavior change. P1 submission, P2 resource lifetime and P3 build restructuring
are deferred. See [baseline protocol and limits](PERFORMANCE_TASK0077.md).

TASK0075 connects bounded original Car wheel-effect producers and terrain emitter
copies to shared AcidFX storage. The visual adapter stages simulation, bindings
and original terrain counters transactionally after existing particle updates;
it never steps physics again. Dynamic/static groups share pools/RNG. Tracks use
native depth; the flat fixture uses host depth for its synthetic floor. See
[wheel effects and boundaries](DRIVING_EFFECTS.md).

TASK0071 changes the Windows default to Vulkan/Forward+. Port-side numeric-GS
colour transport uses HDR colour/depth/reflection targets and LDR presentation;
the native command/state generator is unchanged. A startup autoload checks the
actual backend. Global ProfileDebug exports the actual driver and rendering
method. Per-primitive COLCLAMP and original-runtime parity remain explicit gaps.
See [migration status](PERFORMANCE_TASK0071.md).

TASK-0069 selects official Godot4.7.2 while retaining the existing minimum4.4
GDExtension ABI. The opt-in TrackProfiler autoload publishes cached HP2 metrics
to Performance and samples engine viewport timestamps; it does not own or step
native game state. Render API title labels use the active runtime driver.
CPU transport caches retain native command preparation and exact state/order
boundaries. See [profiling and known limits](PERFORMANCE_TASK0069.md).

TASK0070 replaces the track-specific collector name with global `ProfileDebug`
(`profile_debug.gd`) and draws live graphs/function scopes in the shared native
ImGui frame across all scenes. UI history is bounded and owns no game state;
function scopes are inclusive wall timing, not native sampling. See
[current profiler contract](PROFILE_DEBUG.md).

TASK-0060 adds bounded common-track host residency: lazy source meshes, direct
CPU-to-final batches, inactive draw LRU with coordinated depth/uniform release,
and lazy debug overlays. Native visibility/material/physics logic is unchanged.
See [ownership, diagnostics and tests](TRACK_MEMORY.md).

## A: Recovered game core

`game_core` is a static C++17 library. It owns authoritative game state, recovered
physics, AI, world management, and rules as these become understood. It cannot
include Godot headers or depend on the Godot runtime.

Its layout mirrors the original HP2 source tree where the ELF proves that tree
(TASK-0020, `reverse/naming/task0020-original-source-tree.csv`):

```text
src/game_core/
├── indep/src/     # paths recovered verbatim from ELF compile-time strings
│   ├── fmenus.cpp            0x002B27F8   ..\indep\src\fmenus.cpp
│   ├── eventtree.cpp         0x002BA9F8
│   ├── sound.cpp             0x002B62E8
│   ├── frontendscreens/      options.cpp 0x002B9E00, joystickconfiguration.cpp 0x002BE300
│   └── movieplayer/          movieplayer.cpp 0x002BDD08
├── bSound/        # bWare bSound middleware; src/bJukebox.cpp 0x002C4220,
│   │              # include/bJukebox.h 0x002C4208. The bSound/ root is inferred.
│   ├── include/bJukebox.h
│   └── src/bJukebox.cpp
└── port/          # NO original path recovered: name and location are this project's
    ├── assets/ engine/ frontend/ input/ movieplayer/ police/
    └── scaffold, demo, not_implemented
```

Every file under `indep/` and `bSound/` opens with the ELF string and address that
justifies its path; every file under `port/` opens with a note that its path is not
evidence. A path being original does not make the file's contents original: each is
still a partial port-side reconstruction, not the original translation unit.

Each recovered function links to a reverse record keyed by binary ID and original ELF
virtual address. Keep one record per function, not one enormous decompilation file. Use
explicit fixed-width types and review PS2 overflow, endianness, float precision,
alignment, pointer width, and R5900 ABI assumptions. Never treat a PS2 address as a
native pointer without an evidence-backed mapping.

Identifier naming is unchanged by the restructure: `Sub_<ADDR>` / `Type_<ADDR>` names
stay until a cross-reference pins a recovered identifier to that address. The 74
`Class::Method` and 23 pool-name strings the ELF carries are real original identifiers
but are `string_only` — a string's presence is not proof of which function it names.

## B: Platform abstraction

TASK-0039 extends frontend render packets with optional TL/TR/BR/BL positions, pixel UVs and
independent colours, submitted as RenderingServer polygons. Ordinary rectangle callers remain
valid. Font measurement stays advance-only, while draw consumes recovered pair slices; font
height is derived from glyphs. All shared layout/draw decisions remain native and Godot-free.
See FRONTEND_RENDER_AUDIT.md for coverage and unresolved original sampling/rounding.

TASK-0028 adds compiled class shells and unresolved member entries to `game_core`, with
explicit port-side signatures and throwing bodies. Unknown original paths use
the game_core root per the user's TASK-0029 instruction (superseding `port/class_templates`);
classes with confirmed paths use original directories. Existing class bodies/storage remain
in their current modules. See [CLASS_SOURCE_PATHS.md](CLASS_SOURCE_PATHS.md) for the evidence.
See [CLASS_TEMPLATES.md](CLASS_TEMPLATES.md) for coverage and preservation rules. These shells
do not create runtime state or establish original layouts/ABI and are not wired into gameplay.

Deleted in TASK-0018 (D008). `platform_api` / `src/platform/services.hpp` held
provisional scaffold contracts no recovered caller justified; nothing included
them for a symbol. What replaced them is narrower and real: `port::render::Backend`
and `port::render::Resources` for drawing, `port::assets::Source` for original-asset
reads. Core code depends on those, tests supply fakes without Godot, and the host
owns the concrete implementations and outlives all non-owning references. A new
subsystem introduces the contract its recovered callers require, nothing more.

`src/platform/ps2_asset_root.{hpp,cpp}` remains: it resolves the local ZZDATA root
and is Godot-free. The host supplies only the directory the search starts from.

## C: Godot integration

`ps2_port` is a shared GDExtension linking `game_core` and `godot-cpp`. Its
`PortCore` RefCounted wrapper exposes the sample function. Godot manages the
wrapper lifetime. Future adapters own Godot objects/resources and translate
between native core values and host representations. Godot owns windows,
rendering, input, and audio; recovered core logic controls gameplay decisions.

Dependency direction: Godot scene -> bridge -> core -> platform contracts.
Bridge also depends on godot-cpp and implements platform contracts as needed.
Neither core nor contracts depend on bridge. Core tests link only core/contracts.
No Godot physics delegation is allowed without a recorded, explicit decision.
No simulation thread is created; future scheduling must account for Godot's
main-thread APIs and preserve original timing behavior based on evidence.

## Resources and tools

Extraction/conversion utilities are offline programs under `tools/`. They do not
belong in core gameplay. Original/extracted proprietary content stays ignored.
Commit schemas, scripts, and synthetic fixtures only. `godot/assets/` may contain
reviewed redistributable host assets; raw game resources go in ignored extracted
directories. Reverse scripts should export reproducible metadata, never silently
change Ghidra analysis. Record tool versions and binary hashes with each export.

## Main-loop structure (TASK-0010)

EngineShell in game_core/engine owns the resumable frame orchestration projection. ModuleRegistry binds callsite/target handlers; FrameTrace records provenance and placeholder/error states. State and timing context is separate from host delta; waiting resumes only the original tail. MainLoopObserver is an additive RefCounted bridge and standalone scene. World nested connections remain metadata, preserving dynamic entity loops. See MAIN_LOOP_STRUCTURE.md for APIs and extension rules.

## Rendering: native frames, replaceable backend (TASK-0018 / D008, 2026-10-13)

The earlier checkpoint below is historical. The front end no longer renders through
GDScript. The dependency direction is now:

    game_core (recovered)  ->  port_runtime (Godot-free)  ->  render::Frame
                                                                  |
                                           Ps2RenderHost (RenderingServer) or any backend

`port_runtime` is a static library linking only `game_core`. It holds
`port::runtime::FrontEndRuntime` -- the front end itself: parsed screens and fonts,
the recovered menu group, localisation, the jukebox and sample bank, the recovered
animations, navigation history and the mode-select globals, all returning plain C++
values. It also holds `port::render` (the frame contract and `crop`),
`port::assets::Catalog` (original TexturePack entries decoded from the configured
ZZDATA copy) and `port::runtime::build_frame`, which turns the live authored screen
into a `render::Frame`: layer draw order, authored geometry and scale, the 0x80-is-opaque
colour bytes, per-glyph placement from the original bitmap fonts, the align bit,
and the original clip rectangles from `GetWidgetClip_00145F30`. It links no
backend and includes no Godot header, and `frame_builder_tests` exercises it
against a fake `Resources` and a fake `Backend`.

`Ps2RenderHost` is the Godot backend and the only place that names a device. It
owns the `RenderingServer` RIDs -- one canvas item parented to the host node's,
one 2D texture per original entry actually drawn -- and re-uploads a texture only
when its bitmap revision changes, which is what makes the per-frame movie surfaces
cheap. It takes `render::Frame` and makes no drawing decision. `LiveResources`
resolves an id to an original catalog entry or to a live movie surface
(`kIntroMovieTexture`, `kBackgroundMovieTexture`).

`Ps2FrontEndLayout` is now only a translation shim: it resolves the ZZDATA search
root (which needs ProjectSettings/OS), turns native results into Godot values, and
converts decoded PCM into AudioStreamWAV. It holds no game state, and its surface
is down to the thirteen methods the host actually calls -- no widget, font, colour
or measurement API survives. New host code must not grow it.

Widget measurement is native: sizes come from the original font tables and
TexturePack dimensions through `port::runtime::WidgetSizes`, which also supplies
the `hp2::frontend::WidgetMeasure` seam the recovered animation, viewport and clip
arithmetic consumes. `Ps2FrontEndLayout::set_widget_measurements` remains only for
the older scene and wins when that scene pushes measurements; see TASK-0018 for
the divergence this preserves.

`godot/scenes/frontend_native.tscn` is the project's main scene and the only host.
Its script owns the window, key-to-JOY_EVENT translation, frame delta, audio
hardware and process lifetime, and nothing else -- it never reads a widget,
position, colour or texture. One call per frame,
`Ps2RenderHost.draw(layout, intro, background)`, replaces the former per-widget
Dictionary traffic and `Control._draw`. The previous host
(`scenes/frontend.tscn` + `scripts/frontend.gd`) is deleted.

The View Event Tree screen is native too: `port::runtime::EventTreeView` holds the
open tree and the cursor, the runtime loads and clears it with screen 39, and
`append_event_tree` emits its panels, numbers and `tLineSegment` connectors. The
connectors are axis-aligned, so they are solid-colour quads (`Quad::texture == 0`),
which is the only primitive the frame has beyond a textured quad. Its camera
placement remains explicitly port-side calibration, not recovered.

Widget measurement has no host round trip at all: `FrontEndRuntime::measure_widget`
and the renderer both go through `WidgetSizes` over the original font tables and
TexturePack entries, so the recovered arithmetic and the renderer cannot disagree.

Still host-owned and not yet native: startup/movie/music orchestration and screen
selection.

## Historical: frontend integration checkpoint — TASK-0017 / 2026-10-13

Earlier scaffold descriptions are historical. Current ps2_port is one GDExtension
DLL registering six RefCounted wrappers: samplePortCore, two diagnostic observers,
texture loader, frontend layout/runtime adapter and movie host service. These are
subsystem adapters, not one GodotNode/bridge per native entity or UI widget.
Recovered game_core remains independent of Godot headers; no Godotphysics/vehicle
substitution exists. Core tests build without the optional Godot bridge.

The active frontend currently uses GDScript Control._process to drive native menu
and movie methods, then Control._draw to pull per-widget Dictionaries and draw
textures/glyphs, including hostUV clipping. RenderingServer references in scripts
are frame_post_draw waits for captures. There is no direct C++ RenderingServer
backend or native render-command submission layer. services.hpp::Rendering only
contains provisionalpresent(); it is not a completed RHI. Frontend orchestration
and native-object ownership remain split between GDScript and Ps2FrontEndLayout.
Godot also currently hosts window,input,audio and frame scheduling.

User proposed NativeGameCore→render abstraction→GodotRenderingServer. This is a
proposed target, not implemented architecture or authorization to silently rewrite
recovered functions. A concrete next design would move runtime orchestration into
a Godot-independent native runtime, produce native frame commands/snapshots, and
use a shared RID-owning Godot renderer behind a narrow host entry point. Keep
resource IDs/data/time units independent of Godot; preserve original tick/input
semantics and expose incomplete paths. Observers may stay separate debug wrappers.
Local pinned4.4.1 RenderingServer bindings expose canvas_item_create,
canvas_item_add_texture_rect_region,texture_2d_create/update,mesh_create,
instance_set_transform andfree_rid; this confirms available API, not current use.

## Port-side debug overlay (TASK-0031)

`game_core/extensions/debug_console` owns a bounded thread-safe incoming log queue,
csys command registration, native host snapshots, ImGui lifetime and triangle conversion.
It has no Godot dependency and does not represent recovered PS2 behavior. Ps2RenderHost
owns the font texture and clip-parent/triangle-child RIDs, clipboard and input conversion.
The frontend routes input there first and continues updating game state. Debug FPS shares the native ImGui frame anchored at the viewport's top right. See D010 and DEVELOPMENT.md.

## Debug viewer scaffolds (TASK-0032)

The persistent DebugTools CanvasLayer owns a separate Ps2RenderHost for console/FPS,
so scene changes do not destroy the debug session. Frontend and viewers route input to
it before their own handling. Commands enqueue native ViewerRequests; the overlay drains
one after drawing and dispatches scene transitions on a deferred call. Per-scene exit only
releases that scene's resources. Application exit releases the persistent host.

Godot-free `port::extensions::ViewerSession` owns viewer kind, requested relative source,
explicit unavailable-loader status, fixture flag, camera description and pending actions.
Optional csys registration is separate from that library; console-off Debug uses the same
state/actions through host bindings and buttons. Release rejects viewer actions and direct
scene entry. Synthetic Godot MeshInstances, camera controls and lighting live only in the
platform scene layer; they do not alter the recovered Frame/quad format or implement physics.
No original asset is opened, inspected, decoded or substituted by the fixtures.

## Native vehicle inspection (TASK-0033)

CarViewer now loads original vehicle BIN resources through Godot-free
`port::vehicle::Model`, the bounds-checked geometry reader, requested shared textures and
typed CarRenderInfo slices. Native values own parts, UVs, topology, static attachments and
LOD decisions; `Ps2CarViewer` owns only uploaded meshes, images and debug overlays.
This separate 3D presentation contract does not change frontend Frame/quad semantics.
Host code uses the single asset-root resolver; no proprietary pre-export is required.

The model exposes the complete original car/skin catalog and ID-based loading/rebinding,
with explicit lights-off, zero-steering/zero-spin inputs. The default shader projects original
material blocks, frontend lights and VU base RGBA; raw texture inspection is an explicit
debug override. Original whole-entry methods remain throwing. General GS alpha above128 requires
ordered destination access and fails explicitly; corpus evidence bounds current static
source-over alpha to127. The full GS material pipeline
and partial-frustum cone/clip behavior remain recovery work.

One DebugUiSession owns native ImGui context, atlas, input and batches. ViewerUi and
console draw into the same frame; scene/model actions are queued until frame completion.
Godot controls are not used by either viewer or FPS. Console-off Debug still runs the
viewer UI; Release disables it. See the TASK-0033 decision in DECISIONS.md.
Viewer load completion updates Loading to Loaded or Failed; failure removes stale content.
Clear/Fixture and scene destruction release native/Godot resources without discarding the
persistent debug session. See VEHICLE_RENDERING.md and D012.

## TASK-0033 compact viewer and loading ownership

ViewerUi remains a Godot-free snapshot/action model. DebugUiSession contains root windows
before NewFrame and repairs first-frame draw positions/hit rectangles before Render.
port::vehicle::Loader serializes file IO, parsing, skin binding and representative paint
previews on one native worker. Models are independent snapshots; ModelCache/TextureCache are
single-worker services. DirectorySource::probe uses filesystem metadata; injected Sources may
retain the read-based default for fixtures. Geometry decoding accepts an explicit solid-hash
filter before VIF work. The near viewer requests only body/tire level zero and static attachments.

Ps2CarViewer request_car/pump_load/set_show_lods provide the asynchronous scene boundary.
Tickets carry cancellation tokens; dropped futures cannot mutate scene objects. GPU upload
is cooperative, staged and hidden until commit. Godot resources never enter the worker.
Immutable meshes/images and shaders are reused; per-instance lighting/camera materials remain
separate, including LOD attachment copies. Renderer termination explicitly joins the worker.

## Shared vehicle rendering backend (TASK-0034)

Ps2VehicleRenderer owns GPU upload, material variants and native-selected part submission
for both future runtime consumers and CarViewer. Ps2CarViewer is now a thin debug request-policy wrapper.
The backend is not Debug-gated; debug scene access remains gated separately. Native Model
uses CarRenderInfo's bounded binding/LOD/pose and lighting-delegation slices. Untranslated
whole render entries continue to throw. Viewer preview rotation is port-side camera policy.

Shader authoring and specialization are centralized in src/platform/shaders. CMake embeds
those text sources into a generated header under the build directory and tracks shader-file
changes. Bridge code binds native parameters without owning inline GLSL. Existing GS domain,
alpha/depth and backend limitations are retained; no original reflection pipeline is invented.

TASK-0035 adds an original frontend environment pass to the shared vehicle renderer. Native
address helpers publish tire/blur matrices, strength and environment orientation/ST math.
The host uploads CARSELECTENVMAP and the second original material block as an additive
next pass with depth writes disabled. It does not use a Godot environment/PBR substitute.
Viewer wheel speed is explicit port input in extensions/UI and model orchestration;
recovered math remains Godot-free. Runtime game environment capture is an explicit gap.

TASK-0036: a native vehicle part can own multiple Godot ArrayMesh batches. Preserve one
part transform/visibility root; additional MeshInstance3D children inherit it. Draw records
keep original part/strip identity plus host batch/local surface coordinates. The GPU cache
stores complete ordered batch vectors. Bounds, material/reflection uniforms, wireframe,
LOD copies and cleanup include every child. No native resource or recovered logic changes.

## Driving laboratory (TASK-0038)

CarViewer embeds Test drive as a mode in its existing scene and ImGui panel, using
port::driving::Session (Godot-free tool state),
Ps2DrivingLab (boundary/copy adapter) and the common Ps2VehicleRenderer absolute-pose entry.
One renderer/car selection/camera survives mode changes; returning restores preview
bindings through the cached loader. No additional simulation function-key bindings exist.
NativeBackend now owns an SHA-bound logical ELF/data image and the recovered component
construction/update/force/integration graph. It commits a complete validated next state or
throws, retaining scheduler/RNG state on failure. Original config rows feed the native
algorithms; an explicit synthetic horizontal-plane provider supplies query geometry and
original material1. No Godot physics is used. Session records both inputs and elapsed clock
values; render pacing respects the recovered scheduler minimum. See DRIVING_LAB.md and
D015 continuations for the supported Race4 fixture and unverified equivalence limits.


TASK-0038 third-person redesign replaces the earlier manual debug workflow. The embedded
controller separates Starting/Driving/Suspended/Error; only complete successful native
initialization activates driving. Focus/F8 suspension resumes automatically and discards
the first resumed frame's elapsed time. Keyboard capture zeroes controls rather than
permanently pausing. Pointer intent/capture and camera release are separate from Session
pause. The rear/orbit/recenter camera is an explicit tool with confirmed body+Z host forward;
physics and original wheel render-basis contracts are unchanged. The mode UI exposes only
return/hints/error; Session instrumentation remains test-side. See DRIVING_LAB.md.


## TASK-0042 source ownership and names

Debug-reference call chains confirm additional retail symbol names; exact ECOFF
markers establish CPP ownership and selected declaration headers. See
[DEBUG_CALLCHAIN_RECOVERY.md](DEBUG_CALLCHAIN_RECOVERY.md) and the durable TASK-0042
class/projection registries. Class shells use established declaring scopes;
shared source files merge class implementations without inferring inheritance.
Typed driving projections retain port namespaces and shared port interfaces;
unknown owners remain in port/driving/unresolved_entries.cpp. No Debug ABI or
behavior is transplanted, and whole-entry gaps continue to throw.


## TASK-0043 class mapping interfaces

Confirmed class methods now have whole-entry gap interfaces independently of typed host projections. Static port carriers preserve distinct original declaring scopes when flattened host signatures would collide. They copy neither native object storage nor lifetime semantics. See CLASS_MAPPING_COMPLETION.md.


## TASK-0044: Debug driving has one canonical implementation

The user-selected driving baseline is Debug NFSHP2.ELF SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
This explicitly supersedes the earlier Retail-only driving ownership paragraphs above;
unrelated systems and historical Retail records keep their original version identity.
CarViewer -> Session/NativeBackend -> hp2::recovered::PhysicsCar::Move is the production
chain. Original component algorithms now reside in their evidenced original class/source;
legacy projections forward only. ObjectView is explicit host storage binding, not a PS2 ABI
claim. Debug vtable dispatch, per-operand global mapping and SHA-bound data loading prevent
Retail/Debug address mixing. Synthetic plane, raw input, fixture startup, scheduling services,
snapshot validation, camera and display remain port boundaries. Whole-game World/RealDriver,
track scene and excluded effects/damage paths remain untranslated and throw when reached.
See [DEBUG_DRIVING_AUDIT.md](DEBUG_DRIVING_AUDIT.md) for the complete ledger and limits.


## TASK-0045: surface input and read-only grip publication

Test geometry now resolves supported original terrain IDs at each wheel's native world
query point. Uniform/five-zone settings enter the same Debug PhysicsCar/Wheel chain; no
physics implementation exists in the surface module or Godot. SurfaceSettings are per-step
inputs recorded with Session controls/time. The successful candidate snapshot publishes its
committed settings, SHA-bound material catalogue and stored Wheel/TireInfo diagnostics.
Presentation consumes shared boundaries/IDs/colors (nativeX to hostZ), not separate collision
logic. Unknown units remain raw. The captured-input route still reserves motion for the
camera, but forwards mouse-button releases to ImGui so a world recapture cannot leave its
button held. See DRIVING_GRIP.md and reverse/task0045 for audit and provenance.

## TASK-0046: player input and skid rendering

Driving actions are centralized in project.godot InputMap and driving_input.gd.
The adapter owns continuous device states and queued edge requests; a completed
native step acknowledges one queued request. Session accepts a complete semantic
frame, so zero input cancels pending edges during keyboard capture. Preview and
frontend input contexts are outside this driving-only migration.

NativeBackend supplies fixture state and platform services, calls original
RealDriver input/mode handlers and Drive, then PhysicsCar::Move. It no longer
writes player control outputs or supplies a fixed steering angle. Required player
burnout logic belongs to PhysicsCar. Read-only powertrain fields are published
with the same completed physics snapshot. Full World/Player startup is not implied.

Original Wheel/SkidMaker/SkidSet/SkidSegment code owns eligibility, geometry and
lifetimes; RenderSkids/ePoly owns visibility, intensity and draw-command generation.
The port provides a bounded logical pool, one synthetic clan, elevation service,
original-resource lookup and a Godot upload boundary. RenderSkids runs per view,
separately from physics, and commits its original LRU mutations transactionally.
The renderer does not calculate tire stripes or modify physics. See PLAYER_DRIVING.md.


### TASK-0051 shared track frame and renderer

Track ownership is port::track::Runtime -> native eDisplayFrame/scenery/eModel
commands -> non-Debug Ps2TrackRenderer. Ps2TrackViewer derives from the common
host and only restricts requests in Release. Independent race_scene uses that
same host and the common vehicle/driving runtime. Per-command matrices, local
light contexts, LOD, texture/material/pass and sorting/depth state remain native
values until GPU adaptation. Immutable scene resources outlive frame packets;
unload joins any owned load worker before deleting GPU resources and frame caches.
GPU nodes cache by instance/solid/surface and visibility changes per prepared frame;
normal rendering never reads debug collision overlays. No Godot physics is added.
See docs/TRACK_VIEWER.md and reverse/task0051 for scope and explicit capture gaps.


### TASK-0052 shared visual stages (in progress)

The same `port::track::Runtime` prepares native main and environment-view packets.
Original `CalculateH` consumes the ELF's global screen width (512), independently
of the desktop viewport. Capture camera raster projection accounts for the actual
64-pixel target; the original camera field remains the LOD input. Immutable resource
indices and queue ranks replace repeated linear searches without changing native
command order. Conservative host batches merge only adjacent commands with equal
matrix, lighting, material and complete effect state; environment passes remain
separate and ordered. Batch meshes retain source vertex attributes and indices.

`Ps2TrackRenderer` owns the common sky, scheduled reflection targets/atlas, vehicle
shadow submission, Sun occlusion transfer and postprocess adapters. Postprocess uses
an explicit depth-writing proxy view because Godot Compatibility's depth texture
excludes the transparent queue used to preserve original GS order. Cached variants
retain each source shader's alpha test, projection and depth-write decision. Main
color/history and native depth-to-alpha scratch are distinct resources. Sun consumes
the previously completed 32x32 transfer before requesting the next one. These are
bounded host adaptations; destination-alpha car reflections, per-draw PSMCT16
precision and other recorded gaps still prevent a complete-equivalence claim.

Settings are process-session permission gates shared by browsing, Test drive and
the independent race scene. Original activation conditions decide whether an
allowed effect actually runs. Release retains original defaults without ImGui.
Host resources are released during tree exit, before Godot deletes owned children.
TASK-0052 remains active; current Debug build is promoted for the requested bounded build delivery.


## TASK-0055: host submission policy

The shared Ps2TrackRenderer accepts immutable-before-load host optimization options.
Native runtime/frame packets, original section/LOD/effect scheduling and physics
remain independent of Godot. The optimized candidate retains distinct material identity, avoids
unchanged parameter/batch/sort submissions and retains particle ArrayMesh buffers.
TrackPostprocess receives explicit geometry/material invalidation and a closed
vehicle BASE-uniform writer contract; unknown materials retain full polling.
Baseline and candidate use the same renderer, including race/Test drive/viewer.

Contiguous MultiMesh and exact submitted-triangle occluders are separate experimental
policies. MultiMesh failed exact Track24 color parity and is excluded from the
optimized profile. Opaque queue and occlusion stay off without their independent
visual/performance gates. No original far clip, LOD or effect condition is reduced.


TASK0055 material sharing was removed after actual Track11 capture-atlas changes
with identical native inputs. Other caches do not alias material objects. The
read-only render-target diagnostics are available in Release and do not alter
effect permissions. Exact controlled1080p native-driving-pose/rotated-view
comparisons include actual car reflection binding and all intermediate targets.
Independent-process car-reflection raster repeatability remains a separate
existing uncertainty; no pixel tolerance or speculative shader fix is used.


TASK-0055 acceptance completed:30 tracks,120 route-warmup/600 measured frames,
3 repeats in both frozen54/final55 Release1080p runs. Native packet/pose, original
effect permissions and draw-call distributions match. Median-route improvements:
wall10.03%, CPU prepare27.62%, host submission33.16%; no persistent>5% wall median/
P95/P99 regression. GPU timers unavailable; loading stalls remain. The accepted
resource/particle-reuse profile is now the common project default. Distinct
material identity retained; MultiMesh/occlusion/opaque remain experimental/off.
See [performance report](PERFORMANCE_TASK0055.md). Override at launch with
--track-optimization-profile=baseline. No original-console Verified promotion.

## TASK-0056 optional Godot HLOD policy

The shared renderer can consume port::track::StaticLodScene and
Runtime::prepare_hlod_frame instead of original projected-size selection. The core
emits all original resource slots for original candidate sections; engine visibility
ranges perform final LOD/frustum decisions. Main, depth and capture views share
range/anchor policy and common resource ownership. Original mode remains default.
See GODOT_HLOD.md for contracts, source evidence and explicit boundary differences.

## TASK-0058 supersedes host HLOD

User revoked Godot-managed LOD. Only original production section/tree/instance
selection is accepted; main and independent capture views share the native runtime.
Show LOD remains debug-only. Prior TASK0056 sections are historical. No generated
low meshes or physics changes. See [native visibility integration](NATIVE_TRACK_CULLING.md)
and E0058R01 for evidence, tests, counters and verification limits.

## TASK-0059 visual recovery boundary

The port validates particle alpha per active atlas UV footprint and isolates only
its explicit NotImplemented packet failure. Unimplemented high-alpha quads are
reported/omitted, not normalized or silently marked recovered; base scenery and
supported packets remain alive. Emitted node/stream/alpha ranks compact together.
Immutable alpha bounds cache clears on unload. No recovered simulation/culling or
physics body changed. Evidence E0059P01; high-alpha compositor remains future work.


## TASK-0072: port-side CPU render preparation

Recovered serial commands feed a standard-C++ payload builder under platform/render;
Godot owns native group scheduling and ordered resource commits. No WorkerThreadPool
or engine types enter game_core. Same-view joins retain producer/consumer timing.
See [integration contract](RENDER_PREPARATION_TASK0072.md).


TASK0072 submission workers consume immutable selected-command snapshots through
`platform/render/preparation_work.hpp` and `submit_math.hpp`; they cannot call
Runtime, Godot APIs or mutate caches. The bridge owns main-thread snapshots,
per-view joins and serial state comparisons/commits. Depth synchronization uses
port-side version/dirty policy (`depth_sync.hpp`) with main-thread scene/resource
observers. This seam does not add a recovered PS2 scheduler or replace sector
selection. See RENDER_PREPARATION_TASK0072.md for failure/lifecycle boundaries.

## TASK-0082 vehicle feedback transport

Native engine/horn state and original sample decoding remain Godot independent.
Ps2DrivingRuntime owns cached sample streams and updates the bounded mixer once
per committed frame. driving_audio.gd transports the13 native voices in both lab
and race scenes. Pause stops host voices; restart resets the mixer; resuming can
restart sample phase. This host lifecycle policy is not original SPU emulation.
Lamp material changes consume native light selectors and original ON/OFF textures.
No replacement physics or fabricated projected-light geometry was introduced.

## TASK-0083 collision and tire audio ownership

StaticCollisions retains original collision-state projections beside its witnesses.
NativeBackend copies their state, collision selector and tire mixer with candidate
Memory and only publishes committed audio. Impact requests are buffered and drained
once by the host; scrape voices use stable contact IDs plus native stepped fade;
tire slots use original state and samples. driving_audio.gd owns only stream start,
stop, gain, pitch and cleanup in both driving scenes. The own-car listener binding
and one sound update per committed frame are explicit port-side services. Full
original world/SPU/spatial cadence and omitted effects remain unbound.

## TASK-0084 Test drive camera ownership

DrivingCamera retains the recovered default exterior pose history and accepts
original car state, elapsed time, lookback and optional original topology/shape
services. Ps2DrivingRuntime updates a candidate camera against a private Memory
copy so camera math cannot advance physics or publish partial state. The original
row0 CarTypeInfo fallback is bound from the configured original catalogue when
required. Native pose coordinates map through the existing source-to-host axis seam.

driving_camera.gd owns only Godot default projection and the requested temporary
mouse orbit. driving_lab.gd owns input/lifecycle and consumes the native position,
target and up vector. V changes original lookback rather than renderer transforms.
Idle return reaches the live native frame; viewer pose/projection are restored on
exit. This changes Test drive only; race_scene's separate camera remains scoped
future work. See reverse/task0084 for original camera services still unbound.


## TASK-0085/86 particle presentation

ParticleSimulation owns original emission/RNG and the shared allocation ledger.
The optional GPU backend consumes ordered committed birth/advance/remove batches
using custom shaders; native CPU integration/rendering remains the default.
Host commit/abort isolates failed frames; capture faces do not advance simulation.
Fixed vehicle light sprites use the same presentation layer's persistent attached
billboards, independent of GPU simulation. VehicleFlareState is a Godot-free
snapshot channel; Model retains original markers and textures. Runtime Debug
resources supply original parameters. See PERFORMANCE_TASK0085.md and
VEHICLE_LIGHT_FLARES.md for tested boundaries and the user-waived timing matrix.

## TASK-0096/0098 version-isolated career foundation

hp2::debug EventTree/CarTypeInfo/FrontendPlayerInfo/VisibleCarSet/CarSelector
hold decoded host state for the authorized Debug career behavior. Existing Retail
frontend/profile storage remains independent. Explicit port services bind assets,
translation, original unlock checks, popup submission and continuation; absent
required services throw. These types do not assert original object ABI or allocator
lifetime. Native selection is not yet bound to the production event entry.

VisibleCar ownership is allocated once at selection entry, with stable pointee
identity through original adjacent sorting. Per-input neighbor lookup is constant
time via the original CarIndex and bounded vector storage; no per-frame list copy
or Dictionary publication. Renderer/model cache/async ownership remains future
TASK-0098 work. Debug24-byte and Retail28-byte language records are mapped by
reviewed identities/IDs, never overlaid. See reverse/task0098/selection-core.md.

TASK-0098 continuation: TickServices binds original selector loading/palette/
lighting/effect dependencies to the native001947D0 ordering, without owning a
synthetic race or suppressing missing services. LockedDisplay uses stable native
VisibleCar identity, original spline/timer state and required localized rectangle
submission. Its reward tables decode from SHA-bound Debug memory. Original
static camera-offset stores run once per fresh selection image, separately from
ELF mapping. Constructor/destructor and production lifecycle remain incomplete.

Frontend Frame packets now retain stage1/7 quad ranges alongside the unchanged
flat vector. Concatenation rebases them and coverage validation detects legacy
unclassified packets. Existing canvas submission remains flat. Original selector
stages0/2/4 require a reviewed compositor, separate mirrored-car passes and shared
depth; stage metadata alone does not supply that implementation.

TASK-0098 effects continuation: ParticleSimulation now has an explicit native
frontend branch that rebuilds mutable Hermite bases after auto-emission and before
integration. Its ordinary world/driving branch retains the previous cache path.
SharedParticleState carries the global RNG and1000/400 particle/group accounting;
step_shared publishes a single candidate and ledger only after success. Larger
World owners must clone all participating states together and still supply shared
dynamic-trigger/global-definition ownership. The selector nebula writes shared
ColourMatrix through typed emitter handles; render-before-step timing is retained
as a required caller contract. Production selector ownership remains unbound.

Fresh E-0098-F03 supersedes the earlier compositor shorthand: stages2/4 flush
solids while preserving pending alpha for stage5, and reflected cars suppress
environment-map/sorted-alpha strips with flags0x11. The backend requires that
shared alpha/depth/target domain before formal selection can be enabled.
