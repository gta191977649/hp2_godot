# Architecture and ownership

## A: Recovered game core

`game_core` is a static C++17 library. It owns authoritative game state, recovered
physics, AI, world management, and rules as these become understood. It cannot
include Godot headers or depend on the Godot runtime. It contains the scaffold and two police local bodies over explicit PS2 byte-storage
views. PoliceMemory supplies 32-bit memory reads and an opaque external predicate
contract. These views do not implement original class inheritance, constructors or
the rest of the game; see docs/FULL_RECOVERY.md.

Organize future source as `src/game_core/<subsystem>/<module>.cpp` with cohesive
headers. Each recovered function links to a reverse record keyed by binary ID
and original ELF virtual address. Keep one record per function, not one enormous
decompilation file. Use explicit fixed-width types and review PS2 overflow,
endianness, float precision, alignment, pointer width, and R5900 ABI assumptions.
Never treat a PS2 address as a native pointer without an evidence-backed mapping.

## B: Platform abstraction

`platform_api` is an interface CMake target exposing native C++ contracts in
`src/platform/services.hpp`: rendering, audio, input, filesystem, timing, and
diagnostics. These are provisional scaffold contracts, not recovered signatures.
Extend them only as actual recovered callers justify a requirement.

Core code may depend on these contracts. Tests can supply fakes without Godot.
The host owns concrete services and outlives all non-owning service references;
core code owns its game data. File read failure is explicit; adapters must
document resource lifetimes, units, threading, and errors when implemented.
No concrete rendering/audio/input backend is claimed by the current sample.

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
