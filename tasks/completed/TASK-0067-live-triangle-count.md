# TASK-0067 - Live triangle/primitive count

- Status: completed
- User scope: add realtime rendered triangle count beside the existing draw-call overlay, globally for TrackViewer.
- Read Godot Performance.RENDER_TOTAL_PRIMITIVES_IN_FRAME alongside draw calls. Label Triangles/primitives because Godot also counts line/point primitives; do not pretend this is a triangle-only statistic. Same last-rendered-frame scope, no GPU readback.
- Validate normal Debug build, existing native UI test and explicit port-only source successor. Preserve prior source reviews and inherited auxiliary pins.

- Added the global primitive monitor to the UI snapshot and same-row overlay with N/A fallback. Normal Debug build updates godot/bin/ps2_port.dll; existing native debug_console test passes, headless TrackViewer loads, and TASK0067 exact two-body source successor passes. No FPS benchmark needed.
