# TASK-0018 — Native runtime and replaceable rendering backend

Status: active. User explicitly authorizes implementing NativeGameCore + render
abstraction + GodotRenderingServer, with reverse code independent of every platform.
Preserve all existing uncommitted work; no commits requested. This is a port-side
architecture migration, not new PS2 recovery. Existing addresses/evidence remain.

Scope: current reconstructed frontend is the functioning migration target. Move
frontend state, navigation, updates, layout/glyph/clip command generation and startup
orchestration out of Godot adapters/GDScript into platform-independent C++ runtime.
Introduce typed render frames and native resource interfaces; implement shared
C++ RenderingServer backend owning RIDs. Godot-facing host translates input, time,
resources/audio hardware and diagnostics without game decisions. Keep recovered
logic in original source files; new infrastructure explicitly under port namespaces.

Acceptance: runtime/core compile without Godot; no Godot types or platform-specific
headers in recovered core/render contracts/runtime; fake backend exercises actual
frontend frames/navigation; default Godot scene submits native commands via
RenderingServer; menu/scroll/clip/startup/video/audio regressions pass; Windows DX12
captures and export pass; document dependency graph, resource lifetime, interfaces,
extension steps and unfinished original behavior. No fake3D/physics/AI implementation.

Prior TASK0017 Back/Audio/Display changes retained:18/18native+Godot tests passed,
SubmenuSmoke covered8 Backspace routes/locale direct-pop/viewport/selector/ends;
Display snapshot, changed settings, editor/save/preview branches remain unknown.

User steering: deferRenderingServer; switchtoTASK0019name recovery. Only unlinked
port render/value/asset infrastructure files created sofar. Runtime/RenderingServer
migrationnotimplemented. Preservefiles; no buildintegration or successfulmigration
claimed. Resumeonlywhenuserreturns tothearchitecture task.
