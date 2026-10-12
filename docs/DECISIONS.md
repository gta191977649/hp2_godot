# Decision log

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
