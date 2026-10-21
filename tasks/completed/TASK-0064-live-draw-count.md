# TASK-0064 - Live draw count below viewer coordinates

- Status: completed
- User scope: one realtime Godot global last-rendered-frame draw count below
  TrackViewer coordinates. Port presentation only, no rendering/physics change.
- Use Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME, not estimates or native
  surface counts. Preserve dynamic header height and DPI layout.
- Check: existing native viewer UI test, bridge build, source preservation.

- Implemented engine last-frame total below coordinates, with N/A fallback.
- Validation: normal Debug build updates godot/bin/ps2_port.dll; native debug_console test passed. Explicit TASK0064 source successor preserves recovered bodies. No runtime benchmark launched.
