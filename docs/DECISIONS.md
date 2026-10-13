# Decision log

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
