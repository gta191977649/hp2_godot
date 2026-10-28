## TASK-0098 - Port selection lifetime ownership (2026-10-27)

SelectionPage uses required cancellation/resource leases for host model, renderer,
camera, loader and input registrations. Explicit close preserves the recovered
single-player normal exit sequence, including duplicate preload and whole
FrameEffects group kill. Abnormal destruction/construction rollback only releases
owned resources and restores cached groups. Asynchronous model jobs must not retain a page
pointer; referenced services outlive the owner. This port exception/lifetime policy
adds no original-game behavior or GPU readiness claim. Actual production leases
and the shared colour/depth compositor remain required before formal entry.

## TASK-0096 - Debug single-player Ultimate Racer authority (2026-10-26)

The user explicitly extends Debug NFSHP2.ELF SHA815b60c7...332f1b behavioral authority
from Test drive to the complete single-player Ultimate Racer closure, including
necessary frontend/selection, World/race/AI/police, outcomes and progression.
Retail assets/profile/frontend remain separately identified, with per-field and
per-ID semantic adaptation. Symbol availability is not proof of Retail equivalence.
Actual races are authorized in TASK-0096; TASK-0091's historical no-race scope is
not silently rewritten. Target current machine1920x1080 Vulkan/Forward+60FPS.
Original logic/RNG/timing/object counts remain native; Godot GPU/lighting/instancing
adapters require behavior and render-order equivalence gates. Preserve existing
uncommitted work. Unknown branches and missing original comparisons remain explicit.

## TASK-0095 - Host frontend gamepad ports (2026-10-25)

The first two devices in Godot Input.get_connected_joypads map to original ports0/1.
Keyboard remains port0. Host face buttons A/B/X/Y map to original menu Select/
Back/Square/Cancel; shoulders, dpad, Start and Back buttons forward their original
JOY_EVENT identifiers. Left-stick threshold crossings use an explicit host0.5
dead zone. This is a device adapter, not recovered PS2 calibration/repeat behavior.
Original pad gates remain in portable C++ screen handlers. Physical two-pad testing
and full trigger/repeat/calibration service remain open; native NameEntry verifies
both original player gates and profile separation.

## TASK-0094 - Host display calibration translation (2026-10-25)

Retail001F4B20 stores signed GS display offsets. The host retains these raw values
in the private profile and requests a canvas translation after logical clipping:
X/4 logical pixels and Y logical pixels, scaled with the output viewport. This is
an explicit host display-device policy (four horizontal GS clocks per host pixel),
not verified GS timing equivalence. It leaves authored widget coordinates intact.
Native tests assert the request, not PS2 visual equivalence.

# Decision log

## TASK-0092/0093 - Frontend audio and private Autosave device

Recovered menu state, timing, audio selection/filtering, volume settings and popup
text remain native. The existing private filesystem profile store supplies the
successful available-device path at00245B00. Autosave still enters its authored
screen/popup, consumes input, observes the original strict timing gates and pops
history on completion. No PS2 card selection/capacity/format/overwrite emulation is
claimed. The configured read profile is never a write destination.

The Godot frontend owns a private audio bus. Mono requests set
AudioEffectStereoEnhance.pan_pullout=0 for live downmix; Stereo and original
Pro Logic II requests retain the stereo carrier. No invented surround DSP is
substituted for the unrecovered SPU encoder. Speaker choice persists and Mono
has a live host consumer, but this does not establish PS2 hardware equivalence.
Host API: [Godot AudioEffectStereoEnhance](https://docs.godotengine.org/en/stable/classes/class_audioeffectstereoenhance.html).
Original option IDs and state come exclusively from Retail MCP/original FE data.

## TASK-0081 - Bind recovered frame slices without pretending full route recovery

Use exact original motion/distance bookkeeping and ResetCar leaf in the existing
Memory transaction. Keep explicit port-side slice entry points and throwing full
Car methods until topology/path/Player ownership exists. A deferred reset request
must fail visibly rather than be cleared, ignored or mapped to host session restart.
Original route selection depends on wrapped track progress; available route bytes
do not authorize nearest-XY or spawn substitutes. Recover that dependency next.

## TASK-0075: original wheel effects through one shared AcidFX seam

Recover Debug wheel smoke/spew separately from ground skid marks. Preserve
original terrain lists, static configuration wheel positions, negative intensity,
RNG/pool limits and World update-before-emission order. Effects cannot step or
change recovered forces. The one-car port fixture supplies closest-player
distance0 and the existing outside camera/all-view car mask; other original
view/ownership branches remain explicit gaps. Track effects share native depth
and WorldEffects; the synthetic flat fixture uses host depth with the same original
particle packet. Clear copied emitters/groups on detach while preserving static
effects and RNG. TrackViewer browsing uses Godot defaults75/KEEP_HEIGHT, with
CarViewer/drive/race projection policies retained. See DRIVING_EFFECTS.md.

## TASK-0073 - Automatic recovery accounting (2026-10-20)

Keep evidence statuses authoritative and immutable to progress tooling. Join existing
class/member and Debug-owner registries with SHA/address-bound source definitions;
report shells, direct gaps, unconfirmed bodies and recorded implementation claims
separately. Unknown method totals stay unknown, and shared targets count once per
binary globally. Lexical source presence is not whole-entry or original equivalence.

Save watching is an explicitly started local process; build startup performs an offline
refresh. Read-only MCP directory checks use private complete caches, including oversized
output pagination, and never alter Ghidra or promote recovery status. When MCP omits a
SHA-256, require its import path to equal the configured original ELF and hash that file;
this binds file provenance but does not certify database memory. See [usage](AUTO_PROGRESS.md).

## TASK-0071 - Vulkan Forward+ default (2026-10-19)

The user supersedes TASK0069's Compatibility default: select Windows Vulkan +
Forward+, reject automatic D3D12/OpenGL fallback, preserve explicit diagnostic
backend launches. Keep4.7.2 and the existing godot-cpp pin. FPS improvement is
measured on Track11 at1080p without reducing original effects, visibility or LOD.
Recovered core/physics and original command ordering remain independent of Godot.

Host offscreen HDR transport retains numeric GS RGB through blending and resolves
to the LDR canvas. It is not complete GS framebuffer emulation: per-primitive
COLCLAMP and recorded cross-backend pixel differences remain acceptance gaps.
Do not promote Verified or equate a faster backend with original equivalence.
The source viewport's empty3D pass may be suppressed only behind an independent
switch with exact ownership restoration, pixel checks and paired timing evidence.
See [migration evidence and limits](PERFORMANCE_TASK0071.md).

## TASK-0070 - Global native ImGui profiler (2026-10-19)

User explicitly requested the global profile_debug.gd name, realtime function
tracking throughout the game and native ImGui drawing. Reuse the existing ImGui
frame/input/DPI backend instead of a parallel Control UI. Keep a bounded240-frame
display history separate from bounded JSON recording. Unknown GPU values remain
gaps/N/A; native/GPU totals are not inferred. Scoped script/native-boundary wall
timing is labelled inclusive and does not pretend to be a full native call graph.

## TASK-0069 - Official Godot4.7.2 and opt-in profiling (2026-10-19)

The user explicitly superseded the4.4.1 runtime constraint and requested direct
upgrade, without retaining an old-engine comparison workflow. Default build,
test and launch tools now select official4.7.2.stable.ed1daf0bf. Download:
[official archive](https://godotengine.org/download/archive/4.7.2-stable/).
Keep Compatibility and the existing godot-cpp minimum4.4 ABI; actual4.7.2 host,
GPU-layout, particle, Sun, composite and lifecycle tests support that choice.

Display the active rendering API in the window title. Add opt-in Godot
Performance monitors/viewport timing and bounded profiling, with unknown/disabled
GPU measurements explicitly unavailable. Normal standalone runs incur no enabled
collector. Preserve original gameplay/render semantics. Port CPU caches from
TASK0068 pass same-engine exact native/image fallback checks; independent switches
remain. GPU synchronization and transparency are priorities, not a promised FPS
gain. Existing Sun reduction does not materially eliminate the wait and stays
off. See [observations and limitations](PERFORMANCE_TASK0069.md).

## TASK-0063 - Static render cost inspection (2026-10-19)

User replaced long FPS benchmarking with static/native tests and drawcall census.
Enable mutable-only particle transactions, cached particle transport layout/
parameters and invariant postprocess texture parameters after bounded exact
state/buffer/image parity. No FPS claim follows. Keep Sun reduction/opaque queue
experimental off, encoded depth/history and native culling unchanged. See
[results and limits](PERFORMANCE_TASK0063.md).


## TASK-0052 - Shared visual frames and original resolution for LOD (2026-10-18)

Main and scheduled reflection views submit through the same native Runtime and
Ps2TrackRenderer implementation. Godot owns GPU resources, view targets and the
presentation conversion; original palette, weather, culling, LOD, Sun/shadow and
particle calculations stay in the original declaring TUs or explicit port carriers.
Permission gates are shared for the process session and do not create an effect
whose original activation condition is false. Release uses original default gates.

Use recovered CalculateH with the original ScreenWidth512 for native pixel-size
decisions, independent of desktop output resolution. Do not shorten original far
distance or disable effects to meet performance targets. Preserve command order;
contiguous batching is allowed only with an explicit state/primitive equivalence
check. For the tested Godot Compatibility backend, transparent roads are absent
from hint_depth_texture; matching source-mesh depth variants supply the native
depth-alpha pass. This is a host adapter, not replacement gameplay physics.
Per-draw GS precision and remaining original paths stay explicit gaps until closed.

## TASK-0041 - Authorized debug reference naming (2026-10-17)

The user supplied a symbol-rich PS2 ELF and MAP as reference evidence. Record both
binary hashes and separate addresses. Adopt retail names only with raw exact code,
independently constrained virtual-table correspondence, or reviewed caller/code
bridges. Keep normalized-only matches as candidates. Reference layouts and signatures
do not replace retail behavior or native projection interfaces. Correct allocator
label versus exact C++ spelling explicitly. Read DEBUG_REFERENCE_SYMBOLS.md for
criteria, changed-layout counterevidence and reproduction. No Verified promotion.


## TASK-0039 - Preserve frontend packet corners and distinct text paths (2026-10-17)

Extend the port-side render contract with optional four positions, UVs and colours. The host
submits this data directly; it does not reconstruct layout. Retain ordinary rectangle callers.
Original clipping changes UVs/positions but leaves corner colours unchanged. Keep advance-only
measurement separate from kerned draw and preserve the helper path's scale-squared glyph width
and unscaled button advance, even though these differ from conventional typography. Nonzero
rotated authored quads throw until original matrix/trig is translated. Fractional destination
sizes remain inherited port projection pending FCR31 and GS/host sampling comparison. This is
a presentation adapter decision, not a claim that Godot rasterization equals the GS.

## TASK-0029 — Root defaults and confirmed source directories (2026-10-13)

The user explicitly superseded TASK-0028's unknown-path directory. All former standalone
template pairs now live at the game_core root unless class-specific read-only Ghidra evidence
confirms an original directory. Existing module classes stay in place. Exact original source
filenames remain in the evidence ledger; generated per-class basenames/header extensions are
port-side partitions inside the confirmed directory. Proximity, shared virtual targets and
generic inline-header strings do not move an otherwise unknown class. No middleware root is
invented for the recovered `src\bFile.cpp` path. See CLASS_SOURCE_PATHS.md, E-0029-S01/P01..P09.

## TASK-0028 — Editable class shells with explicit unknown ABI (2026-10-13)

Use confirmed class spellings and address-anchored unknown member names. New entry templates
have explicitly port-side void/no-argument signatures and throw at the original address.
Unknown paths use `port/class_templates`, while existing module placements and bodies are
preserved. Labels that cannot be C++ identifiers remain a verbatim catalogue, per user choice.
Class allocation sizes do not define native layout; shared vtable entries do not define
inheritance. Existing partial helpers may coexist with a throwing full-entry template, with
both linked in the inventory. Generator runs append missing code and never replace edited
bodies or existing records. No behaviour status is promoted by template contract tests.

> Numbering note (TASK-0021): this file carries **two** entries numbered D008 — the one below,
> from TASK-0016, and "One render contract" from TASK-0018 further down. Neither is renumbered,
> because both are referenced elsewhere by number. The next free number is D009.

## D009 — The profile is an opaque block, and saves are written host-side (TASK-0021, 2026-10-13)

The 0x114E0-byte player profile at ELF 0x002FC050 is modelled as an opaque byte array with
named accessors, not as a struct. Only an offset some function was observed to read gets a
name; 0x10E30 of its bytes have no reading instruction behind them in this session's search and
are left unidentified rather than filled in with plausible fields. Adding a field to
`Profile_002FC050` requires citing the instruction that reads it. The reason is the same one
behind AGENT.md R1: a struct declares that its layout is known, and a reader cannot tell a
recovered field from a guessed one once both are members.

This is cheap to hold to because the save path has no serialiser at either end. `FUN_00245D30`
copies a loaded payload onto the profile verbatim and `FUN_00245738` returns the same address as
the store source, so the on-card byte order **is** the in-memory layout and the port can be
faithful byte-for-byte while understanding almost none of it.

The recovered half (`src/game_core/memorycard/`) touches no file and no card: it takes and
returns `std::vector<std::uint8_t>`. The PS2 driver sequencing in the same original translation
unit — `FUN_0012C938` and the `sceMc*` wrappers below it — is not translated at all, because a
host file is not a memory card and reproducing the driver would be port-side invention wearing a
recovered function's name.

Save **writing** is port-side by construction and is bounded deliberately. The port writes
`<save root>/BASLUS-20362HP2/settings.db` — the recovered directory and file names, the
recovered 0x114E0 layout, the recovered size word and checksum — under a host save root
(`HP2_PORT_SAVE_ROOT`, then a `port_save_root` config key, then a per-user location). It never
writes to a memory card, never writes inside the player's ZZDATA tree, and never overwrites a
configured read-only `profile_path`; that last collision is checked and refused rather than
trusted not to happen. Save **reading** stays opt-in and off by default, so with nothing
configured the runtime gets a zeroed profile — which is exactly what `FUN_00245700` hands the
original's own driver before a load, so the front end's behaviour is unchanged by the save
system existing.

Whether a real PS2 or PCSX2 accepts a file this port writes is untested and is TASK-0023.

## D008 — Platform renderer defaults and host movie codec (TASK-0016)

Current engine pin is Godot 4.4.1 with godot-cpp e4b7c25e721ce3435a029087e3917a30aa73f06b
(godot-4.4.1-stable), superseding D002 for current builds. Forward+ uses Windows DX12,
macOS Metal, LinuxBSD Vulkan. Only Windows is validated here. Native Metal requires
Apple Silicon with this engine; Intel uses its MoltenVK fallback.
Sources: [official archive](https://godotengine.org/download/archive/4.4.1-stable/) and
[platform settings](https://docs.godotengine.org/en/4.4/classes/class_projectsettings.html).

Shared LGPL FFmpeg 8.1 MV30 is an explicit port CPU codec, consuming original AVI packets
in-process without asset conversion. Original codec/GS equivalence is unverified. DLLs
stay ignored dependencies; packaging must retain license/source information and allow
replacement of dynamically linked libraries. Port adapter source files enable exceptions
locally to catch NotImplemented without throwing through Godot callbacks.

## D001 — Native core with optional host (2026-10-09)

Use C++17, CMake >=3.24, CTest, and an opt-in GDExtension. Native builds require no
network or Godot dependency. This keeps reverse work testable and isolates the
platform boundary. Tests use explicit failures rather than disabled release asserts.

## D002 — Reproducible initial Godot baseline (2026-10-09)

Target Godot **4.3.stable**, standard Windows x86-64 edition, and godot-cpp
`godot-4.3-stable` at **fbbf9ec4efd8f1055d00edb8d926eef8ba4c2cce**.
Pin the Git submodule to the exact commit. This intentionally conservative
matching release pair is an initialization baseline, not a claim to be latest.
Upgrade engine and bindings together in a separate task with repeat smoke tests.
Sources: [Godot 4.3 download](https://godotengine.org/download/archive/4.3-stable/),
[binding release](https://github.com/godotengine/godot-cpp/releases/tag/godot-4.3-stable).
Dependencies retain their upstream licenses. Do not vendor game code or assets.

## D003 — Explicit provenance and evidence (2026-10-09)

Use a lightweight CSV index plus per-function and per-structure Markdown records.
Binary IDs derive from complete SHA-256 hashes. Status measures workflow progress;
confidence measures analysis certainty. Verified requires original-behavior
comparison evidence for a stated scope. Examples use symbolic dummy tokens,
never plausible numeric addresses mixed into the real index.

## D004 — Local tasks before remote tracking (2026-10-09)

Repository tasks are the durable source of session state. Mirror their ID,
acceptance criteria, dependencies, and links into GitHub Issues once a remote is
configured. No GitHub remote, issue, or initial commit is created automatically.
Future issue creation is a scoped publishing action, not required for local setup.

## D005 — Explicit police storage views (TASK-0009, 2026-10-11)

Use byte arrays and explicit little-endian 32-bit address words for partial police class recovery. Native view classes carry no claimed original vtable/inheritance. Opaque external 0x00231258 behavior remains an adapter contract. Two local bodies enter the native core with PC tests, no Verified promotion; exceptional PS2 float/original execution comparison remain unresolved.

## D006 — Structural shell with explicit observation mode (TASK-0010, 2026-10-12)

Project the observed infinite PS2 loop into one resumable native frame operation. Preserve callsite identity, dynamic branch reads, saved caller operands and signed boundary-counter comparison. A host event replaces busy polling; this is a port design choice, not original interrupt timing. Observation uses labeled fixtures; Strict stops on missing/failed contracts. Host delta never supplies the game-step value implicitly. Preserve old demo/APIs; use a separate observer scene, no simulation thread or Godot physics delegation.


## D007 — Front-end host projection (TASK-0015, 2026-10-12)

Godot frame delta supplies elapsed seconds to the recovered pulse and zero-velocity exit
path; PS2 counter/time-source production is not reconstructed. Font/texture dimensions cross
an explicit measurement seam. All animation arithmetic remains in native core; no Godot Tween
is used. Screen requests are applied after callbacks return to preserve menu-list lifetime.
Host stores authored widget copies for re-entry, a bounded history projection, and class names
separately from document hashes. Missing screen logic raises NotImplemented. The inherited
std::sin pulse implementation does not reproduce original sine-table quantization and remains
an explicit equivalence limitation pending original comparisons and table recovery.

## D008 — One render contract: native frames, replaceable backend (TASK-0018, 2026-10-13)

`port::render` is the project's only rendering contract. The game side produces a
typed `render::Frame` (quads in the original 640x480 authored space, original clip
rectangles already applied by `render::crop`) and hands it to a `render::Backend`
together with a `render::Resources` texture provider. The backend owns every
device object; nothing device-specific reaches the frame.

`src/platform/services.hpp` and the `platform_api` CMake target are deleted
(TASK-0018, 2026-10-13). They held provisional scaffold contracts -- `Rendering`,
`Audio`, `Input`, `FileSystem`, `Timing`, `Diagnostics` -- that no recovered caller
ever justified and that nothing in the tree included for a symbol. The front end's
superseding contracts are `port::render::Backend` for drawing and
`port::assets::Source` for original-asset reads. A future subsystem adds the
contract its recovered callers require, rather than inheriting an invented one.

Frames carry original units and original resource identities: positions and sizes
in the authored 640x480 space, textures keyed by the original TexturePack entry
hash, colours as the PS2 0x80-is-opaque bytes divided by 128. Window scaling,
viewport size, colour management and frame pacing are backend concerns and never
appear in a frame.

`Frame` currently holds only textured quads because that is the only evidence the
front end provides. This is deliberately not a modern RHI: PS2 GS/VU1 command
semantics are not recovered, so no command buffer, pipeline or descriptor model is
asserted. Additional command kinds are added when a recovered caller requires one,
never in advance.

Consequence for the host: `Ps2RenderHost` is the Godot backend and owns the
`RenderingServer` RIDs (canvas items, 2D textures). Godot-side code translates
input, time, audio hardware and file access; it makes no drawing decisions and
holds no per-widget game state.

## D009 — Activation slices call out of the front end through the same named-address seam as Back (TASK-0025, 2026-10-13)

`port::frontend::ScreenActionServices` gains `invoke(elf_address, argument)`, the shape
`BackServices::invoke` already had. The Language Options activation arm (`001F1B08`
message `0x01`) is the first activation slice whose effects leave the front end: it calls the
locale loader `0021E0D0`, conditionally the full profile reset `00183830`, and the manager pop
`00170320(1)`. Those are host services in the port, so the alternative would have been three
more typed callbacks on the struct, or -- worse -- letting the slice reach for the runtime.

Naming the ELF address at the call keeps two properties the project depends on. The host seam
cannot silently substitute a different effect, because the slice says which original function
it is standing in for; and an untranslated dependency fails loudly at the address that is
missing, which is how `00183830` stays an explicit `NotImplemented` instead of a skipped reset.
The order of `invoke` calls is the order the original makes them, so the seam also records the
sequence, not just the set.

This is a port-side dependency seam, not an original ABI: the original passes no such table.
Recovered slices stay free of Godot types and of the runtime's own headers.

Consequence: a new activation slice that only switches screens keeps using `request` /
`exit_animation`; one that changes global game state uses `invoke` and names the address. See
`docs/FRONTEND_MENU_LANGUAGE.md`.

## D010 — Port debug console renders through the existing Godot host (TASK-0031, 2026-10-14)

The pinned imgui-console and its bundled csys/ImGui are development tooling. Owned code
lives in `src/game_core/extensions` under `port::extensions`, free of Godot types. Its
triangle batches are separate from recovered game render commands. Ps2RenderHost uploads
the font atlas, translates input and clipboard, and owns pooled RenderingServer clipping
parents and geometry children. There is no OpenGL backend or additional window.

F8 toggles once per press; open-console input is consumed before menu dispatch without
pausing updates. Debug FPS is independent of both the console build option and detailed
information panel. Frontend errors are mirrored rather than intercepted globally. No
recovery confidence or original-game behavior changes. See the vendor provenance document
for the focus/history patches to the pinned source and retained MIT licenses.

## D011 — Persistent debug session and explicit viewer loader gap (TASK-0032, 2026-10-14)

CarViewer and TrackViewer are complete port-side viewing environments with synthetic
geometry, not partial reconstructions of unknown game loaders. Their native session records
requests and returns an explicit unavailable-loader diagnostic. No ELF address is fabricated,
so this port-side result is not an address-bearing hp2::NotImplemented recovered path.
Future recovered loading must use the existing asset-root resolver and preserve evidence.

The console/FPS host moves to a persistent DebugTools autoload, separate from the frontend's
frame renderer. csys callbacks only queue operations. Scene replacement occurs deferred,
after ImGui drawing, keeping logs/settings/history alive and avoiding callback destruction.
Returning to the frontend creates a new frontend scene and reruns its normal startup path.
Debug console-off builds can use direct viewer launch and buttons; Release disables entry.

The upstream LogWindow paired EndChild only when BeginChild returned true. Restored window
settings/small viewports exposed an unbalanced ImGui window stack. The documented vendor
patch pairs it unconditionally; native small-viewport and host transition tests cover it.

## D012 — Native vehicle slices with an explicit inspection display (TASK-0033, 2026-10-14)

Original geometry, texture rebinding, static attachments and LOD selection run in C++
without Godot types. CarRenderInfo ownership comes from allocator/receiver flow, not
diagnostic-string proximity. Typed recovered slices coexist with throwing whole entries;
template policy identifies the latter explicitly so regeneration cannot promote a partial
overload. Unbound shared helpers retain address names and port-side signatures.

The host uploads original data and offers development overrides for LOD/part visibility,
wireframe and bounds. It changes coordinate basis without changing scale. Material display
is labeled unlit inspection, retaining raw original material/vertex metadata instead of
guessing shader effects. The fully-in-view path supports retaining both faces; partial-view
normal-cone behavior is still untranslated. Successful loading is not material equivalence.

Texture requests are restricted to the two evidenced car/shared packs. Because original
lookup is move-to-front rather than fixed priority, this bounded service rejects duplicate
hashes instead of inventing precedence. Missing/unsupported resources fail explicitly.
The three sample models have no colliding hash between those packs.


## TASK-0033 native ImGui and GS byte-domain viewer adapter (2026-10-14)

Share a single native ImGui context/font/input/frame between console, FPS and viewer
controls. Debug viewer availability does not depend on HP2_ENABLE_DEBUG_CONSOLE;
Release has no viewer UI. UI queues requests, executed after frame rendering.

The recovered VU/base material path uses original parameters/lights and raw GS alpha,
not Godot lighting. Compatibility is the default host backend for encoded RGB blending;
Forward+/Mobile source-over is rejected for this pass. Preserve the existing explicit
RenderingDevice driver settings for manually selected backends. This is a port-side
backend decision, not original platform behavior.

GS ALPHA44 may require a factor above1. Ordinary spatial blending and a single screen
snapshot do not implement the general overlapping case. Conservatively bound supported
alpha per selected draw and explicitly fail unsupported views. Keep TASK-0033 active
while remaining render scope is audited. All228 observed static exterior associations
are proven within the supported alpha range; the general branch remains explicit.
No silent approximate fallback.

## D014 — Compact viewer, lazy LOD comparison and cancellable uploads (2026-10-14)

The viewer defaults to the highest-detail exterior. Show LODs is an explicit side-by-side
comparison, per user choice, not distance-based automatic switching. Each level uses original
body/attachment selection; display offsets and labels are host tooling. Geometry VIF filtering
happens before decoding and material/texture binding. A single native worker and bounded caches
remove blocking IO/decoding from the interactive path; Godot uploads run on the main thread
with a cooperative 2 ms slice. Only the latest request may commit. Retain the previous car while
loading, then atomically replace it; latest-request failure clears all vehicle display resources.
Paint tiles are representative samples of the recovered material pipeline, explicitly labeled
as a viewer preview; unknown original catalog fields remain offset-named.

## D012 — Shared vehicle backend and centralized shader sources (TASK-0034, 2026-10-14)

Keep original resource, part, material and lighting decisions in recovered native class/shared
slices. Extract the former viewer renderer as Ps2VehicleRenderer, with a thin debug request-policy wrapper;
future runtime scenes use the same backend, without Debug gating. This is a host boundary,
not a claim that the complete PS2 renderer is reconstructed.

All shader text belongs under src/platform/shaders, as requested. Build-time embedding avoids
source-path/runtime resource dependencies; CMake watches the shader files. Preview Mode is
explicit port-side camera/presentation behavior and does not supply invented original lights,
environment maps or floor-reflection passes. Correct legacy image-row origin at the vehicle
texture boundary rather than corrupting recovered vertex coordinates or frontend consumers.

The authorized wheel-blur follow-up uses original constructor resources and the same
native model/common backend. The debug checkbox forces an alpha90 overlay; unknown
original speed/time/per-wheel gates are not replaced with fabricated thresholds.
Rigid world placement transforms camera/light inputs through the original class slice.

## TASK-0035 - Original frontend reflection and adjustable wheel preview

Use original CARSELECTENVMAP and VU context2 coordinates/materials/blending in the common
vehicle backend. Keep base and reflection material instances separate per part/LOD so
camera, lighting and animation uniforms cannot leak between copies. Only the known static
frontend environment is supplied; game capture/composition remains untranslated. Do not
replace it with host reflection probes or Fresnel. Shaders stay under platform/shaders.

The user's later request adds a speed slider beneath the single Wheel Blur switch.
Its default80 and range-150..150 rad/s are port choices; the original strength/matrix
calculations consume that explicit input. Preserve phase during speed adjustments, reset
on switch changes, preserve chosen speed/switch across asynchronous car loads.

## TASK-0036 - Partition original strips at the host mesh limit

Do not discard strips, merge unrelated materials or raise a private Godot engine limit.
Split each vehicle solid's drawable strips in original list order into ArrayMeshes bounded
by RenderingServer::MAX_MESH_SURFACES. Use identity child batches beneath one native part
root so existing part transforms, wheel animation and visibility remain authoritative.
Cache the whole batch sequence; keep original strip addresses/indices in draw records.
Use union bounds for framing/LOD spacing, clone each batch in LOD comparisons and bind
wireframe to every draw node. Multi-batch nodes are atomically replaced on async commit
rather than reusing only a parent and leaving stale children. Original PS2 behavior and
Verified status are unchanged by this host-only capacity fix.

## D015 - DrivingLab and explicit recovery boundaries (TASK-0038, 2026-10-17)

Add an independent Debug scene using existing deferred viewer/ImGui/F8 routing and the
shared vehicle renderer. Native session ownership, recording/replay and normalized tool
input live outside Godot. Keep the floor/camera port-side; no contact force, rest state or
fixed60-Hz timing is inferred from the fixture. Full native driving remains unavailable
until its original constructor/control/force/contact/scheduling closure is recovered.

Absolute body/four-wheel publications are separate from the CarViewer preview animator.
Validate before publishing and reject preview integration in driving-pose mode. Failures
retain the last completed snapshot and do not advance recording or counters. Input capture,
F8 and lost focus pause and release controls; resuming is explicit. Existing frontend
lighting is a bounded visualization context, not claimed race lighting.

Original paused RAM captures may corroborate layouts but cannot establish instruction/frame
synchronization or native equivalence. Keep raw data ignored, distinguish non-atomic
wall-clock samples, and leave Verified untouched. The PINE capture tool is read-only.

### D015 continuation: bounded native projections and custom debugger

Keep constructor byte images, drivetrain field projections and solver projections
explicitly port-side until the original allocation/dispatch graph is complete.
Preserve untouched bytes and dynamic global inputs rather than inventing object
defaults. Bind unresolved solver dependencies through throwing interfaces; fixture
hooks never enter production. Do not install the partial projections as whole-entry
implementations. In particular, the exact five iterations alone are not a tire model.

The supplied custom PCSX2 debugger permits controlled original execution to owned
breakpoints. Always pause and remove only owned breakpoints on timeout/failure.
Do not rely on its PC+4 step implementation for branch/delay-slot equivalence.
This tooling decision does not replace R0's original ELF/game-data evidence rules
or promote any function to Verified.


### D015 continuation: drivable native fixture and transactional replay

The user accepted completing the synthetic-plane minimum driving loop before original
numerical equivalence. NativeBackend maps only the authorized SHA-bound ELF load segments
and original config into port-owned logical memory; it does not execute PS2 machine code.
Recovered constructor/reset, engine/transmission, tire/contact forces, body derivatives,
component integration and scheduling now close the supported single-car path. Existing
whole-class templates remain throwing for untranslated callers; DrivingLab explicitly uses
the recovered logical-address implementation. No class-wide completeness is implied.

The fixture chooses identity orientation, zero motion, original forward automatic operation,
geometric wheel/radius ground placement and original Race4 withA758 zero. E-0038-E13 retires
an earlier erroneous blanket mode4 exclusion: this is the running context; the excluded
mode4+A758 diagnostic path is distinct. E-0038-E14 anchors footbrake+218 versus handbrake+21C
to original event strings and writers. Synthetic material1 is the original SKID_ROAD row.
Plane geometry/camera/input scaling are explicit tool choices, not recovered world behavior.

Each native step uses a candidate Memory copy and commits only finite complete publications.
Session records elapsed clock values with input, so replay reproduces original scheduling
quantization independently of render pacing. Host pacing accumulates render intervals to the
recovered lower bound; recovered001A33F0 still selects/clamps/quantizes dt and the mover
subdivides. No Godot physics timestep or invented force is introduced.

At unavoidable original drivetrain0/0 DIV.S sites a narrowly scoped port ISA adapter returns
the signed finite EE saturation bit pattern (E-0038-E12). Host IEEE NaN is not silently fed
into the original branch. Normal finite divisions retain source operation order. This does
not claim general EE exceptional arithmetic/flag equivalence. Instruction evidence and
PC tests establish reconstruction only; nothing is promoted to Verified.


### D015 user correction: Test drive is a CarViewer mode

The user explicitly superseded the separate DrivingLab scene: CarViewer now exposes
Test drive / Return to preview in its existing ImGui window. The same selected car/skin,
renderer, camera and viewing environment are reused. An embedded RefCounted controller
owns native session orchestration; there is no second driving scene or renderer. Cached
reload restores original preview attachment bindings on exit. Preview wheel integration
is suspended while driving and while restoring the cached model. Existing camera keys
remain; newly introduced simulation F5/F6/F7 bindings are removed. Run/Pause/Step/Reset
are panel actions. The legacy drivinglab console command is only a compatibility alias
for this same CarViewer mode.

E-0038-U01 corrects the host steering sign (A-D and negative stickX) against original
001A5D60 basis writes and0013B5B0 axis inversion; recovered force/matrix code is unchanged.
E-0038-W01 corrects the missing render-basis projection after raw001C0D78 wheel publication.
Preview and driving share the recovered00120EF8 attachment composition, including its
separate tire/blur versus brake handling. The simulation pose is not patched by a guessed
half-turn. User-facing controls and visualization remain explicitly port-side.


### D015 follow-up: immediate drive entry and per-event input capture

Test drive now starts the native Session immediately on entry. The prior mode entered
paused and required a second Run click despite presenting itself as a driving entry.
After explicit Pause/F8/focus loss, resume still requires Run. No new key bindings.

CarViewer keyboard events use console-visible/keyboard-capture checks; mouse events
retain combined mouse capture for camera isolation. Reusing the mouse-hover predicate
for keyboard gating had discarded WASD whenever the cursor remained over ImGui. The
normal Godot Input.parse_input_event path is now covered with W/A/S/D press/release,
panel hover, actual native process/motion and existing F8/focus/preview restoration tests.
This correction changes only port-side lifecycle/input, never recovered driving physics.


### D015 accepted redesign: direct third-person driving, no manual debug workflow

The user explicitly replaces the earlier Run/Pause/Step/record UI and manual resume rule.
Test drive validates its first complete native step before activating a rear third-person
camera and relative mouse capture. Only return/hints/error remain in the mode panel. S is
footbrake only; Session instrumentation stays internal. Native algorithms and raw/render
wheel contracts remain untouched. Camera constants are explicit port policy: max(4,1.5L)
distance, bounds-center target,15degree pitch,60degree FOV,0.15degree/pixel sensitivity,
5..65degree pitch clamp,1second idle and4/s exponential shortest-angle recenter.

Temporary focus/F8 suspension and terminal native/render errors are distinct states.
Closing F8/regaining focus auto-resumes with cleared keys and the first elapsed interval
discarded, including notification-triggered resume. Explicit errors retain the last full
pose, show the reason and never auto-retry. Mouse intent survives temporary suspension,
Esc explicitly releases it and world click recaptures. Actual keyboard capture supplies
zero controls while stepping; mouse capture alone does not own WASD. Camera release no
longer calls native pause. Preview selection/settings/camera return through the same cached
renderer. This supersedes all earlier instructions to click Run after focus/F8.

Headless DisplayServer does not establish actual OS mouse capture: the controller tracks
capture intent separately and applies it to Input.mouse_mode. Headless input/camera tests
are supplemented by an owned-window SendInput acceptance driver with foreground-PID checks,
a separate owned focus probe and22 real interactions. Test data/screenshots remain ignored.


## TASK-0042: source ownership follows exact Debug markers

Adopt original CPP and unambiguous HPP spellings only after independently binding
a retail entry to the reference symbol and reading the exact ECOFF file marker.
Merge same-TU class shells; separate proven base/other declaring owners. Source
ownership does not establish inheritance, original ABI or behavioral equivalence.
For inline definitions, an out-of-line host projection may use its recorded
compilation TU with that distinction documented. Retain unknown companion
headers and unresolved entries as explicitly port-side partitions. Shared host
projection headers and private helper namespaces avoid claiming an invented
original header layout. Existing arithmetic bodies and throwing whole entries
are preserved; constructor/destructor callable placeholders retain addresses.


## TASK-0043: complete gap coverage and scoped name adoption

Emit confirmed Retail methods only; catalogue unmapped Debug methods without invented addresses. Keep constructor/destructor/operator and ambiguous overload gap entries address-named when original signatures are unconfirmed. Use typed static host carriers to distinguish projected functions whose original owners differ. Never infer a struct or field name from a containing function address or matching Debug offset alone. The 25 removed aliases are only pinned sole-throw duplicates with canonical same-address gaps. Existing implementations remain intact.


## TASK-0044: user-authorized Debug behavior and canonical driving owners

The approved plan selects Debug NFSHP2.ELF (full hash in DEBUG_DRIVING_AUDIT.md) as behavior,
layout, vtable and static-data authority for the supported driving closure. Debug symbols
alone are insufficient: decompilation was checked against instructions, adjustments, source
markers and operands. Preserve Retail evidence instead of relabeling it. No build date or
binary equivalence is inferred. The dedicated loader/config rejects other hashes without
Retail fallback; an explicit mapping replaces each historical global operand, never a delta.

Original-class bodies are the only algorithm source. An instance ObjectView carries Memory
and a logical PS2 address; factories/services explicitly bridge host lifetime and external
world queries. Compatibility adapters may forward; a second composite solver is removed.
Class generation preserves historical attribution and marks these owners superseded rather
than emitting duplicate gap stubs. Cross-binary symbol rows survive generation/address
collisions. Reviewed before/after token pins distinguish intentional reconstruction from
historical body-preservation claims. Host correctness and source integrity do not establish
EE arithmetic equivalence. Retain explicit unsupported-path throws and separate original
numerical certification from migration/tool acceptance.


## TASK-0045: original material data, port geometry and raw diagnostics

The approved first selector exposes ROAD1/DIRT2/GRASS6/SAND7/SNOW8 only. All33 original
terrain records are classified; water/wall/object responses and full topology are not inferred
from friction values. Preserve exact original coefficients and noise; do not exaggerate their
differences. Native+X boundaries25/50/75/100 and tool colors/labels are synthetic test policy.
Each wheel queries independently; no pose/gear reset accompanies material selection. Settings
commit with a complete candidate and are recorded/replayed with controls/time.

Read stored original Wheel values after Move/UpdateCar, including distinct traction/catchup,
slip/usage/force phases, and separate tire/material coefficients. Do not recompute a combined
grip score or assert SI units. Godot remains input/presentation only, the panel is read-only,
expanded diagnostics scroll independently of Return, and preview material/labels restore on
exit. Retain existing immediate WASD/camera/lifecycle without new function keys or Run/Pause.
Successor source review appends strict TASK0045 pins to a frozen fingerprint of the unchanged
TASK0044 state. A diagnostic-only VRSQRT address correction is separately evidenced; source
checks are not numerical verification.

## TASK-0046: original player controls and semantic driving actions

The approved task supersedes the earlier S-brake-only tool policy: use Debug
RealDriver's brake/reverse state machine, original shift/mode handlers and handbrake.
Default automatic, Q down/E up and Space held are tool InputMap bindings, not PS2
button claims. Manual GUI N calls the original desired-gear setter. No direct R
shortcut or invented speed threshold is added. Speed remains explicitly raw until
a unit conversion is evidenced; gear/RPM are read from original completed state.

Device mappings live in one InputMap and one semantic adapter, with queued presses
retained until a successful native step. C++ receives actions rather than key codes.
This is the reusable driving seam for later unified input, without changing unrelated
preview/frontend contexts or introducing a remapping settings screen now.

Skids use the original Wheel -> SkidMaker -> SkidSet/Segment -> RenderSkids/ePoly
chain. Keep original capacity, compression, material behavior, distance fade and
render-driven LRU; do not add timed fade or Godot decals. Pool/one-clan topology and
camera remain explicit synthetic-world boundaries. Debug drawing-context selection
must be recovered separately from Retail rather than reusing Retail texture flags.

Append a strict TASK-0046 source review to frozen TASK-0044/0045 states; never refresh
their accepted pins. PC tests and real-window usability are separate from original
numerical certification, which still requires a synchronized matching Debug scene.


## TASK-0048: Retail track assets with explicit Debug driving adaptation

Track catalogue, geometry, topology and texture bytes remain Retail-owned. Recovered
operations live in original declaring classes; port adapters own host buffers and
Godot resources. Debug driving accepts an immutable track contact provider and an
explicit 33-row terrain mapping, proven by independent table identities and scalar
bits. It never reinterprets Retail structures or selects friction from texture names.

Static track-polygon collision uses original vehicle CV/fallback geometry, wafer
witnesses, VClip, contact-time backup and impulse response. BaseModelName selects the
CV; catalogue resource_path handles aliased car directories. Candidate collision state
commits with candidate vehicle memory, preserving exact retry after failure. Anchored
Smokeable objects belong to the excluded breakaway/dynamic lifecycle. Unsupported
water, ownership, damage or effect branches fail with specific diagnostics.

Track rendering retains original mip images and register fields; host derivative LOD
is a documented projection. Environment reflection and original camera world LOD are
not claimed. PC acceptance never promotes functions to Verified. Historical source
review pins remain immutable, with separately scoped TASK-0048 successor reviews.


## TASK-0049: display the queried contact plane, not a substitute mesh

Track debug rendering is port-side presentation of original Topology polygons. Eligible
XY footprints use the exact stored elevation plane and terrain identity; a raw quad's
fourth corner can be off that plane. Preserve all road layers and original perimeter
edges, excluding original flags2/8 and exact zero-Z planes. Do not impose a slope cutoff
or modify physics. Debug colors, transparency and0.015 host-up display offset are tool
choices. Five colors share the synthetic Test drive palette, other IDs use deterministic
colors, missing catalogue names are magenta/Unknown. Overlay geometry is cached once per
track; controls remain independent and browsing-only.

Track texture orientation belongs in its adapter: indexed legacy decode's internal row
reversal is undone with its existing flip option; original linear32 row order stays
unchanged. Apply this to base and every original mip, never based on alpha or asset names.
Historical source-review JSON remains immutable; TASK0049 is an explicit successor.


## TASK-0050: retain original units and correct the outside projection axis

The track/car scale investigation found consistent original units across geometry,
topology, wheel configuration, vehicle CVs and original scenery culling bounds. Do not
apply guessed global scale factors. Runtime Godot roots also report unit scale.

The shared tool camera's60-degree vertical FOV differs from Debug DriveCameraMover
Update0015C2B0 OUTSIDE FieldOfView0x32DC. Original CalculateH001042E8 uses screen width,
so the port projection adapter now uses KEEP_WIDTH and converts that exact angle to
71.52099609375 degrees. Save/restore aspect policy with FOV on leaving driving.
This bounded correction does not recover the original follow/spring/obstruction camera
or establish the sole cause of a screenshot's perceived size differences. No C++body
identity changes, historical review-pin updates, physics or asset scale changes.


## TASK-0051: one track frame pipeline, bounded GPU state adaptation

Use one native Runtime and non-Debug Ps2TrackRenderer for browsing and the independent
single-car runtime scene. Original functions retain their declaring owners. Viewer
controls/debug overlays remain host concerns. Native commands retain original base
and packet320 passes; GPU fusion is permitted only after all sampled mip alpha bounds
prove identical coverage and the original positive GS blend equations prove the same
RGB result. Do not clamp original alpha to128. Compatibility GPU probes check byte-domain
copy/add composition and the observed depth-minus-sorting_offset order; the host cancels
origin depth to submit strict native order. Original normal target Z24 chooses the
Bias0/1 projection; unsupported Bias2 fails. Dynamic context2 six-view reflection remains
an explicitly reported dependency gap for two static model families, with base-pass
presentation identified. This does not recover frontend race start or full game flow.


## TASK-0053: preserve per-view layers for custom projection

Custom-projected sky, Sun and particles use camera-frustum custom CPU bounds.
RenderingServer ignore_culling bypassed camera layer masks and is prohibited for
these nodes. Source-depth proxies copy the custom bounds. This is host transport,
not an original visibility-distance change. See E-0053-H01 and E-0053-S01.


## TASK-0054: screen packet projection follows actual GPU target sign

Top-origin CPU screen packets for Sun/particles use shared screen_projection.hpp.
Custom POSITION bypasses the engine projection, so clipY follows the GPU
PROJECTION_MATRIX[1][1] sign exactly once. Both color and depth proxies retain this
vertex path. This fixes measured vertical mirroring without changing world anchors
or original camera/simulation math. See E-0054-P01 and E-0054-T01.


## TASK-0055: preserve original packets and gate host optimizations

Keep Compatibility as the pinned backend. Before-load configuration controls only
host resource/submission policy; Release needs no ImGui. Retain a frozen TASK0054
DLL and baseline mode for reproducible comparisons. Pixel equality includes the
intermediate color and encoded-depth targets, not just final appearance.
MultiMesh is experimental/off after Track24 pose50 exact color counterevidence;
no numerical tolerance or asset-name exception is introduced. Opaque queue and
occlusion are independently opt-in experiments until proven. A project default
may change only after all-route correctness and relative performance acceptance.


Do not share world ShaderMaterial objects in TASK0055: controlled parameter
eligibility was insufficient to preserve independent-process environment atlas
output (107pixels differed in3/3 samples; disabling sharing restored exact atlas
output3/3). Preserve distinct identity and cache immutable geometry/textures/
shaders and redundant submissions instead. MultiMesh also remains experimental
off. Occlusion/opaque probes found zero eligible work and provide no usefulness
proof. Release target access is read-only, for untimed evidence capture.


TASK-0055 acceptance completed:30 tracks,120 route-warmup/600 measured frames,
3 repeats in both frozen54/final55 Release1080p runs. Native packet/pose, original
effect permissions and draw-call distributions match. Median-route improvements:
wall10.03%, CPU prepare27.62%, host submission33.16%; no persistent>5% wall median/
P95/P99 regression. GPU timers unavailable; loading stalls remain. The accepted
resource/particle-reuse profile is now the common project default. Distinct
material identity retained; MultiMesh/occlusion/opaque remain experimental/off.
See [performance report](PERFORMANCE_TASK0055.md). Override at launch with
--track-optimization-profile=baseline. No original-console Verified promotion.

## TASK-0056: user-authorized engine-selected scenery LOD

User explicitly chose fully engine-controlled switching with original HP2 meshes,
accepting different boundaries. Use Godot4.4.1 visibility ranges, not new simplified
meshes or a replacement physical model. Original core LOD remains intact and default.
The alternative is a named port-side backend, shared by every consumer. Keep original
section constraints, shaders/materials/effects; do not batch across range anchors.
Use transformed-AABB-center bounds and identical depth-proxy ranges. Keep experimental
MultiMesh/occlusion/opaque combinations disabled and material identity distinct.
Initial capture instances warm on first scheduled use; report this hitch and resident
node/memory regressions. Promotion requires measurements, not an assumption that
engine-managed LOD must improve FPS. See docs/GODOT_HLOD.md and E0056H01.

TASK0056 necessary dependency E0056T01: all-level uploads exposed original model
texture-table default lookup, previously omitted by the port. Fresh paired ELF
analysis and original DEFAULTTEXTURE pixels justify restoring the bounded resident
lookup in eSolid::FixTextureTable/GetTextureInfo. Apply it in the shared loader for
both backends; retain source aliases and raw bytes, reject unknown target bindings.
No original existing selection/physics function body is replaced. Prewarm/cancel
must preserve the visual frame counter; allocation bookkeeping is not visual time.

## TASK-0058 supersedes host HLOD

User revoked Godot-managed LOD. Only original production section/tree/instance
selection is accepted; main and independent capture views share the native runtime.
Show LOD remains debug-only. Prior TASK0056 sections are historical. No generated
low meshes or physics changes. See [native visibility integration](NATIVE_TRACK_CULLING.md)
and E0058R01 for evidence, tests, counters and verification limits.

## TASK-0059: isolate an explicitly reported particle gap from track lifetime

A finite camera move must not cause a particle validation NotImplemented to unload
all track resources. Catch only that per-quad alpha guard at the host boundary,
record the gap, omit that unsupported packet, and retain supported scene work.
Do not invent a blend or clamp original RGBA. Full signed high-alpha compositing
remains incomplete. Reset Camera is a port UI action restoring current track start.

## TASK-0060: bound host residency independently of native visibility

Preserve original selection and selected passes. Upload source meshes on demand;
merge exact decoded attributes without temporary GPU upload/readback. Pin active
views; bound inactive caches and release proxy/uniform references with source
eviction. Limits are host policies, not recovered PS2 thresholds. Debug overlays
are lazy; release raw bundle storage after all decoders finish. Retain CPU data
still used at runtime. Payload estimates and process memory are separate metrics;
ownership zero alone is not proof of no leaks. See [memory notes](TRACK_MEMORY.md).

## TASK-0061: one logical-to-physical host UI contract

Keep ImGui layout/style logical; scale framebuffer geometry/clipping and invert
physical mouse coordinates. Rebuild fonts only on DPI changes. Windows effective
per-window DPI follows OS settings; a shared service handles host Control text in
Debug/Release, preserving baseline sizes and lifecycle. Do not change original
glyph metrics, world labels, render resolution or game units. TrackViewer labels
live camera HP2/Godot axes explicitly. See [UI_DPI.md](UI_DPI.md) and E0061U01.

## TASK-0065: preserve vertex arithmetic while reducing Track11 submissions

Carry original matrices as per-vertex custom attributes for contiguous unlit Copy commands; keep local attributes/order and share the resulting mesh with encoded depth. CPU pretransformation failed exact depth parity. Accept the optimized default only on Track11 main views without car visual consumers; other tracks/captures retain baseline after observed pixel differences. Explicit option enables experiments elsewhere. Preserve Compatibility and current engine version; no unsupported FPS or PS2-equivalence claim. See E0065H01 and PERFORMANCE_TASK0065.md.

## TASK-0066: global rendering optimization, no track allowlists

The user explicitly requires global optimizations. Remove the Track11-only default guard and configuration flag; all tracks use the same matrix-batch eligibility. Keep generic material/order/lighting/capture/owner/LOD state constraints, without substituting a different ID list. Track24's known colour discrepancy remains recorded while globally enabled at the user's request; no claim of all-track pixel equivalence. Future optimizations must be state-based and global. Preserve immutable historical review bytes through explicit historical auxiliary snapshots, and carry inherited auxiliary pins into the successor's live scope.


## TASK-0072: parallelize transport preparation after native selection

Use existing Frame/DrawCommand output as the seam, rather than adding a speculative
PS2 main-loop API. Immutable CPU geometry/matrices may be prepared by Godot native
group tasks; cache/RID/material/node mutation remains ordered on the main thread.
Keep original sector selection, physics and reflection scheduling serial. Worker
limits0/2/4 are independent of Safe/Separate; defaults0/Safe pending measured image,
lifecycle and tail-latency gates. See RENDER_PREPARATION_TASK0072.md.


## TASK-0072: prepare desired submission values, commit serially

Extend the existing transport worker seam to per-view pure CPU math, not recovered
Runtime/Memory or Godot setters. Snapshot on main, compute fixed immutable slots,
join locally and compare desired values against latest committed Draw state in
original order. Keep Godot-free math/policy headers in platform/render; Godot
registry, observer, cache and upload ownership stay in godot_bridge. Bulk staged
versions and source-specific structure/field notifications reduce repeated engine
reads. Unknown sources fall back; residency and Separate stay off. New candidate
math is default-off pending independent visual/lifecycle/performance gates.


## TASK-0074: keep wireframe debugging in the host UI adapter

Append a clearly labelled port-only Debug effect descriptor in track_controls.gd
and intercept that descriptor's action before reaching recovered effect controls.
A lazily created renderer-owned helper switches the direct/root and compositor
colour viewport debug modes, follows colour viewport recreation and restores
previous root state on exit. Encoded depth/history/reflection passes and native
commands are unchanged. Disabled mode has no helper process or mesh generation.

## TASK-0076: preserve loaded assets across render gaps; capture is opt-in

Per-frame render/readback exceptions no longer invalidate the loaded track or
collision environment. Host diagnostics distinguish asset ok from render_ok;
failed frames report and retry, while explicit unload/load failure retain their
existing teardown. This addresses destructive exception lifetime, without claiming
the exact user screenshot exception was identified. Dynamic Environment reflection
capture defaults off (also on Restore defaults); explicit effect opt-in remains.
This user-requested performance policy is port-side, not original PS2 behavior.
# TASK-0077 P0 - Measured work owns the export boundary (2026-10-21)

Keep Godot and the existing native frame contracts. Benchmark collection may
stop at its bounded sample limit without exporting; only the benchmark releases
the export hold after measurement, including long stress runs. Ordinary Profile
capture retains automatic export. Snapshot, source hashing, IO and async dispatch
are reported outside measured rendering; unavailable whole-frame GPU time stays
unavailable. Explicit environment-capture pressure is separate from TASK0076's
default-off policy. A/A controls and pre/post input hashes are prerequisites for
candidate diagnostics. Configured build recipes do not attest a copied DLL.
Fukami supplies engineering ideas only, never HP2 behavior evidence. P1-P3 remain
separate follow-up work; no core/physics changes or optimization defaults here.


## TASK-0078 - Compact generated progress dashboard (2026-10-21)

The user's concise-layout request supersedes the old mixed manual/generated
tracker format. PROGRESS.md contains only a large evidenced-body Retail count,
a default-collapsed CLASS table and default-collapsed Functions grouped by explicit
class. Nested groups also omit open. Preserve old prose in the archive and keep
continuation in task files and PROGRESS_LOG.md. The full catalog is SHA-separated;
unknown classes/names are never inferred. Class percentages cover bound methods
only; the headline denominator covers the full Retail inventory, with shared
entries deduplicated. Existing status/evidence classifications remain independent
of presentation. No core, runtime or original-verification change.

## TASK-0082: original sample transport (2026-10-21)

Use original ADPCM samples/loop flags and recovered native pitch/gain/mixing rules
with Godot only owning playback streams. Keep unsupported SPU/stereo/replay effects
outside the bounded implementation and visible in evidence; never synthesize a
replacement engine oscillator. Ordinary H input also selects the original horn
headlight flash. Bind human input directly on the playerless host fixture rather
than invoking the AI horn scheduler or constructing a fake original Player.

## TASK-0083: collision and tire audio (2026-10-21)

Retain original impact delay/variant/gain/pitch rules and collision lifetime using
port-owned event state in the existing candidate-frame transaction. Preserve
original scrape-before-impact RNG order, persistent sample ownership and stepped
fade. Bind the single-car fixture listener to its car position; spatial camera,
SPU and multi-player services remain outside this slice. Tire audio reads original
slip/traction/terrain state independently of skid-mark generation. Use existing
original-sample host transport, with native selectors/mixers free of Godot types.
See reverse/task0083/collision-events.md for lifecycle and comparison limitations.

## TASK-0084 recovered camera pose with host projection/controls

Use the recovered Debug DriveCameraMover fullscreen outside/default lookback-mode1
pose as Test drive's default. Native state and original topology clearance stay
independent of Godot. The user's explicit request supersedes TASK0050 projection
policy for Test drive: use Camera3D defaults (75vertical FOV/KEEP_HEIGHT). Preserve
original-FOV evidence as historical fact; it does not justify overriding this user
preference. Independent race-scene projection is outside the requested scope.

V is a host toggle of original lookback0/1, distinct from original POV cycling.
Mouse temporary orbit and1.5second idle return are an explicitly requested host
overlay. The native camera continues updating beneath it; return reaches the exact
current native pose. Do not insert invented camera pitch from authored Angle:
the supported exterior path discards that final scratch rotation before calling
IsoProjectionMatrix; E-0084-C01 includes disassembly counterevidence.

## TASK-0085/86 approved GPU presentation policy

The user explicitly authorizes Godot GPU particle simulation/rendering while
preserving recovered emission rules, original assets/curves and physical driving
behavior. GPU arithmetic/sorting differences must be measured and documented,
not promoted as original equivalence. Preserve a native reference backend and
switch default only after the accepted performance/visual gates. TASK0086 then
restores original marker/parameter-driven light flare billboards, not road beams.


## TASK-0086 attached light sprites

Use original ePositionMarker/pivot/Param normals and Debug flare parameters, not
Godot light presets or projected cones. Produce native fixed billboard descriptions
and reuse static host quads/materials. Keep original GS texture/depth state and
explicit gaps for unsupported routes. Read tables from the user's SHA-checked
ELF; proprietary raw assets stay ignored. Existing original horn callback controls
noncop flash or cop FAST state; no additional native state setter. The user's
request to stop repeated benchmarks leaves GPU optional/native default with no
P95/FPS acceptance claim. No Verified promotion without original runtime evidence.


## TASK-0087 original zero-interval emission retries

Allow original clamped-zero emission intervals to consume the next original RNG
iteration; they are legitimate for variance greater than1. Replace premature zero
rejection with an explicit port-only2^20iteration execution budget pertrigger.
Budget exhaustion throws and rolls back, never silently truncates a frame. Keep
original poollimits and original rates unchanged. This is a host hang policy, not
an original game limit or Verified claim. No further performance matrix.

## TASK-0088 requested camera controls and projection correction

The user supersedes TASK0084's V/lookback and Godot-default projection policy:
V selects original available driving POVs, including the forward-facing bumper
view, with each recovered view's original projection. B holds original lookback
and release restores the selected POV; it is not a latched toggle. Mouse temporary
orbit/idle return remain host policy. Recover original bumper pose, POV admission
and FOV before implementation; do not infer them from an exterior camera rotated
by half a turn. Input cleanup on focus/UI/pause is host lifecycle policy.

## TASK-0089 water visual/audio ownership

Retain original DriveOver1/2 terrain groups, pair-midpoint emitter positions,
intensity blending and raw timed wiper state. Water state and splash requests
advance in the native candidate transaction; visual particles use the existing
once-per-visual-epoch transaction. Keep sample RNG/cooldown and resets native.
Godot only transports original InGame samples and original rigid-animation wiper
matrices. Missing optional wiper geometry remains absent; unresolved required
animation remains explicit. For newly bound wiper textures, original
GetTextureInfo's evidenced default19006835 is a recovered resource lookup policy,
not a generated replacement. No general material fallback is introduced.

Original replay capture ownership, complete World sound scheduling and the
separate rain-screen RainSplash system are not inferred from this water slice.
Port input recording remains separate from original replay capture. No benchmark
or original-runtime equivalence promotion.

## 2026-10-26 TASK-0098 selection presentation services

Keep Debug decoded camera/pose/light/alpha behavior in independent native storage with original names/addresses. Existing shared native geometry/texture worker and bounded staged Godot uploads may carry actual named selector solids; they must retain generation cancellation and never fabricate a vehicle identity for PLATFORM/CARCOVER. Typed C++ pose/light handoff avoids per-part/per-frame Dictionary construction. Individual source ownership not yet established is explicitly a port projection, not an asserted original file/ABI. Required Tick/locked/help/speech/render dependencies fail explicitly until bound. Fresh eDisplayFrame stages require reviewed multi-stage composition; do not substitute the model viewer or assume all UI overlays all3D. These seams do not authorize synthetic World/AI/outcomes.

## 2026-10-26 TASK-0098 Tick and render packet boundaries

Preserve original selector load polling and unload/load/begin ordering behind
typed required services; a superseded completed load is not permission to publish
the wrong selected vehicle. Restore static camera-offset initialization separately
from SHA-checked ELF mapping. Locked description lookup retains original ordered
task/event/points data and localization arguments, with explicit frame-owned text
rendering. No guessed unlock text or silent missing-effect update is admitted.

Carry stage1/7 ranges as optional native Frame metadata while retaining existing
quad order/backend compatibility. Validate complete known coverage before a future
compositor relies on it; legacy event-tree appends remain explicitly incomplete.
This does not authorize replacing original stage0/2/4 shared-depth/reflection
ordering with a generic vehicle viewport between two canvas layers.

## 2026-10-26 TASK-0098 mutable frontend effects and shared budgets

Project the original general inverse/Hermite operations rather than substituting
the existing affine inverse or simplifying away original matrix products. Only
initialized colour and consumed extra X/Y output lanes are claimed; original
unwritten stack Z/W remain unresolved and unused by rendering. Stage the rewritten
definitions with particles so a failed basis rebuild cannot publish partial data.
Do not move original after-render FrameEffects stepping before drawing.

Use explicit port-owned shared1000-particle/400-group accounting and RNG for
multiple auto-emitter systems. Emitter600 is initial capacity with original pool
overflow growth, not a limit. The new shared step uses one candidate copy, not
an additional nested full-simulation transaction. Future World dynamic triggers
and cross-system definitions require their own reviewed binding.

For the reached zero-speed nebula normalization at Debug00128510, add a narrow
port VRSQRT-one/zero instruction carrier using the already established finite
zero-denominator policy. Original instruction/zero-speed asset inputs are the
game evidence; PCSX2 v2.8.0 VUops is an ISA implementation reference only. No
epsilon, guessed direction, new particle rate, general VU exception model or
original equivalence claim. See reverse/task0098/frontend-effects.md E-0098-P03.

Retain E-0098-F03 counterevidence explicitly: reflection0x11 disables envmap and
sorted alpha, and stage5 owns the shared pending-alpha flush. Implementing five
flattened viewport images would not satisfy those original conditions.

## 2026-10-28 first-event scheduling and shared native launch

The user's first-real-event milestone revises scheduling: progress from bounded original launch parameters/participants into TASK-0099 shared World construction while TASK-0098 formal frontend/compositor/private-profile bindings remain explicitly incomplete. No task acceptance is waived, no synthetic race/outcome or reduced police/traffic configuration is authorized. Port service fixtures test native boundaries only. Original StartRace requests GameFlow unload/load rather than directly substituting the single-car scene. Session ownership must keep tree/state/catalog/memory alive through callbacks and isolate loading failure before publishing a running race.

World registration and startup use one shared Memory. An EXCLUDE_FROM_ALL Release standalone benchmark measures image clone/destroy baseline separately from gameplay; no FPS inference or backend transaction replacement is made. Dirty-page work must cover retained mutable views and all write/allocation paths before replacing existing rollback behavior. No Godot physics/navigation substitution, per-car whole-world copies or proprietary input staging.
