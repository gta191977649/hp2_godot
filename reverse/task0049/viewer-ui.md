# E-0049-U01: TrackViewer diagnostic display integration

This successor changes port presentation only. No original algorithm or original material color claim is made. TASK0048 historical baselines and both source review documents remain unchanged.

The track browse panel exposes independent, initially false Render Collision and Render Material controls without requiring Advanced. They are disabled until the loaded original topology cache reports debug_ready. Material legend entries carry original terrain ID/name and polygon count with deterministic diagnostic colors; unknown IDs remain explicit as magenta/Unknown legend rows. Native overlays use cached geometry and update visibility only when toggled. Clear, failure and track replacement release cached display resources.

The GDScript owner retains desired session flags across switching, failure and clear. Entering driving suppresses effective overlay flags; returning restores browse choices. Controls are not exposed in driving. Native setters return void; the adapter reads get_state after each setter. Command line --track-render-collision and --track-render-material initialize browse preferences for repeatable captures.

Read-only state contract: render_collision, render_material, debug_ready, debug_diagnostic, material_legend [{id,name,color,polygon_count}]. Native debug_triangles, debug_edges, debug_fill_visible and debug_edges_visible support lifecycle inspection. ImGui actions use render_collision/render_material with enabled bool.

Validation sources: src/tests/debug_console_tests.cpp exercises real ImGui pointer input for disabled controls and independent toggles; godot/scripts/debug/track_smoke.gd covers all four combinations, switch, missing load, reload, driving suppression, return and clear. Native geometry/texture tests cover G01/T01 separately. These are host integration checks, not original-runtime Verified results.

The successor audit derives the approved post-D05 state entirely from immutable TASK0048 review deltas, delegates validation to every predecessor, and authorizes only exact before/after function identities listed in source-review.json. There is no accept-current or baseline refresh mode. CLI entry points43,44,46 and48 delegate49; historical Python APIs retain their prior behavior.
