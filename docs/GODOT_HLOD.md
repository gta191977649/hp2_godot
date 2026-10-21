# Archived Godot HLOD experiment (TASK-0056)

TASK0058 supersedes this design at the user's request. The backend is disabled;
`godot_hlod` now fails explicitly. Historical results below are retained, not current
launch instructions. Production uses [native culling](NATIVE_TRACK_CULLING.md).

This is a user-authorized **host rendering policy**, not a reconstruction of HP2's
LOD switch. Default remains `original`. It consumes the original authored models,
textures, material states and thresholds; no meshes are simplified or generated.
Original collision/vehicle physics and original selection functions are unchanged.

## Launch and API

Ignored snapshots and shortcuts: `local/task0056/delivery/`.
Use `Launch TrackViewer Godot HLOD.cmd`, `Launch Race Release Godot HLOD.cmd` or
`Launch Race Release Original LOD.cmd`. Resources are workspace junctions; these
are local working snapshots, not portable exports. Original assets/ELFs stay external.
CLI: `--track-lod-backend=original|godot_hlod`.

Common `Ps2TrackRenderer` methods (Debug and Release):

- `configure_lod_backend("godot_hlod")`: before loading only; returns ok/error/state.
- `get_lod_state()`: backend, resource slots, resident surface groups, missing model
  slots, load duration, per-view logical focal values and engine-global draw/memory counters.
- `get_lod_ranges(instance, view)`: on-demand anchor/section/model/availability and
  distance intervals. Host view0 is main;1..6 are separate capture faces. These are
  host pool keys, not original eView IDs.

Backend changes require clear/reload. Invalid values and combinations with experimental
instancing/occlusion/opaque policies fail atomically. Accepted resource/particle caches
remain available. Unsafe material sharing is not re-enabled.

## Data and execution

`port::track::StaticLodScene` describes immutable source slots and resource references.
Its indices borrow the owning immutable Scene's geometry/material/texture resources.
A static original-tree leaf order is retained, but Godot performs frustum/range culling.
`Runtime::prepare_hlod_frame` uses original VisibleZone/section selection and original
model/material/light/pass assembly, emitting **all** slots for candidate sections;
it does not call original DrawAScenery/TreeCull or choose a level from camera distance.
Original opaque texture grouping and alpha radix order are retained over this expanded
stream. Loading warm-up does not seed the live zone/texture-chain ordering state.

Main-view level nodes/materials are warmed incrementally during load. Capture views
reuse uploaded geometry/textures/shaders and allocate their own persistent nodes on
first scheduled capture. Thus the first capture can still hitch; it is not fully
prewarmed during loading. Nodes/resources are retained until track clear, with only
section activation, view-sensitive uniforms/facades, sorting and changed ranges updated.

For effective radius `R = source radius + 4`, logical focal H and integer threshold T,
the host upper distance is `R * (1 + H/max(2,T))`. Slots exceeding maximum projected
size H are unreachable. Earlier slots keep precedence; empty hashes are skipped;
missing Solid slots retain an explicit empty interval and diagnostic, rather than
silently drawing a lower model. Duplicate model slots keep independent ranges/facades.
The farthest interval also preserves the original size2 cutoff.

Godot4.4.1 uses transformed AABB center for visibility ranges. Each source instance's
levels use conservative bounds centered on one original anchor, including identity
world geometry, facades, depth proxies and extra environment passes. Inclusive range
endpoints are separated by one representable float (`nextafter`). No margins or
alpha fades are enabled. Main and capture projections configure independent intervals;
FOV changes update them without per-frame host level selection. Hidden section nodes
also refresh their cached range when they return after an FOV change.

Only compatible adjacent surfaces within one instance AND one level can merge. No
cross-anchor HLOD batches or section-wide MultiMesh are used. Boundaries/frustum tests
can differ from original PS2 behavior; this is the explicitly accepted tradeoff.

## Validation and limitations

Evidence E0056H01: `reverse/task0056/host.md`; original input E-0051-S01/S02.
Unit tests cover interval inversion, unreachable/overlapping/empty/duplicate/missing
slots, single-model cutoff, invalid inputs and no host distance selection. All30
catalogue assets are scanned for resource descriptors and view-specific interval validity.
GPU probes test engine-controlled near/far/near changes, actual track camera/FOV changes,
matching level centers and depth-proxy ranges, clear/invalid/reload, and seven distinct
driving/capture view policies. Existing source/physics preservation remains audited.

Resident level/triangle counts are **candidates**, not actual visible draw counts.
Performance uses actual engine draw/primitive/memory counters and wall-frame timing.
Godot Compatibility GPU timers may remain unavailable; do not infer GPU time by subtraction.

The first unmerged prototype had58470 main surface nodes on Track11. Restricting
compatible merges to a common anchor/level reduced that to32596; capture views add
more. More resident nodes/materials, initial capture work and loading costs can outweigh
removing HP2's cheap LOD test. This backend is experimental and is not promoted by its
successful rendering tests. No original-console Verified claim, complete game-world
restoration or guaranteed FPS improvement is made. TASK0052/53 gaps remain independent.

Final timing and delivery hashes are recorded in the task and ignored BUILD-INFO.json.

### Original texture binding dependency

All-level warm-up exposed an existing dependency omitted by the earlier port:
eSolid::FixTextureTable (Retail0010BDB8) requests default lookup; GetTextureInfo
(Retail001ABF48) uses the original DEFAULTTEXTURE hash19006835 when appropriate.
Fresh Retail/Debug bytes and original GLOBALB header confirm the rule. Restored
functions live in their original declaring scopes, with explicit resident-set carriers.
The loader decodes actual original pixels/mips only when needed and records source
NameHash aliases in Scene::texture_fixups; render-surface texture_hash identifies
the bound TextureInfo. Both backends receive this shared correction. No guessed or
generated replacement texture is used. A static model requesting an unsupported
original render-target binding remains an explicit NotImplemented path.

The initial host scan passed24 tracks and rejected61..66 for unresolved texture;
after this restoration all30 load/view/clear checks pass. Original missing model
references remain visible diagnostics with empty intervals; this does not establish
complete original world-resource recovery. Details: E0056T01 in reverse/task0056/textures.md.

Prewarm and cancellation preserve the visual frame counter; allocating thousands
of level nodes must not change the original reflection update phase. Native driving
and shader/effect algorithms remain separate from engine distance selection.

### Authored coverage limitation (Track11 sample)

E0056H02: of8405 non-sky instances,6922 have one distinct model reference and1483
have two. All24 two-model tuples are REDWOOD/SHRB/FORESTLINE resources. The50
RD_SECTION/ROADVENT tuples have one; sampled terrain/tunnel tuples also have one.
Thus HLOD does not simplify the road/terrain mesh: it only switches where the
original instance actually supplies alternatives. Section visibility and small-size
cutoff are different optimizations. This is one track, not a catalogue-wide claim.
