# TASK-0052 host integration (in progress)

E-0052-H01 describes port-side resource/session/GPU/UI plumbing. It does not assert
new original-game functions. Original behavior references are the paired Retail /
Debug records and E0052D01..D14, E-0052-E01..E07 and E-0052-P01..P06.

One Ps2TrackRenderer owns the native Runtime and main/capture submissions for both
viewer and independent race scene. Shared permission settings expose actual activation
and unavailable reasons; they do not invent effects. Public timestep input supplies
host presentation time to the recovered WorldEffects adapter once per main frame.
Native resource indexes replace linear lookup without changing first-match semantics.
Contiguous GPU batches preserve attributes, triangles and complete state; alpha depth
must match because additional Sun/particle keys can interleave. Environment multi-pass
commands remain separate. GPU nodes/materials/meshes are cached, and explicit mesh /
material notifications maintain the separately rendered native-depth proxies.

Common alpha ranks reconstruct scenery insertion order then append Sun and static
WorldEffects keys, following the evidenced eDisplayFrame order; the original radix
sort determines their order. Unrepresented FrameEffects/car key producers and unsorted
particle texture chains are explicit gaps. Sky uses the host CLIP_SPACE_FAR constant;
the old Compatibility +w substitution covered opaque vehicles. An asset-free GPU
fixture includes opaque and transparent foreground and a negative control.

All ownership cleanup occurs before child nodes are recursively deleted. Load failures
clear partial resources and surface the exact diagnostic. Debug render-target readback
is a diagnostic interface; Release has no binding for it. Profiling reports are host
measurements only, not original-equivalence evidence. The current Debug DLL is promoted to the normal application for the requested build delivery.
