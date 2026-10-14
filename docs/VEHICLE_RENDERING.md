# Vehicle rendering recovery — TASK-0033

Target: SLUS_203.62, executable revision unknown, SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
No original hardware/PCSX2 comparison or Verified claim.

## Implemented native slices

`CarRenderInfo` is bound through the named allocator and constructor receiver flow.
Its address-named typed overloads recover geometry names, static texture substitutions,
projected-size LOD selection and zero-spin tire/brake assembly. Whole constructor and
render entries still throw. Exact diagnostic method names are not address-bound.
`CarPartCuller` remains an empty shell because its function ownership is unproven.

The global original car catalog supplies model prefixes, type values and four wheel
translations. The original catalog preserves 64 cars and 235 color rows in source order.
Car+0x60 resource text supplies shared geometry mapping, including HP versions; the
viewer explicitly selects BIN instead of the original LZC extension. Missing resources
remain listed. Selection uses original car/skin IDs, and skin replacement reuses cached
geometry bytes, then reruns texture substitutions, material lookup and UV adjustment.
Lights off and zero steering are explicit viewer inputs. Shared
trigonometry retains the recovered polynomial instead of substituting ideal rotations.

The geometry reader parses original chunk bounds/alignment, texture/material tables,
positions, UVs and packed vertex words. VU RINIT/RNEXT scheduling supplies ADC suppression
bits; triangle-list expansion retains the original strip queue. Every strip's emitted
triangle count is checked against the original entry. Header authoring matrices are
preserved as metadata, not applied speculatively to already positioned vertices.

Runtime texture resolution uses only the car and shared GLOBAL archives, retaining source
provenance. Original static substitutions resolve skin, window and light placeholders.
Unknown/missing textures and unsupported direct-color data fail explicitly. No exported
mesh or image is required or checked in.

| Original sample | Asset solids | Decoded vertices | All-asset triangles |
|---|---:|---:|---:|
| 911TURBO | 16 | 14,893 | 9,007 |
| BMWM5COP | 13 | 13,304 | 7,810 |
| 4DR_SEDAN | 5 | 1,628 | 920 |

These totals include unused variants. The viewer selects one body LOD and the matching
static attachments; its visible triangle count is smaller.

## Host display boundary

`port::render::VehicleGeometry` and `port::vehicle::Model` contain no Godot types.
`Ps2CarViewer` uploads geometry and decoded textures, instantiates native-selected parts
and releases their resources. The host maps source `(x,y,z)` to `(y,z,x)` with unchanged
scale and determinant +1. Native matrices remain row-vector values; the bridge changes
basis only. The host derives a projection scale from viewport/FOV, then calls the recovered
projection and LOD slices. This scale mapping is port-side, not an original view ABI.

The default pass projects original material registration and `00116828` coefficients,
initialized frontend `00115978` light vectors, VU reflection/base-RGBA arithmetic,
packed normals/colors and original texture MODULATE/TEST state. No Godot lights shade
vehicle surfaces. `CUSTOM0` retains unnormalized source normals, and the shader adapts
GS screen-linear Gouraud interpolation separately from perspective UV. The first material
block is the explicit normal-view selection (`0010A438`, original alternate selector zero).
The second block is recovered and tested but alternate rendering is outside this slice.

Vehicle palette alpha stays in GS byte units; frontend decoding retains its existing
expanded alpha API. Original clamp-both-axes and MXL0 linear-magnification/nearest-
minification are projected to host sampling. Host derivatives, rasterization, clipping,
float arithmetic and blend rounding are not claimed bit-identical to GS hardware.
Compatibility is the viewer baseline because its encoded framebuffer blend matches the
recovered GS RGB domain; Forward+/Mobile linear blending is explicitly rejected for
this source-over path. The host shader uses original byte-domain material calculations.

The general GS source-over equation allows `As/128` above unity, which ordinary Godot
spatial blending cannot represent. A synthetic flag4 material fixture demonstrates that
range, and the viewer explicitly rejects it rather than clamping. It is outside the observed
static exterior slice: all 228 available car/skin associations have strip flag4 clear for
source-over, and all 172 indexed ALPHA44 archive entries have texture+0x74 zero and raw
alpha at most 127. Initialized frontend lighting gives vertex alpha 128 for these surfaces,
so combined alpha remains at most 127 for every camera. The sole nonindexed ALPHA44
entry is the excluded 32-bit DASHWINDOW. No required current static source-over path was
found to need negative destination coefficients. This corpus proof does not implement the
general branch. Raw texture inspection remains an explicit debug override.

Both faces are retained for the bounded fully-in-view submission slice. Partial-frustum
normal-cone culling, original VU clipping, damage, reflections and full render entry remain
untranslated. Whole entry stubs still throw; the partial APIs do not imply whole-function
completion. No PCSX2/hardware comparison has been made.

## Evidence and continuation

- E-0033-G01/G02/G03: original geometry loader, CPU accessors and three asset comparisons.
- E-0033-T03: original VU ADC scheduling and emitted GIF primitive selection.
- E-0033-G04: bounded fully-in-view and partial-frustum strip-dispatch findings.
- E-0033-B01/B02/B03: CarRenderInfo binding, static LOD and shared projection.
- E-0033-B04/B05/B06: catalog, original sine/rotation and frontend attachment matrices.
- E-0033-B07/B08/B09/B10: paint/material table binding, static light/window substitutions.
- E-0033-B13/B14/B15: material blocks, frontend light inputs and original car/skin/resource catalog.
- E-0033-G05: VU base RGBA/reflection, source normal scaling and independent instruction oracle.
- E-0033-T06/T07: GS texture/TEST/filter/raw alpha and independently indexed depth-write state.
- E-0033-T01/T02/T04: texture entries/alignment, decode coverage and shared pack lookup.

Per-function records live under `reverse/functions/<binary-id>/vehicle/`; geometry
structure evidence is under the corresponding `reverse/structures` binary directory.
The VU/texture summary ledgers retain precise instruction/asset locations. Raw code,
asset bytes and window captures stay in ignored `local/task0033/`.

Next work: compare original appearance and recover remaining material routes and
partial-frustum cone/clip behavior. General flag4 source-over would require ordered
destination access, but is outside the observed supported static corpus. Do not
mark TASK-0033 fully restored based on the inspection material or successful host tests.


The final window inspection found and fixed an initial ZMASK indexing error (T07).
TEST selection uses texture+0x4F/+0x74; depth writes independently use+0x50 and the
original phase gate. All722 copy-equation headers enable+0x50; source-over/additive
headers disable it. Paint now occludes interior surfaces; no depth prepass was invented.
The GPU oracle includes a later source-over surface behind paint and checks it remains
occluded. The normal target/depth-enabled phase is an explicit static viewer input.

The actual MetPaintSilver attribute fixture also matches CPU/GPU RGB(190,190,211)
when texture+0x74 correctly enables flag4. A scratch CPU probe initially omitted that
flag; the earlier supposed5-byte GPU discrepancy is retired, not accepted as tolerance.

## TASK-0034 — UV origin, preview and common backend

E-0034-U01 corrects the legacy decoder's bottom-first image output for the vehicle path.
The vehicle adapter now restores source-row order; it does not invert the original UVs,
guess per-model fixes or change frontend texture decoding. The earlier provisional false-flip
convention is retired. The two-row indexed texture regression checks actual texel placement,
and Corvette/CL55 runtime captures were inspected.

E-0034-M01 rereads CarRenderInfo00120EF8's lighting-context delegation to00115978.
A typed matrix-only class overload exposes that bounded static frontend call; Model uses
it for part light blocks, together with the existing recovered binding/pose/LOD slices.
The whole render-entry stub and untranslated effects remain explicit. No invented class
ownership is assigned to shared00115978 or the VU program.

Ps2VehicleRenderer is now the common Godot upload/material/submission backend, available
independently of Debug. Ps2CarViewer is a thin Debug-only request-policy wrapper for the debug scene. Numeric GPU
checks instantiate the common backend. Native original-class/math/resource code remains
free of Godot, and extensions only own debugging UI/session functionality.

All shader sources and specialization text live under src/platform/shaders: car_base.gdshader,
car_modulate.glsl, car_wireframe.gdshader and the source builder. CMake embeds their contents
into the extension and tracks source changes, so export/runtime requires no source directory.
The recovered VU material/light/GS coefficients still drive those shaders; no Godot PBR or
viewer-only light rig replaces them. TASK-0035 below adds the original frontend environment pass. Floor reflection, live game
environment capture and full alternate paths remain untranslated; no Verified claim.

Preview Mode is explicitly port-side presentation: continuous12-degree/second camera orbit,
mouse orbit/pan/zoom offsets,35-degree FOV and low view angle. Mouse dragging or console/UI
capture does not suspend rotation. It hides the debug floor/grid/axes; these settings are not
claimed to be recovered PS2 camera constants. Turning it off restores the debug environment.

CarViewer's **Wheel Blur (high speed)** enables tire rotation and the original `WHEEL_BLUR`
overlay. The **Wheel speed (rad/s)** slider underneath adjusts preview angular velocity
from -150 to150: negative reverses, zero stops. Default80 rad/s is a port-side demonstration
input, not an original vehicle physics constant. Original per-wheel strength is
clamp(abs(velocity)*0.02,0,1), with alpha scale90; the blur layer has its own original
non-spinning matrix. Changes update existing resources and continue while F8 is open.
Disabling resets the static pose; selected speed and switch survive car changes.
`--wheel-blur` enables it on direct launch. Original contact/damage gates and physics remain
untranslated. Missing resources fail explicitly; no synthetic blur texture is generated.

## TASK-0035 - Wheel animation, environment pass and mirror skins

Read-only Ghidra findings E-0035-W01/W02 confirm separate tire/blur matrices and per-wheel
angular-velocity opacity. CarRenderInfo delegates bounded math through address helpers;
the native model publishes matrices/intensity, and Ps2VehicleRenderer updates existing
nodes/uniforms without loading assets. Preview steering and suspension are zero. No recovered
physics is replaced by viewer simulation. The original whole-entry stubs remain throws.

E-0035-R01/R02/R03 trace frontend CARSELECTENVMAP from FRONTEND/FRONTB.BUN through
00103E68/00112858, environment view0011B520/001175C8 and VU002A8AD8. Glass and body
reflection use the second original material block, original reflected-ray ST/alpha and
context2 additive blend (Cs*As/128+Cd), with depth writes suppressed. The 256x256 linear32
texture retains raw alpha and source-row order. No generated texture, Fresnel or host PBR.
Shader source stays under src/platform/shaders; the backend is shared with runtime vehicles.
Compatibility is required for the bounded encoded-color additive route.

E-0035-M01 corrects omitted mirror texture patch pairs:0011E860 binds object+0x528's
SKIN1/SKIN1B substitutions to both mirror solids. Previously they could retain red SKIN1
on a silver body. The fix uses native constructor bindings, and actual MCLAREN assets are
checked across every skin. It applies to all cars using those original mirror bindings.

Confirmed native/GPU math is not PS2 frame equivalence. Live game six-face environment
capture/composition, other input offsets, destination-read alpha>128, full render entry,
contact/damage wheel gates and original runtime/hardware comparison remain open. Window
captures are ignored local artifacts, never committed assets. See active TASK-0035 and its
address records for evidence, tests and continuation.

## TASK-0036 - Host mesh surface capacity

An original solid may contain more drawable strips than Godot's MAX_MESH_SURFACES (256
in the pinned4.4 bridge). ELISE and ELISEHP exercise this path. The shared adapter now
partitions the original ordered surface list into bounded ArrayMesh batches rather than
rejecting or truncating the solid. Each strip retains its vertices, indices, texture, shader,
material and reflection pass. Batch0 is the native part's MeshInstance3D root; subsequent
batches are identity-transform child meshes. One native part still controls transform,
wheel animation and visibility. This is host capacity adaptation, not recovered PS2 logic.

The GPU cache owns all ordered mesh batches, and incremental upload resumes at the exact
original strip cursor. Bounds/LOD spacing union every batch; LOD comparison copies clone
all corresponding materials/reflection passes; wireframe reaches all batches. Batched roots
are replaced atomically at async commit, while complete GPU meshes are reused. Single-mesh
node reuse remains unchanged. Port metadata hp2_part_name provides identity independently
of Godot's duplicate-node display naming. Per-part draw_surfaces reports the original native
strip count, independently checked against uploaded child meshes by car_surface_batches.

`car_surface_batch_smoke.tscn` loads every available original catalog ID (first skin),
checks per-part surface/visible triangle coverage and the host limit, then exercises an
oversized car through async/warm skin load, wireframe, visibility, LOD copies and cleanup.
No proprietary fixtures are checked in; it resolves configured original assets at runtime.
