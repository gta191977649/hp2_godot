# E-0049-H01 - Track debug host rendering

The Godot bridge consumes DebugGeometry built on the CPU worker from the immutable
TrackEnvironment topology and the SHA-bound Debug terrain catalogue. Retail/Debug ID
compatibility remains the explicit TASK0048 table mapping. Two ArrayMeshes cache fill
triangles and perimeter edges for each loaded track; toggles only set visibility and a
material-mode shader uniform. The bridge maps nativeXYZ to hostYZX exactly as existing
track rendering. Both modes share one fill and keep normal depth occlusion, with no
Godot physics query or grip modification.

The0.015 host-up offset, unlit display, blue collision tint, alpha0.26 collision /0.88
material and pale outlines are host inspection choices, not PS2 constants. Unknown IDs
remain marked magenta by the core helper and named Unknown in the legend. Loading,
clear, failure and destruction release debug resources with the scene; bool preferences
persist only for the current viewer. GDScript hides effective flags in driving mode.

Actual Compatibility windows inspect default-off, collision-only, material-only and
combined display at Track11's original start. Proprietary screenshots and logs stay
under ignored local/task0049. This is PC display validation, not original certification.
