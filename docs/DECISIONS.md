# Decision log

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
