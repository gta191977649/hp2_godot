TASK-0076 completed: dynamic vehicle reflection capture defaults off; render gaps
retain loaded track/collision and report recoverable errors. Native/host/Vulkan
checks pass; normal Debug DLL rebuilt. Exact screenshot exception remains unknown.
See [evidence and continuation](../reverse/task0076/source-review.md).

TASK-0075 completed: TrackViewer retains default75/KEEP_HEIGHT browsing projection;
original Debug wheel smoke/tire-spew shares AcidFX resources, pools/RNG and track
rendering. Native/host/Vulkan checks and exact visual-on/off driving comparisons
pass. Full effect remainder and original certification remain explicit gaps.
See [wheel effects](DRIVING_EFFECTS.md) and [current physics gaps](DRIVING_GAPS.md).

TASK-0074 completed: TrackViewer Effects > Debug (Godot) > Wireframe debug
checkbox added, default off. Isolated Vulkan UI/pixel/depth/lifecycle check passes;
script-only host diagnostic, no native rebuild or original Verified claim.

TASK-0072 active: bulk depth staging/source-field tracking and per-view CPU math
workers implemented; native/build/source checks pass. Safe driving parity8759
checks passes, but consolidated driving/moving-rotation2-worker combination
regresses P95/P99 and stays off. Defaults0/Safe; no60FPS claim. User renewed
necessary Godot permission; no further window run is queued. Follow up export
boundary, worker granularity/compiler coverage and source lifecycle churn. See
[integration](RENDER_PREPARATION_TASK0072.md) and [results](PERFORMANCE_TASK0072.md).

TASK-0071 active: Windows Vulkan/Forward+ default and bounded profile startup work.
An owned benchmark DLL lock caused the reported link failure; stopping it restored
`./build.ps1` exit0. Subsequent tests use an isolated project. Track11 pixel/
COLCLAMP gaps and strict performance/PS2 acceptance remain open; see
[migration status and continuation](PERFORMANCE_TASK0071.md).

TASK-0070 completed: global profile_debug.gd plus live native ImGui graphs and function hotspots across frontend/viewer/game; pause/inspect/clear/export, actual game traces and source preservation pass. [Usage and timing contract](PROFILE_DEBUG.md).

TASK-0069 completed: default official Godot4.7.2, active API title and bounded engine timing diagnosis. Build/import, native/host checks, five GPU regressions and source preservation pass. [Measured bottlenecks and remaining limits](PERFORMANCE_TASK0069.md). Broader GPU optimization work remains active in TASK0068.

TASK-0067 completed: live Godot Triangles/primitives total beside TrackViewer draw calls; normal Debug build, UI/scene checks and source preservation pass.

TASK-0066 completed: matrix batching enabled globally; removed Track11 allowlist/config/diagnostic. Common state fallbacks remain. Track24 draw reductions764->266/799->231 confirmed; known colour discrepancy remains unresolved. [Current global policy](PERFORMANCE_TASK0066.md).

TASK-0065 completed: accepted Track11-only matrix batching (1080p dense totals2409->669 and3003->689), viewport diagnostics and snapshot export. Car consumers/other tracks retain baseline after failed experiments. [Results and limits](PERFORMANCE_TASK0065.md).

TASK-0064 completed: live Godot draw-call total below TrackViewer coordinates; normal Debug build and native UI check passed.

# Port progress

TASK-0073 completed: [automatic recovery tracking](AUTO_PROGRESS.md) now joins
registered Retail/Debug classes with current source and evidence, separating shells,
stubs and partial implementations. Start `./tools/track_progress.ps1` once for save
refresh; builds also refresh automatically. Native build, 22 tooling tests, both
generated checks and live MCP catalogs passed. Unknown totals and original-runtime
verification remain explicit; this task changes no recovered behavior.

Latest completed: [TASK-0063](../tasks/completed/TASK-0063-render-cost-analysis.md)
adds CPU-only30-track draw submission census, precise cost diagnostics and
default particle/postprocess redundant-work caches after bounded parity checks.
User replaced long FPS sampling with static tests/statistics. No60FPS or total
GPU draw-budget acceptance claim. [Analysis](PERFORMANCE_TASK0063.md).


Latest viewer control: [TASK-0062](../tasks/completed/TASK-0062-track-render-props.md)
adds default-on Visibility **Render props**. Native/settings, Debug build,
headless controls and actual ImGui off/on checks pass (1186 ->581 ->1186 draws).
Port-only transformed-scenery gate; no original-game verification.


Latest completed: [TASK-0061](../tasks/completed/TASK-0061-viewer-coordinates-dpi.md)
adds live camera HP2/Godot coordinates and shared ImGui/host text DPI scaling.
Track11 windows100/150/200percent, Debug/Release builds and input/lifecycle checks
PASS. Physical cross-monitor movement remains untested. See [UI_DPI.md](UI_DPI.md).

Latest completed: [TASK-0060](../tasks/completed/TASK-0060-track-memory.md) bounds
host track residency. Track11 Debug/Release private-byte peaks drop from about
4.14/4.13GiB to1.87/1.85GiB; native route hashes and scoped GPU pixels match.
Owned categories clear to zero; process retention is not fully attributed.
See [memory ownership and reproduction](TRACK_MEMORY.md). PC-only, not Verified.

Latest completed: [TASK-0059](../tasks/completed/TASK-0059-track-camera-disappearance.md) fixes total track disappearance during motion/rotation by isolating explicitly reported unsupported particle alpha packets; corrects active-atlas bounds and Reset Camera. Track11 Debug/Release delivery checks pass; high-alpha compositor remains a declared gap, not Verified. See [native rendering notes](NATIVE_TRACK_CULLING.md).

Latest investigation: [TASK-0057](../tasks/completed/TASK-0057-track-culling-generated-lod-research.md) completed; original hierarchical track culling confirmed and one-track Godot generated-LOD feasibility measured. See [findings and proposed strategy](TRACK_CULLING_AND_GENERATED_LOD.md). No default change.


Latest completed: [TASK-0055](../tasks/completed/TASK-0055-godot-render-optimization.md)
adds accepted host cache/particle-buffer reuse to the common renderer. Full30-track
Release1080p120/600/3 paired gate PASS: frame time-10.03%, host submit-33.16%; no
persistent wall median/P95/P99 regression, native/effect/draw inputs unchanged.
Debug118/118 and Release98/98 PASS. Safe profile is now default; experimental
MultiMesh/occlusion/opaque and unsafe material sharing stay off. See
[performance results](PERFORMANCE_TASK0055.md). Loading stalls and TASK0052 effect
recovery/TASK0053 repeatability gaps remain; strict60FPS is deferred. Next action:
TASK0053 loading/driver profiling, preserving native rendering/effect rules.

A PS2-to-PC native port of *Need for Speed: Hot Pursuit 2* (`SLUS_203.62`). This file is the
**tracker**: where recovery stands, what is blocking it, and what to do next. It is the file
`AGENTS.md`'s startup protocol means.

The tables below are generated from the repository's own records by
`reverse/scripts/generate_progress.py`, in the spirit of gta-reversed's
`docs/ReversedClasses.md` â€” a number written by hand goes stale without anyone noticing, and a
progress figure nobody can reproduce is worse than none. Regenerate after changing records,
tasks or `not_implemented` sites:

TASK-0073 adds [automatic save tracking](AUTO_PROGRESS.md): run
`./tools/track_progress.ps1` once to watch relevant inputs. Builds refresh once as
well. Class coverage now joins current Retail/Debug registries and source evidence;
empty shells are not implementation credit. Use `./tools/track_progress.ps1 -Check`
for a deterministic offline freshness check.

```bash
py -3 reverse/scripts/recover_classes.py --emit    # after any class-recovery change
py -3 reverse/scripts/generate_class_templates.py --write # after class/member or template changes
py -3 reverse/scripts/generate_progress.py
```

The session-by-session narrative â€” what each session looked at, found, got wrong and corrected â€”
lives in **[PROGRESS_LOG.md](PROGRESS_LOG.md)**. Nothing was deleted when this file was split
out of it.

## Disclaimers â€” read before quoting any number here

**No part of this port is verified against the original.** `Verified` requires a recorded
comparison against PS2 hardware or PCSX2 over a stated scope (AGENT.md R1). That comparison has
not been performed, so the ladder's top rung is empty and every percentage below describes *how
much has been read and translated*, not *how much is correct*. Host tests passing is not
evidence of equivalence, and the tables must never be read as if it were.

**The denominator is the whole ELF**, library and CRT code included, so the headline percentage
understates progress on the parts that matter and will never reach 100%. It is a floor on what
is left, not a completion estimate.

**Records are not uniform depth.** A `Decompiled` row means one function's behaviour was read
and written down; it says nothing about its callees, its structure layouts or its globals. A
subsystem with many records can still be unusable.

**The untranslated-boundary count moves in both directions.** Reversing a function usually
reveals further untranslated dependencies, so that number rising is normal progress, not
regression.

## Recovery

Two views of the same work: **[Classes](#classes)** below is the per-class breakdown -- every
class name the binary gives up, its member functions, and how far each has got. The function
totals here are the whole-image counts behind it.

<!-- generated:stats:begin -->
<!-- Generated by reverse/scripts/generate_progress.py on 2026-10-20. Do not edit by hand. -->

### Functions with a record: 1653 of 7544 in the ELF [22%]

Binary: `7335b8ed4794…` (unknown executable build; SYSTEM.CNF VER 1.00), `815b60c7f473…` (2002-07-08 Debug Build user label; embedded date not independently confirmed).

The denominator is every function `docs/assessment/FUNCTION_INVENTORY.csv` found in the ELF,
including library and CRT code the port will never need, so treat it as a floor on how much is
left rather than a measure of how much matters. Read the disclaimers above before quoting either
number.

| Status | Records | Share of records | Meaning |
|---|---|---|---|
| Verified | 0 | 0.0% | compared against the original — **nothing is here, by rule** |
| Tested | 117 | 7.1% | translated and covered by host tests |
| Implemented | 93 | 5.6% | translated into `src/`, with the ELF address on the code |
| Decompiled | 388 | 23% | behaviour read out of the ELF and written down |
| Identified | 1054 | 64% | address and role recorded; no decompilation kept |
| _off-ladder_ | 1 | 0.1% | status values not on the ladder: Reconstructed |

#### By subsystem

| Subsystem | Records | Verified | Tested | Implemented | Decompiled | Identified |
|---|---|---|---|---|---|---|
| audio | 44 | 0 | 0 | 0 | 44 | 0 |
| containers | 17 | 0 | 0 | 0 | 6 | 11 |
| driving | 129 | 0 | 91 | 37 | 1 | 0 |
| engine | 6 | 0 | 3 | 0 | 3 | 0 |
| frontend | 289 | 0 | 8 | 37 | 243 | 1 |
| input | 1 | 0 | 0 | 0 | 1 | 0 |
| memory | 6 | 0 | 0 | 0 | 6 | 0 |
| memorycard | 5 | 0 | 0 | 0 | 5 | 0 |
| platform | 1 | 0 | 0 | 0 | 1 | 0 |
| police | 8 | 0 | 2 | 0 | 6 | 0 |
| reference | 704 | 0 | 0 | 0 | 3 | 701 |
| startup | 4 | 0 | 0 | 0 | 4 | 0 |
| track | 15 | 0 | 0 | 14 | 0 | 0 |
| track_environment | 36 | 0 | 0 | 0 | 36 | 0 |
| unknown | 347 | 0 | 3 | 3 | 0 | 341 |
| vehicle | 41 | 0 | 10 | 2 | 29 | 0 |
| **total** | **1653** | **0** | **117** | **93** | **388** | **1054** |

Structure layout records: **16**, against 1653 function records -- roughly 103 to one. That
ratio is the honest shape of the gap: a translated function whose structure layout is unknown is
still guesswork at the field level. 8 of them still sit directly in `reverse/structures/` rather
than the `<binary-id>/<subsystem>/` layout AGENT.md R6 specifies.

#### Other binary records (separate denominators)

| Binary SHA-256 | Records | Verified | Tested | Implemented | Decompiled | Identified |
|---|---|---|---|---|---|---|
| `815b60c7f473…` | 385 | 0 | 0 | 297 | 86 | 0 |

Debug driving records are counted separately. They do not establish Retail equivalence or original-runtime verification.

<!-- generated:stats:end -->

## Classes

<!-- generated:classes:begin -->
<!-- Generated by reverse/scripts/generate_progress.py on 2026-10-20. Do not edit by hand. -->

### Automatic class and function coverage

Generated from current registries, address-anchored definitions and evidence records. All method totals are unknown; percentages cover bound methods only. An evidenced body is a recorded implementation claim, not proof of complete behavior. Partial counts detect direct gaps only; transitive dependencies and unsupported C++ syntax require review.

[Machine-readable report](../reverse/progress.json) · [Watcher and MCP usage](AUTO_PROGRESS.md)

#### `sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`

398 registered classes; 105 have no bound methods. 1056 unique bound entries (shared entries counted once).

| Missing/unresolved | Throwing stub | Unconfirmed body | Partial | Evidenced body | Host-test claims | Original-verification claims |
|---|---|---|---|---|---|---|
| 85 | 895 | 43 | 6 | 27 | 62 | 0 |

<details>
<summary>All registered classes</summary>

| Class | Bound | Missing | Stub | Unconfirmed | Partial | Evidenced | Evidenced / bound |
|---|---|---|---|---|---|---|---|
| `AIActionAccidentRecovery` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `AIActionAvoidCars` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `AIActionAvoidRoadBlock` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `AIActionBlock` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `AIActionDraft` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `AIActionGetUnstuck` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `AIActionIJustGotHit` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `AIActionJustDrive` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `AIActionNudgeCar` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `AIActionPass` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `AIActionPullOver` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `AICarController` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `AICarSound` | 14 | 0 | 14 | 0 | 0 | 0 | 0.0% |
| `AIDriver` | 17 | 0 | 9 | 8 | 0 | 0 | 0.0% |
| `AIEngineSound` | 8 | 0 | 8 | 0 | 0 | 0 | 0.0% |
| `AcidEffectSystem` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `Aerodynamics` | 3 | 3 | 0 | 0 | 0 | 0 | 0.0% |
| `AnimationGroup` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `AsyncChunkPri` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `AsyncEntry` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `AudioOptionsScreen` | 5 | 0 | 5 | 0 | 0 | 0 | 0.0% |
| `Autosaver` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `Average` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `AverageInRealController` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `BaseAICarSound` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `BaseCarSound` | 7 | 0 | 7 | 0 | 0 | 0 | 0.0% |
| `BasePlayerCarSound` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `BaseTireSound` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `BeACopChallengeSetup` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `BeACopResults1P` | 6 | 0 | 4 | 2 | 0 | 0 | 0.0% |
| `BeACopResults2P` | 6 | 0 | 4 | 2 | 0 | 0 | 0.0% |
| `BeTheCopSetup` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `BeltMover` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `BestLapTimes` | 6 | 0 | 4 | 2 | 0 | 0 | 0.0% |
| `BestTotalTimes` | 6 | 0 | 4 | 2 | 0 | 0 | 0.0% |
| `BodyDamage` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `BootMemcardLoader` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `BumperCameraMover` | 16 | 0 | 5 | 11 | 0 | 0 | 0.0% |
| `CD Directory` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `CDBFileSys` | 43 | 0 | 43 | 0 | 0 | 0 | 0.0% |
| `CameraDataItems` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `Car` | 8 | 4 | 4 | 0 | 0 | 0 | 0.0% |
| `Car (cop)` | 40 | 9 | 25 | 6 | 0 | 0 | 0.0% |
| `CarCollisionBody` | 18 | 0 | 18 | 0 | 0 | 0 | 0.0% |
| `CarCollisionPredict` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `CarController` | 5 | 0 | 5 | 0 | 0 | 0 | 0.0% |
| `CarDamage` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `CarDriver` | 11 | 0 | 11 | 0 | 0 | 0 | 0.0% |
| `CarList` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `CarLoader` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `CarMover` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `CarPartCuller` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `CarRenderInfo` | 2 | 0 | 0 | 2 | 0 | 0 | 0.0% |
| `CarRenderInfo Saved Alpha Bits` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `CarSelector` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `CarState` | 7 | 5 | 2 | 0 | 0 | 0 | 0.0% |
| `CarTypeInfo` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `ChangeableMenuItem` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `Chopper` | 5 | 0 | 5 | 0 | 0 | 0 | 0.0% |
| `ChopperSound` | 8 | 0 | 8 | 0 | 0 | 0 | 0.0% |
| `Collision` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `CollisionBody` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `CollisionBodyNumberAllocator` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `CollisionBodyTable` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `CollisionBodyWitnessBitField` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `CopDriver` | 38 | 0 | 31 | 6 | 0 | 1 | 2.6% |
| `CopFillerSpeech` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `CopLocationFillerClass` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `CopManager` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `CopMover` | 5 | 0 | 4 | 0 | 0 | 1 | 20.0% |
| `CopSpeedFillerClass` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `CopTacticMiscFillerClass` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `CopsOffModeSelect` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `CopsOnModeSelect` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `CountdownCameraCoordinator` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `Credits movie player` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `CreditsLine` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `CreditsScroller` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `CubicCameraMover` | 16 | 0 | 4 | 12 | 0 | 0 | 0.0% |
| `CyclicBuffer` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `DCTImage` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `DCTTable` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `DMAC` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `DataBeTheCopSetup` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `DataEventSelectScreen` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `DataLapKnockoutSetup` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `DataRaceKnockoutSetup` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `DataSingleRaceSetup` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `DebugPauseMenu` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `DebugWorldCameraMover` | 16 | 0 | 5 | 11 | 0 | 0 | 0.0% |
| `DeliveryMover` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `DeltaLineSPeed Average` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `DemoCameraCoordinator` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `DemoDiscManager` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `DemoModeReplay` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `DriveCameraMover` | 17 | 0 | 12 | 5 | 0 | 0 | 0.0% |
| `DriveCameraMover(DCC)` | 16 | 0 | 11 | 5 | 0 | 0 | 0.0% |
| `DriveCameraMover(NisCC)` | 16 | 0 | 11 | 5 | 0 | 0 | 0.0% |
| `DriveCameraMover(Player)` | 16 | 0 | 11 | 5 | 0 | 0 | 0.0% |
| `DriveTarget` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `DriveTrain` | 9 | 9 | 0 | 0 | 0 | 0 | 0.0% |
| `DrivingChoice` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `DrivingInformation` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `DrivingLocation` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `DroppedSpikes` | 7 | 0 | 7 | 0 | 0 | 0 | 0.0% |
| `DuplicatedSolid` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `DuplicatedStripEntryData` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `DuplicatedStripEntryTable` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `DynamicCollisionBody` | 14 | 0 | 14 | 0 | 0 | 0 | 0.0% |
| `EndGameMenu` | 7 | 0 | 5 | 2 | 0 | 0 | 0.0% |
| `EndGameMenuItem` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `EndGameMenusBase` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `EndGameScreens` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `Engine` | 5 | 5 | 0 | 0 | 0 | 0 | 0.0% |
| `EventInfo` | 10 | 0 | 10 | 0 | 0 | 0 | 0.0% |
| `EventKoState` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `EventLeaf` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `EventRankState` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `EventRewardScreen` | 6 | 0 | 4 | 2 | 0 | 0 | 0.0% |
| `EventTimeState` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `EventTournamentState` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `EventTree` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `Events Unlocked` | 6 | 2 | 2 | 2 | 0 | 0 | 0.0% |
| `FMenus Highlight Animation` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `FancyJoyEventHandler` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `FernShakerInfoTable` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `FlailerBitmapChunk` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `FlailerMenu` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `FlailerObjectChunk` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `FlailerRenderManager` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `FlailerScreenList` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `FlailerScreenNode` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `FlailerTextChunk` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `Font` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `FreeRunSetup` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `FrontendDatabase` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `FrontendPlayerInfo` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `FuturePoints` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `GarageCameraMover` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `GlobalBackgroundAnimation` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `GrandSceneryCullInfo` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `Graph` | 1 | 1 | 0 | 0 | 0 | 0 | 0.0% |
| `HeliPoly` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `HeliSheet` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `HelpAlphaAnimation` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `HornSound3d` | 5 | 0 | 5 | 0 | 0 | 0 | 0.0% |
| `HudAnimator` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `HudTexturePack` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `Huffman` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `Joylog from Codeine` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `Joystick` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `JoystickCalibrationScreen` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `JoystickPreconfig` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `JukeBoxMusicScreen` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `LanguageBuffer` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `LanguageManager` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `LanguageOptionsScreenBase` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `LapKnockoutSetup` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `LapTimesDisplay` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `Linear` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `LoadedSkin` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `LoadedTextureData` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `MV3Codec` | 7 | 0 | 7 | 0 | 0 | 0 | 0.0% |
| `MaxCameraMover` | 16 | 0 | 4 | 12 | 0 | 0 | 0.0% |
| `Mellovator` | 9 | 0 | 9 | 0 | 0 | 0 | 0.0% |
| `MellowMover` | 5 | 0 | 5 | 0 | 0 | 0 | 0.0% |
| `MemcardLoader` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `MemoryPool` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `MumboJumboScreen` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `Munition` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `MunitionEffectsHandler` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `NIS PreviewRoute` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `NameEntry` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `NearbyCarList` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `NearestRoutePointInfo` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `NewMainMenu` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `NisCameraCoordinator` | 5 | 0 | 5 | 0 | 0 | 0 | 0.0% |
| `NisManager` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `NullController` | 8 | 0 | 7 | 1 | 0 | 0 | 0.0% |
| `NullDriver` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `NullMover` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `NumberAllocator` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `OptionsScreenBase` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `OrbitCarCameraMover` | 16 | 0 | 7 | 9 | 0 | 0 | 0.0% |
| `PauseAudioOpts` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `PauseDisplayOpts` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `PauseMenu` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `PauseOptions` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `PauseRaceObjective` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `PauseViewControls` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `Phsycho Body Pid` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `Phsycho Heading Pid` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `Phsycho Velocity Pid` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `PhysicsCar` | 23 | 18 | 3 | 1 | 1 | 0 | 0.0% |
| `PhysicsComponent` | 1 | 1 | 0 | 0 | 0 | 0 | 0.0% |
| `PhysicsSystem` | 7 | 2 | 0 | 0 | 3 | 2 | 28.6% |
| `PhysicsVehicle` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `PlayerCarSound` | 14 | 0 | 14 | 0 | 0 | 0 | 0.0% |
| `PlayerEngineSound` | 9 | 0 | 9 | 0 | 0 | 0 | 0.0% |
| `PlayerTireSound` | 10 | 0 | 10 | 0 | 0 | 0 | 0.0% |
| `Points Rewards` | 6 | 2 | 2 | 2 | 0 | 0 | 0.0% |
| `PreRace2PBeACop` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `PreRaceBlurb` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `PreRaceStats` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `PreviewPause` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `PreviewPausePoint` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `Progress Average` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `PsychoDriver` | 34 | 0 | 28 | 6 | 0 | 0 | 0.0% |
| `QueuedJoystickEvents` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `QuickSpline` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `QuickSplineInRealController` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `Race` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `RaceCoordinator` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `RaceEvent` | 7 | 0 | 7 | 0 | 0 | 0 | 0.0% |
| `RaceParameters` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `RaceResultsScreen` | 6 | 0 | 6 | 0 | 0 | 0 | 0.0% |
| `RaceSetupBaseClass` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `RacingCar` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `RainSplash` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `RealController` | 14 | 0 | 14 | 0 | 0 | 0 | 0.0% |
| `RealDriver` | 30 | 4 | 20 | 6 | 0 | 0 | 0.0% |
| `RearViewMirrorCameraMover` | 16 | 0 | 4 | 12 | 0 | 0 | 0.0% |
| `ReplayLogBuffer` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `ReplayManager` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `ReplayScreen` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `ReplaySnapshot` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `ResourceFile` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `Reward Car Model` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `RewardTrackModel` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `RigidBody` | 11 | 3 | 0 | 2 | 0 | 6 | 54.5% |
| `RingBuffer` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `Rocket` | 7 | 0 | 7 | 0 | 0 | 0 | 0.0% |
| `ScreenStack` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `SelectCarScreen` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `SelectTrackScreen` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `SelectableTrack` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `SelectableTrackManager` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `SelectionSet` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `SingleRaceSetup` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `SirenSound` | 8 | 0 | 8 | 0 | 0 | 0 | 0.0% |
| `SirenSound3d` | 5 | 0 | 5 | 0 | 0 | 0 | 0.0% |
| `SkidSegment` | 3 | 3 | 0 | 0 | 0 | 0 | 0.0% |
| `SlotPool` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `SlotPoolManager` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `SmokeBomb` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `Smokeable` | 18 | 0 | 18 | 0 | 0 | 0 | 0.0% |
| `Sound3d` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `Sound3dSample` | 6 | 0 | 6 | 0 | 0 | 0 | 0.0% |
| `SoundArea` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `Spline` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `StaticCollisionBody` | 20 | 0 | 20 | 0 | 0 | 0 | 0.0% |
| `StillCameraMover` | 16 | 0 | 3 | 13 | 0 | 0 | 0.0% |
| `Stomper Stack` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `Suspension` | 5 | 0 | 0 | 0 | 2 | 3 | 60.0% |
| `Table` | 4 | 3 | 0 | 0 | 0 | 1 | 25.0% |
| `Task Rewards` | 6 | 2 | 2 | 2 | 0 | 0 | 0.0% |
| `Temp Fred For Load` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `TerrainEffectEmitters` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `TextureInfo` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `TexturePack` | 6 | 0 | 6 | 0 | 0 | 0 | 0.0% |
| `TextureTable` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `TimeAroundTrackTable` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `TimeOfDayTrack` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `Timer` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `TitleScreen` | 5 | 0 | 1 | 4 | 0 | 0 | 0.0% |
| `TopologyCoordinate` | 1 | 0 | 0 | 1 | 0 | 0 | 0.0% |
| `TopologyTree` | 6 | 0 | 2 | 4 | 0 | 0 | 0.0% |
| `TourneyPoints` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `TourneyStandingsScreen` | 6 | 0 | 4 | 2 | 0 | 0 | 0.0% |
| `TrackCarCameraMover` | 17 | 0 | 6 | 11 | 0 | 0 | 0.0% |
| `TrackFlyCameraMover` | 17 | 0 | 6 | 11 | 0 | 0 | 0.0% |
| `TrackKnockoutSetup` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `TrackMap` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `TrackMarkerFile` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `TrackPathCoordinate` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `TrackPathManager` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `TrackPolygon` | 1 | 0 | 0 | 1 | 0 | 0 | 0.0% |
| `TrackPolygonCollisionBody` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `TrackRecordDisplayer` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `TrackRoute` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `TrackRouteEditor` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `TrackRouteEditorCameraMover` | 16 | 0 | 3 | 13 | 0 | 0 | 0.0% |
| `TrackRouteManager` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `TrackRoutePoint` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `TrackedController` | 9 | 0 | 9 | 0 | 0 | 0 | 0.0% |
| `TrafficCar` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `TrafficDriver` | 17 | 0 | 9 | 8 | 0 | 0 | 0.0% |
| `TrafficTeleporter` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `TransientStats` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `UserNameOptionsScreen` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `VCEdge` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `VCFace` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `VCPolyhedron` | 8 | 0 | 8 | 0 | 0 | 0 | 0.0% |
| `VCTransform` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `VCTransformedFeature` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `VCVertex` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `VCVoronoiClip` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `VCWitness` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `Very Stuck Progress Average` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `VifController` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `VisibleCar` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `VisibleDrivingLine` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `VisibleZone` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `Wheel` | 20 | 6 | 0 | 0 | 1 | 13 | 65.0% |
| `WheelDevice` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `World` | 5 | 0 | 5 | 0 | 0 | 0 | 0.0% |
| `ZoneFreezeCameraMover` | 16 | 0 | 3 | 13 | 0 | 0 | 0.0% |
| `ZonePreview Emitters` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `ZonePreviewCameraMover` | 16 | 0 | 6 | 10 | 0 | 0 | 0.0% |
| `actionHelicopter_t` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `bEffect` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `bFile` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `bList` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `bRepeat` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `bSample` | 16 | 0 | 16 | 0 | 0 | 0 | 0.0% |
| `bSampleStream` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `bSegment` | 5 | 0 | 5 | 0 | 0 | 0 | 0.0% |
| `bSound` | 7 | 0 | 7 | 0 | 0 | 0 | 0.0% |
| `bVoice` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `cDBFileSys` | 19 | 0 | 19 | 0 | 0 | 0 | 0.0% |
| `cDBFilesBase` | 24 | 0 | 24 | 0 | 0 | 0 | 0.0% |
| `cJukeboxPlayer` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `cMC` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `cPlayerSettings` | 6 | 0 | 6 | 0 | 0 | 0 | 0.0% |
| `cPoint` | 22 | 0 | 22 | 0 | 0 | 0 | 0.0% |
| `cSEDWedge` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `eAnimation` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `eAnimationSolid` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `eDmaChain` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `eFrameMallocBuffer0` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `eFrameMallocBuffer1` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `eModel` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `ePoly` | 1 | 1 | 0 | 0 | 0 | 0 | 0.0% |
| `eRigidAnimation` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `eSolid` | 6 | 0 | 6 | 0 | 0 | 0 | 0.0% |
| `eStripData` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `eStripDataE` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `eTextureDownloader` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `eTransferRectQueue` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `eView` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `feAnimAlphaElement` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `fragmentedVector<transition>` | 1 | 1 | 0 | 0 | 0 | 0 | 0.0% |
| `fsm` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `kAITireSound` | 13 | 0 | 12 | 1 | 0 | 0 | 0.0% |
| `mExitScreenAnimation` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `m_ptr_SOUND_Memory_EE` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `nfsFsm` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `pCarTexturePack` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `pDynamicTexturePack` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `pInitialSpotlightAlphas` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `pInitialSpotlightColours` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `pTrackSelectTexturePack` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `rotorChatterAction_t` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `rotorLocationAction_t` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `rotorSpikeFailedAction_t` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `rotorTrafficAction_t` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `speech::state_t` | 2 | 2 | 0 | 0 | 0 | 0 | 0.0% |
| `tAICop` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `tAnimatedBlurb` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `tCarLogoAnimation` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `tCopManager` | 12 | 0 | 12 | 0 | 0 | 0 | 0.0% |
| `tCubic1D` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `tCubic2D` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `tCubic3D` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `tCustomizeHUDScreen` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `tDisplayOptionsScreen` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `tEnvelope` | 3 | 0 | 3 | 0 | 0 | 0 | 0.0% |
| `tEventGroup` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `tEventLeaf` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `tEventTreeView` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `tExitScreenAnimation` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `tGlobalBackgroundAnimation` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `tGlobalDisplaySettings` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `tHelpMenu` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `tKeyboard` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `tLeftRightArrowScroll` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `tLineSegment` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `tMainMenuTrackDesc` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `tMenuAnimElement` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `tNearFollowTarget` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `tOptionsScreen` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `tParallaxer` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `tPointsThingy` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `tPopupMenu` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `tPursuit` | 4 | 0 | 4 | 0 | 0 | 0 | 0.0% |
| `tRandomlyPickableTrackInfo` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `tScrollingMenuList` | 5 | 0 | 5 | 0 | 0 | 0 | 0.0% |
| `tSlideAnimation` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `tSpikeStrip` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `tStartRaceScreenManager` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `tStatsColumn` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `tTotalTimeRecord` | 0 | 0 | 0 | 0 | 0 | 0 | unknown |
| `tTrackGraph` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `tTrackLogoAnimation` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `tTreeReward` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `tUnlockMap` | 1 | 0 | 1 | 0 | 0 | 0 | 0.0% |
| `tWordQueue` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |
| `vAABB` | 2 | 0 | 2 | 0 | 0 | 0 | 0.0% |

</details>

#### `sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`

21 registered classes; 0 have no bound methods. 394 unique bound entries (shared entries counted once).

| Missing/unresolved | Throwing stub | Unconfirmed body | Partial | Evidenced body | Host-test claims | Original-verification claims |
|---|---|---|---|---|---|---|
| 331 | 0 | 0 | 15 | 48 | 0 | 0 |

<details>
<summary>All registered classes</summary>

| Class | Bound | Missing | Stub | Unconfirmed | Partial | Evidenced | Evidenced / bound |
|---|---|---|---|---|---|---|---|
| `Aerodynamics` | 4 | 3 | 0 | 0 | 0 | 1 | 25.0% |
| `Car` | 82 | 75 | 0 | 0 | 3 | 4 | 4.9% |
| `CarDriver` | 2 | 2 | 0 | 0 | 0 | 0 | 0.0% |
| `CarState` | 18 | 12 | 0 | 0 | 0 | 6 | 33.3% |
| `DriveTrain` | 38 | 37 | 0 | 0 | 0 | 1 | 2.6% |
| `Engine` | 34 | 33 | 0 | 0 | 0 | 1 | 2.9% |
| `Graph` | 3 | 3 | 0 | 0 | 0 | 0 | 0.0% |
| `PhysicsCar` | 16 | 2 | 0 | 0 | 4 | 10 | 62.5% |
| `PhysicsComponent` | 4 | 3 | 0 | 0 | 0 | 1 | 25.0% |
| `PhysicsSystem` | 11 | 9 | 0 | 0 | 0 | 2 | 18.2% |
| `RealDriver` | 10 | 0 | 0 | 0 | 5 | 5 | 50.0% |
| `ReplayManager` | 1 | 0 | 0 | 0 | 0 | 1 | 100.0% |
| `RigidBody` | 46 | 44 | 0 | 0 | 0 | 2 | 4.3% |
| `SkidMaker` | 2 | 0 | 0 | 0 | 0 | 2 | 100.0% |
| `SkidSegment` | 3 | 0 | 0 | 0 | 0 | 3 | 100.0% |
| `SkidSet` | 6 | 0 | 0 | 0 | 1 | 5 | 83.3% |
| `Suspension` | 29 | 29 | 0 | 0 | 0 | 0 | 0.0% |
| `Table` | 5 | 4 | 0 | 0 | 0 | 1 | 20.0% |
| `TwoWheelAckermanSteering` | 7 | 5 | 0 | 0 | 0 | 2 | 28.6% |
| `Wheel` | 71 | 70 | 0 | 0 | 1 | 0 | 0.0% |
| `ePoly` | 2 | 0 | 0 | 0 | 1 | 1 | 50.0% |

</details>

244 bound entries need claim/source review; details are in the JSON report. No record status is automatically promoted or downgraded.

<!-- generated:classes:end -->

## The frontier

<!-- generated:boundaries:begin -->
<!-- Generated by reverse/scripts/generate_progress.py on 2026-10-20. Do not edit by hand. -->

### Explicit untranslated boundaries: 1272 call sites, 1046 distinct ELF addresses

Each row is a recovered path that is reached but not translated, and that therefore throws
`hp2::NotImplemented` naming its ELF address (AGENT.md R3). This is the work queue: a path
leaves this table by being reversed, never by being given a default.

<details>
<summary>See all call sites</summary>

| ELF address | What is missing | Call site |
|---|---|---|
| `(runtime value: address)` | Back settings comparison state missing | [src/game_core/indep/src/frontendscreens/options.cpp:166](../src/game_core/indep/src/frontendscreens/options.cpp:166) |
| `(runtime value: address)` | Required mover external service is absent | [src/game_core/port/driving/projection_support.hpp:51](../src/game_core/port/driving/projection_support.hpp:51) |
| `(runtime value: address)` | nonfinite/out-of-range EE skid conversion | [src/game_core/indep/src/skids.cpp:20](../src/game_core/indep/src/skids.cpp:20) |
| `(runtime value: address.c_str())` | screen item activation handler not translated | [src/port/runtime/frontend_runtime.cpp:482](../src/port/runtime/frontend_runtime.cpp:482) |
| `(runtime value: hex(address).c_str())` | activation side-effect dependency not translated | [src/port/runtime/frontend_runtime.cpp:457](../src/port/runtime/frontend_runtime.cpp:457) |
| `(runtime value: instruction_address)` | Unsupported operand outside port-side zero-denominator EE DIV.S projection | [src/game_core/port/driving/ee_scalar.hpp:12](../src/game_core/port/driving/ee_scalar.hpp:12) |
| `0x00100A50` | TextureInfo::GetScroll (signature and full body unresolved) | [src/game_core/src/texturee.cpp:13](../src/game_core/src/texturee.cpp:13) |
| `0x00100AF8` | TextureInfo::GetAlphaChannel (signature and full body unresolved) | [src/game_core/src/texturee.cpp:18](../src/game_core/src/texturee.cpp:18) |
| `0x00100B98` | TextureInfo::GetColourChannel (signature and full body unresolved) | [src/game_core/src/texturee.cpp:23](../src/game_core/src/texturee.cpp:23) |
| `0x00100BF0` | TextureInfo::Print (signature and full body unresolved) | [src/game_core/src/texturee.cpp:28](../src/game_core/src/texturee.cpp:28) |
| `0x001010A0` | VifController::Reset (signature and full body unresolved) | [src/game_core/src/dmadump.cpp:30](../src/game_core/src/dmadump.cpp:30) |
| `0x00101948` | DMAC::DumpGifTag (signature and full body unresolved) | [src/game_core/src/dmadump.cpp:13](../src/game_core/src/dmadump.cpp:13) |
| `0x00101BB8` | DMAC::dmaTransferAddress (signature and full body unresolved) | [src/game_core/src/dmadump.cpp:18](../src/game_core/src/dmadump.cpp:18) |
| `0x00102028` | DMAC::FastSync (signature and full body unresolved) | [src/game_core/src/dmadump.cpp:23](../src/game_core/src/dmadump.cpp:23) |
| `0x00103FF0` | eSolid::FixStripEntryTable (signature and full body unresolved) | [src/game_core/src/ecstasy/ecstasye.cpp:62](../src/game_core/src/ecstasy/ecstasye.cpp:62) |
| `0x00104070` | eSolid::SetEnvironmentMapping (signature and full body unresolved) | [src/game_core/src/ecstasy/ecstasye.cpp:67](../src/game_core/src/ecstasy/ecstasye.cpp:67) |
| `0x001047E0` | eDmaChain::Init (signature and full body unresolved) | [src/game_core/src/ecstasy/ecstasye.cpp:34](../src/game_core/src/ecstasy/ecstasye.cpp:34) |
| `0x001057D8` | Scenery model references an original render-target texture; host binding not restored | [src/game_core/port/track/assets.cpp:255](../src/game_core/port/track/assets.cpp:255) |
| `0x001058D8` | Skid draw target alpha test outside the normal recovered target | [src/platform/godot_bridge/ps2_driving_runtime.cpp:179](../src/platform/godot_bridge/ps2_driving_runtime.cpp:179) |
| `0x00107410` | nonfinite or out-of-range EE integer conversion | [src/game_core/src/ecstasy/ecstasye.cpp:159](../src/game_core/src/ecstasy/ecstasye.cpp:159) |
| `0x001075B8` | eView::Sub_001075B8 (signature and full body unresolved) | [src/game_core/src/ecstasy/ecstasye.cpp:113](../src/game_core/src/ecstasy/ecstasye.cpp:113) |
| `0x00107600` | eView::Sub_00107600 (signature and full body unresolved) | [src/game_core/src/ecstasy/ecstasye.cpp:118](../src/game_core/src/ecstasy/ecstasye.cpp:118) |
| `0x001076F8` | eView::Sub_001076F8 (signature and full body unresolved) | [src/game_core/src/ecstasy/ecstasye.cpp:123](../src/game_core/src/ecstasy/ecstasye.cpp:123) |
| `0x00108AE0` | Skid GS alpha above128 requires destination-read rendering | [src/platform/godot_bridge/ps2_driving_runtime.cpp:175](../src/platform/godot_bridge/ps2_driving_runtime.cpp:175) |
| `0x00108AE0` | Skid TEX1 outside the recovered MXL0/MMAG1/MMIN3 slice | [src/platform/godot_bridge/ps2_driving_runtime.cpp:171](../src/platform/godot_bridge/ps2_driving_runtime.cpp:171) |
| `0x00108AE0` | Skid texture TFX/ALPHA outside the recovered host copy/source-over slice | [src/platform/godot_bridge/ps2_driving_runtime.cpp:169](../src/platform/godot_bridge/ps2_driving_runtime.cpp:169) |
| `0x00108CC0` | GS source-over alpha above128 needs a destination-read renderer; select explicit unlit inspection to inspect geometry | [src/platform/godot_bridge/ps2_vehicle_renderer.cpp:566](../src/platform/godot_bridge/ps2_vehicle_renderer.cpp:566) |
| `0x00108CC0` | Track TEX1 outside bounded original register fields | [src/game_core/port/track/texture_state.hpp:17](../src/game_core/port/track/texture_state.hpp:17) |
| `0x00108CC0` | Vehicle TEX1 outside MXL0/MMAG1/MMIN3/L0/K0 | [src/port/vehicle/texture_state.hpp:51](../src/port/vehicle/texture_state.hpp:51) |
| `0x00108CC0` | Vehicle static ALPHA equation other than copy/source-over | [src/port/vehicle/texture_state.hpp:53](../src/port/vehicle/texture_state.hpp:53) |
| `0x00108CC0` | Vehicle texture TFX other than MODULATE | [src/port/vehicle/texture_state.hpp:48](../src/port/vehicle/texture_state.hpp:48) |
| `0x00109260` | eTextureDownloader::Init (signature and full body unresolved) | [src/game_core/src/ecstasy/ecstasye.cpp:94](../src/game_core/src/ecstasy/ecstasye.cpp:94) |
| `0x0010AC50` | Environment packet31C destination-alpha composition is not implemented by the source-alpha vehicle shader | [src/game_core/port/track/reflection.hpp:20](../src/game_core/port/track/reflection.hpp:20) |
| `0x0010AC50` | Unsupported GS blend equation | [src/game_core/port/track/effects.hpp:154](../src/game_core/port/track/effects.hpp:154) |
| `0x0010AC50` | Unsupported GS integer blend equation | [src/game_core/port/track/effects.hpp:163](../src/game_core/port/track/effects.hpp:163) |
| `0x0010BD38` | eSolid::GetBoundingBox (signature and full body unresolved) | [src/game_core/src/ecstasy/ecstasye.cpp:72](../src/game_core/src/ecstasy/ecstasye.cpp:72) |
| `0x0010C158` | eSolid::FixStripSTs (signature and full body unresolved) | [src/game_core/src/ecstasy/ecstasye.cpp:77](../src/game_core/src/ecstasy/ecstasye.cpp:77) |
| `0x0010C4B0` | eSolid::ReplaceLightMaterial (signature and full body unresolved) | [src/game_core/src/ecstasy/ecstasye.cpp:82](../src/game_core/src/ecstasy/ecstasye.cpp:82) |
| `0x0010C5B0` | eModel::UnInit (signature and full body unresolved) | [src/game_core/src/ecstasy/ecstasye.cpp:41](../src/game_core/src/ecstasy/ecstasye.cpp:41) |
| `0x0010C878` | eModel::ApplyReplacementTextureTable (signature and full body unresolved) | [src/game_core/src/ecstasy/ecstasye.cpp:46](../src/game_core/src/ecstasy/ecstasye.cpp:46) |
| `0x0010CB00` | eModel::IsLit (signature and full body unresolved) | [src/game_core/src/ecstasy/ecstasye.cpp:51](../src/game_core/src/ecstasy/ecstasye.cpp:51) |
| `0x00110368` | ePoly::Render3D host DMA submission service | [src/game_core/src/ecstasy/ecstasye.cpp:144](../src/game_core/src/ecstasy/ecstasye.cpp:144) |
| `0x00110C60` | eTransferRectQueue::Sub_00110C60 (signature and full body unresolved) | [src/game_core/src/ecstasy/ecstasye.cpp:101](../src/game_core/src/ecstasy/ecstasye.cpp:101) |
| `0x00110CD8` | eTransferRectQueue::GetTransferRect (signature and full body unresolved) | [src/game_core/src/ecstasy/ecstasye.cpp:106](../src/game_core/src/ecstasy/ecstasye.cpp:106) |
| `0x00114BB0` | eSolid::Sub_00114BB0 (signature and full body unresolved) | [src/game_core/src/ecstasy/ecstasye.cpp:87](../src/game_core/src/ecstasy/ecstasye.cpp:87) |
| `0x00118A60` | Postprocess packet layout outside original512-wide target | [src/game_core/port/track/postprocess.cpp:23](../src/game_core/port/track/postprocess.cpp:23) |
| `0x00119F70` | Nondefault DOF jitter table extent not established | [src/game_core/src/ecstasy/eenveffectse.cpp:96](../src/game_core/src/ecstasy/eenveffectse.cpp:96) |
| `0x0011A9F0` | MainDisp alpha unavailable after a depth-alpha-disabled frame without an alpha-writing post pass | [src/platform/godot_bridge/track_postprocess.cpp:298](../src/platform/godot_bridge/track_postprocess.cpp:298) |
| `0x0011BA88` | unsupported polynomial range reduction | [src/game_core/src/ecstasy/emathe.cpp:55](../src/game_core/src/ecstasy/emathe.cpp:55) |
| `0x0011BF98` | eStripDataE::GetElementXYZ (signature and full body unresolved) | [src/game_core/src/ecstasy/estripdatae.cpp:62](../src/game_core/src/ecstasy/estripdatae.cpp:62) |
| `0x0011C090` | eStripDataE::GetElementST0 (signature and full body unresolved) | [src/game_core/src/ecstasy/estripdatae.cpp:67](../src/game_core/src/ecstasy/estripdatae.cpp:67) |
| `0x0011C180` | eStripDataE::GetElementRGBA (signature and full body unresolved) | [src/game_core/src/ecstasy/estripdatae.cpp:91](../src/game_core/src/ecstasy/estripdatae.cpp:91) |
| `0x0011C248` | eStripData::GetNumVerts (signature and full body unresolved) | [src/game_core/src/ecstasy/estripdatae.cpp:74](../src/game_core/src/ecstasy/estripdatae.cpp:74) |
| `0x0011C268` | eStripData::GetST0 (signature and full body unresolved) | [src/game_core/src/ecstasy/estripdatae.cpp:79](../src/game_core/src/ecstasy/estripdatae.cpp:79) |
| `0x0011C2B8` | eStripData::GetXYZ (signature and full body unresolved) | [src/game_core/src/ecstasy/estripdatae.cpp:84](../src/game_core/src/ecstasy/estripdatae.cpp:84) |
| `0x0011D720` | Joystick classification service is absent | [src/game_core/indep/src/physicscar.cpp:181](../src/game_core/indep/src/physicscar.cpp:181) |
| `0x0011D720` | Joystick type service absent | [src/game_core/indep/src/realdriver.cpp:90](../src/game_core/indep/src/realdriver.cpp:90) |
| `0x0011E4E0` | Car::TriggerEffect missing emitter services | [src/game_core/src/care.cpp:36](../src/game_core/src/care.cpp:36) |
| `0x0011E4E0` | Car::TriggerEffect non-wheel effect binding | [src/game_core/src/care.cpp:33](../src/game_core/src/care.cpp:33) |
| `0x0011E4E0` | Car::TriggerEffect non-wheel emitter position | [src/game_core/src/care.cpp:35](../src/game_core/src/care.cpp:35) |
| `0x0011E4E0` | Car::TriggerEffect service missing | [src/game_core/indep/src/car.cpp:22](../src/game_core/indep/src/car.cpp:22) |
| `0x0011E4E0` | Terrain type outside recovered emitter table | [src/game_core/port/driving/car_effects.cpp:39](../src/game_core/port/driving/car_effects.cpp:39) |
| `0x0011E860` | CarRenderInfo::Sub_0011E860 full constructor: allocation, materials, markers and state | [src/game_core/src/care.cpp:395](../src/game_core/src/care.cpp:395) |
| `0x00120EF8` | CarRenderInfo::Sub_00120EF8 full render: PS2 submission, live-car state and effects | [src/game_core/src/care.cpp:398](../src/game_core/src/care.cpp:398) |
| `0x00127418` | AcidEffectSystem::Sub_00127418 (signature and full body unresolved) | [src/game_core/src/acidfxe.cpp:17](../src/game_core/src/acidfxe.cpp:17) |
| `0x00127518` | Dynamic AcidEmitter requires its original emitter data | [src/game_core/port/track/particles.cpp:53](../src/game_core/port/track/particles.cpp:53) |
| `0x00127688` | AcidEffectSystem::RemoveAutoEmitter (signature and full body unresolved) | [src/game_core/src/acidfxe.cpp:22](../src/game_core/src/acidfxe.cpp:22) |
| `0x00127700` | Non-progressing original emission interval requires bounded execution policy | [src/game_core/src/acidfxe.cpp:112](../src/game_core/src/acidfxe.cpp:112) |
| `0x00128DA0` | World particle nonzero matrix pack requires separate CPU packet audit | [src/platform/godot_bridge/track_particle_renderer.cpp:115](../src/platform/godot_bridge/track_particle_renderer.cpp:115) |
| `0x00128DA0` | World particle solid texture-chain submission is not yet connected | [src/platform/godot_bridge/track_particle_renderer.cpp:115](../src/platform/godot_bridge/track_particle_renderer.cpp:115) |
| `0x00128EF8` | Wheel particle display requires the supported all-view car visibility mask | [src/platform/godot_bridge/ps2_driving_runtime.cpp:53](../src/platform/godot_bridge/ps2_driving_runtime.cpp:53) |
| `0x00129880` | RainSplash::Recalc (signature and full body unresolved) | [src/game_core/src/acidfxe.cpp:31](../src/game_core/src/acidfxe.cpp:31) |
| `0x0012AE28` | cMC::MCCloseResult (signature and full body unresolved) | [src/game_core/src/memorycard.cpp:13](../src/game_core/src/memorycard.cpp:13) |
| `0x0012B0F0` | cMC::MCMakeDirResult (signature and full body unresolved) | [src/game_core/src/memorycard.cpp:18](../src/game_core/src/memorycard.cpp:18) |
| `0x0012B3B8` | WheelDevice::Sub_0012B3B8 (signature and full body unresolved) | [src/game_core/src/logitechwheel.cpp:13](../src/game_core/src/logitechwheel.cpp:13) |
| `0x0012BC68` | WheelDevice::RecalculateDeviceNumber (signature and full body unresolved) | [src/game_core/src/logitechwheel.cpp:18](../src/game_core/src/logitechwheel.cpp:18) |
| `0x0012C938` | cDBFileSys::Service (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:17](../src/game_core/src/dbfiles.cpp:17) |
| `0x0012CCA8` | cDBFileSys::SetState (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:23](../src/game_core/src/dbfiles.cpp:23) |
| `0x0012CD00` | cDBFileSys::Format (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:29](../src/game_core/src/dbfiles.cpp:29) |
| `0x0012CD78` | cDBFileSys::Unformat (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:35](../src/game_core/src/dbfiles.cpp:35) |
| `0x0012D1A0` | cDBFileSys::Sub_0012D1A0 (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:40](../src/game_core/src/dbfiles.cpp:40) |
| `0x0012D1E0` | cDBFileSys::Sub_0012D1E0 (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:45](../src/game_core/src/dbfiles.cpp:45) |
| `0x0012D220` | cDBFileSys::CheckFreeSpace (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:51](../src/game_core/src/dbfiles.cpp:51) |
| `0x0012D2A8` | cDBFileSys::GetFreeSpaceOnDevice (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:57](../src/game_core/src/dbfiles.cpp:57) |
| `0x0012D2E8` | cDBFileSys::GetFileListing (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:63](../src/game_core/src/dbfiles.cpp:63) |
| `0x0012D3D8` | cDBFileSys::CheckDevice (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:69](../src/game_core/src/dbfiles.cpp:69) |
| `0x0012D518` | cDBFileSys::IsDevicePresent (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:75](../src/game_core/src/dbfiles.cpp:75) |
| `0x0012D560` | cDBFileSys::GetCurrentDevice (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:81](../src/game_core/src/dbfiles.cpp:81) |
| `0x0012D5F8` | cDBFileSys::GetNextDevice (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:87](../src/game_core/src/dbfiles.cpp:87) |
| `0x0012D690` | cDBFileSys::GetPrevDevice (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:93](../src/game_core/src/dbfiles.cpp:93) |
| `0x0012DDC0` | cDBFileSys::SetLastError (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:99](../src/game_core/src/dbfiles.cpp:99) |
| `0x0012DEE0` | cDBFileSys::Sub_0012DEE0 (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:104](../src/game_core/src/dbfiles.cpp:104) |
| `0x0012E0F0` | cDBFileSys::Sub_0012E0F0 (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:109](../src/game_core/src/dbfiles.cpp:109) |
| `0x0012E190` | cDBFileSys::GetFileName (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:115](../src/game_core/src/dbfiles.cpp:115) |
| `0x0012E1A8` | cDBFileSys::GetFileCount (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:121](../src/game_core/src/dbfiles.cpp:121) |
| `0x0012E1B8` | cDBFilesBase::SetLastError (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:178](../src/game_core/src/dbfiles.cpp:178) |
| `0x0012E1C0` | cDBFilesBase::GetState (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:183](../src/game_core/src/dbfiles.cpp:183) |
| `0x0012E1C8` | cDBFilesBase::SetState (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:188](../src/game_core/src/dbfiles.cpp:188) |
| `0x0012E1F0` | cDBFilesBase::GetFreeSpaceOnDevice (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:193](../src/game_core/src/dbfiles.cpp:193) |
| `0x0012E1F8` | cDBFilesBase::GetMaxDeviceSupported (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:198](../src/game_core/src/dbfiles.cpp:198) |
| `0x0012E200` | cDBFilesBase::GetPrevDevice (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:203](../src/game_core/src/dbfiles.cpp:203) |
| `0x0012E208` | cDBFilesBase::GetNextDevice (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:208](../src/game_core/src/dbfiles.cpp:208) |
| `0x0012E210` | cDBFilesBase::GetCurrentDevice (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:213](../src/game_core/src/dbfiles.cpp:213) |
| `0x0012E218` | cDBFilesBase::IsDevicePresent (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:218](../src/game_core/src/dbfiles.cpp:218) |
| `0x0012E220` | cDBFilesBase::CheckDevice (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:223](../src/game_core/src/dbfiles.cpp:223) |
| `0x0012E228` | cDBFilesBase::GetFileName (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:228](../src/game_core/src/dbfiles.cpp:228) |
| `0x0012E230` | cDBFilesBase::Sub_0012E230 (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:233](../src/game_core/src/dbfiles.cpp:233) |
| `0x0012E238` | cDBFilesBase::GetFileCount (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:238](../src/game_core/src/dbfiles.cpp:238) |
| `0x0012E240` | cDBFilesBase::GetFileListing (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:243](../src/game_core/src/dbfiles.cpp:243) |
| `0x0012E248` | cDBFilesBase::GetDeviceName (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:248](../src/game_core/src/dbfiles.cpp:248) |
| `0x0012E250` | cDBFilesBase::CheckFreeSpace (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:253](../src/game_core/src/dbfiles.cpp:253) |
| `0x0012E258` | cDBFilesBase::CalculateFreeSpaceRequired (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:258](../src/game_core/src/dbfiles.cpp:258) |
| `0x0012E260` | cDBFilesBase::Sub_0012E260 (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:263](../src/game_core/src/dbfiles.cpp:263) |
| `0x0012E268` | cDBFilesBase::SetFileIndex (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:268](../src/game_core/src/dbfiles.cpp:268) |
| `0x0012E270` | cDBFilesBase::SetFileData (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:273](../src/game_core/src/dbfiles.cpp:273) |
| `0x0012E2B8` | cDBFilesBase::Unformat (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:278](../src/game_core/src/dbfiles.cpp:278) |
| `0x0012E2C0` | cDBFilesBase::Format (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:283](../src/game_core/src/dbfiles.cpp:283) |
| `0x0012E2C8` | cDBFilesBase::Service (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:288](../src/game_core/src/dbfiles.cpp:288) |
| `0x0012E2D0` | cDBFilesBase::Sub_0012E2D0 (signature and full body unresolved) | [src/game_core/src/dbfiles.cpp:293](../src/game_core/src/dbfiles.cpp:293) |
| `0x0012E6C8` | DemoDiscManager::Init (signature and full body unresolved) | [src/game_core/src/demodisc.cpp:18](../src/game_core/src/demodisc.cpp:18) |
| `0x0012EB00` | DemoDiscManager::SetEndReason (signature and full body unresolved) | [src/game_core/src/demodisc.cpp:23](../src/game_core/src/demodisc.cpp:23) |
| `0x0012FFC8` | AIDriver::Sub_0012FFC8 (signature and full body unresolved) | [src/game_core/indep/src/aidriver.cpp:15](../src/game_core/indep/src/aidriver.cpp:15) |
| `0x00130090` | AIDriver::Sub_00130090 (signature and full body unresolved) | [src/game_core/indep/src/aidriver.cpp:20](../src/game_core/indep/src/aidriver.cpp:20) |
| `0x00130108` | AIDriver::Sub_00130108 (signature and full body unresolved) | [src/game_core/indep/src/aidriver.cpp:25](../src/game_core/indep/src/aidriver.cpp:25) |
| `0x00130188` | AIDriver::Sub_00130188 (signature and full body unresolved) | [src/game_core/indep/src/aidriver.cpp:30](../src/game_core/indep/src/aidriver.cpp:30) |
| `0x00130238` | AIDriver::Sub_00130238 (signature and full body unresolved) | [src/game_core/indep/src/aidriver.cpp:35](../src/game_core/indep/src/aidriver.cpp:35) |
| `0x00130890` | AIDriver::Sub_00130890 (signature and full body unresolved) | [src/game_core/indep/src/aidriver.cpp:40](../src/game_core/indep/src/aidriver.cpp:40) |
| `0x00130958` | CopDriver::Sub_00130958 (signature and full body unresolved) | [src/game_core/port/police/cop_control.cpp:83](../src/game_core/port/police/cop_control.cpp:83) |
| `0x00130958` | PsychoDriver::Sub_00130958 (signature and full body unresolved) | [src/game_core/indep/src/psychodriver.cpp:15](../src/game_core/indep/src/psychodriver.cpp:15) |
| `0x00130958` | RealDriver::Sub_00130958 (signature and full body unresolved) | [src/game_core/indep/src/realdriver.cpp:130](../src/game_core/indep/src/realdriver.cpp:130) |
| `0x001309F0` | CarMover::DoSnapshot (signature and full body unresolved) | [src/game_core/indep/src/car.cpp:171](../src/game_core/indep/src/car.cpp:171) |
| `0x001309F8` | NullMover::Move (signature and full body unresolved) | [src/game_core/indep/src/car.cpp:265](../src/game_core/indep/src/car.cpp:265) |
| `0x001315F8` | Car::SetFrictionBoost (signature and full body unresolved) | [src/game_core/indep/src/car.cpp:241](../src/game_core/indep/src/car.cpp:241) |
| `0x00132960` | Car::CalculateDebugChecksum (signature and full body unresolved) | [src/game_core/indep/src/car.cpp:246](../src/game_core/indep/src/car.cpp:246) |
| `0x00133988` | Car::TriggerCarTerrainEffects remaining drive-over/water/damage/nitro/flat-tire branches | [src/game_core/indep/src/car.cpp:65](../src/game_core/indep/src/car.cpp:65) |
| `0x00134DB8` | Car::GetWheelTraction (signature and full body unresolved) | [src/game_core/indep/src/car.cpp:251](../src/game_core/indep/src/car.cpp:251) |
| `0x00134E00` | Car::GetWheelsOnGround (signature and full body unresolved) | [src/game_core/indep/src/car.cpp:256](../src/game_core/indep/src/car.cpp:256) |
| `0x00136AF8` | NullMover::Sub_00136AF8 (signature and full body unresolved) | [src/game_core/indep/src/car.cpp:270](../src/game_core/indep/src/car.cpp:270) |
| `0x00136B78` | CarMover::Sub_00136B78 (signature and full body unresolved) | [src/game_core/indep/src/car.cpp:207](../src/game_core/indep/src/car.cpp:207) |
| `0x00136BB8` | CopDriver::Sub_00136BB8 (signature and full body unresolved) | [src/game_core/port/police/cop_control.cpp:88](../src/game_core/port/police/cop_control.cpp:88) |
| `0x00136BC0` | CopDriver::Sub_00136BC0 (signature and full body unresolved) | [src/game_core/port/police/cop_control.cpp:93](../src/game_core/port/police/cop_control.cpp:93) |
| `0x00136BD0` | CopDriver::Sub_00136BD0 (signature and full body unresolved) | [src/game_core/port/police/cop_control.cpp:98](../src/game_core/port/police/cop_control.cpp:98) |
| `0x00136BD8` | CopDriver::Sub_00136BD8 (signature and full body unresolved) | [src/game_core/port/police/cop_control.cpp:103](../src/game_core/port/police/cop_control.cpp:103) |
| `0x00136BE0` | CopDriver::Sub_00136BE0 (signature and full body unresolved) | [src/game_core/port/police/cop_control.cpp:108](../src/game_core/port/police/cop_control.cpp:108) |
| `0x00136BE8` | CopDriver::Sub_00136BE8 (signature and full body unresolved) | [src/game_core/port/police/cop_control.cpp:113](../src/game_core/port/police/cop_control.cpp:113) |
| `0x00136C00` | CarDriver::GetSteerToPoint (signature and full body unresolved) | [src/game_core/indep/src/car.cpp:117](../src/game_core/indep/src/car.cpp:117) |
| `0x00136C08` | CarDriver::SetPhysicsMode (signature and full body unresolved) | [src/game_core/indep/src/car.cpp:122](../src/game_core/indep/src/car.cpp:122) |
| `0x00136C10` | CarDriver::GetDesiredTimeToSpeed (signature and full body unresolved) | [src/game_core/indep/src/car.cpp:127](../src/game_core/indep/src/car.cpp:127) |
| `0x00136C20` | CarDriver::GetDesiredSpeed (signature and full body unresolved) | [src/game_core/indep/src/car.cpp:132](../src/game_core/indep/src/car.cpp:132) |
| `0x00136C30` | CarDriver::ClearSteeringMemory (signature and full body unresolved) | [src/game_core/indep/src/car.cpp:137](../src/game_core/indep/src/car.cpp:137) |
| `0x00136C38` | CarDriver::ZeroSteeringMemory (signature and full body unresolved) | [src/game_core/indep/src/car.cpp:142](../src/game_core/indep/src/car.cpp:142) |
| `0x00136C40` | CarDriver::MaxSpeedToTarget (signature and full body unresolved) | [src/game_core/indep/src/car.cpp:147](../src/game_core/indep/src/car.cpp:147) |
| `0x00136C50` | CarDriver::IsHonking (signature and full body unresolved) | [src/game_core/indep/src/car.cpp:152](../src/game_core/indep/src/car.cpp:152) |
| `0x00136C58` | CopDriver::Sub_00136C58 (signature and full body unresolved) | [src/game_core/port/police/cop_control.cpp:134](../src/game_core/port/police/cop_control.cpp:134) |
| `0x00136C58` | PsychoDriver::Sub_00136C58 (signature and full body unresolved) | [src/game_core/indep/src/psychodriver.cpp:36](../src/game_core/indep/src/psychodriver.cpp:36) |
| `0x00136C58` | RealDriver::Sub_00136C58 (signature and full body unresolved) | [src/game_core/indep/src/realdriver.cpp:151](../src/game_core/indep/src/realdriver.cpp:151) |
| `0x00136C60` | CopDriver::Sub_00136C60 (signature and full body unresolved) | [src/game_core/port/police/cop_control.cpp:139](../src/game_core/port/police/cop_control.cpp:139) |
| `0x00136C60` | PsychoDriver::Sub_00136C60 (signature and full body unresolved) | [src/game_core/indep/src/psychodriver.cpp:41](../src/game_core/indep/src/psychodriver.cpp:41) |
| `0x00136C60` | RealDriver::Sub_00136C60 (signature and full body unresolved) | [src/game_core/indep/src/realdriver.cpp:156](../src/game_core/indep/src/realdriver.cpp:156) |
| `0x00136C68` | CarDriver::SetSteerToPoint (signature and full body unresolved) | [src/game_core/indep/src/car.cpp:157](../src/game_core/indep/src/car.cpp:157) |
| `0x00136C70` | CarDriver::SetTarget (signature and full body unresolved) | [src/game_core/indep/src/car.cpp:162](../src/game_core/indep/src/car.cpp:162) |
| `0x00136CE8` | CopDriver::Sub_00136CE8 (signature and full body unresolved) | [src/game_core/port/police/cop_control.cpp:148](../src/game_core/port/police/cop_control.cpp:148) |
| `0x00136CE8` | PsychoDriver::Sub_00136CE8 (signature and full body unresolved) | [src/game_core/indep/src/psychodriver.cpp:50](../src/game_core/indep/src/psychodriver.cpp:50) |
| `0x00136CE8` | RealDriver::Sub_00136CE8 (signature and full body unresolved) | [src/game_core/indep/src/realdriver.cpp:165](../src/game_core/indep/src/realdriver.cpp:165) |
| `0x00136CF0` | CopDriver::Sub_00136CF0 (signature and full body unresolved) | [src/game_core/port/police/cop_control.cpp:153](../src/game_core/port/police/cop_control.cpp:153) |
| `0x00136CF0` | PsychoDriver::Sub_00136CF0 (signature and full body unresolved) | [src/game_core/indep/src/psychodriver.cpp:55](../src/game_core/indep/src/psychodriver.cpp:55) |
| `0x00136CF0` | RealDriver::Sub_00136CF0 (signature and full body unresolved) | [src/game_core/indep/src/realdriver.cpp:170](../src/game_core/indep/src/realdriver.cpp:170) |
| `0x00136D00` | CarDriver::Sub_00136D00 (signature and full body unresolved) | [src/game_core/indep/src/car.cpp:200](../src/game_core/indep/src/car.cpp:200) |
| `0x001370C0` | Debug PhysicsCar initialization requires an attached simulation backend | [src/game_core/port/driving/session.cpp:102](../src/game_core/port/driving/session.cpp:102) |
| `0x00137748` | Unknown final virtual target | [src/game_core/indep/src/physicscar.cpp:102](../src/game_core/indep/src/physicscar.cpp:102) |
| `0x001378C0` | Explicit original temporary W projection is absent | [src/game_core/indep/src/physicscar.cpp:77](../src/game_core/indep/src/physicscar.cpp:77) |
| `0x001378C0` | Explicit original temporary W projection is absent | [src/game_core/indep/src/physicscar.cpp:88](../src/game_core/indep/src/physicscar.cpp:88) |
| `0x00137D18` | Excluded original Race4 diagnostic position-restore branch | [src/game_core/indep/src/physicscar.cpp:153](../src/game_core/indep/src/physicscar.cpp:153) |
| `0x00137D18` | Excluded original Race4 diagnostic position-restore branch | [src/game_core/indep/src/physicscar.cpp:282](../src/game_core/indep/src/physicscar.cpp:282) |
| `0x00137D18` | Explicit original temporary W projection is absent | [src/game_core/indep/src/physicscar.cpp:300](../src/game_core/indep/src/physicscar.cpp:300) |
| `0x00137D18` | Unrecovered steering virtual target | [src/game_core/indep/src/physicscar.cpp:163](../src/game_core/indep/src/physicscar.cpp:163) |
| `0x00138D90` | Final constructor synchronization service is absent | [src/game_core/indep/src/physicscar.cpp:103](../src/game_core/indep/src/physicscar.cpp:103) |
| `0x00138D90` | unknown body pre-force virtual entry | [src/game_core/indep/src/physicscar.cpp:357](../src/game_core/indep/src/physicscar.cpp:357) |
| `0x00139678` | PhysicsVehicle::Sub_00139678 (signature and full body unresolved) | [src/game_core/indep/src/physicscar.cpp:450](../src/game_core/indep/src/physicscar.cpp:450) |
| `0x00139760` | PsychoDriver::Sub_00139760 (signature and full body unresolved) | [src/game_core/indep/src/psychodriver.cpp:63](../src/game_core/indep/src/psychodriver.cpp:63) |
| `0x001398E8` | PsychoDriver::Sub_001398E8 (signature and full body unresolved) | [src/game_core/indep/src/psychodriver.cpp:68](../src/game_core/indep/src/psychodriver.cpp:68) |
| `0x00139968` | PsychoDriver::Sub_00139968 (signature and full body unresolved) | [src/game_core/indep/src/psychodriver.cpp:73](../src/game_core/indep/src/psychodriver.cpp:73) |
| `0x00139980` | PsychoDriver::Sub_00139980 (signature and full body unresolved) | [src/game_core/indep/src/psychodriver.cpp:78](../src/game_core/indep/src/psychodriver.cpp:78) |
| `0x00139B28` | PsychoDriver::Sub_00139B28 (signature and full body unresolved) | [src/game_core/indep/src/psychodriver.cpp:83](../src/game_core/indep/src/psychodriver.cpp:83) |
| `0x00139B68` | PsychoDriver::Sub_00139B68 (signature and full body unresolved) | [src/game_core/indep/src/psychodriver.cpp:88](../src/game_core/indep/src/psychodriver.cpp:88) |
| `0x00139C48` | PsychoDriver::Sub_00139C48 (signature and full body unresolved) | [src/game_core/indep/src/psychodriver.cpp:93](../src/game_core/indep/src/psychodriver.cpp:93) |
| `0x0013A3E0` | PsychoDriver::SetTarget (signature and full body unresolved) | [src/game_core/indep/src/psychodriver.cpp:98](../src/game_core/indep/src/psychodriver.cpp:98) |
| `0x0013A638` | PsychoDriver::Sub_0013A638 (signature and full body unresolved) | [src/game_core/indep/src/psychodriver.cpp:103](../src/game_core/indep/src/psychodriver.cpp:103) |
| `0x0013A658` | PsychoDriver::Sub_0013A658 (signature and full body unresolved) | [src/game_core/indep/src/psychodriver.cpp:108](../src/game_core/indep/src/psychodriver.cpp:108) |
| `0x0013A678` | PsychoDriver::Sub_0013A678 (signature and full body unresolved) | [src/game_core/indep/src/psychodriver.cpp:113](../src/game_core/indep/src/psychodriver.cpp:113) |
| `0x0013AC80` | PsychoDriver::Sub_0013AC80 (signature and full body unresolved) | [src/game_core/indep/src/psychodriver.cpp:118](../src/game_core/indep/src/psychodriver.cpp:118) |
| `0x0013ACD8` | PsychoDriver::Sub_0013ACD8 (signature and full body unresolved) | [src/game_core/indep/src/psychodriver.cpp:123](../src/game_core/indep/src/psychodriver.cpp:123) |
| `0x0013ACE8` | PsychoDriver::Sub_0013ACE8 (signature and full body unresolved) | [src/game_core/indep/src/psychodriver.cpp:128](../src/game_core/indep/src/psychodriver.cpp:128) |
| `0x0013ACF0` | PsychoDriver::Sub_0013ACF0 (signature and full body unresolved) | [src/game_core/indep/src/psychodriver.cpp:133](../src/game_core/indep/src/psychodriver.cpp:133) |
| `0x0013AD00` | PsychoDriver::Sub_0013AD00 (signature and full body unresolved) | [src/game_core/indep/src/psychodriver.cpp:138](../src/game_core/indep/src/psychodriver.cpp:138) |
| `0x0013AD10` | PsychoDriver::Sub_0013AD10 (signature and full body unresolved) | [src/game_core/indep/src/psychodriver.cpp:143](../src/game_core/indep/src/psychodriver.cpp:143) |
| `0x0013AD90` | RealDriver::Sub_0013AD90 (signature and full body unresolved) | [src/game_core/indep/src/realdriver.cpp:178](../src/game_core/indep/src/realdriver.cpp:178) |
| `0x0013AE58` | RealDriver::Sub_0013AE58 (signature and full body unresolved) | [src/game_core/indep/src/realdriver.cpp:183](../src/game_core/indep/src/realdriver.cpp:183) |
| `0x0013AEB8` | RealDriver::DoSnapshot (signature and full body unresolved) | [src/game_core/indep/src/realdriver.cpp:189](../src/game_core/indep/src/realdriver.cpp:189) |
| `0x0013B080` | RealDriver::AddJoyHandlers (signature and full body unresolved) | [src/game_core/indep/src/realdriver.cpp:195](../src/game_core/indep/src/realdriver.cpp:195) |
| `0x0013B200` | Joystick presence service absent | [src/game_core/indep/src/realdriver.cpp:118](../src/game_core/indep/src/realdriver.cpp:118) |
| `0x0013B200` | Player nitro/radial-blur path is outside the playerless test fixture | [src/game_core/indep/src/realdriver.cpp:123](../src/game_core/indep/src/realdriver.cpp:123) |
| `0x0013B200` | Route diagnostic service absent | [src/game_core/indep/src/realdriver.cpp:114](../src/game_core/indep/src/realdriver.cpp:114) |
| `0x0013B250` | RealDriver::RemoveJoyHandlers (signature and full body unresolved) | [src/game_core/indep/src/realdriver.cpp:201](../src/game_core/indep/src/realdriver.cpp:201) |
| `0x0013BC90` | Player::RerouteJoyEvent dependency is not bound | [src/game_core/indep/src/realdriver.cpp:57](../src/game_core/indep/src/realdriver.cpp:57) |
| `0x0013BD70` | Player::RerouteJoyEvent dependency is not bound | [src/game_core/indep/src/realdriver.cpp:65](../src/game_core/indep/src/realdriver.cpp:65) |
| `0x0013C420` | RealDriver::StartHonkHorn (signature and full body unresolved) | [src/game_core/indep/src/realdriver.cpp:214](../src/game_core/indep/src/realdriver.cpp:214) |
| `0x0013C460` | RealDriver::StopHonkHorn (signature and full body unresolved) | [src/game_core/indep/src/realdriver.cpp:220](../src/game_core/indep/src/realdriver.cpp:220) |
| `0x0013CAE0` | RealDriver::GetDriverNumber (signature and full body unresolved) | [src/game_core/indep/src/realdriver.cpp:226](../src/game_core/indep/src/realdriver.cpp:226) |
| `0x0013CAF0` | RealDriver::SetDesiredSpeed (signature and full body unresolved) | [src/game_core/indep/src/realdriver.cpp:232](../src/game_core/indep/src/realdriver.cpp:232) |
| `0x0013CD68` | ReplaySnapshot::GetChecksum (signature and full body unresolved) | [src/game_core/indep/src/replay.cpp:37](../src/game_core/indep/src/replay.cpp:37) |
| `0x0013CE78` | ReplaySnapshot::ReadData (signature and full body unresolved) | [src/game_core/indep/src/replay.cpp:42](../src/game_core/indep/src/replay.cpp:42) |
| `0x0013E070` | Replay interesting-event service absent | [src/game_core/indep/src/realdriver.cpp:80](../src/game_core/indep/src/realdriver.cpp:80) |
| `0x0013EE08` | DemoModeReplay::Sub_0013EE08 (signature and full body unresolved) | [src/game_core/indep/src/demomodereplay.cpp:13](../src/game_core/indep/src/demomodereplay.cpp:13) |
| `0x0013EE58` | DemoModeReplay::Loader (signature and full body unresolved) | [src/game_core/indep/src/demomodereplay.cpp:18](../src/game_core/indep/src/demomodereplay.cpp:18) |
| `0x00140098` | Font::GetKerning (signature and full body unresolved) | [src/game_core/indep/src/flailerdb.cpp:83](../src/game_core/indep/src/flailerdb.cpp:83) |
| `0x00140098` | font pair range outside original table | [src/port/runtime/frame_builder.cpp:40](../src/port/runtime/frame_builder.cpp:40) |
| `0x001401D8` | Font::GetWidestCharacter (signature and full body unresolved) | [src/game_core/indep/src/flailerdb.cpp:88](../src/game_core/indep/src/flailerdb.cpp:88) |
| `0x00140638` | missing background widget measurement | [src/game_core/indep/src/fmenus.cpp:1200](../src/game_core/indep/src/fmenus.cpp:1200) |
| `0x00140638` | missing background widget measurement | [src/game_core/indep/src/fmenus.cpp:1230](../src/game_core/indep/src/fmenus.cpp:1230) |
| `0x001407D0` | FlailerObjectChunk::SetScaleX (signature and full body unresolved) | [src/game_core/indep/src/flailerdb.cpp:37](../src/game_core/indep/src/flailerdb.cpp:37) |
| `0x001407F8` | FlailerObjectChunk::SetScaleY (signature and full body unresolved) | [src/game_core/indep/src/flailerdb.cpp:42](../src/game_core/indep/src/flailerdb.cpp:42) |
| `0x00140A20` | FlailerObjectChunk::SetTopLeftX (signature and full body unresolved) | [src/game_core/indep/src/flailerdb.cpp:47](../src/game_core/indep/src/flailerdb.cpp:47) |
| `0x00140A48` | FlailerObjectChunk::SetTopLeftY (signature and full body unresolved) | [src/game_core/indep/src/flailerdb.cpp:52](../src/game_core/indep/src/flailerdb.cpp:52) |
| `0x00140CA0` | FlailerTextChunk::SetColor (signature and full body unresolved) | [src/game_core/indep/src/flailerdb.cpp:71](../src/game_core/indep/src/flailerdb.cpp:71) |
| `0x00140ED0` | FlailerTextChunk::SetAlpha (signature and full body unresolved) | [src/game_core/indep/src/flailerdb.cpp:76](../src/game_core/indep/src/flailerdb.cpp:76) |
| `0x001411D8` | FlailerBitmapChunk::SetAlpha (signature and full body unresolved) | [src/game_core/indep/src/flailerdb.cpp:20](../src/game_core/indep/src/flailerdb.cpp:20) |
| `0x00141258` | FlailerBitmapChunk::SetRotation (signature and full body unresolved) | [src/game_core/indep/src/flailerdb.cpp:25](../src/game_core/indep/src/flailerdb.cpp:25) |
| `0x00141288` | FlailerBitmapChunk::SetColor (signature and full body unresolved) | [src/game_core/indep/src/flailerdb.cpp:30](../src/game_core/indep/src/flailerdb.cpp:30) |
| `0x001428F0` | frontend rotated quad matrix not translated | [src/port/runtime/frame_builder.cpp:249](../src/port/runtime/frame_builder.cpp:249) |
| `0x00144860` | selector target bounds unavailable | [src/game_core/indep/src/fmenus.cpp:1084](../src/game_core/indep/src/fmenus.cpp:1084) |
| `0x00145AF0` | FlailerRenderManager::Sub_00145AF0 (signature and full body unresolved) | [src/game_core/indep/src/flailerdb.cpp:59](../src/game_core/indep/src/flailerdb.cpp:59) |
| `0x00145CF8` | FlailerRenderManager::IsScreenVisible (signature and full body unresolved) | [src/game_core/indep/src/flailerdb.cpp:64](../src/game_core/indep/src/flailerdb.cpp:64) |
| `0x00145F30` | clip-mask measurement missing | [src/game_core/indep/src/fmenus.cpp:932](../src/game_core/indep/src/fmenus.cpp:932) |
| `0x00146AE8` | TimeAroundTrackTable::GetTimeTableEntry (signature and full body unresolved) | [src/game_core/indep/src/race.cpp:42](../src/game_core/indep/src/race.cpp:42) |
| `0x00148218` | Race::SortRacingCarsByLeadership (signature and full body unresolved) | [src/game_core/indep/src/race.cpp:23](../src/game_core/indep/src/race.cpp:23) |
| `0x001491C0` | Race::GetFinisher (signature and full body unresolved) | [src/game_core/indep/src/race.cpp:28](../src/game_core/indep/src/race.cpp:28) |
| `0x001493B8` | RacingCar::Sub_001493B8 (signature and full body unresolved) | [src/game_core/indep/src/race.cpp:35](../src/game_core/indep/src/race.cpp:35) |
| `0x0014D338` | World::BuildCarTable (signature and full body unresolved) | [src/game_core/indep/src/world.cpp:36](../src/game_core/indep/src/world.cpp:36) |
| `0x0014D508` | World::GetCarByNumber (signature and full body unresolved) | [src/game_core/indep/src/world.cpp:41](../src/game_core/indep/src/world.cpp:41) |
| `0x0014D638` | World::ResetTimeScale (signature and full body unresolved) | [src/game_core/indep/src/world.cpp:46](../src/game_core/indep/src/world.cpp:46) |
| `0x0014E3E8` | World::Sub_0014E3E8 (signature and full body unresolved) | [src/game_core/indep/src/world.cpp:51](../src/game_core/indep/src/world.cpp:51) |
| `0x0014E440` | World::Sub_0014E440 (signature and full body unresolved) | [src/game_core/indep/src/world.cpp:56](../src/game_core/indep/src/world.cpp:56) |
| `0x0014F340` | VisibleZone::FindIndexTo (signature and full body unresolved) | [src/game_core/indep/src/scenery.cpp:31](../src/game_core/indep/src/scenery.cpp:31) |
| `0x001517D8` | TrackPolygon::PointInShaft (signature and full body unresolved) | [src/game_core/indep/src/track.cpp:63](../src/game_core/indep/src/track.cpp:63) |
| `0x00151CC0` | Wheel skid endpoint topology elevation service | [src/game_core/indep/src/wheel.cpp:480](../src/game_core/indep/src/wheel.cpp:480) |
| `0x00152070` | TopologyCoordinate::GetElevation (signature and full body unresolved) | [src/game_core/indep/src/track.cpp:24](../src/game_core/indep/src/track.cpp:24) |
| `0x00152B10` | TopologyTree::Sub_00152B10 (signature and full body unresolved) | [src/game_core/indep/src/track.cpp:31](../src/game_core/indep/src/track.cpp:31) |
| `0x00152C10` | TopologyTree::Loader (signature and full body unresolved) | [src/game_core/indep/src/track.cpp:36](../src/game_core/indep/src/track.cpp:36) |
| `0x00152D30` | TopologyTree::Unloader (signature and full body unresolved) | [src/game_core/indep/src/track.cpp:41](../src/game_core/indep/src/track.cpp:41) |
| `0x00152DB0` | TopologyTree::GetSquareCorner (signature and full body unresolved) | [src/game_core/indep/src/track.cpp:46](../src/game_core/indep/src/track.cpp:46) |
| `0x00152E08` | TopologyTree::GetSquare (signature and full body unresolved) | [src/game_core/indep/src/track.cpp:51](../src/game_core/indep/src/track.cpp:51) |
| `0x00152F38` | TopologyTree::GetIntersectingPolygons (signature and full body unresolved) | [src/game_core/indep/src/track.cpp:56](../src/game_core/indep/src/track.cpp:56) |
| `0x00153420` | requested track catalog row is absent | [src/port/assets/event_name_catalog.hpp:61](../src/port/assets/event_name_catalog.hpp:61) |
| `0x00153608` | track catalog chunk 0x34201 missing or malformed | [src/port/assets/event_name_catalog.hpp:26](../src/port/assets/event_name_catalog.hpp:26) |
| `0x00153A00` | QueuedJoystickEvents::AddEvent (signature and full body unresolved) | [src/game_core/indep/src/joystick.cpp:20](../src/game_core/indep/src/joystick.cpp:20) |
| `0x00153A50` | QueuedJoystickEvents::RemoveEvent (signature and full body unresolved) | [src/game_core/indep/src/joystick.cpp:25](../src/game_core/indep/src/joystick.cpp:25) |
| `0x00153C20` | Joystick::FindEventNode (signature and full body unresolved) | [src/game_core/indep/src/joystick.cpp:13](../src/game_core/indep/src/joystick.cpp:13) |
| `0x00155948` | tEnvelope::GetIndex (signature and full body unresolved) | [src/game_core/indep/src/rumble.cpp:13](../src/game_core/indep/src/rumble.cpp:13) |
| `0x001559A8` | tEnvelope::OutOfRange (signature and full body unresolved) | [src/game_core/indep/src/rumble.cpp:18](../src/game_core/indep/src/rumble.cpp:18) |
| `0x001559F0` | tEnvelope::GetSlope (signature and full body unresolved) | [src/game_core/indep/src/rumble.cpp:23](../src/game_core/indep/src/rumble.cpp:23) |
| `0x001577D0` | BumperCameraMover::Sub_001577D0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:15](../src/game_core/indep/src/cameramover.cpp:15) |
| `0x001577D0` | CubicCameraMover::Sub_001577D0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:109](../src/game_core/indep/src/cameramover.cpp:109) |
| `0x001577D0` | DebugWorldCameraMover::Sub_001577D0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:200](../src/game_core/indep/src/cameramover.cpp:200) |
| `0x001577D0` | DriveCameraMover::Sub_001577D0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:291](../src/game_core/indep/src/cameramover.cpp:291) |
| `0x001577D0` | MaxCameraMover::Sub_001577D0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:382](../src/game_core/indep/src/cameramover.cpp:382) |
| `0x001577D0` | OrbitCarCameraMover::Sub_001577D0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:473](../src/game_core/indep/src/cameramover.cpp:473) |
| `0x001577D0` | RearViewMirrorCameraMover::Sub_001577D0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:564](../src/game_core/indep/src/cameramover.cpp:564) |
| `0x001577D0` | StillCameraMover::Sub_001577D0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:655](../src/game_core/indep/src/cameramover.cpp:655) |
| `0x001577D0` | TrackCarCameraMover::Sub_001577D0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:746](../src/game_core/indep/src/cameramover.cpp:746) |
| `0x001577D0` | TrackFlyCameraMover::Sub_001577D0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:837](../src/game_core/indep/src/cameramover.cpp:837) |
| `0x001577D0` | TrackRouteEditorCameraMover::Sub_001577D0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:928](../src/game_core/indep/src/cameramover.cpp:928) |
| `0x001577D0` | ZoneFreezeCameraMover::Sub_001577D0 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:25](../src/game_core/indep/src/zonecameras.cpp:25) |
| `0x001577D8` | BumperCameraMover::Sub_001577D8 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:20](../src/game_core/indep/src/cameramover.cpp:20) |
| `0x001577D8` | CubicCameraMover::Sub_001577D8 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:114](../src/game_core/indep/src/cameramover.cpp:114) |
| `0x001577D8` | DebugWorldCameraMover::Sub_001577D8 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:205](../src/game_core/indep/src/cameramover.cpp:205) |
| `0x001577D8` | DriveCameraMover::Sub_001577D8 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:296](../src/game_core/indep/src/cameramover.cpp:296) |
| `0x001577D8` | MaxCameraMover::Sub_001577D8 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:387](../src/game_core/indep/src/cameramover.cpp:387) |
| `0x001577D8` | OrbitCarCameraMover::Sub_001577D8 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:478](../src/game_core/indep/src/cameramover.cpp:478) |
| `0x001577D8` | RearViewMirrorCameraMover::Sub_001577D8 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:569](../src/game_core/indep/src/cameramover.cpp:569) |
| `0x001577D8` | StillCameraMover::Sub_001577D8 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:660](../src/game_core/indep/src/cameramover.cpp:660) |
| `0x001577D8` | TrackCarCameraMover::Sub_001577D8 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:751](../src/game_core/indep/src/cameramover.cpp:751) |
| `0x001577D8` | TrackFlyCameraMover::Sub_001577D8 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:842](../src/game_core/indep/src/cameramover.cpp:842) |
| `0x001577D8` | TrackRouteEditorCameraMover::Sub_001577D8 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:933](../src/game_core/indep/src/cameramover.cpp:933) |
| `0x001577D8` | ZoneFreezeCameraMover::Sub_001577D8 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:30](../src/game_core/indep/src/zonecameras.cpp:30) |
| `0x00157850` | BumperCameraMover::Sub_00157850 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:25](../src/game_core/indep/src/cameramover.cpp:25) |
| `0x00157850` | CubicCameraMover::Sub_00157850 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:119](../src/game_core/indep/src/cameramover.cpp:119) |
| `0x00157850` | DebugWorldCameraMover::Sub_00157850 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:210](../src/game_core/indep/src/cameramover.cpp:210) |
| `0x00157850` | DriveCameraMover::Sub_00157850 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:301](../src/game_core/indep/src/cameramover.cpp:301) |
| `0x00157850` | MaxCameraMover::Sub_00157850 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:392](../src/game_core/indep/src/cameramover.cpp:392) |
| `0x00157850` | OrbitCarCameraMover::Sub_00157850 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:483](../src/game_core/indep/src/cameramover.cpp:483) |
| `0x00157850` | RearViewMirrorCameraMover::Sub_00157850 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:574](../src/game_core/indep/src/cameramover.cpp:574) |
| `0x00157850` | StillCameraMover::Sub_00157850 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:665](../src/game_core/indep/src/cameramover.cpp:665) |
| `0x00157850` | TrackCarCameraMover::Sub_00157850 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:756](../src/game_core/indep/src/cameramover.cpp:756) |
| `0x00157850` | TrackFlyCameraMover::Sub_00157850 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:847](../src/game_core/indep/src/cameramover.cpp:847) |
| `0x00157850` | TrackRouteEditorCameraMover::Sub_00157850 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:938](../src/game_core/indep/src/cameramover.cpp:938) |
| `0x00157850` | ZoneFreezeCameraMover::Sub_00157850 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:35](../src/game_core/indep/src/zonecameras.cpp:35) |
| `0x00157920` | BumperCameraMover::Sub_00157920 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:30](../src/game_core/indep/src/cameramover.cpp:30) |
| `0x00157920` | CubicCameraMover::Sub_00157920 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:124](../src/game_core/indep/src/cameramover.cpp:124) |
| `0x00157920` | DebugWorldCameraMover::Sub_00157920 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:215](../src/game_core/indep/src/cameramover.cpp:215) |
| `0x00157920` | MaxCameraMover::Sub_00157920 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:397](../src/game_core/indep/src/cameramover.cpp:397) |
| `0x00157920` | OrbitCarCameraMover::Sub_00157920 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:488](../src/game_core/indep/src/cameramover.cpp:488) |
| `0x00157920` | RearViewMirrorCameraMover::Sub_00157920 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:579](../src/game_core/indep/src/cameramover.cpp:579) |
| `0x00157920` | StillCameraMover::Sub_00157920 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:670](../src/game_core/indep/src/cameramover.cpp:670) |
| `0x00157920` | TrackCarCameraMover::Sub_00157920 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:761](../src/game_core/indep/src/cameramover.cpp:761) |
| `0x00157920` | TrackFlyCameraMover::Sub_00157920 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:852](../src/game_core/indep/src/cameramover.cpp:852) |
| `0x00157920` | TrackRouteEditorCameraMover::Sub_00157920 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:943](../src/game_core/indep/src/cameramover.cpp:943) |
| `0x00157920` | ZoneFreezeCameraMover::Sub_00157920 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:40](../src/game_core/indep/src/zonecameras.cpp:40) |
| `0x00157920` | ZonePreviewCameraMover::Sub_00157920 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:119](../src/game_core/indep/src/zonecameras.cpp:119) |
| `0x00157E48` | OrbitCarCameraMover::Sub_00157E48 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:493](../src/game_core/indep/src/cameramover.cpp:493) |
| `0x00158080` | OrbitCarCameraMover::Sub_00158080 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:498](../src/game_core/indep/src/cameramover.cpp:498) |
| `0x001580E0` | OrbitCarCameraMover::Sub_001580E0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:503](../src/game_core/indep/src/cameramover.cpp:503) |
| `0x00158100` | OrbitCarCameraMover::Sub_00158100 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:508](../src/game_core/indep/src/cameramover.cpp:508) |
| `0x001581F8` | OrbitCarCameraMover::Sub_001581F8 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:513](../src/game_core/indep/src/cameramover.cpp:513) |
| `0x00158298` | OrbitCarCameraMover::Sub_00158298 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:518](../src/game_core/indep/src/cameramover.cpp:518) |
| `0x00158800` | BumperCameraMover::Sub_00158800 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:35](../src/game_core/indep/src/cameramover.cpp:35) |
| `0x00158910` | BumperCameraMover::Sub_00158910 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:40](../src/game_core/indep/src/cameramover.cpp:40) |
| `0x001589A8` | BumperCameraMover::Sub_001589A8 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:45](../src/game_core/indep/src/cameramover.cpp:45) |
| `0x00158A68` | BumperCameraMover::Sub_00158A68 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:50](../src/game_core/indep/src/cameramover.cpp:50) |
| `0x00158F78` | RearViewMirrorCameraMover::Sub_00158F78 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:584](../src/game_core/indep/src/cameramover.cpp:584) |
| `0x00158FE0` | RearViewMirrorCameraMover::Sub_00158FE0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:589](../src/game_core/indep/src/cameramover.cpp:589) |
| `0x00158FF8` | RearViewMirrorCameraMover::Sub_00158FF8 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:594](../src/game_core/indep/src/cameramover.cpp:594) |
| `0x00159130` | TrackCarCameraMover::Sub_00159130 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:766](../src/game_core/indep/src/cameramover.cpp:766) |
| `0x00159490` | TrackCarCameraMover::Sub_00159490 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:771](../src/game_core/indep/src/cameramover.cpp:771) |
| `0x00159520` | TrackCarCameraMover::Sub_00159520 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:776](../src/game_core/indep/src/cameramover.cpp:776) |
| `0x00159540` | TrackCarCameraMover::CalcSpringForce (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:1031](../src/game_core/indep/src/cameramover.cpp:1031) |
| `0x00159588` | TrackCarCameraMover::Sub_00159588 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:781](../src/game_core/indep/src/cameramover.cpp:781) |
| `0x00159D08` | TrackFlyCameraMover::Sub_00159D08 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:857](../src/game_core/indep/src/cameramover.cpp:857) |
| `0x00159DE8` | TrackFlyCameraMover::Sub_00159DE8 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:862](../src/game_core/indep/src/cameramover.cpp:862) |
| `0x00159E78` | TrackFlyCameraMover::Sub_00159E78 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:867](../src/game_core/indep/src/cameramover.cpp:867) |
| `0x00159E98` | TrackFlyCameraMover::Sub_00159E98 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:872](../src/game_core/indep/src/cameramover.cpp:872) |
| `0x0015A2A0` | TrackFlyCameraMover::IsFinished (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:1038](../src/game_core/indep/src/cameramover.cpp:1038) |
| `0x0015A340` | MaxCameraMover::Sub_0015A340 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:402](../src/game_core/indep/src/cameramover.cpp:402) |
| `0x0015A3F8` | MaxCameraMover::Sub_0015A3F8 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:407](../src/game_core/indep/src/cameramover.cpp:407) |
| `0x0015A410` | MaxCameraMover::Sub_0015A410 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:412](../src/game_core/indep/src/cameramover.cpp:412) |
| `0x0015B268` | DebugWorldCameraMover::Sub_0015B268 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:220](../src/game_core/indep/src/cameramover.cpp:220) |
| `0x0015B318` | DebugWorldCameraMover::Sub_0015B318 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:225](../src/game_core/indep/src/cameramover.cpp:225) |
| `0x0015B360` | DebugWorldCameraMover::Sub_0015B360 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:230](../src/game_core/indep/src/cameramover.cpp:230) |
| `0x0015B4D0` | DebugWorldCameraMover::Sub_0015B4D0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:235](../src/game_core/indep/src/cameramover.cpp:235) |
| `0x0015B878` | DebugWorldCameraMover::Sub_0015B878 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:240](../src/game_core/indep/src/cameramover.cpp:240) |
| `0x0015C010` | TrackRouteEditorCameraMover::Sub_0015C010 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:948](../src/game_core/indep/src/cameramover.cpp:948) |
| `0x0015C058` | TrackRouteEditorCameraMover::Sub_0015C058 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:953](../src/game_core/indep/src/cameramover.cpp:953) |
| `0x0015C0A0` | TrackRouteEditorCameraMover::Sub_0015C0A0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:958](../src/game_core/indep/src/cameramover.cpp:958) |
| `0x0015C138` | DriveCameraMover::Sub_0015C138 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:306](../src/game_core/indep/src/cameramover.cpp:306) |
| `0x0015C1D0` | DriveCameraMover::GetCorrectionPitch (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:1010](../src/game_core/indep/src/cameramover.cpp:1010) |
| `0x0015C258` | DriveCameraMover::Sub_0015C258 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:311](../src/game_core/indep/src/cameramover.cpp:311) |
| `0x0015C400` | DriveCameraMover::Sub_0015C400 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:316](../src/game_core/indep/src/cameramover.cpp:316) |
| `0x0015C470` | DriveCameraMover::Sub_0015C470 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:321](../src/game_core/indep/src/cameramover.cpp:321) |
| `0x0015C528` | DriveCameraMover::Sub_0015C528 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:326](../src/game_core/indep/src/cameramover.cpp:326) |
| `0x0015C760` | DriveCameraMover::Sub_0015C760 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:331](../src/game_core/indep/src/cameramover.cpp:331) |
| `0x0015C7A0` | DriveCameraMover::Sub_0015C7A0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:336](../src/game_core/indep/src/cameramover.cpp:336) |
| `0x0015C838` | DriveCameraMover::Sub_0015C838 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:341](../src/game_core/indep/src/cameramover.cpp:341) |
| `0x0015C880` | DriveCameraMover::Sub_0015C880 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:346](../src/game_core/indep/src/cameramover.cpp:346) |
| `0x0015E5E8` | GarageCameraMover::PrevTrack (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:1019](../src/game_core/indep/src/cameramover.cpp:1019) |
| `0x0015E610` | GarageCameraMover::NextTrack (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:1024](../src/game_core/indep/src/cameramover.cpp:1024) |
| `0x0015E6A0` | StillCameraMover::Sub_0015E6A0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:675](../src/game_core/indep/src/cameramover.cpp:675) |
| `0x0015E718` | StillCameraMover::Sub_0015E718 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:680](../src/game_core/indep/src/cameramover.cpp:680) |
| `0x0015E730` | StillCameraMover::Sub_0015E730 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:685](../src/game_core/indep/src/cameramover.cpp:685) |
| `0x0015E7B0` | CubicCameraMover::Sub_0015E7B0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:129](../src/game_core/indep/src/cameramover.cpp:129) |
| `0x0015EA30` | CubicCameraMover::Sub_0015EA30 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:134](../src/game_core/indep/src/cameramover.cpp:134) |
| `0x0015ECE0` | CubicCameraMover::Sub_0015ECE0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:139](../src/game_core/indep/src/cameramover.cpp:139) |
| `0x0015FA78` | CubicCameraMover::Sub_0015FA78 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:144](../src/game_core/indep/src/cameramover.cpp:144) |
| `0x0015FAF0` | MaxCameraMover::Sub_0015FAF0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:417](../src/game_core/indep/src/cameramover.cpp:417) |
| `0x0015FAF8` | TrackFlyCameraMover::Sub_0015FAF8 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:877](../src/game_core/indep/src/cameramover.cpp:877) |
| `0x0015FB00` | TrackCarCameraMover::Sub_0015FB00 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:786](../src/game_core/indep/src/cameramover.cpp:786) |
| `0x0015FB08` | RearViewMirrorCameraMover::Sub_0015FB08 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:599](../src/game_core/indep/src/cameramover.cpp:599) |
| `0x0015FB10` | BumperCameraMover::Sub_0015FB10 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:55](../src/game_core/indep/src/cameramover.cpp:55) |
| `0x0015FB18` | OrbitCarCameraMover::Sub_0015FB18 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:523](../src/game_core/indep/src/cameramover.cpp:523) |
| `0x0015FBF0` | DriveCameraMover::Sub_0015FBF0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:351](../src/game_core/indep/src/cameramover.cpp:351) |
| `0x0015FC20` | DriveCameraMover::Sub_0015FC20 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:356](../src/game_core/indep/src/cameramover.cpp:356) |
| `0x0015FCB0` | CubicCameraMover::Sub_0015FCB0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:149](../src/game_core/indep/src/cameramover.cpp:149) |
| `0x0015FCB0` | DebugWorldCameraMover::Sub_0015FCB0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:245](../src/game_core/indep/src/cameramover.cpp:245) |
| `0x0015FCB0` | MaxCameraMover::Sub_0015FCB0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:422](../src/game_core/indep/src/cameramover.cpp:422) |
| `0x0015FCB0` | RearViewMirrorCameraMover::Sub_0015FCB0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:604](../src/game_core/indep/src/cameramover.cpp:604) |
| `0x0015FCB0` | StillCameraMover::Sub_0015FCB0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:690](../src/game_core/indep/src/cameramover.cpp:690) |
| `0x0015FCB0` | TrackRouteEditorCameraMover::Sub_0015FCB0 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:963](../src/game_core/indep/src/cameramover.cpp:963) |
| `0x0015FCB0` | ZoneFreezeCameraMover::Sub_0015FCB0 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:45](../src/game_core/indep/src/zonecameras.cpp:45) |
| `0x0015FCB0` | ZonePreviewCameraMover::Sub_0015FCB0 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:124](../src/game_core/indep/src/zonecameras.cpp:124) |
| `0x0015FCF8` | DebugWorldCameraMover::Sub_0015FCF8 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:250](../src/game_core/indep/src/cameramover.cpp:250) |
| `0x0015FCF8` | StillCameraMover::Sub_0015FCF8 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:695](../src/game_core/indep/src/cameramover.cpp:695) |
| `0x0015FCF8` | TrackRouteEditorCameraMover::Sub_0015FCF8 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:968](../src/game_core/indep/src/cameramover.cpp:968) |
| `0x0015FCF8` | ZoneFreezeCameraMover::Sub_0015FCF8 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:50](../src/game_core/indep/src/zonecameras.cpp:50) |
| `0x0015FCF8` | ZonePreviewCameraMover::Sub_0015FCF8 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:129](../src/game_core/indep/src/zonecameras.cpp:129) |
| `0x0015FD10` | BumperCameraMover::Sub_0015FD10 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:60](../src/game_core/indep/src/cameramover.cpp:60) |
| `0x0015FD10` | CubicCameraMover::Sub_0015FD10 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:154](../src/game_core/indep/src/cameramover.cpp:154) |
| `0x0015FD10` | DebugWorldCameraMover::Sub_0015FD10 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:255](../src/game_core/indep/src/cameramover.cpp:255) |
| `0x0015FD10` | MaxCameraMover::Sub_0015FD10 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:427](../src/game_core/indep/src/cameramover.cpp:427) |
| `0x0015FD10` | OrbitCarCameraMover::Sub_0015FD10 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:528](../src/game_core/indep/src/cameramover.cpp:528) |
| `0x0015FD10` | RearViewMirrorCameraMover::Sub_0015FD10 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:609](../src/game_core/indep/src/cameramover.cpp:609) |
| `0x0015FD10` | StillCameraMover::Sub_0015FD10 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:700](../src/game_core/indep/src/cameramover.cpp:700) |
| `0x0015FD10` | TrackCarCameraMover::Sub_0015FD10 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:791](../src/game_core/indep/src/cameramover.cpp:791) |
| `0x0015FD10` | TrackFlyCameraMover::Sub_0015FD10 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:882](../src/game_core/indep/src/cameramover.cpp:882) |
| `0x0015FD10` | TrackRouteEditorCameraMover::Sub_0015FD10 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:973](../src/game_core/indep/src/cameramover.cpp:973) |
| `0x0015FD10` | ZoneFreezeCameraMover::Sub_0015FD10 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:55](../src/game_core/indep/src/zonecameras.cpp:55) |
| `0x0015FD10` | ZonePreviewCameraMover::Sub_0015FD10 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:134](../src/game_core/indep/src/zonecameras.cpp:134) |
| `0x0015FD18` | BumperCameraMover::Sub_0015FD18 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:65](../src/game_core/indep/src/cameramover.cpp:65) |
| `0x0015FD18` | CubicCameraMover::Sub_0015FD18 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:159](../src/game_core/indep/src/cameramover.cpp:159) |
| `0x0015FD18` | DebugWorldCameraMover::Sub_0015FD18 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:260](../src/game_core/indep/src/cameramover.cpp:260) |
| `0x0015FD18` | MaxCameraMover::Sub_0015FD18 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:432](../src/game_core/indep/src/cameramover.cpp:432) |
| `0x0015FD18` | OrbitCarCameraMover::Sub_0015FD18 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:533](../src/game_core/indep/src/cameramover.cpp:533) |
| `0x0015FD18` | RearViewMirrorCameraMover::Sub_0015FD18 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:614](../src/game_core/indep/src/cameramover.cpp:614) |
| `0x0015FD18` | StillCameraMover::Sub_0015FD18 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:705](../src/game_core/indep/src/cameramover.cpp:705) |
| `0x0015FD18` | TrackCarCameraMover::Sub_0015FD18 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:796](../src/game_core/indep/src/cameramover.cpp:796) |
| `0x0015FD18` | TrackFlyCameraMover::Sub_0015FD18 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:887](../src/game_core/indep/src/cameramover.cpp:887) |
| `0x0015FD18` | TrackRouteEditorCameraMover::Sub_0015FD18 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:978](../src/game_core/indep/src/cameramover.cpp:978) |
| `0x0015FD18` | ZoneFreezeCameraMover::Sub_0015FD18 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:60](../src/game_core/indep/src/zonecameras.cpp:60) |
| `0x0015FD18` | ZonePreviewCameraMover::Sub_0015FD18 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:139](../src/game_core/indep/src/zonecameras.cpp:139) |
| `0x0015FD20` | BumperCameraMover::Sub_0015FD20 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:70](../src/game_core/indep/src/cameramover.cpp:70) |
| `0x0015FD20` | CubicCameraMover::Sub_0015FD20 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:164](../src/game_core/indep/src/cameramover.cpp:164) |
| `0x0015FD20` | DebugWorldCameraMover::Sub_0015FD20 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:265](../src/game_core/indep/src/cameramover.cpp:265) |
| `0x0015FD20` | MaxCameraMover::Sub_0015FD20 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:437](../src/game_core/indep/src/cameramover.cpp:437) |
| `0x0015FD20` | OrbitCarCameraMover::Sub_0015FD20 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:538](../src/game_core/indep/src/cameramover.cpp:538) |
| `0x0015FD20` | RearViewMirrorCameraMover::Sub_0015FD20 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:619](../src/game_core/indep/src/cameramover.cpp:619) |
| `0x0015FD20` | StillCameraMover::Sub_0015FD20 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:710](../src/game_core/indep/src/cameramover.cpp:710) |
| `0x0015FD20` | TrackCarCameraMover::Sub_0015FD20 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:801](../src/game_core/indep/src/cameramover.cpp:801) |
| `0x0015FD20` | TrackFlyCameraMover::Sub_0015FD20 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:892](../src/game_core/indep/src/cameramover.cpp:892) |
| `0x0015FD20` | TrackRouteEditorCameraMover::Sub_0015FD20 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:983](../src/game_core/indep/src/cameramover.cpp:983) |
| `0x0015FD20` | ZoneFreezeCameraMover::Sub_0015FD20 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:65](../src/game_core/indep/src/zonecameras.cpp:65) |
| `0x0015FD20` | ZonePreviewCameraMover::Sub_0015FD20 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:144](../src/game_core/indep/src/zonecameras.cpp:144) |
| `0x0015FD28` | BumperCameraMover::Sub_0015FD28 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:75](../src/game_core/indep/src/cameramover.cpp:75) |
| `0x0015FD28` | CubicCameraMover::Sub_0015FD28 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:169](../src/game_core/indep/src/cameramover.cpp:169) |
| `0x0015FD28` | DebugWorldCameraMover::Sub_0015FD28 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:270](../src/game_core/indep/src/cameramover.cpp:270) |
| `0x0015FD28` | MaxCameraMover::Sub_0015FD28 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:442](../src/game_core/indep/src/cameramover.cpp:442) |
| `0x0015FD28` | OrbitCarCameraMover::Sub_0015FD28 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:543](../src/game_core/indep/src/cameramover.cpp:543) |
| `0x0015FD28` | RearViewMirrorCameraMover::Sub_0015FD28 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:624](../src/game_core/indep/src/cameramover.cpp:624) |
| `0x0015FD28` | StillCameraMover::Sub_0015FD28 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:715](../src/game_core/indep/src/cameramover.cpp:715) |
| `0x0015FD28` | TrackCarCameraMover::Sub_0015FD28 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:806](../src/game_core/indep/src/cameramover.cpp:806) |
| `0x0015FD28` | TrackFlyCameraMover::Sub_0015FD28 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:897](../src/game_core/indep/src/cameramover.cpp:897) |
| `0x0015FD28` | TrackRouteEditorCameraMover::Sub_0015FD28 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:988](../src/game_core/indep/src/cameramover.cpp:988) |
| `0x0015FD28` | ZoneFreezeCameraMover::Sub_0015FD28 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:70](../src/game_core/indep/src/zonecameras.cpp:70) |
| `0x0015FD28` | ZonePreviewCameraMover::Sub_0015FD28 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:149](../src/game_core/indep/src/zonecameras.cpp:149) |
| `0x0015FD30` | BumperCameraMover::Sub_0015FD30 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:80](../src/game_core/indep/src/cameramover.cpp:80) |
| `0x0015FD30` | CubicCameraMover::Sub_0015FD30 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:174](../src/game_core/indep/src/cameramover.cpp:174) |
| `0x0015FD30` | DebugWorldCameraMover::Sub_0015FD30 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:275](../src/game_core/indep/src/cameramover.cpp:275) |
| `0x0015FD30` | MaxCameraMover::Sub_0015FD30 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:447](../src/game_core/indep/src/cameramover.cpp:447) |
| `0x0015FD30` | OrbitCarCameraMover::Sub_0015FD30 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:548](../src/game_core/indep/src/cameramover.cpp:548) |
| `0x0015FD30` | RearViewMirrorCameraMover::Sub_0015FD30 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:629](../src/game_core/indep/src/cameramover.cpp:629) |
| `0x0015FD30` | StillCameraMover::Sub_0015FD30 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:720](../src/game_core/indep/src/cameramover.cpp:720) |
| `0x0015FD30` | TrackCarCameraMover::Sub_0015FD30 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:811](../src/game_core/indep/src/cameramover.cpp:811) |
| `0x0015FD30` | TrackFlyCameraMover::Sub_0015FD30 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:902](../src/game_core/indep/src/cameramover.cpp:902) |
| `0x0015FD30` | TrackRouteEditorCameraMover::Sub_0015FD30 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:993](../src/game_core/indep/src/cameramover.cpp:993) |
| `0x0015FD30` | ZoneFreezeCameraMover::Sub_0015FD30 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:75](../src/game_core/indep/src/zonecameras.cpp:75) |
| `0x0015FD30` | ZonePreviewCameraMover::Sub_0015FD30 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:154](../src/game_core/indep/src/zonecameras.cpp:154) |
| `0x0015FD38` | BumperCameraMover::Sub_0015FD38 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:85](../src/game_core/indep/src/cameramover.cpp:85) |
| `0x0015FD38` | CubicCameraMover::Sub_0015FD38 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:179](../src/game_core/indep/src/cameramover.cpp:179) |
| `0x0015FD38` | DriveCameraMover::Sub_0015FD38 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:361](../src/game_core/indep/src/cameramover.cpp:361) |
| `0x0015FD38` | MaxCameraMover::Sub_0015FD38 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:452](../src/game_core/indep/src/cameramover.cpp:452) |
| `0x0015FD38` | RearViewMirrorCameraMover::Sub_0015FD38 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:634](../src/game_core/indep/src/cameramover.cpp:634) |
| `0x0015FD38` | StillCameraMover::Sub_0015FD38 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:725](../src/game_core/indep/src/cameramover.cpp:725) |
| `0x0015FD38` | TrackCarCameraMover::Sub_0015FD38 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:816](../src/game_core/indep/src/cameramover.cpp:816) |
| `0x0015FD38` | TrackFlyCameraMover::Sub_0015FD38 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:907](../src/game_core/indep/src/cameramover.cpp:907) |
| `0x0015FD38` | TrackRouteEditorCameraMover::Sub_0015FD38 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:998](../src/game_core/indep/src/cameramover.cpp:998) |
| `0x0015FD38` | ZoneFreezeCameraMover::Sub_0015FD38 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:80](../src/game_core/indep/src/zonecameras.cpp:80) |
| `0x0015FD38` | ZonePreviewCameraMover::Sub_0015FD38 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:159](../src/game_core/indep/src/zonecameras.cpp:159) |
| `0x0015FD40` | BumperCameraMover::Sub_0015FD40 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:90](../src/game_core/indep/src/cameramover.cpp:90) |
| `0x0015FD40` | CubicCameraMover::Sub_0015FD40 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:184](../src/game_core/indep/src/cameramover.cpp:184) |
| `0x0015FD40` | DriveCameraMover::Sub_0015FD40 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:366](../src/game_core/indep/src/cameramover.cpp:366) |
| `0x0015FD40` | MaxCameraMover::Sub_0015FD40 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:457](../src/game_core/indep/src/cameramover.cpp:457) |
| `0x0015FD40` | RearViewMirrorCameraMover::Sub_0015FD40 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:639](../src/game_core/indep/src/cameramover.cpp:639) |
| `0x0015FD40` | StillCameraMover::Sub_0015FD40 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:730](../src/game_core/indep/src/cameramover.cpp:730) |
| `0x0015FD40` | TrackCarCameraMover::Sub_0015FD40 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:821](../src/game_core/indep/src/cameramover.cpp:821) |
| `0x0015FD40` | TrackFlyCameraMover::Sub_0015FD40 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:912](../src/game_core/indep/src/cameramover.cpp:912) |
| `0x0015FD40` | TrackRouteEditorCameraMover::Sub_0015FD40 (signature and full body unresolved) | [src/game_core/indep/src/cameramover.cpp:1003](../src/game_core/indep/src/cameramover.cpp:1003) |
| `0x0015FD40` | ZoneFreezeCameraMover::Sub_0015FD40 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:85](../src/game_core/indep/src/zonecameras.cpp:85) |
| `0x0015FD40` | ZonePreviewCameraMover::Sub_0015FD40 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:164](../src/game_core/indep/src/zonecameras.cpp:164) |
| `0x00160298` | ZoneFreezeCameraMover::Sub_00160298 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:90](../src/game_core/indep/src/zonecameras.cpp:90) |
| `0x00161000` | ZoneFreezeCameraMover::Sub_00161000 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:95](../src/game_core/indep/src/zonecameras.cpp:95) |
| `0x001610A8` | ZoneFreezeCameraMover::Sub_001610A8 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:100](../src/game_core/indep/src/zonecameras.cpp:100) |
| `0x00161F98` | ZonePreviewCameraMover::Sub_00161F98 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:169](../src/game_core/indep/src/zonecameras.cpp:169) |
| `0x00163620` | ZonePreviewCameraMover::Sub_00163620 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:174](../src/game_core/indep/src/zonecameras.cpp:174) |
| `0x00163790` | ZonePreviewCameraMover::Sub_00163790 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:179](../src/game_core/indep/src/zonecameras.cpp:179) |
| `0x00163978` | ZonePreviewCameraMover::Sub_00163978 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:184](../src/game_core/indep/src/zonecameras.cpp:184) |
| `0x001639A0` | ZonePreviewCameraMover::Sub_001639A0 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:189](../src/game_core/indep/src/zonecameras.cpp:189) |
| `0x001639D0` | ZonePreviewCameraMover::Sub_001639D0 (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:194](../src/game_core/indep/src/zonecameras.cpp:194) |
| `0x001657A0` | PreviewPause::GetPoint (signature and full body unresolved) | [src/game_core/indep/src/zonecameras.cpp:201](../src/game_core/indep/src/zonecameras.cpp:201) |
| `0x00167A08` | TrafficCar::Sub_00167A08 (signature and full body unresolved) | [src/game_core/indep/src/trafficcar.cpp:16](../src/game_core/indep/src/trafficcar.cpp:16) |
| `0x00167A60` | TrafficCar::Sub_00167A60 (signature and full body unresolved) | [src/game_core/indep/src/trafficcar.cpp:21](../src/game_core/indep/src/trafficcar.cpp:21) |
| `0x00167AF8` | TrafficCar::Sub_00167AF8 (signature and full body unresolved) | [src/game_core/indep/src/trafficcar.cpp:26](../src/game_core/indep/src/trafficcar.cpp:26) |
| `0x00168088` | TrafficCar::Sub_00168088 (signature and full body unresolved) | [src/game_core/indep/src/trafficcar.cpp:31](../src/game_core/indep/src/trafficcar.cpp:31) |
| `0x0016A130` | FlailerMenu::GetCurrentItemNumber (signature and full body unresolved) | [src/game_core/indep/src/fmenus.cpp:1298](../src/game_core/indep/src/fmenus.cpp:1298) |
| `0x0016A5B0` | FlailerMenu::FindItem (signature and full body unresolved) | [src/game_core/indep/src/fmenus.cpp:1303](../src/game_core/indep/src/fmenus.cpp:1303) |
| `0x0016ADB0` | registered document not present in loaded bundle | [src/port/runtime/frontend_runtime.cpp:675](../src/port/runtime/frontend_runtime.cpp:675) |
| `0x0016B340` | FlailerScreenNode::FindFlailerScreen (signature and full body unresolved) | [src/game_core/indep/src/fmenus.cpp:1324](../src/game_core/indep/src/fmenus.cpp:1324) |
| `0x0016B378` | FlailerScreenNode::SetFilterPort (signature and full body unresolved) | [src/game_core/indep/src/fmenus.cpp:1329](../src/game_core/indep/src/fmenus.cpp:1329) |
| `0x0016B3B0` | FlailerScreenNode::IsPortFiltered (signature and full body unresolved) | [src/game_core/indep/src/fmenus.cpp:1334](../src/game_core/indep/src/fmenus.cpp:1334) |
| `0x0016B418` | history overflow | [src/port/runtime/frontend_runtime.cpp:751](../src/port/runtime/frontend_runtime.cpp:751) |
| `0x0016B418` | history overflow behavior not recovered | [src/port/runtime/frontend_runtime.cpp:678](../src/port/runtime/frontend_runtime.cpp:678) |
| `0x0016B438` | ScreenStack::Pop (signature and full body unresolved) | [src/game_core/indep/src/fmenus.cpp:1341](../src/game_core/indep/src/fmenus.cpp:1341) |
| `0x0016B5A8` | tPopupMenu::GetCurrentMenuItem (signature and full body unresolved) | [src/game_core/indep/src/fmenus.cpp:1350](../src/game_core/indep/src/fmenus.cpp:1350) |
| `0x0016BAC0` | pop-up / help menu joypad handling (POPUP_1..3 handler table unrecovered) | [src/game_core/indep/src/fmenus.cpp:718](../src/game_core/indep/src/fmenus.cpp:718) |
| `0x0016C4C0` | target menu construction failed | [src/port/runtime/frontend_runtime.cpp:686](../src/port/runtime/frontend_runtime.cpp:686) |
| `0x0016C670` | Back class registration missing | [src/port/runtime/frontend_runtime.cpp:731](../src/port/runtime/frontend_runtime.cpp:731) |
| `0x0016C670` | Back side-effect dependency not translated | [src/port/runtime/frontend_runtime.cpp:748](../src/port/runtime/frontend_runtime.cpp:748) |
| `0x0016C670` | Back side-effect service missing | [src/game_core/indep/src/frontendscreens/options.cpp:170](../src/game_core/indep/src/frontendscreens/options.cpp:170) |
| `0x0016C670` | screen Back handler not recovered | [src/game_core/indep/src/frontendscreens/options.cpp:266](../src/game_core/indep/src/frontendscreens/options.cpp:266) |
| `0x0016C888` | FlailerScreenList::FindScreen (signature and full body unresolved) | [src/game_core/indep/src/fmenus.cpp:1312](../src/game_core/indep/src/fmenus.cpp:1312) |
| `0x0016C8C8` | requested class not recovered | [src/port/runtime/frontend_runtime.cpp:673](../src/port/runtime/frontend_runtime.cpp:673) |
| `0x0016CA08` | FlailerScreenList::IsAnyPopupActive (signature and full body unresolved) | [src/game_core/indep/src/fmenus.cpp:1317](../src/game_core/indep/src/fmenus.cpp:1317) |
| `0x0016F190` | TitleScreen::Sub_0016F190 (signature and full body unresolved) | [src/game_core/indep/src/fmenus.cpp:1291](../src/game_core/indep/src/fmenus.cpp:1291) |
| `0x0016FBB0` | global FMenus constructor bindings (main-menu pulse is implemented separately) | [src/game_core/indep/src/fmenus.cpp:723](../src/game_core/indep/src/fmenus.cpp:723) |
| `0x001715C0` | PauseAudioOpts::FindSlider (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/pausemenu.cpp:43](../src/game_core/indep/src/frontendscreens/pausemenu.cpp:43) |
| `0x001841A8` | FrontendDatabase::UseTheRaceEvent (signature and full body unresolved) | [src/game_core/indep/src/fred.cpp:17](../src/game_core/indep/src/fred.cpp:17) |
| `0x001855F0` | FrontendDatabase::IsChallengeGame (signature and full body unresolved) | [src/game_core/indep/src/fred.cpp:22](../src/game_core/indep/src/fred.cpp:22) |
| `0x00187080` | TransientStats::GetDriver (signature and full body unresolved) | [src/game_core/indep/src/fred.cpp:36](../src/game_core/indep/src/fred.cpp:36) |
| `0x00187928` | FrontendPlayerInfo::FlushAvailableCars (signature and full body unresolved) | [src/game_core/indep/src/fred.cpp:29](../src/game_core/indep/src/fred.cpp:29) |
| `0x00187EB8` | original car database name lookup missing | [src/game_core/indep/src/eventtree.cpp:652](../src/game_core/indep/src/eventtree.cpp:652) |
| `0x00187F98` | car catalog chunk34600 missing | [src/port/assets/event_name_catalog.hpp:44](../src/port/assets/event_name_catalog.hpp:44) |
| `0x001884B8` | CarTypeInfo::GetNumAvailableSkins (signature and full body unresolved) | [src/game_core/indep/src/carinfo.cpp:14](../src/game_core/indep/src/carinfo.cpp:14) |
| `0x00188DE8` | non-progressing exceptional table scan | [src/game_core/indep/src/engine.cpp:176](../src/game_core/indep/src/engine.cpp:176) |
| `0x00189238` | Spline::Sub_00189238 (signature and full body unresolved) | [src/game_core/indep/src/spline.cpp:13](../src/game_core/indep/src/spline.cpp:13) |
| `0x0018A030` | Spline::LinearEstimateLength (signature and full body unresolved) | [src/game_core/indep/src/spline.cpp:18](../src/game_core/indep/src/spline.cpp:18) |
| `0x0018AAB0` | engine curve evaluation dependency | [src/game_core/port/driving/projection_support.hpp:110](../src/game_core/port/driving/projection_support.hpp:110) |
| `0x0018C380` | TrafficDriver::Sub_0018C380 (signature and full body unresolved) | [src/game_core/indep/src/trafficdriver.cpp:15](../src/game_core/indep/src/trafficdriver.cpp:15) |
| `0x0018C418` | TrafficDriver::Sub_0018C418 (signature and full body unresolved) | [src/game_core/indep/src/trafficdriver.cpp:20](../src/game_core/indep/src/trafficdriver.cpp:20) |
| `0x0018C478` | TrafficDriver::Sub_0018C478 (signature and full body unresolved) | [src/game_core/indep/src/trafficdriver.cpp:25](../src/game_core/indep/src/trafficdriver.cpp:25) |
| `0x0018C4D8` | TrafficDriver::Sub_0018C4D8 (signature and full body unresolved) | [src/game_core/indep/src/trafficdriver.cpp:30](../src/game_core/indep/src/trafficdriver.cpp:30) |
| `0x0018CAE8` | TrafficDriver::Sub_0018CAE8 (signature and full body unresolved) | [src/game_core/indep/src/trafficdriver.cpp:35](../src/game_core/indep/src/trafficdriver.cpp:35) |
| `0x0018CB10` | TrafficDriver::Sub_0018CB10 (signature and full body unresolved) | [src/game_core/indep/src/trafficdriver.cpp:40](../src/game_core/indep/src/trafficdriver.cpp:40) |
| `0x0018D258` | BaseCarSound::Sub_0018D258 (signature and full body unresolved) | [src/game_core/indep/src/carsounds.cpp:133](../src/game_core/indep/src/carsounds.cpp:133) |
| `0x0018D340` | BaseCarSound::Update (signature and full body unresolved) | [src/game_core/indep/src/carsounds.cpp:138](../src/game_core/indep/src/carsounds.cpp:138) |
| `0x0018D480` | BaseCarSound::GetHornSample (signature and full body unresolved) | [src/game_core/indep/src/carsounds.cpp:31](../src/game_core/indep/src/carsounds.cpp:31) |
| `0x0018D550` | BaseCarSound::Mute (signature and full body unresolved) | [src/game_core/indep/src/carsounds.cpp:36](../src/game_core/indep/src/carsounds.cpp:36) |
| `0x0018D750` | BaseCarSound::InSoundSphere (signature and full body unresolved) | [src/game_core/indep/src/carsounds.cpp:143](../src/game_core/indep/src/carsounds.cpp:143) |
| `0x0018D830` | BaseCarSound::DeactivateSound (signature and full body unresolved) | [src/game_core/indep/src/carsounds.cpp:41](../src/game_core/indep/src/carsounds.cpp:41) |
| `0x0018D880` | Port CVT.W.S exceptional operand (Debug instruction anchor) | [src/game_core/port/driving/adapters.cpp:29](../src/game_core/port/driving/adapters.cpp:29) |
| `0x0018D8D8` | BaseCarSound::ActivateSound (signature and full body unresolved) | [src/game_core/indep/src/carsounds.cpp:46](../src/game_core/indep/src/carsounds.cpp:46) |
| `0x0018D9A0` | PlayerCarSound::Sub_0018D9A0 (signature and full body unresolved) | [src/game_core/indep/src/PlayerCarSound.cpp:20](../src/game_core/indep/src/PlayerCarSound.cpp:20) |
| `0x0018DB78` | BasePlayerCarSound::Sub_0018DB78 (signature and full body unresolved) | [src/game_core/indep/src/carsounds.cpp:55](../src/game_core/indep/src/carsounds.cpp:55) |
| `0x0018DBF0` | BasePlayerCarSound::Update (signature and full body unresolved) | [src/game_core/indep/src/carsounds.cpp:60](../src/game_core/indep/src/carsounds.cpp:60) |
| `0x0018DD10` | BasePlayerCarSound::Mute (signature and full body unresolved) | [src/game_core/indep/src/carsounds.cpp:65](../src/game_core/indep/src/carsounds.cpp:65) |
| `0x0018DDA0` | BasePlayerCarSound::GetHornSample (signature and full body unresolved) | [src/game_core/indep/src/carsounds.cpp:70](../src/game_core/indep/src/carsounds.cpp:70) |
| `0x0018E6E8` | AICarSound::Sub_0018E6E8 (signature and full body unresolved) | [src/game_core/AICarSound.cpp:21](../src/game_core/AICarSound.cpp:21) |
| `0x0018E7E8` | BaseAICarSound::Sub_0018E7E8 (signature and full body unresolved) | [src/game_core/indep/src/carsounds.cpp:17](../src/game_core/indep/src/carsounds.cpp:17) |
| `0x0018E800` | BaseAICarSound::Update (signature and full body unresolved) | [src/game_core/indep/src/carsounds.cpp:22](../src/game_core/indep/src/carsounds.cpp:22) |
| `0x0018F160` | HornSound3d::ActivateSound (signature and full body unresolved) | [src/game_core/indep/src/carsounds.cpp:77](../src/game_core/indep/src/carsounds.cpp:77) |
| `0x0018F188` | HornSound3d::DeactivateSound (signature and full body unresolved) | [src/game_core/indep/src/carsounds.cpp:82](../src/game_core/indep/src/carsounds.cpp:82) |
| `0x0018F208` | HornSound3d::Mute (signature and full body unresolved) | [src/game_core/indep/src/carsounds.cpp:87](../src/game_core/indep/src/carsounds.cpp:87) |
| `0x0018F218` | HornSound3d::Update (signature and full body unresolved) | [src/game_core/indep/src/carsounds.cpp:92](../src/game_core/indep/src/carsounds.cpp:92) |
| `0x0018F2B0` | HornSound3d::Sub_0018F2B0 (signature and full body unresolved) | [src/game_core/indep/src/carsounds.cpp:97](../src/game_core/indep/src/carsounds.cpp:97) |
| `0x0018F388` | SirenSound::Sub_0018F388 (signature and full body unresolved) | [src/game_core/SirenSound.cpp:13](../src/game_core/SirenSound.cpp:13) |
| `0x0018F4D0` | SirenSound3d::ActivateSound (signature and full body unresolved) | [src/game_core/indep/src/carsounds.cpp:106](../src/game_core/indep/src/carsounds.cpp:106) |
| `0x0018F508` | SirenSound3d::DeactivateSound (signature and full body unresolved) | [src/game_core/indep/src/carsounds.cpp:111](../src/game_core/indep/src/carsounds.cpp:111) |
| `0x0018F528` | SirenSound3d::Mute (signature and full body unresolved) | [src/game_core/indep/src/carsounds.cpp:116](../src/game_core/indep/src/carsounds.cpp:116) |
| `0x0018F548` | SirenSound3d::Update (signature and full body unresolved) | [src/game_core/indep/src/carsounds.cpp:121](../src/game_core/indep/src/carsounds.cpp:121) |
| `0x0018F710` | SirenSound3d::Sub_0018F710 (signature and full body unresolved) | [src/game_core/indep/src/carsounds.cpp:126](../src/game_core/indep/src/carsounds.cpp:126) |
| `0x0018FCB8` | Average::Record (signature and full body unresolved) | [src/game_core/indep/src/table.cpp:46](../src/game_core/indep/src/table.cpp:46) |
| `0x0018FD50` | Average::Reset (signature and full body unresolved) | [src/game_core/indep/src/table.cpp:51](../src/game_core/indep/src/table.cpp:51) |
| `0x0018FDA0` | Average::Flush (signature and full body unresolved) | [src/game_core/indep/src/table.cpp:56](../src/game_core/indep/src/table.cpp:56) |
| `0x00190050` | Linear::Init (signature and full body unresolved) | [src/game_core/indep/src/table.cpp:68](../src/game_core/indep/src/table.cpp:68) |
| `0x00190670` | AIDriver::Sub_00190670 (signature and full body unresolved) | [src/game_core/indep/src/aidriver.cpp:45](../src/game_core/indep/src/aidriver.cpp:45) |
| `0x00190670` | TrafficDriver::Sub_00190670 (signature and full body unresolved) | [src/game_core/indep/src/trafficdriver.cpp:45](../src/game_core/indep/src/trafficdriver.cpp:45) |
| `0x00190708` | AIDriver::Sub_00190708 (signature and full body unresolved) | [src/game_core/indep/src/aidriver.cpp:50](../src/game_core/indep/src/aidriver.cpp:50) |
| `0x00190708` | TrafficDriver::Sub_00190708 (signature and full body unresolved) | [src/game_core/indep/src/trafficdriver.cpp:50](../src/game_core/indep/src/trafficdriver.cpp:50) |
| `0x00190760` | AIDriver::Sub_00190760 (signature and full body unresolved) | [src/game_core/indep/src/aidriver.cpp:55](../src/game_core/indep/src/aidriver.cpp:55) |
| `0x00190760` | TrafficDriver::Sub_00190760 (signature and full body unresolved) | [src/game_core/indep/src/trafficdriver.cpp:55](../src/game_core/indep/src/trafficdriver.cpp:55) |
| `0x001907B8` | AIDriver::Sub_001907B8 (signature and full body unresolved) | [src/game_core/indep/src/aidriver.cpp:60](../src/game_core/indep/src/aidriver.cpp:60) |
| `0x001907B8` | TrafficDriver::Sub_001907B8 (signature and full body unresolved) | [src/game_core/indep/src/trafficdriver.cpp:60](../src/game_core/indep/src/trafficdriver.cpp:60) |
| `0x00190850` | AIDriver::Sub_00190850 (signature and full body unresolved) | [src/game_core/indep/src/aidriver.cpp:65](../src/game_core/indep/src/aidriver.cpp:65) |
| `0x00191138` | AIDriver::Sub_00191138 (signature and full body unresolved) | [src/game_core/indep/src/aidriver.cpp:70](../src/game_core/indep/src/aidriver.cpp:70) |
| `0x00191138` | TrafficDriver::Sub_00191138 (signature and full body unresolved) | [src/game_core/indep/src/trafficdriver.cpp:65](../src/game_core/indep/src/trafficdriver.cpp:65) |
| `0x00191D10` | AICarController::ClearStickyDecisions (signature and full body unresolved) | [src/game_core/indep/src/aicarcontroller.cpp:36](../src/game_core/indep/src/aicarcontroller.cpp:36) |
| `0x00191D90` | AICarController::GetStickyIndex (signature and full body unresolved) | [src/game_core/indep/src/aicarcontroller.cpp:41](../src/game_core/indep/src/aicarcontroller.cpp:41) |
| `0x00192668` | AIDriver::Sub_00192668 (signature and full body unresolved) | [src/game_core/indep/src/aidriver.cpp:75](../src/game_core/indep/src/aidriver.cpp:75) |
| `0x00192668` | TrafficDriver::Sub_00192668 (signature and full body unresolved) | [src/game_core/indep/src/trafficdriver.cpp:70](../src/game_core/indep/src/trafficdriver.cpp:70) |
| `0x00192780` | TrafficDriver::Sub_00192780 (signature and full body unresolved) | [src/game_core/indep/src/trafficdriver.cpp:75](../src/game_core/indep/src/trafficdriver.cpp:75) |
| `0x00192F10` | AIDriver::Sub_00192F10 (signature and full body unresolved) | [src/game_core/indep/src/aidriver.cpp:80](../src/game_core/indep/src/aidriver.cpp:80) |
| `0x00192F10` | TrafficDriver::Sub_00192F10 (signature and full body unresolved) | [src/game_core/indep/src/trafficdriver.cpp:80](../src/game_core/indep/src/trafficdriver.cpp:80) |
| `0x00193650` | AIDriver::Sub_00193650 (signature and full body unresolved) | [src/game_core/indep/src/aidriver.cpp:85](../src/game_core/indep/src/aidriver.cpp:85) |
| `0x00193650` | TrafficDriver::Sub_00193650 (signature and full body unresolved) | [src/game_core/indep/src/trafficdriver.cpp:85](../src/game_core/indep/src/trafficdriver.cpp:85) |
| `0x00193758` | AICarController::ForceBoost (signature and full body unresolved) | [src/game_core/indep/src/aicarcontroller.cpp:24](../src/game_core/indep/src/aicarcontroller.cpp:24) |
| `0x00193780` | AICarController::ReleaseBoost (signature and full body unresolved) | [src/game_core/indep/src/aicarcontroller.cpp:29](../src/game_core/indep/src/aicarcontroller.cpp:29) |
| `0x00193D90` | NearbyCarList::GetCarsIndexInArray (signature and full body unresolved) | [src/game_core/indep/src/aicarcontroller.cpp:57](../src/game_core/indep/src/aicarcontroller.cpp:57) |
| `0x00193E68` | DrivingInformation::Clear (signature and full body unresolved) | [src/game_core/indep/src/aicarcontroller.cpp:50](../src/game_core/indep/src/aicarcontroller.cpp:50) |
| `0x001942D0` | VisibleCar::SetCurrentSkinNumber (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/selectcar.cpp:27](../src/game_core/indep/src/frontendscreens/selectcar.cpp:27) |
| `0x00194350` | VisibleCar::NextSkin (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/selectcar.cpp:32](../src/game_core/indep/src/frontendscreens/selectcar.cpp:32) |
| `0x00194380` | VisibleCar::PrevSkin (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/selectcar.cpp:37](../src/game_core/indep/src/frontendscreens/selectcar.cpp:37) |
| `0x00195C40` | CarSelector::SetupCarOverrideModel (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/selectcar.cpp:13](../src/game_core/indep/src/frontendscreens/selectcar.cpp:13) |
| `0x00198878` | SelectCarScreen::AreAllSelectorsReady (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/selectcar.cpp:20](../src/game_core/indep/src/frontendscreens/selectcar.cpp:20) |
| `0x001995E0` | ResourceFile::GetSize (signature and full body unresolved) | [src/game_core/indep/src/resourceloader.cpp:15](../src/game_core/indep/src/resourceloader.cpp:15) |
| `0x00199BB8` | component state pointer virtual target | [src/game_core/indep/src/componentphysics.cpp:86](../src/game_core/indep/src/componentphysics.cpp:86) |
| `0x00199BB8` | post integration dispatch | [src/game_core/indep/src/componentphysics.cpp:90](../src/game_core/indep/src/componentphysics.cpp:90) |
| `0x00199BB8` | unknown post-integrate target | [src/game_core/port/driving/adapters.cpp:19](../src/game_core/port/driving/adapters.cpp:19) |
| `0x00199F98` | component derivative dispatch | [src/game_core/indep/src/componentphysics.cpp:57](../src/game_core/indep/src/componentphysics.cpp:57) |
| `0x00199F98` | untranslated derivative target | [src/game_core/port/driving/adapters.cpp:17](../src/game_core/port/driving/adapters.cpp:17) |
| `0x0019A440` | DemoCameraCoordinator::Sub_0019A440 (signature and full body unresolved) | [src/game_core/indep/src/cameracoordinator.cpp:41](../src/game_core/indep/src/cameracoordinator.cpp:41) |
| `0x0019A4A0` | DemoCameraCoordinator::Sub_0019A4A0 (signature and full body unresolved) | [src/game_core/indep/src/cameracoordinator.cpp:46](../src/game_core/indep/src/cameracoordinator.cpp:46) |
| `0x0019A4B8` | DemoCameraCoordinator::Sub_0019A4B8 (signature and full body unresolved) | [src/game_core/indep/src/cameracoordinator.cpp:51](../src/game_core/indep/src/cameracoordinator.cpp:51) |
| `0x0019B130` | CountdownCameraCoordinator::Sub_0019B130 (signature and full body unresolved) | [src/game_core/indep/src/cameracoordinator.cpp:15](../src/game_core/indep/src/cameracoordinator.cpp:15) |
| `0x0019B188` | CountdownCameraCoordinator::Sub_0019B188 (signature and full body unresolved) | [src/game_core/indep/src/cameracoordinator.cpp:20](../src/game_core/indep/src/cameracoordinator.cpp:20) |
| `0x0019B1A0` | CountdownCameraCoordinator::Sub_0019B1A0 (signature and full body unresolved) | [src/game_core/indep/src/cameracoordinator.cpp:25](../src/game_core/indep/src/cameracoordinator.cpp:25) |
| `0x0019B2A8` | RigidBody debug printing service | [src/game_core/indep/src/rigidbody.cpp:240](../src/game_core/indep/src/rigidbody.cpp:240) |
| `0x0019B4B0` | NisCameraCoordinator::Sub_0019B4B0 (signature and full body unresolved) | [src/game_core/indep/src/cameracoordinator.cpp:67](../src/game_core/indep/src/cameracoordinator.cpp:67) |
| `0x0019B5F0` | NisCameraCoordinator::Sub_0019B5F0 (signature and full body unresolved) | [src/game_core/indep/src/cameracoordinator.cpp:72](../src/game_core/indep/src/cameracoordinator.cpp:72) |
| `0x0019B658` | NisCameraCoordinator::Sub_0019B658 (signature and full body unresolved) | [src/game_core/indep/src/cameracoordinator.cpp:77](../src/game_core/indep/src/cameracoordinator.cpp:77) |
| `0x0019B660` | NisCameraCoordinator::NumCameraKeys (signature and full body unresolved) | [src/game_core/indep/src/cameracoordinator.cpp:98](../src/game_core/indep/src/cameracoordinator.cpp:98) |
| `0x0019B688` | NisCameraCoordinator::GetCameraKeys (signature and full body unresolved) | [src/game_core/indep/src/cameracoordinator.cpp:103](../src/game_core/indep/src/cameracoordinator.cpp:103) |
| `0x0019B6B0` | CameraDataItems::Sub_0019B6B0 (signature and full body unresolved) | [src/game_core/indep/src/cameracoordinator.cpp:86](../src/game_core/indep/src/cameracoordinator.cpp:86) |
| `0x0019BAB8` | CameraDataItems::Sub_0019BAB8 (signature and full body unresolved) | [src/game_core/indep/src/cameracoordinator.cpp:91](../src/game_core/indep/src/cameracoordinator.cpp:91) |
| `0x0019C290` | untranslated component count virtual target | [src/game_core/port/driving/projection_support.hpp:186](../src/game_core/port/driving/projection_support.hpp:186) |
| `0x0019C808` | Unknown component virtual target at slot +0x44 | [src/game_core/port/driving/projection_support.hpp:35](../src/game_core/port/driving/projection_support.hpp:35) |
| `0x0019E048` | SelectableTrack::GetTimeOfDay (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/selecttrack.cpp:36](../src/game_core/indep/src/frontendscreens/selecttrack.cpp:36) |
| `0x0019E0A8` | SelectableTrack::ScrollTimeOfDay (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/selecttrack.cpp:41](../src/game_core/indep/src/frontendscreens/selecttrack.cpp:41) |
| `0x0019E7B0` | SelectableTrackManager::GetNextTrack (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/selecttrack.cpp:50](../src/game_core/indep/src/frontendscreens/selecttrack.cpp:50) |
| `0x0019E7D0` | SelectableTrackManager::GetPrevTrack (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/selecttrack.cpp:55](../src/game_core/indep/src/frontendscreens/selecttrack.cpp:55) |
| `0x0019EF00` | TrackRecordDisplayer::Sub_0019EF00 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/selecttrack.cpp:62](../src/game_core/indep/src/frontendscreens/selecttrack.cpp:62) |
| `0x001A0C50` | Joylog observation service absent | [src/game_core/indep/src/timer.cpp:41](../src/game_core/indep/src/timer.cpp:41) |
| `0x001A0C50` | Joylog replay service absent | [src/game_core/indep/src/timer.cpp:36](../src/game_core/indep/src/timer.cpp:36) |
| `0x001A0DD8` | CopMover::Sub_001A0DD8 (signature and full body unresolved) | [src/game_core/port/police/cop_control.cpp:209](../src/game_core/port/police/cop_control.cpp:209) |
| `0x001A0E88` | CopMover::Sub_001A0E88 (signature and full body unresolved) | [src/game_core/port/police/cop_control.cpp:214](../src/game_core/port/police/cop_control.cpp:214) |
| `0x001A0F20` | CopMover::Sub_001A0F20 (signature and full body unresolved) | [src/game_core/port/police/cop_control.cpp:219](../src/game_core/port/police/cop_control.cpp:219) |
| `0x001A1030` | CopMover::Sub_001A1030 (signature and full body unresolved) | [src/game_core/port/police/cop_control.cpp:224](../src/game_core/port/police/cop_control.cpp:224) |
| `0x001A1CE8` | CopDriver::Sub_001A1CE8 (signature and full body unresolved) | [src/game_core/port/police/cop_control.cpp:160](../src/game_core/port/police/cop_control.cpp:160) |
| `0x001A1E30` | CopDriver::Sub_001A1E30 (signature and full body unresolved) | [src/game_core/port/police/cop_control.cpp:165](../src/game_core/port/police/cop_control.cpp:165) |
| `0x001A2110` | CopDriver::Think (signature and full body unresolved) | [src/game_core/port/police/cop_control.cpp:171](../src/game_core/port/police/cop_control.cpp:171) |
| `0x001A2AC4` | VU reciprocal-square-root exceptional operand | [src/game_core/indep/src/wheel.cpp:300](../src/game_core/indep/src/wheel.cpp:300) |
| `0x001A2B58` | CopDriver::DoSnapshot (signature and full body unresolved) | [src/game_core/port/police/cop_control.cpp:177](../src/game_core/port/police/cop_control.cpp:177) |
| `0x001A2CF8` | Wheel::MaybeMakeSkids requires explicit skid/topology services | [src/game_core/indep/src/wheel.cpp:414](../src/game_core/indep/src/wheel.cpp:414) |
| `0x001A3B28` | Timer::GetHoursMinsSeconds (signature and full body unresolved) | [src/game_core/indep/src/timer.cpp:17](../src/game_core/indep/src/timer.cpp:17) |
| `0x001A3B98` | INT_MIN timer negation behavior not modeled | [src/game_core/indep/src/eventtree.cpp:596](../src/game_core/indep/src/eventtree.cpp:596) |
| `0x001A3B98` | original timer formatter missing | [src/game_core/indep/src/eventtree.cpp:682](../src/game_core/indep/src/eventtree.cpp:682) |
| `0x001A3B98` | original timer formatter missing | [src/game_core/indep/src/eventtree.cpp:716](../src/game_core/indep/src/eventtree.cpp:716) |
| `0x001A3CF0` | Timer::CountDown (signature and full body unresolved) | [src/game_core/indep/src/timer.cpp:22](../src/game_core/indep/src/timer.cpp:22) |
| `0x001A5FC0` | Contact provider is absent | [src/game_core/port/driving/projection_support.hpp:31](../src/game_core/port/driving/projection_support.hpp:31) |
| `0x001A6330` | missing +0x90 wheel projection | [src/game_core/indep/src/suspension.cpp:68](../src/game_core/indep/src/suspension.cpp:68) |
| `0x001A6370` | missing wheel/body projections | [src/game_core/indep/src/suspension.cpp:83](../src/game_core/indep/src/suspension.cpp:83) |
| `0x001A74C8` | eRigidAnimation::GetMatrixAtFrame (signature and full body unresolved) | [src/game_core/indep/src/eanimationsolid.cpp:20](../src/game_core/indep/src/eanimationsolid.cpp:20) |
| `0x001A7518` | eRigidAnimation::GetMatrixAtTime (signature and full body unresolved) | [src/game_core/indep/src/eanimationsolid.cpp:25](../src/game_core/indep/src/eanimationsolid.cpp:25) |
| `0x001A78E0` | eAnimationSolid::GetTranslation (signature and full body unresolved) | [src/game_core/indep/src/eanimationsolid.cpp:13](../src/game_core/indep/src/eanimationsolid.cpp:13) |
| `0x001A8150` | nonzero underflowed VU reciprocal-square-root segment | [src/game_core/indep/src/skids.cpp:63](../src/game_core/indep/src/skids.cpp:63) |
| `0x001A8798` | Original skid texture lookup service | [src/game_core/indep/src/skids.cpp:122](../src/game_core/indep/src/skids.cpp:122) |
| `0x001A8798` | Skid pool allocation service | [src/game_core/indep/src/skids.cpp:120](../src/game_core/indep/src/skids.cpp:120) |
| `0x001A8798` | Skid pool and synthetic clan services are not bound | [src/game_core/port/driving/skid_services.cpp:18](../src/game_core/port/driving/skid_services.cpp:18) |
| `0x001AB3C8` | replay snapshot outside driving scope | [src/game_core/indep/src/aerodynamics.cpp:27](../src/game_core/indep/src/aerodynamics.cpp:27) |
| `0x001ABF48` | GetTextureInfo: original DEFAULTTEXTURE absent, bBreak path | [src/game_core/indep/src/texture.cpp:21](../src/game_core/indep/src/texture.cpp:21) |
| `0x001AC3F0` | TexturePack::GetLoadedTexture (signature and full body unresolved) | [src/game_core/indep/src/texture.cpp:26](../src/game_core/indep/src/texture.cpp:26) |
| `0x001AC468` | TexturePack::GetTexture (signature and full body unresolved) | [src/game_core/indep/src/texture.cpp:31](../src/game_core/indep/src/texture.cpp:31) |
| `0x001AC4B0` | TexturePack::GetNthTexture (signature and full body unresolved) | [src/game_core/indep/src/texture.cpp:36](../src/game_core/indep/src/texture.cpp:36) |
| `0x001AC780` | TexturePack::FindLoadedTextureData (signature and full body unresolved) | [src/game_core/indep/src/texture.cpp:41](../src/game_core/indep/src/texture.cpp:41) |
| `0x001AC7D0` | TexturePack::FindNotYetLoadedNeighbourTextureData (signature and full body unresolved) | [src/game_core/indep/src/texture.cpp:46](../src/game_core/indep/src/texture.cpp:46) |
| `0x001AD008` | TexturePack::GetLoadedSize (signature and full body unresolved) | [src/game_core/indep/src/texture.cpp:51](../src/game_core/indep/src/texture.cpp:51) |
| `0x001AEFD0` | jukebox name missing type field | [src/game_core/bSound/src/bJukebox.cpp:52](../src/game_core/bSound/src/bJukebox.cpp:52) |
| `0x001AF050` | jukebox song type outside recovered matrix | [src/game_core/bSound/src/bJukebox.cpp:74](../src/game_core/bSound/src/bJukebox.cpp:74) |
| `0x001B02B8` | LoadedSkin::Sub_001B02B8 (signature and full body unresolved) | [src/game_core/indep/src/carloader.cpp:20](../src/game_core/indep/src/carloader.cpp:20) |
| `0x001B0518` | CarLoader::FindLoadedSkin (signature and full body unresolved) | [src/game_core/indep/src/carloader.cpp:13](../src/game_core/indep/src/carloader.cpp:13) |
| `0x001B4D48` | BodyDamage::SetDamage (signature and full body unresolved) | [src/game_core/indep/src/bodydamage.cpp:13](../src/game_core/indep/src/bodydamage.cpp:13) |
| `0x001B4E10` | BodyDamage::GetSectorNumber (signature and full body unresolved) | [src/game_core/indep/src/bodydamage.cpp:18](../src/game_core/indep/src/bodydamage.cpp:18) |
| `0x001B4F10` | BodyDamage::GetDamageAmount (signature and full body unresolved) | [src/game_core/indep/src/bodydamage.cpp:23](../src/game_core/indep/src/bodydamage.cpp:23) |
| `0x001B50C8` | BodyDamage::GetVisibleTotalDamage (signature and full body unresolved) | [src/game_core/indep/src/bodydamage.cpp:28](../src/game_core/indep/src/bodydamage.cpp:28) |
| `0x001B7AB0` | RaceSetupBaseClass::FindChangeable (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/racesetup.cpp:80](../src/game_core/indep/src/frontendscreens/racesetup.cpp:80) |
| `0x001B7AE8` | RaceSetupBaseClass::FindChangeableByLanguageID (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/racesetup.cpp:85](../src/game_core/indep/src/frontendscreens/racesetup.cpp:85) |
| `0x001B8AF0` | DataSingleRaceSetup::InitWithDefaults (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/racesetup.cpp:73](../src/game_core/indep/src/frontendscreens/racesetup.cpp:73) |
| `0x001B8F38` | DataLapKnockoutSetup::InitWithDefaults (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/racesetup.cpp:59](../src/game_core/indep/src/frontendscreens/racesetup.cpp:59) |
| `0x001B9218` | DataRaceKnockoutSetup::InitWithDefaults (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/racesetup.cpp:66](../src/game_core/indep/src/frontendscreens/racesetup.cpp:66) |
| `0x001B93E0` | DataBeTheCopSetup::InitWithDefaults (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/racesetup.cpp:52](../src/game_core/indep/src/frontendscreens/racesetup.cpp:52) |
| `0x001B9CD0` | vAABB::Contains (signature and full body unresolved) | [src/game_core/indep/src/volumetree.cpp:9](../src/game_core/indep/src/volumetree.cpp:9) |
| `0x001B9D40` | vAABB::Encloses (signature and full body unresolved) | [src/game_core/indep/src/volumetree.cpp:14](../src/game_core/indep/src/volumetree.cpp:14) |
| `0x001BCA08` | TrackMap::EmitPoint (signature and full body unresolved) | [src/game_core/indep/src/trackmap.cpp:15](../src/game_core/indep/src/trackmap.cpp:15) |
| `0x001BDE60` | TrackMarkerFile::GetTargetedMarker (signature and full body unresolved) | [src/game_core/indep/src/trackmarkers.cpp:15](../src/game_core/indep/src/trackmarkers.cpp:15) |
| `0x001BEAD0` | Munition::Backup (signature and full body unresolved) | [src/game_core/indep/src/munitions.cpp:54](../src/game_core/indep/src/munitions.cpp:54) |
| `0x001BEC28` | SmokeBomb::Sub_001BEC28 (signature and full body unresolved) | [src/game_core/indep/src/munitions.cpp:98](../src/game_core/indep/src/munitions.cpp:98) |
| `0x001BED20` | SmokeBomb::MaybeExplodeWarhead (signature and full body unresolved) | [src/game_core/indep/src/munitions.cpp:103](../src/game_core/indep/src/munitions.cpp:103) |
| `0x001BED78` | SmokeBomb::Move (signature and full body unresolved) | [src/game_core/indep/src/munitions.cpp:108](../src/game_core/indep/src/munitions.cpp:108) |
| `0x001BEF20` | DroppedSpikes::Sub_001BEF20 (signature and full body unresolved) | [src/game_core/indep/src/munitions.cpp:17](../src/game_core/indep/src/munitions.cpp:17) |
| `0x001BEF90` | DroppedSpikes::DoSnapshot (signature and full body unresolved) | [src/game_core/indep/src/munitions.cpp:22](../src/game_core/indep/src/munitions.cpp:22) |
| `0x001BF078` | DroppedSpikes::MaybeExplodeWarhead (signature and full body unresolved) | [src/game_core/indep/src/munitions.cpp:27](../src/game_core/indep/src/munitions.cpp:27) |
| `0x001BF0D8` | DroppedSpikes::Move (signature and full body unresolved) | [src/game_core/indep/src/munitions.cpp:32](../src/game_core/indep/src/munitions.cpp:32) |
| `0x001BF160` | DroppedSpikes::ConsiderImpulse (signature and full body unresolved) | [src/game_core/indep/src/munitions.cpp:37](../src/game_core/indep/src/munitions.cpp:37) |
| `0x001BF178` | DroppedSpikes::ApplyImpulse (signature and full body unresolved) | [src/game_core/indep/src/munitions.cpp:42](../src/game_core/indep/src/munitions.cpp:42) |
| `0x001BF3D8` | Rocket::Sub_001BF3D8 (signature and full body unresolved) | [src/game_core/indep/src/munitions.cpp:61](../src/game_core/indep/src/munitions.cpp:61) |
| `0x001BF4D0` | Rocket::MaybeExplodeWarhead (signature and full body unresolved) | [src/game_core/indep/src/munitions.cpp:66](../src/game_core/indep/src/munitions.cpp:66) |
| `0x001BF598` | Rocket::Move (signature and full body unresolved) | [src/game_core/indep/src/munitions.cpp:71](../src/game_core/indep/src/munitions.cpp:71) |
| `0x001BF8F8` | Rocket::ConsiderImpulse (signature and full body unresolved) | [src/game_core/indep/src/munitions.cpp:76](../src/game_core/indep/src/munitions.cpp:76) |
| `0x001BF908` | Rocket::ApplyImpulse (signature and full body unresolved) | [src/game_core/indep/src/munitions.cpp:81](../src/game_core/indep/src/munitions.cpp:81) |
| `0x001BF910` | Rocket::DoSnapshot (signature and full body unresolved) | [src/game_core/indep/src/munitions.cpp:86](../src/game_core/indep/src/munitions.cpp:86) |
| `0x001BFD10` | Rocket::GetSmokeType (signature and full body unresolved) | [src/game_core/indep/src/munitions.cpp:91](../src/game_core/indep/src/munitions.cpp:91) |
| `0x001BFD90` | DroppedSpikes::GetSmokeType (signature and full body unresolved) | [src/game_core/indep/src/munitions.cpp:47](../src/game_core/indep/src/munitions.cpp:47) |
| `0x001BFE10` | SmokeBomb::GetSmokeType (signature and full body unresolved) | [src/game_core/indep/src/munitions.cpp:113](../src/game_core/indep/src/munitions.cpp:113) |
| `0x001C0090` | NullController::Sub_001C0090 (signature and full body unresolved) | [src/game_core/indep/src/car.cpp:87](../src/game_core/indep/src/car.cpp:87) |
| `0x001C00A8` | CarController::Sub_001C00A8 (signature and full body unresolved) | [src/game_core/indep/src/carcontroller.cpp:37](../src/game_core/indep/src/carcontroller.cpp:37) |
| `0x001C00D0` | NullController::Sub_001C00D0 (signature and full body unresolved) | [src/game_core/indep/src/car.cpp:95](../src/game_core/indep/src/car.cpp:95) |
| `0x001C0100` | CarController::SetToPhysicsMode (signature and full body unresolved) | [src/game_core/indep/src/carcontroller.cpp:15](../src/game_core/indep/src/carcontroller.cpp:15) |
| `0x001C0140` | CarController::ForceBoost (signature and full body unresolved) | [src/game_core/indep/src/carcontroller.cpp:20](../src/game_core/indep/src/carcontroller.cpp:20) |
| `0x001C0160` | CarController::ReleaseBoost (signature and full body unresolved) | [src/game_core/indep/src/carcontroller.cpp:25](../src/game_core/indep/src/carcontroller.cpp:25) |
| `0x001C0198` | CarController::DebugDisplay (signature and full body unresolved) | [src/game_core/indep/src/carcontroller.cpp:30](../src/game_core/indep/src/carcontroller.cpp:30) |
| `0x001C0780` | CarState::ResetCarForSound (signature and full body unresolved) | [src/game_core/indep/src/carstate.cpp:29](../src/game_core/indep/src/carstate.cpp:29) |
| `0x001C0850` | CarState::BeginTimestep (signature and full body unresolved) | [src/game_core/indep/src/carstate.cpp:34](../src/game_core/indep/src/carstate.cpp:34) |
| `0x001C1340` | TrackRoutePoint::Sub_001C1340 (signature and full body unresolved) | [src/game_core/indep/src/trackroute.cpp:42](../src/game_core/indep/src/trackroute.cpp:42) |
| `0x001C1408` | TrackRoutePoint::GetPosition (signature and full body unresolved) | [src/game_core/indep/src/trackroute.cpp:47](../src/game_core/indep/src/trackroute.cpp:47) |
| `0x001C1490` | TrackRoutePoint::SetWidth (signature and full body unresolved) | [src/game_core/indep/src/trackroute.cpp:52](../src/game_core/indep/src/trackroute.cpp:52) |
| `0x001C1910` | TrackRoute::GetRelativePoint (signature and full body unresolved) | [src/game_core/indep/src/trackroute.cpp:16](../src/game_core/indep/src/trackroute.cpp:16) |
| `0x001C1D30` | TrackRoute::FindClosestPointAhead (signature and full body unresolved) | [src/game_core/indep/src/trackroute.cpp:21](../src/game_core/indep/src/trackroute.cpp:21) |
| `0x001C24A8` | TrackRouteEditor recording is not recovered | [src/game_core/indep/src/trackrouteeditor.cpp:18](../src/game_core/indep/src/trackrouteeditor.cpp:18) |
| `0x001C2780` | TrackRoute::DistanceIsWithinRoute (signature and full body unresolved) | [src/game_core/indep/src/trackroute.cpp:26](../src/game_core/indep/src/trackroute.cpp:26) |
| `0x001C28A0` | TrackRouteManager::AddRouteToRightList (signature and full body unresolved) | [src/game_core/indep/src/trackroute.cpp:35](../src/game_core/indep/src/trackroute.cpp:35) |
| `0x001C3718` | FancyJoyEventHandler::IsButtonPressedRepeat (signature and full body unresolved) | [src/game_core/indep/src/trackrouteeditor.cpp:25](../src/game_core/indep/src/trackrouteeditor.cpp:25) |
| `0x001C4188` | TrackRouteEditor::GetCameraPosition (signature and full body unresolved) | [src/game_core/indep/src/trackrouteeditor.cpp:32](../src/game_core/indep/src/trackrouteeditor.cpp:32) |
| `0x001C8660` | AIActionPullOver::GetDistanceToRightEdgeOfRoute (signature and full body unresolved) | [src/game_core/indep/src/aiplan.cpp:103](../src/game_core/indep/src/aiplan.cpp:103) |
| `0x001C9018` | AIActionGetUnstuck::GetGoalTrackRoutePoint (signature and full body unresolved) | [src/game_core/indep/src/aiplan.cpp:96](../src/game_core/indep/src/aiplan.cpp:96) |
| `0x001CD408` | NearestRoutePointInfo::IsValid (signature and full body unresolved) | [src/game_core/indep/src/aiutil.cpp:13](../src/game_core/indep/src/aiutil.cpp:13) |
| `0x001CDDE0` | SoundArea::Initialize (signature and full body unresolved) | [src/game_core/indep/src/soundscape.cpp:13](../src/game_core/indep/src/soundscape.cpp:13) |
| `0x001D0058` | VCTransform::TransformVector (signature and full body unresolved) | [src/game_core/indep/src/voronoiclip.cpp:69](../src/game_core/indep/src/voronoiclip.cpp:69) |
| `0x001D00B0` | VCTransform::TransformPoint (signature and full body unresolved) | [src/game_core/indep/src/voronoiclip.cpp:74](../src/game_core/indep/src/voronoiclip.cpp:74) |
| `0x001D00F8` | VCTransformedFeature::TransformVertex (signature and full body unresolved) | [src/game_core/indep/src/voronoiclip.cpp:81](../src/game_core/indep/src/voronoiclip.cpp:81) |
| `0x001D0168` | VCTransformedFeature::TransformEdge (signature and full body unresolved) | [src/game_core/indep/src/voronoiclip.cpp:86](../src/game_core/indep/src/voronoiclip.cpp:86) |
| `0x001D0228` | VCVoronoiClip::Sub_001D0228 (signature and full body unresolved) | [src/game_core/indep/src/voronoiclip.cpp:110](../src/game_core/indep/src/voronoiclip.cpp:110) |
| `0x001D0250` | VCVoronoiClip::ClipPlanes (signature and full body unresolved) | [src/game_core/indep/src/voronoiclip.cpp:115](../src/game_core/indep/src/voronoiclip.cpp:115) |
| `0x001D0890` | VCWitness::Sub_001D0890 (signature and full body unresolved) | [src/game_core/indep/src/voronoiclip.cpp:122](../src/game_core/indep/src/voronoiclip.cpp:122) |
| `0x001D2800` | VCVertex::SetVoronoiPlanes (signature and full body unresolved) | [src/game_core/indep/src/voronoiclip.cpp:93](../src/game_core/indep/src/voronoiclip.cpp:93) |
| `0x001D2950` | VCVertex::GetNextEdgeInFace (signature and full body unresolved) | [src/game_core/indep/src/voronoiclip.cpp:98](../src/game_core/indep/src/voronoiclip.cpp:98) |
| `0x001D29E8` | VCVertex::Move (signature and full body unresolved) | [src/game_core/indep/src/voronoiclip.cpp:103](../src/game_core/indep/src/voronoiclip.cpp:103) |
| `0x001D2A48` | VCEdge::SetVoronoiPlanes (signature and full body unresolved) | [src/game_core/indep/src/voronoiclip.cpp:13](../src/game_core/indep/src/voronoiclip.cpp:13) |
| `0x001D30A8` | VCFace::Move (signature and full body unresolved) | [src/game_core/indep/src/voronoiclip.cpp:20](../src/game_core/indep/src/voronoiclip.cpp:20) |
| `0x001D3100` | VCPolyhedron::Sub_001D3100 (signature and full body unresolved) | [src/game_core/indep/src/voronoiclip.cpp:27](../src/game_core/indep/src/voronoiclip.cpp:27) |
| `0x001D3200` | VCPolyhedron::Init (signature and full body unresolved) | [src/game_core/indep/src/voronoiclip.cpp:32](../src/game_core/indep/src/voronoiclip.cpp:32) |
| `0x001D3398` | VCPolyhedron::NewVoronoiPlanes (signature and full body unresolved) | [src/game_core/indep/src/voronoiclip.cpp:37](../src/game_core/indep/src/voronoiclip.cpp:37) |
| `0x001D3430` | VCPolyhedron::NewVertex (signature and full body unresolved) | [src/game_core/indep/src/voronoiclip.cpp:42](../src/game_core/indep/src/voronoiclip.cpp:42) |
| `0x001D3498` | VCPolyhedron::NewEdge (signature and full body unresolved) | [src/game_core/indep/src/voronoiclip.cpp:47](../src/game_core/indep/src/voronoiclip.cpp:47) |
| `0x001D3540` | VCPolyhedron::NewFace (signature and full body unresolved) | [src/game_core/indep/src/voronoiclip.cpp:52](../src/game_core/indep/src/voronoiclip.cpp:52) |
| `0x001D3BA8` | VCPolyhedron::GetFeatureCode (signature and full body unresolved) | [src/game_core/indep/src/voronoiclip.cpp:57](../src/game_core/indep/src/voronoiclip.cpp:57) |
| `0x001D3C60` | VCPolyhedron::GetFeature (signature and full body unresolved) | [src/game_core/indep/src/voronoiclip.cpp:62](../src/game_core/indep/src/voronoiclip.cpp:62) |
| `0x001D5008` | collision damage/player/police effects are outside the native playerless fixture | [src/game_core/indep/src/collisionbody.cpp:385](../src/game_core/indep/src/collisionbody.cpp:385) |
| `0x001D5188` | animated collision reaction is outside the native playerless fixture | [src/game_core/indep/src/collisionbody.cpp:386](../src/game_core/indep/src/collisionbody.cpp:386) |
| `0x001D5BF8` | NumberAllocator::AllocateNumber (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:224](../src/game_core/indep/src/collisionbody.cpp:224) |
| `0x001D5CB8` | NumberAllocator::FreeNumber (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:229](../src/game_core/indep/src/collisionbody.cpp:229) |
| `0x001D69B0` | CollisionBody::Sub_001D69B0 (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:135](../src/game_core/indep/src/collisionbody.cpp:135) |
| `0x001D6A98` | CollisionBody::DoSnapshot (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:140](../src/game_core/indep/src/collisionbody.cpp:140) |
| `0x001D7028` | DynamicCollisionBody::Sub_001D7028 (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:152](../src/game_core/indep/src/collisionbody.cpp:152) |
| `0x001D7050` | DynamicCollisionBody::DoSnapshot (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:157](../src/game_core/indep/src/collisionbody.cpp:157) |
| `0x001D7408` | DynamicCollisionBody::Interpolate (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:162](../src/game_core/indep/src/collisionbody.cpp:162) |
| `0x001D7660` | DynamicCollisionBody::Backup (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:167](../src/game_core/indep/src/collisionbody.cpp:167) |
| `0x001D7710` | DynamicCollisionBody::RecomputeAuxilliaries (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:172](../src/game_core/indep/src/collisionbody.cpp:172) |
| `0x001D7740` | DynamicCollisionBody::GetCurrentVelocities (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:177](../src/game_core/indep/src/collisionbody.cpp:177) |
| `0x001D7760` | DynamicCollisionBody::GetTestImpulseVelocities (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:182](../src/game_core/indep/src/collisionbody.cpp:182) |
| `0x001D7E08` | DynamicCollisionBody::ChangeInEnergy (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:187](../src/game_core/indep/src/collisionbody.cpp:187) |
| `0x001D7FA0` | CarCollisionBody::Sub_001D7FA0 (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:41](../src/game_core/indep/src/collisionbody.cpp:41) |
| `0x001D8058` | CarCollisionBody::DoSnapshot (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:46](../src/game_core/indep/src/collisionbody.cpp:46) |
| `0x001D8138` | CarCollisionBody::SetupPhysicsPointers (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:51](../src/game_core/indep/src/collisionbody.cpp:51) |
| `0x001D8198` | CarCollisionBody::SetToPhysicsMode (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:56](../src/game_core/indep/src/collisionbody.cpp:56) |
| `0x001D81F0` | CarCollisionBody::Move (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:61](../src/game_core/indep/src/collisionbody.cpp:61) |
| `0x001D8270` | CarCollisionBody::AreCollisionsEnabled (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:66](../src/game_core/indep/src/collisionbody.cpp:66) |
| `0x001D8298` | CarCollisionBody::GetMassInverse (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:71](../src/game_core/indep/src/collisionbody.cpp:71) |
| `0x001D82D0` | CarCollisionBody::GetInertiaTensorInverse (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:76](../src/game_core/indep/src/collisionbody.cpp:76) |
| `0x001D8318` | CarCollisionBody::ApplyImpulse (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:81](../src/game_core/indep/src/collisionbody.cpp:81) |
| `0x001D8880` | TrackPolygonCollisionBody::Sub_001D8880 (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:338](../src/game_core/indep/src/collisionbody.cpp:338) |
| `0x001D8918` | TrackPolygonCollisionBody::DoSnapshot (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:343](../src/game_core/indep/src/collisionbody.cpp:343) |
| `0x001D8B18` | TrackPolygonCollisionBody::GetPosition (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:348](../src/game_core/indep/src/collisionbody.cpp:348) |
| `0x001D8B28` | TrackPolygonCollisionBody::GetTerrainType (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:353](../src/game_core/indep/src/collisionbody.cpp:353) |
| `0x001D8E58` | CarCollisionBody::GetFriction (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:86](../src/game_core/indep/src/collisionbody.cpp:86) |
| `0x001D8E68` | CarCollisionBody::GetBounciness (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:91](../src/game_core/indep/src/collisionbody.cpp:91) |
| `0x001D8E78` | CarCollisionBody::GetTerrainType (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:96](../src/game_core/indep/src/collisionbody.cpp:96) |
| `0x001D8E80` | CarCollisionBody::IsBlinking (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:101](../src/game_core/indep/src/collisionbody.cpp:101) |
| `0x001D8ED8` | CarCollisionBody::IsMoveable (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:106](../src/game_core/indep/src/collisionbody.cpp:106) |
| `0x001D8EE0` | CarCollisionBody::ConsiderImpulse (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:111](../src/game_core/indep/src/collisionbody.cpp:111) |
| `0x001D8EF0` | CarCollisionBody::GetRotation (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:116](../src/game_core/indep/src/collisionbody.cpp:116) |
| `0x001D8F10` | CarCollisionBody::GetVelocity (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:121](../src/game_core/indep/src/collisionbody.cpp:121) |
| `0x001D8F30` | CarCollisionBody::GetPosition (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:126](../src/game_core/indep/src/collisionbody.cpp:126) |
| `0x001D8F98` | DynamicCollisionBody::ApplyImpulse (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:192](../src/game_core/indep/src/collisionbody.cpp:192) |
| `0x001D8FA0` | DynamicCollisionBody::GetInertiaTensorInverse (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:197](../src/game_core/indep/src/collisionbody.cpp:197) |
| `0x001D8FB0` | DynamicCollisionBody::GetMassInverse (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:202](../src/game_core/indep/src/collisionbody.cpp:202) |
| `0x001D8FC0` | DynamicCollisionBody::SetToPhysicsMode (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:207](../src/game_core/indep/src/collisionbody.cpp:207) |
| `0x001D8FC8` | DynamicCollisionBody::GetMaxBackupTime (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:212](../src/game_core/indep/src/collisionbody.cpp:212) |
| `0x001D8FD0` | DynamicCollisionBody::SetPosition (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:217](../src/game_core/indep/src/collisionbody.cpp:217) |
| `0x001D9048` | StaticCollisionBody::ChangeInEnergy (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:236](../src/game_core/indep/src/collisionbody.cpp:236) |
| `0x001D9058` | StaticCollisionBody::IsMoveable (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:241](../src/game_core/indep/src/collisionbody.cpp:241) |
| `0x001D9060` | StaticCollisionBody::AreCollisionsEnabled (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:246](../src/game_core/indep/src/collisionbody.cpp:246) |
| `0x001D9070` | StaticCollisionBody::ApplyImpulse (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:251](../src/game_core/indep/src/collisionbody.cpp:251) |
| `0x001D9078` | StaticCollisionBody::ConsiderImpulse (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:256](../src/game_core/indep/src/collisionbody.cpp:256) |
| `0x001D9088` | StaticCollisionBody::SetToPhysicsMode (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:261](../src/game_core/indep/src/collisionbody.cpp:261) |
| `0x001D9090` | StaticCollisionBody::GetFriction (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:266](../src/game_core/indep/src/collisionbody.cpp:266) |
| `0x001D90A0` | StaticCollisionBody::GetBounciness (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:271](../src/game_core/indep/src/collisionbody.cpp:271) |
| `0x001D90B0` | StaticCollisionBody::GetInertiaTensorInverse (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:276](../src/game_core/indep/src/collisionbody.cpp:276) |
| `0x001D90C0` | StaticCollisionBody::GetMassInverse (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:281](../src/game_core/indep/src/collisionbody.cpp:281) |
| `0x001D90D0` | StaticCollisionBody::GetTestImpulseVelocities (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:286](../src/game_core/indep/src/collisionbody.cpp:286) |
| `0x001D90F0` | StaticCollisionBody::GetCurrentVelocities (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:291](../src/game_core/indep/src/collisionbody.cpp:291) |
| `0x001D9110` | StaticCollisionBody::RecomputeAuxilliaries (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:296](../src/game_core/indep/src/collisionbody.cpp:296) |
| `0x001D9118` | StaticCollisionBody::Interpolate (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:301](../src/game_core/indep/src/collisionbody.cpp:301) |
| `0x001D9158` | StaticCollisionBody::Backup (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:306](../src/game_core/indep/src/collisionbody.cpp:306) |
| `0x001D9160` | StaticCollisionBody::Move (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:311](../src/game_core/indep/src/collisionbody.cpp:311) |
| `0x001D9168` | StaticCollisionBody::GetMaxBackupTime (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:316](../src/game_core/indep/src/collisionbody.cpp:316) |
| `0x001D9178` | StaticCollisionBody::SetPosition (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:321](../src/game_core/indep/src/collisionbody.cpp:321) |
| `0x001D9180` | StaticCollisionBody::GetRotation (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:326](../src/game_core/indep/src/collisionbody.cpp:326) |
| `0x001D9190` | StaticCollisionBody::GetVelocity (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:331](../src/game_core/indep/src/collisionbody.cpp:331) |
| `0x001D91D0` | CollisionBody::IsBlinking (signature and full body unresolved) | [src/game_core/indep/src/collisionbody.cpp:145](../src/game_core/indep/src/collisionbody.cpp:145) |
| `0x001DA780` | tCubic1D::MakeCoeffs (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:129](../src/game_core/indep/src/point.cpp:129) |
| `0x001DA7D0` | tCubic1D::GetVal (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:134](../src/game_core/indep/src/point.cpp:134) |
| `0x001DA800` | tCubic1D::GetdVal (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:139](../src/game_core/indep/src/point.cpp:139) |
| `0x001DA830` | tCubic1D::GetddVal (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:144](../src/game_core/indep/src/point.cpp:144) |
| `0x001DAA00` | tCubic2D::SetVal (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:151](../src/game_core/indep/src/point.cpp:151) |
| `0x001DAA50` | tCubic2D::SetdVal (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:156](../src/game_core/indep/src/point.cpp:156) |
| `0x001DAAA0` | tCubic2D::SetValDesired (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:161](../src/game_core/indep/src/point.cpp:161) |
| `0x001DABF8` | tCubic3D::SetVal (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:168](../src/game_core/indep/src/point.cpp:168) |
| `0x001DAC68` | tCubic3D::SetdVal (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:173](../src/game_core/indep/src/point.cpp:173) |
| `0x001DACD8` | tCubic3D::SetValDesired (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:178](../src/game_core/indep/src/point.cpp:178) |
| `0x001DB3D0` | cPoint::PointVelocity (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:17](../src/game_core/indep/src/point.cpp:17) |
| `0x001DB810` | cPoint::SetPosX (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:22](../src/game_core/indep/src/point.cpp:22) |
| `0x001DB870` | cPoint::SetPosZ (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:27](../src/game_core/indep/src/point.cpp:27) |
| `0x001DB940` | cPoint::SetPosDesiredX (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:32](../src/game_core/indep/src/point.cpp:32) |
| `0x001DB9A0` | cPoint::SetPosDesiredZ (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:37](../src/game_core/indep/src/point.cpp:37) |
| `0x001DBAB8` | cPoint::SetdPosZ (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:42](../src/game_core/indep/src/point.cpp:42) |
| `0x001DBC28` | cPoint::SetRotX (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:47](../src/game_core/indep/src/point.cpp:47) |
| `0x001DBC50` | cPoint::SetRotY (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:52](../src/game_core/indep/src/point.cpp:52) |
| `0x001DBC78` | cPoint::SetRotZ (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:57](../src/game_core/indep/src/point.cpp:57) |
| `0x001DBD38` | cPoint::SetRotDesiredX (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:62](../src/game_core/indep/src/point.cpp:62) |
| `0x001DBD60` | cPoint::SetRotDesiredY (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:67](../src/game_core/indep/src/point.cpp:67) |
| `0x001DBD88` | cPoint::SetRotDesiredZ (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:72](../src/game_core/indep/src/point.cpp:72) |
| `0x001DBE48` | cPoint::SetdRotX (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:77](../src/game_core/indep/src/point.cpp:77) |
| `0x001DBE70` | cPoint::SetdRotY (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:82](../src/game_core/indep/src/point.cpp:82) |
| `0x001DBE98` | cPoint::SetdRotZ (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:87](../src/game_core/indep/src/point.cpp:87) |
| `0x001DBF70` | cPoint::SetScale (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:92](../src/game_core/indep/src/point.cpp:92) |
| `0x001DBF98` | cPoint::SetdScale (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:97](../src/game_core/indep/src/point.cpp:97) |
| `0x001DBFC0` | cPoint::SetYScale (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:102](../src/game_core/indep/src/point.cpp:102) |
| `0x001DBFE8` | cPoint::SetdYScale (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:107](../src/game_core/indep/src/point.cpp:107) |
| `0x001DC010` | cPoint::SetScaleDesired (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:112](../src/game_core/indep/src/point.cpp:112) |
| `0x001DC040` | cPoint::SetYScaleDesired (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:117](../src/game_core/indep/src/point.cpp:117) |
| `0x001DC070` | cPoint::SetDuration (signature and full body unresolved) | [src/game_core/indep/src/point.cpp:122](../src/game_core/indep/src/point.cpp:122) |
| `0x001DC5A8` | original track database name lookup missing | [src/game_core/indep/src/eventtree.cpp:656](../src/game_core/indep/src/eventtree.cpp:656) |
| `0x001DCE18` | EndGameScreens::GetNextScreen (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:350](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:350) |
| `0x001DCF00` | EndGameScreens::MoveBackScreen (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:355](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:355) |
| `0x001DD0F0` | EndGameMenusBase::HandleEvent (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:331](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:331) |
| `0x001DD5D0` | TourneyStandingsScreen::Sub_001DD5D0 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:298](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:298) |
| `0x001DDC10` | BestLapTimes::Sub_001DDC10 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:90](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:90) |
| `0x001DDFD0` | BestTotalTimes::Sub_001DDFD0 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:125](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:125) |
| `0x001DEF38` | EventRewardScreen::Sub_001DEF38 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:212](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:212) |
| `0x001DFFE8` | RaceResultsScreen::Sub_001DFFE8 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:254](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:254) |
| `0x001E0838` | RaceResultsScreen::Scroll (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:260](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:260) |
| `0x001E0868` | RaceResultsScreen::Tick (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:266](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:266) |
| `0x001E0898` | BeACopResults1P::Sub_001E0898 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:17](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:17) |
| `0x001E0A10` | BeACopResults2P::Sub_001E0A10 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:52](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:52) |
| `0x001E0BC8` | BeACopResults2P::Sub_001E0BC8 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:57](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:57) |
| `0x001E12B8` | EndGameMenu::Sub_001E12B8 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:158](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:158) |
| `0x001E1918` | EndGameMenu::FindEndGameMenuItem (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:343](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:343) |
| `0x001E19E0` | EndGameMenu::Sub_001E19E0 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:163](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:163) |
| `0x001E2270` | EndGameMenu::Sub_001E2270 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:168](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:168) |
| `0x001E2340` | BeACopResults2P::Sub_001E2340 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:62](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:62) |
| `0x001E2358` | BeACopResults1P::Sub_001E2358 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:22](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:22) |
| `0x001E2370` | BestTotalTimes::Sub_001E2370 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:130](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:130) |
| `0x001E2388` | BestLapTimes::Sub_001E2388 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:95](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:95) |
| `0x001E23A0` | TourneyStandingsScreen::Sub_001E23A0 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:303](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:303) |
| `0x001E23B8` | RaceResultsScreen::Sub_001E23B8 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:271](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:271) |
| `0x001E24F8` | EventRewardScreen::Sub_001E24F8 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:217](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:217) |
| `0x001E2580` | EndGameMenusBase::Render (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:336](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:336) |
| `0x001E2588` | BeACopResults1P::Sub_001E2588 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:29](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:29) |
| `0x001E2588` | BeACopResults2P::Sub_001E2588 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:67](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:67) |
| `0x001E2588` | BestLapTimes::Sub_001E2588 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:102](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:102) |
| `0x001E2588` | BestTotalTimes::Sub_001E2588 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:137](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:137) |
| `0x001E2588` | EndGameMenu::Sub_001E2588 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:175](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:175) |
| `0x001E2588` | EventRewardScreen::Sub_001E2588 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:224](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:224) |
| `0x001E2588` | TourneyStandingsScreen::Sub_001E2588 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:310](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:310) |
| `0x001E2590` | BeACopResults1P::Sub_001E2590 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:34](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:34) |
| `0x001E2590` | BeACopResults2P::Sub_001E2590 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:72](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:72) |
| `0x001E2590` | BestLapTimes::Sub_001E2590 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:107](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:107) |
| `0x001E2590` | BestTotalTimes::Sub_001E2590 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:142](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:142) |
| `0x001E2590` | EndGameMenu::Sub_001E2590 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:180](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:180) |
| `0x001E2590` | EventRewardScreen::Sub_001E2590 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:229](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:229) |
| `0x001E2590` | TourneyStandingsScreen::Sub_001E2590 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/endgamescreens.cpp:315](../src/game_core/indep/src/frontendscreens/endgamescreens.cpp:315) |
| `0x001E2848` | tStartRaceScreenManager::AddScreen (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/startracescreens.cpp:38](../src/game_core/indep/src/frontendscreens/startracescreens.cpp:38) |
| `0x001E2D00` | PreRaceStats::GetStatsForRacePosition (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/startracescreens.cpp:29](../src/game_core/indep/src/frontendscreens/startracescreens.cpp:29) |
| `0x001E4170` | tHelpMenu::IsActive (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/helpmenu.cpp:15](../src/game_core/indep/src/frontendscreens/helpmenu.cpp:15) |
| `0x001E6010` | CarCollisionPredict::IsThisCarStoppedDirectlyInFrontOfMe (signature and full body unresolved) | [src/game_core/indep/src/carcollisiondetect.cpp:13](../src/game_core/indep/src/carcollisiondetect.cpp:13) |
| `0x001E8560` | Chopper::GetMaxDeceleration (signature and full body unresolved) | [src/game_core/indep/src/chopper.cpp:38](../src/game_core/indep/src/chopper.cpp:38) |
| `0x001E9AE8` | Chopper::SetRotateTarget (signature and full body unresolved) | [src/game_core/indep/src/chopper.cpp:43](../src/game_core/indep/src/chopper.cpp:43) |
| `0x001E9E40` | Chopper::CalcChopperRotation (signature and full body unresolved) | [src/game_core/indep/src/chopper.cpp:48](../src/game_core/indep/src/chopper.cpp:48) |
| `0x001E9E80` | Chopper::SubtractFromYaw (signature and full body unresolved) | [src/game_core/indep/src/chopper.cpp:53](../src/game_core/indep/src/chopper.cpp:53) |
| `0x001EA578` | Chopper::RenderChopper (signature and full body unresolved) | [src/game_core/indep/src/chopper.cpp:58](../src/game_core/indep/src/chopper.cpp:58) |
| `0x001EB050` | bRepeat::WriteToLocation (signature and full body unresolved) | [src/game_core/indep/src/brepeat.cpp:13](../src/game_core/indep/src/brepeat.cpp:13) |
| `0x001EB0C0` | bRepeat::ReadFromLocation (signature and full body unresolved) | [src/game_core/indep/src/brepeat.cpp:18](../src/game_core/indep/src/brepeat.cpp:18) |
| `0x001EB498` | bRepeat::PrepareToRemoveData (signature and full body unresolved) | [src/game_core/indep/src/brepeat.cpp:23](../src/game_core/indep/src/brepeat.cpp:23) |
| `0x001EB6A8` | HeliPoly::IsPointInPoly (signature and full body unresolved) | [src/game_core/indep/src/helisheet.cpp:18](../src/game_core/indep/src/helisheet.cpp:18) |
| `0x001EB840` | HeliPoly::GetZValue (signature and full body unresolved) | [src/game_core/indep/src/helisheet.cpp:23](../src/game_core/indep/src/helisheet.cpp:23) |
| `0x001EC510` | ReplayScreen::HasCameraMover (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/replayscreen.cpp:16](../src/game_core/indep/src/frontendscreens/replayscreen.cpp:16) |
| `0x001ED410` | ReplayScreen::GetTimelinePositionX (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/replayscreen.cpp:21](../src/game_core/indep/src/frontendscreens/replayscreen.cpp:21) |
| `0x001EDC50` | RaceParameters::ClearEliminatedDrivers (signature and full body unresolved) | [src/game_core/indep/src/raceparameters.cpp:13](../src/game_core/indep/src/raceparameters.cpp:13) |
| `0x001EDCD0` | RaceParameters::GetNumEliminated (signature and full body unresolved) | [src/game_core/indep/src/raceparameters.cpp:18](../src/game_core/indep/src/raceparameters.cpp:18) |
| `0x001EDDB8` | RaceParameters::GetLocationNumber (signature and full body unresolved) | [src/game_core/indep/src/raceparameters.cpp:23](../src/game_core/indep/src/raceparameters.cpp:23) |
| `0x001EE120` | tGlobalDisplaySettings::CycleLeaderboardMode (signature and full body unresolved) | [src/game_core/indep/src/raceparameters.cpp:67](../src/game_core/indep/src/raceparameters.cpp:67) |
| `0x001EE1F8` | cPlayerSettings::ToggleGauges (signature and full body unresolved) | [src/game_core/indep/src/raceparameters.cpp:35](../src/game_core/indep/src/raceparameters.cpp:35) |
| `0x001EE230` | cPlayerSettings::ToggleLapInfo (signature and full body unresolved) | [src/game_core/indep/src/raceparameters.cpp:40](../src/game_core/indep/src/raceparameters.cpp:40) |
| `0x001EE258` | cPlayerSettings::CycleSplitTime (signature and full body unresolved) | [src/game_core/indep/src/raceparameters.cpp:45](../src/game_core/indep/src/raceparameters.cpp:45) |
| `0x001EE2B0` | cPlayerSettings::ToggleRacePosition (signature and full body unresolved) | [src/game_core/indep/src/raceparameters.cpp:50](../src/game_core/indep/src/raceparameters.cpp:50) |
| `0x001EE2D8` | cPlayerSettings::ToggleLookback (signature and full body unresolved) | [src/game_core/indep/src/raceparameters.cpp:55](../src/game_core/indep/src/raceparameters.cpp:55) |
| `0x001EE300` | cPlayerSettings::CycleAlternateCamera (signature and full body unresolved) | [src/game_core/indep/src/raceparameters.cpp:60](../src/game_core/indep/src/raceparameters.cpp:60) |
| `0x001EE390` | RaceParameters::IsFreeRun (signature and full body unresolved) | [src/game_core/indep/src/raceparameters.cpp:28](../src/game_core/indep/src/raceparameters.cpp:28) |
| `0x001EE6C0` | display preparation not translated | [src/port/runtime/frontend_runtime.cpp:449](../src/port/runtime/frontend_runtime.cpp:449) |
| `0x001EEBB8` | tOptionsScreen::Sub_001EEBB8 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/options.cpp:370](../src/game_core/indep/src/frontendscreens/options.cpp:370) |
| `0x001EEC50` | tOptionsScreen::Sub_001EEC50 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/options.cpp:375](../src/game_core/indep/src/frontendscreens/options.cpp:375) |
| `0x001EECA8` | tOptionsScreen::Sub_001EECA8 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/options.cpp:380](../src/game_core/indep/src/frontendscreens/options.cpp:380) |
| `0x001EECA8` | unmatched item uses unresolved default target at 0x00331660 | [src/game_core/indep/src/frontendscreens/options.cpp:107](../src/game_core/indep/src/frontendscreens/options.cpp:107) |
| `0x001EF3C8` | AudioOptionsScreen::Sub_001EF3C8 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/options.cpp:307](../src/game_core/indep/src/frontendscreens/options.cpp:307) |
| `0x001EFA00` | AudioOptionsScreen::Sub_001EFA00 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/options.cpp:312](../src/game_core/indep/src/frontendscreens/options.cpp:312) |
| `0x001EFA68` | AudioOptionsScreen::FindSlider (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/options.cpp:389](../src/game_core/indep/src/frontendscreens/options.cpp:389) |
| `0x001EFB78` | AudioOptionsScreen::Sub_001EFB78 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/options.cpp:317](../src/game_core/indep/src/frontendscreens/options.cpp:317) |
| `0x001EFB80` | AudioOptionsScreen::Sub_001EFB80 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/options.cpp:322](../src/game_core/indep/src/frontendscreens/options.cpp:322) |
| `0x001F02D8` | JukeBoxMusicScreen::Sub_001F02D8 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/options.cpp:329](../src/game_core/indep/src/frontendscreens/options.cpp:329) |
| `0x001F0460` | JukeBoxMusicScreen::Sub_001F0460 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/options.cpp:334](../src/game_core/indep/src/frontendscreens/options.cpp:334) |
| `0x001F0D10` | JukeBoxMusicScreen::Sub_001F0D10 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/options.cpp:339](../src/game_core/indep/src/frontendscreens/options.cpp:339) |
| `0x001F1528` | UserNameOptionsScreen::DisplayGender (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/options.cpp:420](../src/game_core/indep/src/frontendscreens/options.cpp:420) |
| `0x001F1A78` | LanguageOptionsScreenBase::Sub_001F1A78 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/options.cpp:396](../src/game_core/indep/src/frontendscreens/options.cpp:396) |
| `0x001F1B08` | activation side-effect service missing | [src/game_core/indep/src/frontendscreens/options.cpp:129](../src/game_core/indep/src/frontendscreens/options.cpp:129) |
| `0x001F1B08` | language state missing | [src/game_core/indep/src/frontendscreens/options.cpp:209](../src/game_core/indep/src/frontendscreens/options.cpp:209) |
| `0x001F33B8` | tDisplayOptionsScreen::Sub_001F33B8 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/options.cpp:348](../src/game_core/indep/src/frontendscreens/options.cpp:348) |
| `0x001F3588` | tDisplayOptionsScreen::Sub_001F3588 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/options.cpp:353](../src/game_core/indep/src/frontendscreens/options.cpp:353) |
| `0x001F35E0` | tDisplayOptionsScreen::Sub_001F35E0 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/options.cpp:358](../src/game_core/indep/src/frontendscreens/options.cpp:358) |
| `0x001F3A28` | tDisplayOptionsScreen::Sub_001F3A28 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/options.cpp:363](../src/game_core/indep/src/frontendscreens/options.cpp:363) |
| `0x001F3F20` | tCustomizeHUDScreen::Sub_001F3F20 (signature and full body unresolved) | [src/game_core/tCustomizeHUDScreen.cpp:13](../src/game_core/tCustomizeHUDScreen.cpp:13) |
| `0x001F4178` | tCustomizeHUDScreen::Sub_001F4178 (signature and full body unresolved) | [src/game_core/tCustomizeHUDScreen.cpp:18](../src/game_core/tCustomizeHUDScreen.cpp:18) |
| `0x001F46D8` | tCustomizeHUDScreen::HandleEvent (signature and full body unresolved) | [src/game_core/tCustomizeHUDScreen.cpp:24](../src/game_core/tCustomizeHUDScreen.cpp:24) |
| `0x001F4DC8` | OptionsScreenBase::Sub_001F4DC8 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/options.cpp:403](../src/game_core/indep/src/frontendscreens/options.cpp:403) |
| `0x001F4E68` | OptionsScreenBase::HandleHighlight (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/options.cpp:408](../src/game_core/indep/src/frontendscreens/options.cpp:408) |
| `0x001F5B80` | OptionsScreenBase::HandleEvent (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/options.cpp:413](../src/game_core/indep/src/frontendscreens/options.cpp:413) |
| `0x001F5C28` | Autosave history push service missing | [src/game_core/indep/src/frontendscreens/options.cpp:175](../src/game_core/indep/src/frontendscreens/options.cpp:175) |
| `0x001F5C28` | controller Autosave history service missing | [src/game_core/indep/src/frontendscreens/joystickconfiguration.cpp:18](../src/game_core/indep/src/frontendscreens/joystickconfiguration.cpp:18) |
| `0x001F5D00` | DataEventSelectScreen::GetSelectedEvent (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/campaigntrees.cpp:13](../src/game_core/indep/src/frontendscreens/campaigntrees.cpp:13) |
| `0x001F5D28` | DataEventSelectScreen::SetSelectedEvent (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/campaigntrees.cpp:18](../src/game_core/indep/src/frontendscreens/campaigntrees.cpp:18) |
| `0x001F5DD0` | NUM_LAPS requires first track row | [src/game_core/indep/src/eventtree.cpp:691](../src/game_core/indep/src/eventtree.cpp:691) |
| `0x001F6220` | reward kind has no text-producing arm in original | [src/game_core/indep/src/eventtree.cpp:659](../src/game_core/indep/src/eventtree.cpp:659) |
| `0x001F6440` | reward preview car/track model renderer not translated | [src/port/runtime/frontend_runtime.cpp:406](../src/port/runtime/frontend_runtime.cpp:406) |
| `0x001F65C8` | tTreeReward::HandleMessages (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/campaigntrees.cpp:74](../src/game_core/indep/src/frontendscreens/campaigntrees.cpp:74) |
| `0x001F6A28` | event car/race selection dependencies not translated | [src/port/runtime/frontend_runtime.cpp:384](../src/port/runtime/frontend_runtime.cpp:384) |
| `0x001F6CF0` | authored HLINE/VLINE template unavailable | [src/port/runtime/event_tree_view.cpp:347](../src/port/runtime/event_tree_view.cpp:347) |
| `0x001F6CF0` | authored line texture unavailable | [src/port/runtime/event_tree_view.cpp:350](../src/port/runtime/event_tree_view.cpp:350) |
| `0x001F6FA0` | tEventLeaf::Sub_001F6FA0 (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:840](../src/game_core/indep/src/eventtree.cpp:840) |
| `0x001F72B0` | tUnlockMap::AddGoal (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/campaigntrees.cpp:81](../src/game_core/indep/src/frontendscreens/campaigntrees.cpp:81) |
| `0x001F77F0` | event31 tree gate unavailable | [src/port/runtime/event_tree_view.cpp:150](../src/port/runtime/event_tree_view.cpp:150) |
| `0x001F7FA8` | tEventLeaf::CalculateGeneration (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/campaigntrees.cpp:41](../src/game_core/indep/src/frontendscreens/campaigntrees.cpp:41) |
| `0x001F7FE8` | tEventGroup::IsMember (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/campaigntrees.cpp:34](../src/game_core/indep/src/frontendscreens/campaigntrees.cpp:34) |
| `0x001F8AD8` | tParallaxer::Sub_001F8AD8 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/campaigntrees.cpp:55](../src/game_core/indep/src/frontendscreens/campaigntrees.cpp:55) |
| `0x001F8C10` | car logo widget C6398C48 missing | [src/game_core/indep/src/eventtree.cpp:732](../src/game_core/indep/src/eventtree.cpp:732) |
| `0x001F8C10` | tCarLogoAnimation::Sub_001F8C10 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/campaigntrees.cpp:25](../src/game_core/indep/src/frontendscreens/campaigntrees.cpp:25) |
| `0x001F8D20` | car logo animator not bound | [src/game_core/indep/src/eventtree.cpp:739](../src/game_core/indep/src/eventtree.cpp:739) |
| `0x001F8E18` | car logo index outside catalog | [src/port/assets/event_name_catalog.hpp:67](../src/port/assets/event_name_catalog.hpp:67) |
| `0x001F8FB0` | car logo animator not bound | [src/game_core/indep/src/eventtree.cpp:763](../src/game_core/indep/src/eventtree.cpp:763) |
| `0x001F90F0` | tTrackLogoAnimation::Sub_001F90F0 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/campaigntrees.cpp:62](../src/game_core/indep/src/frontendscreens/campaigntrees.cpp:62) |
| `0x001F90F0` | track logo widget F60114E4 missing | [src/game_core/indep/src/eventtree.cpp:788](../src/game_core/indep/src/eventtree.cpp:788) |
| `0x001F91D0` | tTrackLogoAnimation::Show (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/campaigntrees.cpp:67](../src/game_core/indep/src/frontendscreens/campaigntrees.cpp:67) |
| `0x001F91F0` | track logo animator not bound | [src/game_core/indep/src/eventtree.cpp:800](../src/game_core/indep/src/eventtree.cpp:800) |
| `0x001F9318` | track logo animator not bound | [src/game_core/indep/src/eventtree.cpp:810](../src/game_core/indep/src/eventtree.cpp:810) |
| `0x001FA840` | tEventTreeView::FindLeafByEventNumber (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/campaigntrees.cpp:48](../src/game_core/indep/src/frontendscreens/campaigntrees.cpp:48) |
| `0x001FC580` | EventLeaf::SetScrollNotch (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:906](../src/game_core/indep/src/eventtree.cpp:906) |
| `0x001FCC88` | EventLeaf::FillVerts (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:911](../src/game_core/indep/src/eventtree.cpp:911) |
| `0x001FD050` | EventLeaf::GetNeighbour (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:916](../src/game_core/indep/src/eventtree.cpp:916) |
| `0x001FD0B0` | EventLeaf::GetReward (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:921](../src/game_core/indep/src/eventtree.cpp:921) |
| `0x001FDFE8` | EventTree::GetEventNumber (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:954](../src/game_core/indep/src/eventtree.cpp:954) |
| `0x001FE988` | event profile slices overflow the 0x100-byte block, which the | [src/game_core/indep/src/eventtree.cpp:548](../src/game_core/indep/src/eventtree.cpp:548) |
| `0x001FEE80` | Topology diagnostic service absent | [src/game_core/indep/src/realdriver.cpp:116](../src/game_core/indep/src/realdriver.cpp:116) |
| `0x001FFF60` | EventTree::SetEvents (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:959](../src/game_core/indep/src/eventtree.cpp:959) |
| `0x00200018` | EventRankState::GetStatus (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:928](../src/game_core/indep/src/eventtree.cpp:928) |
| `0x00200068` | EventTimeState::GetStatus (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:935](../src/game_core/indep/src/eventtree.cpp:935) |
| `0x002001A8` | EventKoState::GetRaceIndex (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:899](../src/game_core/indep/src/eventtree.cpp:899) |
| `0x00200638` | EventTournamentState::Reset (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:942](../src/game_core/indep/src/eventtree.cpp:942) |
| `0x002006A0` | point-count table index outside recovered eight bytes | [src/game_core/indep/src/eventtree.cpp:38](../src/game_core/indep/src/eventtree.cpp:38) |
| `0x00200710` | EventTournamentState::RacerRank (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:947](../src/game_core/indep/src/eventtree.cpp:947) |
| `0x00200878` | EventInfo::GetCar (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:847](../src/game_core/indep/src/eventtree.cpp:847) |
| `0x002008B8` | EventInfo::GetGoal (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:852](../src/game_core/indep/src/eventtree.cpp:852) |
| `0x002008F8` | EventInfo::GetTrack (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:857](../src/game_core/indep/src/eventtree.cpp:857) |
| `0x00200938` | EventInfo::GetReward (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:862](../src/game_core/indep/src/eventtree.cpp:862) |
| `0x00200978` | EventInfo::GetOpponent (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:867](../src/game_core/indep/src/eventtree.cpp:867) |
| `0x00200D48` | EventInfo::GetRankState (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:872](../src/game_core/indep/src/eventtree.cpp:872) |
| `0x00200D70` | EventInfo::GetTournamentState (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:877](../src/game_core/indep/src/eventtree.cpp:877) |
| `0x00200D90` | EventInfo::GetKoState (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:882](../src/game_core/indep/src/eventtree.cpp:882) |
| `0x00200DB0` | EventInfo::GetTimeState (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:887](../src/game_core/indep/src/eventtree.cpp:887) |
| `0x00200E78` | goal type has no result-text arm in original | [src/game_core/indep/src/eventtree.cpp:720](../src/game_core/indep/src/eventtree.cpp:720) |
| `0x00200E78` | result index beyond nine recovered placing text IDs | [src/game_core/indep/src/eventtree.cpp:723](../src/game_core/indep/src/eventtree.cpp:723) |
| `0x002010B8` | event 31's gate reads DAT_0031D7F4+0x180 / +0x1B8, which are not | [src/game_core/indep/src/eventtree.cpp:195](../src/game_core/indep/src/eventtree.cpp:195) |
| `0x002010B8` | event 32 requires events 1..31 of BOTH trees to be Won, which | [src/game_core/indep/src/eventtree.cpp:204](../src/game_core/indep/src/eventtree.cpp:204) |
| `0x002010B8` | event32 completion event unavailable | [src/port/runtime/event_tree_view.cpp:113](../src/port/runtime/event_tree_view.cpp:113) |
| `0x002010B8` | event32 completion event unavailable | [src/port/runtime/event_tree_view.cpp:192](../src/port/runtime/event_tree_view.cpp:192) |
| `0x00201350` | EventInfo::GetRaceCount (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:892](../src/game_core/indep/src/eventtree.cpp:892) |
| `0x00201558` | RaceEvent::RacerIndexFromDriverNumber (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:966](../src/game_core/indep/src/eventtree.cpp:966) |
| `0x00201578` | RaceEvent::DriverNumberFromRacerIndex (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:971](../src/game_core/indep/src/eventtree.cpp:971) |
| `0x00201858` | RaceEvent::GetRankState (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:976](../src/game_core/indep/src/eventtree.cpp:976) |
| `0x00201880` | RaceEvent::GetTournamentState (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:981](../src/game_core/indep/src/eventtree.cpp:981) |
| `0x002018A0` | RaceEvent::GetKoState (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:986](../src/game_core/indep/src/eventtree.cpp:986) |
| `0x002018C0` | RaceEvent::GetTimeState (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:991](../src/game_core/indep/src/eventtree.cpp:991) |
| `0x00202088` | RaceEvent::GetRaceCount (signature and full body unresolved) | [src/game_core/indep/src/eventtree.cpp:996](../src/game_core/indep/src/eventtree.cpp:996) |
| `0x00202F60` | DeliveryMover::Sub_00202F60 (signature and full body unresolved) | [src/game_core/indep/src/deliverymover.cpp:15](../src/game_core/indep/src/deliverymover.cpp:15) |
| `0x00202F78` | DeliveryMover::Sub_00202F78 (signature and full body unresolved) | [src/game_core/indep/src/deliverymover.cpp:20](../src/game_core/indep/src/deliverymover.cpp:20) |
| `0x002030C0` | DeliveryMover::Sub_002030C0 (signature and full body unresolved) | [src/game_core/indep/src/deliverymover.cpp:25](../src/game_core/indep/src/deliverymover.cpp:25) |
| `0x002033D8` | DeliveryMover::Sub_002033D8 (signature and full body unresolved) | [src/game_core/indep/src/deliverymover.cpp:30](../src/game_core/indep/src/deliverymover.cpp:30) |
| `0x00204098` | BeltMover::Sub_00204098 (signature and full body unresolved) | [src/game_core/indep/src/beltmover.cpp:19](../src/game_core/indep/src/beltmover.cpp:19) |
| `0x002043A0` | BeltMover::Sub_002043A0 (signature and full body unresolved) | [src/game_core/indep/src/beltmover.cpp:24](../src/game_core/indep/src/beltmover.cpp:24) |
| `0x00204468` | BeltMover::DoSnapshot (signature and full body unresolved) | [src/game_core/indep/src/beltmover.cpp:30](../src/game_core/indep/src/beltmover.cpp:30) |
| `0x00204D78` | BeltMover::Move (signature and full body unresolved) | [src/game_core/indep/src/beltmover.cpp:36](../src/game_core/indep/src/beltmover.cpp:36) |
| `0x002059D0` | cSEDWedge::S_DebugPrint (signature and full body unresolved) | [src/game_core/indep/src/speech/sedwedge.cpp:13](../src/game_core/indep/src/speech/sedwedge.cpp:13) |
| `0x00207C68` | TrackPathManager::FindClosestPoint (signature and full body unresolved) | [src/game_core/indep/src/trackpath.cpp:53](../src/game_core/indep/src/trackpath.cpp:53) |
| `0x00207ED0` | TrackPathManager::FindClosestAdjacentPointOnSamePath (signature and full body unresolved) | [src/game_core/indep/src/trackpath.cpp:58](../src/game_core/indep/src/trackpath.cpp:58) |
| `0x00208598` | TrackPathManager::CompareDistances (signature and full body unresolved) | [src/game_core/indep/src/trackpath.cpp:63](../src/game_core/indep/src/trackpath.cpp:63) |
| `0x002085E8` | TrackPathManager::ComparePercentDistances (signature and full body unresolved) | [src/game_core/indep/src/trackpath.cpp:68](../src/game_core/indep/src/trackpath.cpp:68) |
| `0x002086B8` | TrackPathCoordinate::Sub_002086B8 (signature and full body unresolved) | [src/game_core/indep/src/trackpath.cpp:16](../src/game_core/indep/src/trackpath.cpp:16) |
| `0x00208B28` | Sound3d::Activate (signature and full body unresolved) | [src/game_core/indep/src/sound3d.cpp:15](../src/game_core/indep/src/sound3d.cpp:15) |
| `0x00208C78` | Sound3d::UpdateCamRelativeDistAndVelocity (signature and full body unresolved) | [src/game_core/indep/src/sound3d.cpp:20](../src/game_core/indep/src/sound3d.cpp:20) |
| `0x00208FE0` | Sound3d::AzimuthToPan (signature and full body unresolved) | [src/game_core/indep/src/sound3d.cpp:27](../src/game_core/indep/src/sound3d.cpp:27) |
| `0x002091A0` | Sound3d::Sub_002091A0 (signature and full body unresolved) | [src/game_core/indep/src/sound3d.cpp:32](../src/game_core/indep/src/sound3d.cpp:32) |
| `0x002095D0` | Sound3dSample::Sub_002095D0 (signature and full body unresolved) | [src/game_core/indep/src/sound3dsample.cpp:13](../src/game_core/indep/src/sound3dsample.cpp:13) |
| `0x002096A0` | Sound3dSample::Sub_002096A0 (signature and full body unresolved) | [src/game_core/indep/src/sound3dsample.cpp:18](../src/game_core/indep/src/sound3dsample.cpp:18) |
| `0x002096F8` | Sound3dSample::Sub_002096F8 (signature and full body unresolved) | [src/game_core/indep/src/sound3dsample.cpp:23](../src/game_core/indep/src/sound3dsample.cpp:23) |
| `0x00209DC0` | Sound3dSample::ActivateSound (signature and full body unresolved) | [src/game_core/indep/src/sound3dsample.cpp:28](../src/game_core/indep/src/sound3dsample.cpp:28) |
| `0x00209E18` | Sound3dSample::DeactivateSound (signature and full body unresolved) | [src/game_core/indep/src/sound3dsample.cpp:33](../src/game_core/indep/src/sound3dsample.cpp:33) |
| `0x00209E98` | Sound3dSample::Mute (signature and full body unresolved) | [src/game_core/indep/src/sound3dsample.cpp:38](../src/game_core/indep/src/sound3dsample.cpp:38) |
| `0x0020BE48` | actionHelicopter_t::execute (signature and full body unresolved) | [src/game_core/indep/src/speech/action.cpp:13](../src/game_core/indep/src/speech/action.cpp:13) |
| `0x0020BFB0` | fsm::handleEvent (signature and full body unresolved) | [src/game_core/indep/src/speech/fsm.cpp:13](../src/game_core/indep/src/speech/fsm.cpp:13) |
| `0x0020D8C0` | nfsFsm::Sub_0020D8C0 (signature and full body unresolved) | [src/game_core/indep/src/speech/nfsfsm.cpp:13](../src/game_core/indep/src/speech/nfsfsm.cpp:13) |
| `0x0020E3C8` | rotorSpikeFailedAction_t::executeAction (signature and full body unresolved) | [src/game_core/indep/src/speech/nfsfsm.cpp:36](../src/game_core/indep/src/speech/nfsfsm.cpp:36) |
| `0x0020E3E8` | rotorLocationAction_t::executeAction (signature and full body unresolved) | [src/game_core/indep/src/speech/nfsfsm.cpp:29](../src/game_core/indep/src/speech/nfsfsm.cpp:29) |
| `0x0020E410` | rotorTrafficAction_t::executeAction (signature and full body unresolved) | [src/game_core/indep/src/speech/nfsfsm.cpp:43](../src/game_core/indep/src/speech/nfsfsm.cpp:43) |
| `0x0020E438` | rotorChatterAction_t::executeAction (signature and full body unresolved) | [src/game_core/indep/src/speech/nfsfsm.cpp:22](../src/game_core/indep/src/speech/nfsfsm.cpp:22) |
| `0x0020FC70` | PlayerEngineSound::Sub_0020FC70 (signature and full body unresolved) | [src/game_core/indep/src/carsoundengine.cpp:68](../src/game_core/indep/src/carsoundengine.cpp:68) |
| `0x0020FE70` | PlayerEngineSound::Sub_0020FE70 (signature and full body unresolved) | [src/game_core/indep/src/carsoundengine.cpp:73](../src/game_core/indep/src/carsoundengine.cpp:73) |
| `0x00210400` | PlayerEngineSound::DeactivateSound (signature and full body unresolved) | [src/game_core/indep/src/carsoundengine.cpp:79](../src/game_core/indep/src/carsoundengine.cpp:79) |
| `0x00210408` | PlayerEngineSound::ActivateSound (signature and full body unresolved) | [src/game_core/indep/src/carsoundengine.cpp:85](../src/game_core/indep/src/carsoundengine.cpp:85) |
| `0x00210410` | PlayerEngineSound::Mute (signature and full body unresolved) | [src/game_core/indep/src/carsoundengine.cpp:91](../src/game_core/indep/src/carsoundengine.cpp:91) |
| `0x002105D0` | PlayerEngineSound::ResolveTables (signature and full body unresolved) | [src/game_core/indep/src/carsoundengine.cpp:104](../src/game_core/indep/src/carsoundengine.cpp:104) |
| `0x00211390` | PlayerEngineSound::Update (signature and full body unresolved) | [src/game_core/indep/src/carsoundengine.cpp:97](../src/game_core/indep/src/carsoundengine.cpp:97) |
| `0x002117C8` | AIEngineSound::Sub_002117C8 (signature and full body unresolved) | [src/game_core/indep/src/carsoundengine.cpp:19](../src/game_core/indep/src/carsoundengine.cpp:19) |
| `0x002118A0` | AIEngineSound::Update (signature and full body unresolved) | [src/game_core/indep/src/carsoundengine.cpp:25](../src/game_core/indep/src/carsoundengine.cpp:25) |
| `0x00211A80` | AIEngineSound::Mute (signature and full body unresolved) | [src/game_core/indep/src/carsoundengine.cpp:31](../src/game_core/indep/src/carsoundengine.cpp:31) |
| `0x00211D58` | AIEngineSound::DeactivateSound (signature and full body unresolved) | [src/game_core/indep/src/carsoundengine.cpp:37](../src/game_core/indep/src/carsoundengine.cpp:37) |
| `0x00211D60` | AIEngineSound::ActivateSound (signature and full body unresolved) | [src/game_core/indep/src/carsoundengine.cpp:43](../src/game_core/indep/src/carsoundengine.cpp:43) |
| `0x00212418` | AIEngineSound::Sub_00212418 (signature and full body unresolved) | [src/game_core/indep/src/carsoundengine.cpp:48](../src/game_core/indep/src/carsoundengine.cpp:48) |
| `0x002124F0` | kAITireSound::Sub_002124F0 (signature and full body unresolved) | [src/game_core/indep/src/carsoundtires.cpp:80](../src/game_core/indep/src/carsoundtires.cpp:80) |
| `0x002125A0` | BaseTireSound::Mute (signature and full body unresolved) | [src/game_core/indep/src/carsoundtires.cpp:102](../src/game_core/indep/src/carsoundtires.cpp:102) |
| `0x00212950` | PlayerTireSound::Sub_00212950 (signature and full body unresolved) | [src/game_core/indep/src/carsoundtires.cpp:19](../src/game_core/indep/src/carsoundtires.cpp:19) |
| `0x00212CF0` | PlayerTireSound::Sub_00212CF0 (signature and full body unresolved) | [src/game_core/indep/src/carsoundtires.cpp:24](../src/game_core/indep/src/carsoundtires.cpp:24) |
| `0x00212E88` | PlayerTireSound::DeactivateSound (signature and full body unresolved) | [src/game_core/indep/src/carsoundtires.cpp:30](../src/game_core/indep/src/carsoundtires.cpp:30) |
| `0x00212E90` | PlayerTireSound::ActivateSound (signature and full body unresolved) | [src/game_core/indep/src/carsoundtires.cpp:36](../src/game_core/indep/src/carsoundtires.cpp:36) |
| `0x002134D8` | PlayerTireSound::Mute (signature and full body unresolved) | [src/game_core/indep/src/carsoundtires.cpp:42](../src/game_core/indep/src/carsoundtires.cpp:42) |
| `0x002135A0` | PlayerTireSound::Update (signature and full body unresolved) | [src/game_core/indep/src/carsoundtires.cpp:48](../src/game_core/indep/src/carsoundtires.cpp:48) |
| `0x00214890` | kAITireSound::ActivateSound (signature and full body unresolved) | [src/game_core/indep/src/carsoundtires.cpp:124](../src/game_core/indep/src/carsoundtires.cpp:124) |
| `0x00214898` | kAITireSound::DeactivateSound (signature and full body unresolved) | [src/game_core/indep/src/carsoundtires.cpp:129](../src/game_core/indep/src/carsoundtires.cpp:129) |
| `0x002148E0` | kAITireSound::Mute (signature and full body unresolved) | [src/game_core/indep/src/carsoundtires.cpp:134](../src/game_core/indep/src/carsoundtires.cpp:134) |
| `0x00214928` | kAITireSound::Update (signature and full body unresolved) | [src/game_core/indep/src/carsoundtires.cpp:139](../src/game_core/indep/src/carsoundtires.cpp:139) |
| `0x002149A0` | kAITireSound::Sub_002149A0 (signature and full body unresolved) | [src/game_core/indep/src/carsoundtires.cpp:144](../src/game_core/indep/src/carsoundtires.cpp:144) |
| `0x002149F8` | PlayerTireSound::ScaleRoadNoise (signature and full body unresolved) | [src/game_core/indep/src/carsoundtires.cpp:54](../src/game_core/indep/src/carsoundtires.cpp:54) |
| `0x00214A00` | PlayerTireSound::TriggerTireBlowOut (signature and full body unresolved) | [src/game_core/indep/src/carsoundtires.cpp:60](../src/game_core/indep/src/carsoundtires.cpp:60) |
| `0x00214A68` | BaseTireSound::ScaleRoadNoise (signature and full body unresolved) | [src/game_core/indep/src/carsoundtires.cpp:107](../src/game_core/indep/src/carsoundtires.cpp:107) |
| `0x00214A70` | BaseTireSound::TriggerTireBlowOut (signature and full body unresolved) | [src/game_core/indep/src/carsoundtires.cpp:112](../src/game_core/indep/src/carsoundtires.cpp:112) |
| `0x00214A78` | BaseTireSound::Sub_00214A78 (signature and full body unresolved) | [src/game_core/indep/src/carsoundtires.cpp:117](../src/game_core/indep/src/carsoundtires.cpp:117) |
| `0x00214D68` | ChopperSound::Sub_00214D68 (signature and full body unresolved) | [src/game_core/indep/src/choppersound.cpp:19](../src/game_core/indep/src/choppersound.cpp:19) |
| `0x00214DF0` | ChopperSound::Sub_00214DF0 (signature and full body unresolved) | [src/game_core/indep/src/choppersound.cpp:24](../src/game_core/indep/src/choppersound.cpp:24) |
| `0x00214F30` | ChopperSound::Update (signature and full body unresolved) | [src/game_core/indep/src/choppersound.cpp:30](../src/game_core/indep/src/choppersound.cpp:30) |
| `0x00215000` | ChopperSound::Mute (signature and full body unresolved) | [src/game_core/indep/src/choppersound.cpp:36](../src/game_core/indep/src/choppersound.cpp:36) |
| `0x00215028` | ChopperSound::DeactivateSound (signature and full body unresolved) | [src/game_core/indep/src/choppersound.cpp:42](../src/game_core/indep/src/choppersound.cpp:42) |
| `0x00215050` | ChopperSound::ActivateSound (signature and full body unresolved) | [src/game_core/indep/src/choppersound.cpp:48](../src/game_core/indep/src/choppersound.cpp:48) |
| `0x00216C18` | Quick Race leaves the front end: it writes the race configuration block | [src/game_core/port/frontend/frontendscreens/modeselect.cpp:133](../src/game_core/port/frontend/frontendscreens/modeselect.cpp:133) |
| `0x00217020` | exit-screen-animation service missing | [src/game_core/port/frontend/frontendscreens/modeselect.cpp:18](../src/game_core/port/frontend/frontendscreens/modeselect.cpp:18) |
| `0x002197F0` | MellowMover::Sub_002197F0 (signature and full body unresolved) | [src/game_core/indep/src/mellowmover.cpp:65](../src/game_core/indep/src/mellowmover.cpp:65) |
| `0x002198C8` | MellowMover::DoSnapshot (signature and full body unresolved) | [src/game_core/indep/src/mellowmover.cpp:70](../src/game_core/indep/src/mellowmover.cpp:70) |
| `0x00219D38` | MellowMover::Move (signature and full body unresolved) | [src/game_core/indep/src/mellowmover.cpp:75](../src/game_core/indep/src/mellowmover.cpp:75) |
| `0x00219ED8` | MellowMover::ComputeAuxiliaries (signature and full body unresolved) | [src/game_core/indep/src/mellowmover.cpp:80](../src/game_core/indep/src/mellowmover.cpp:80) |
| `0x00219F50` | MellowMover::ResolveForces (signature and full body unresolved) | [src/game_core/indep/src/mellowmover.cpp:85](../src/game_core/indep/src/mellowmover.cpp:85) |
| `0x0021A638` | Mellovator::Sub_0021A638 (signature and full body unresolved) | [src/game_core/indep/src/mellowmover.cpp:18](../src/game_core/indep/src/mellowmover.cpp:18) |
| `0x0021A680` | Mellovator::GetDerivative (signature and full body unresolved) | [src/game_core/indep/src/mellowmover.cpp:23](../src/game_core/indep/src/mellowmover.cpp:23) |
| `0x0021A8C8` | Mellovator::ComputeAuxiliaries (signature and full body unresolved) | [src/game_core/indep/src/mellowmover.cpp:28](../src/game_core/indep/src/mellowmover.cpp:28) |
| `0x0021AED8` | Mellovator::ResolveForces (signature and full body unresolved) | [src/game_core/indep/src/mellowmover.cpp:33](../src/game_core/indep/src/mellowmover.cpp:33) |
| `0x0021B7F0` | Mellovator::Sub_0021B7F0 (signature and full body unresolved) | [src/game_core/indep/src/mellowmover.cpp:38](../src/game_core/indep/src/mellowmover.cpp:38) |
| `0x0021B880` | Mellovator::Sub_0021B880 (signature and full body unresolved) | [src/game_core/indep/src/mellowmover.cpp:43](../src/game_core/indep/src/mellowmover.cpp:43) |
| `0x0021B8A8` | Mellovator::Sub_0021B8A8 (signature and full body unresolved) | [src/game_core/indep/src/mellowmover.cpp:48](../src/game_core/indep/src/mellowmover.cpp:48) |
| `0x0021B8D0` | Mellovator::Sub_0021B8D0 (signature and full body unresolved) | [src/game_core/indep/src/mellowmover.cpp:53](../src/game_core/indep/src/mellowmover.cpp:53) |
| `0x0021B8D8` | Mellovator::GetSize (signature and full body unresolved) | [src/game_core/indep/src/mellowmover.cpp:58](../src/game_core/indep/src/mellowmover.cpp:58) |
| `0x0021BDC8` | DriveTarget::GetLastIndexToCheckForDrivability (signature and full body unresolved) | [src/game_core/indep/src/drivetarget.cpp:13](../src/game_core/indep/src/drivetarget.cpp:13) |
| `0x0021DAC8` | DriveTarget::RecalculateLength (signature and full body unresolved) | [src/game_core/indep/src/drivetarget.cpp:18](../src/game_core/indep/src/drivetarget.cpp:18) |
| `0x0021E0D0` | asset root unavailable | [src/port/runtime/frontend_runtime.cpp:702](../src/port/runtime/frontend_runtime.cpp:702) |
| `0x0021E0D0` | locale asset unavailable | [src/port/runtime/frontend_runtime.cpp:707](../src/port/runtime/frontend_runtime.cpp:707) |
| `0x0021E0D0` | unknown locale | [src/port/runtime/frontend_runtime.cpp:701](../src/port/runtime/frontend_runtime.cpp:701) |
| `0x0021E2E8` | LanguageManager::GetString (signature and full body unresolved) | [src/game_core/indep/src/language.cpp:13](../src/game_core/indep/src/language.cpp:13) |
| `0x0021E3D0` | LanguageManager::Loader (signature and full body unresolved) | [src/game_core/indep/src/language.cpp:18](../src/game_core/indep/src/language.cpp:18) |
| `0x0021E428` | LanguageManager::Unloader (signature and full body unresolved) | [src/game_core/indep/src/language.cpp:23](../src/game_core/indep/src/language.cpp:23) |
| `0x0021E560` | event text localization entry unavailable | [src/port/runtime/frontend_runtime.cpp:534](../src/port/runtime/frontend_runtime.cpp:534) |
| `0x0021E560` | result localization lookup missing | [src/game_core/indep/src/eventtree.cpp:724](../src/game_core/indep/src/eventtree.cpp:724) |
| `0x0021EB20` | Collision::IsTwoCarCollision (signature and full body unresolved) | [src/game_core/indep/src/collision.cpp:13](../src/game_core/indep/src/collision.cpp:13) |
| `0x0021EB48` | Collision::GetCollidingCars (signature and full body unresolved) | [src/game_core/indep/src/collision.cpp:18](../src/game_core/indep/src/collision.cpp:18) |
| `0x0021EBD8` | Collision::IsSmokeableInvolved (signature and full body unresolved) | [src/game_core/indep/src/collision.cpp:23](../src/game_core/indep/src/collision.cpp:23) |
| `0x00220690` | Smokeable::Sub_00220690 (signature and full body unresolved) | [src/game_core/indep/src/smokeable.cpp:16](../src/game_core/indep/src/smokeable.cpp:16) |
| `0x002208A0` | Smokeable::DoSnapshot (signature and full body unresolved) | [src/game_core/indep/src/smokeable.cpp:21](../src/game_core/indep/src/smokeable.cpp:21) |
| `0x00220CE8` | Smokeable::Move (signature and full body unresolved) | [src/game_core/indep/src/smokeable.cpp:26](../src/game_core/indep/src/smokeable.cpp:26) |
| `0x002212D0` | Smokeable::MaybeExplodeWarhead (signature and full body unresolved) | [src/game_core/indep/src/smokeable.cpp:31](../src/game_core/indep/src/smokeable.cpp:31) |
| `0x00221340` | Smokeable::ConsiderImpulse (signature and full body unresolved) | [src/game_core/indep/src/smokeable.cpp:36](../src/game_core/indep/src/smokeable.cpp:36) |
| `0x00221C78` | IsBeingPursued service absent | [src/game_core/indep/src/car.cpp:283](../src/game_core/indep/src/car.cpp:283) |
| `0x00221F00` | Smokeable::GetFriction (signature and full body unresolved) | [src/game_core/indep/src/smokeable.cpp:41](../src/game_core/indep/src/smokeable.cpp:41) |
| `0x00221F10` | Smokeable::GetBounciness (signature and full body unresolved) | [src/game_core/indep/src/smokeable.cpp:46](../src/game_core/indep/src/smokeable.cpp:46) |
| `0x00221F20` | Smokeable::GetTerrainType (signature and full body unresolved) | [src/game_core/indep/src/smokeable.cpp:51](../src/game_core/indep/src/smokeable.cpp:51) |
| `0x00221F30` | Smokeable::IsMoveable (signature and full body unresolved) | [src/game_core/indep/src/smokeable.cpp:56](../src/game_core/indep/src/smokeable.cpp:56) |
| `0x00221F58` | Smokeable::GetInertiaTensorInverse (signature and full body unresolved) | [src/game_core/indep/src/smokeable.cpp:61](../src/game_core/indep/src/smokeable.cpp:61) |
| `0x00221F88` | Smokeable::GetMassInverse (signature and full body unresolved) | [src/game_core/indep/src/smokeable.cpp:66](../src/game_core/indep/src/smokeable.cpp:66) |
| `0x00221FB8` | Smokeable::AreCollisionsEnabled (signature and full body unresolved) | [src/game_core/indep/src/smokeable.cpp:71](../src/game_core/indep/src/smokeable.cpp:71) |
| `0x00221FD8` | Smokeable::IsBlinking (signature and full body unresolved) | [src/game_core/indep/src/smokeable.cpp:76](../src/game_core/indep/src/smokeable.cpp:76) |
| `0x00221FF0` | Smokeable::ApplyImpulse (signature and full body unresolved) | [src/game_core/indep/src/smokeable.cpp:81](../src/game_core/indep/src/smokeable.cpp:81) |
| `0x00222010` | Smokeable::GetRotation (signature and full body unresolved) | [src/game_core/indep/src/smokeable.cpp:86](../src/game_core/indep/src/smokeable.cpp:86) |
| `0x00222020` | Smokeable::GetVelocity (signature and full body unresolved) | [src/game_core/indep/src/smokeable.cpp:91](../src/game_core/indep/src/smokeable.cpp:91) |
| `0x00222030` | Smokeable::GetPosition (signature and full body unresolved) | [src/game_core/indep/src/smokeable.cpp:96](../src/game_core/indep/src/smokeable.cpp:96) |
| `0x00222050` | Smokeable::GetSmokeType (signature and full body unresolved) | [src/game_core/indep/src/smokeable.cpp:101](../src/game_core/indep/src/smokeable.cpp:101) |
| `0x002244A0` | tCopManager::GetNextPursuitID (signature and full body unresolved) | [src/game_core/indep/src/copmgr.cpp:13](../src/game_core/indep/src/copmgr.cpp:13) |
| `0x00224F70` | tCopManager::GetCopByCar (signature and full body unresolved) | [src/game_core/indep/src/copmgr.cpp:18](../src/game_core/indep/src/copmgr.cpp:18) |
| `0x002254E8` | tCopManager::GetCurrentCopSpeaker (signature and full body unresolved) | [src/game_core/indep/src/copmgr.cpp:23](../src/game_core/indep/src/copmgr.cpp:23) |
| `0x002255B8` | tCopManager::ReserveCopSpeaker (signature and full body unresolved) | [src/game_core/indep/src/copmgr.cpp:28](../src/game_core/indep/src/copmgr.cpp:28) |
| `0x00225610` | tCopManager::ReleaseCopSpeaker (signature and full body unresolved) | [src/game_core/indep/src/copmgr.cpp:33](../src/game_core/indep/src/copmgr.cpp:33) |
| `0x00225C40` | tCopManager::GetArrestingOfficer (signature and full body unresolved) | [src/game_core/indep/src/copmgr.cpp:38](../src/game_core/indep/src/copmgr.cpp:38) |
| `0x00226238` | tCopManager::GetActiveRoadBlock (signature and full body unresolved) | [src/game_core/indep/src/copmgr.cpp:43](../src/game_core/indep/src/copmgr.cpp:43) |
| `0x00226448` | tCopManager::GetRoadBlockFromLoadedDataIndex (signature and full body unresolved) | [src/game_core/indep/src/copmgr.cpp:48](../src/game_core/indep/src/copmgr.cpp:48) |
| `0x00226968` | tCopManager::GetNumActiveRoadBlocks (signature and full body unresolved) | [src/game_core/indep/src/copmgr.cpp:53](../src/game_core/indep/src/copmgr.cpp:53) |
| `0x00226BB0` | tCopManager::GetSpeedTrapFromLoadedDataIndex (signature and full body unresolved) | [src/game_core/indep/src/copmgr.cpp:58](../src/game_core/indep/src/copmgr.cpp:58) |
| `0x00227438` | tCopManager::IsBeingPursued (signature and full body unresolved) | [src/game_core/indep/src/copmgr.cpp:63](../src/game_core/indep/src/copmgr.cpp:63) |
| `0x002289F8` | tCopManager::ShouldDisplayBustedText (signature and full body unresolved) | [src/game_core/indep/src/copmgr.cpp:68](../src/game_core/indep/src/copmgr.cpp:68) |
| `0x00229ED0` | tPursuit::SelectPrimaryCop (signature and full body unresolved) | [src/game_core/indep/src/pursuit.cpp:20](../src/game_core/indep/src/pursuit.cpp:20) |
| `0x0022BB28` | tPursuit::IsCopNearChasedCar (signature and full body unresolved) | [src/game_core/indep/src/pursuit.cpp:25](../src/game_core/indep/src/pursuit.cpp:25) |
| `0x0022BC08` | tPursuit::IsRacingWithMe (signature and full body unresolved) | [src/game_core/indep/src/pursuit.cpp:30](../src/game_core/indep/src/pursuit.cpp:30) |
| `0x0022C920` | tPursuit::HasThisCop (signature and full body unresolved) | [src/game_core/indep/src/pursuit.cpp:35](../src/game_core/indep/src/pursuit.cpp:35) |
| `0x0022D080` | tNearFollowTarget::GetVelocity (signature and full body unresolved) | [src/game_core/indep/src/pursuit.cpp:13](../src/game_core/indep/src/pursuit.cpp:13) |
| `0x0022ED68` | tAICop::CanPatrollerSeeThisCar (signature and full body unresolved) | [src/game_core/indep/src/cop.cpp:13](../src/game_core/indep/src/cop.cpp:13) |
| `0x00230BF8` | tAICop::IsNearPursuitTarget (signature and full body unresolved) | [src/game_core/indep/src/cop.cpp:18](../src/game_core/indep/src/cop.cpp:18) |
| `0x002319D8` | CopSpeedFillerClass::Speak (signature and full body unresolved) | [src/game_core/indep/src/spacefiller.cpp:37](../src/game_core/indep/src/spacefiller.cpp:37) |
| `0x00231A20` | CopSpeedFillerClass::IsAppropriate (signature and full body unresolved) | [src/game_core/indep/src/spacefiller.cpp:42](../src/game_core/indep/src/spacefiller.cpp:42) |
| `0x00231AA0` | CopSpeedFillerClass::GetBank (signature and full body unresolved) | [src/game_core/indep/src/spacefiller.cpp:47](../src/game_core/indep/src/spacefiller.cpp:47) |
| `0x00231AA8` | CopTacticMiscFillerClass::Speak (signature and full body unresolved) | [src/game_core/indep/src/spacefiller.cpp:54](../src/game_core/indep/src/spacefiller.cpp:54) |
| `0x00231B08` | CopTacticMiscFillerClass::IsAppropriate (signature and full body unresolved) | [src/game_core/indep/src/spacefiller.cpp:59](../src/game_core/indep/src/spacefiller.cpp:59) |
| `0x00231B60` | CopTacticMiscFillerClass::GetBank (signature and full body unresolved) | [src/game_core/indep/src/spacefiller.cpp:64](../src/game_core/indep/src/spacefiller.cpp:64) |
| `0x00231B68` | CopLocationFillerClass::Speak (signature and full body unresolved) | [src/game_core/indep/src/spacefiller.cpp:20](../src/game_core/indep/src/spacefiller.cpp:20) |
| `0x00231BA0` | CopLocationFillerClass::IsAppropriate (signature and full body unresolved) | [src/game_core/indep/src/spacefiller.cpp:25](../src/game_core/indep/src/spacefiller.cpp:25) |
| `0x00231C00` | CopLocationFillerClass::GetBank (signature and full body unresolved) | [src/game_core/indep/src/spacefiller.cpp:30](../src/game_core/indep/src/spacefiller.cpp:30) |
| `0x00231C08` | CopFillerSpeech::GetWeight (signature and full body unresolved) | [src/game_core/indep/src/spacefiller.cpp:13](../src/game_core/indep/src/spacefiller.cpp:13) |
| `0x00232268` | CyclicBuffer::DataAvail (signature and full body unresolved) | [src/game_core/indep/src/movieplayer/cyclicbuffer.cpp:13](../src/game_core/indep/src/movieplayer/cyclicbuffer.cpp:13) |
| `0x00232498` | DCTImage::IDCTRowReference (signature and full body unresolved) | [src/game_core/indep/src/movieplayer/dctblock.cpp:13](../src/game_core/indep/src/movieplayer/dctblock.cpp:13) |
| `0x00234A68` | DCTImage::IDCTZero (signature and full body unresolved) | [src/game_core/indep/src/movieplayer/dctimage.cpp:13](../src/game_core/indep/src/movieplayer/dctimage.cpp:13) |
| `0x00236D90` | DCTTable::ScaleTable (signature and full body unresolved) | [src/game_core/indep/src/movieplayer/dcttable.cpp:13](../src/game_core/indep/src/movieplayer/dcttable.cpp:13) |
| `0x00236DF8` | DCTTable::ConvertQualityToScale (signature and full body unresolved) | [src/game_core/indep/src/movieplayer/dcttable.cpp:18](../src/game_core/indep/src/movieplayer/dcttable.cpp:18) |
| `0x00236EE8` | Huffman::NumBits (signature and full body unresolved) | [src/game_core/indep/src/movieplayer/huffman2.cpp:13](../src/game_core/indep/src/movieplayer/huffman2.cpp:13) |
| `0x00236F18` | Huffman::STORE (signature and full body unresolved) | [src/game_core/indep/src/movieplayer/huffman2.cpp:18](../src/game_core/indep/src/movieplayer/huffman2.cpp:18) |
| `0x00237228` | Huffman::GetBits (signature and full body unresolved) | [src/game_core/indep/src/movieplayer/huffman2.cpp:23](../src/game_core/indep/src/movieplayer/huffman2.cpp:23) |
| `0x00237660` | Huffman::GETBITS (signature and full body unresolved) | [src/game_core/indep/src/movieplayer/huffman2.cpp:28](../src/game_core/indep/src/movieplayer/huffman2.cpp:28) |
| `0x00237760` | non-PCM movie audio | [src/game_core/indep/src/movieplayer/movieplayer.cpp:59](../src/game_core/indep/src/movieplayer/movieplayer.cpp:59) |
| `0x00238E68` | MV3Codec::Sub_00238E68 (signature and full body unresolved) | [src/game_core/indep/src/movieplayer/mv3codec.cpp:13](../src/game_core/indep/src/movieplayer/mv3codec.cpp:13) |
| `0x00238FB0` | MV3Codec::QueryDecompressEx (signature and full body unresolved) | [src/game_core/indep/src/movieplayer/mv3codecdecomp.cpp:13](../src/game_core/indep/src/movieplayer/mv3codecdecomp.cpp:13) |
| `0x00238FB0` | non-MV30 AVI codec | [src/game_core/indep/src/movieplayer/movieplayer.cpp:47](../src/game_core/indep/src/movieplayer/movieplayer.cpp:47) |
| `0x00238FB0` | unsupported MV30 bitmap format | [src/game_core/indep/src/movieplayer/movieplayer.cpp:56](../src/game_core/indep/src/movieplayer/movieplayer.cpp:56) |
| `0x00239050` | MV3Codec::BeginDecompress (signature and full body unresolved) | [src/game_core/indep/src/movieplayer/mv3codecdecomp.cpp:18](../src/game_core/indep/src/movieplayer/mv3codecdecomp.cpp:18) |
| `0x002390B8` | MV3Codec::BeginDecompressEx (signature and full body unresolved) | [src/game_core/indep/src/movieplayer/mv3codecdecomp.cpp:23](../src/game_core/indep/src/movieplayer/mv3codecdecomp.cpp:23) |
| `0x002390E8` | MV3Codec::DecompressFrame (signature and full body unresolved) | [src/game_core/indep/src/movieplayer/mv3codecdecomp.cpp:28](../src/game_core/indep/src/movieplayer/mv3codecdecomp.cpp:28) |
| `0x00239118` | MV3Codec::DecompressFrameEx (signature and full body unresolved) | [src/game_core/indep/src/movieplayer/mv3codecdecomp.cpp:33](../src/game_core/indep/src/movieplayer/mv3codecdecomp.cpp:33) |
| `0x00239148` | host FFmpeg MV30 decoder missing | [src/game_core/indep/src/movieplayer/movieplayer.cpp:96](../src/game_core/indep/src/movieplayer/movieplayer.cpp:96) |
| `0x00239200` | MV3Codec::EndDecompress (signature and full body unresolved) | [src/game_core/indep/src/movieplayer/mv3codecdecomp.cpp:38](../src/game_core/indep/src/movieplayer/mv3codecdecomp.cpp:38) |
| `0x002393A0` | RingBuffer::GetLastFrame (signature and full body unresolved) | [src/game_core/indep/src/movieplayer/ringbuffer.cpp:13](../src/game_core/indep/src/movieplayer/ringbuffer.cpp:13) |
| `0x002395A0` | RingBuffer::ContinuousAvail (signature and full body unresolved) | [src/game_core/indep/src/movieplayer/ringbuffer.cpp:18](../src/game_core/indep/src/movieplayer/ringbuffer.cpp:18) |
| `0x00239A58` | RingBuffer::GetFrame (signature and full body unresolved) | [src/game_core/indep/src/movieplayer/ringbuffer.cpp:23](../src/game_core/indep/src/movieplayer/ringbuffer.cpp:23) |
| `0x00239B48` | RingBuffer::ReleaseFrame (signature and full body unresolved) | [src/game_core/indep/src/movieplayer/ringbuffer.cpp:28](../src/game_core/indep/src/movieplayer/ringbuffer.cpp:28) |
| `0x00239C90` | DrivingLocation::DoesThisRoutePointMatchThisRoute (signature and full body unresolved) | [src/game_core/indep/src/drivinglocation.cpp:13](../src/game_core/indep/src/drivinglocation.cpp:13) |
| `0x0023A070` | TrackedController::Sub_0023A070 (signature and full body unresolved) | [src/game_core/indep/src/trackedcontroller.cpp:47](../src/game_core/indep/src/trackedcontroller.cpp:47) |
| `0x0023A088` | TrackedController::Think (signature and full body unresolved) | [src/game_core/indep/src/trackedcontroller.cpp:52](../src/game_core/indep/src/trackedcontroller.cpp:52) |
| `0x0023A0F0` | TrackedController::DoSnapshot (signature and full body unresolved) | [src/game_core/indep/src/trackedcontroller.cpp:57](../src/game_core/indep/src/trackedcontroller.cpp:57) |
| `0x0023A178` | TrackedController::SetCurrentRoute (signature and full body unresolved) | [src/game_core/indep/src/trackedcontroller.cpp:15](../src/game_core/indep/src/trackedcontroller.cpp:15) |
| `0x0023A198` | TrackedController::GetCurrentRoute (signature and full body unresolved) | [src/game_core/indep/src/trackedcontroller.cpp:20](../src/game_core/indep/src/trackedcontroller.cpp:20) |
| `0x0023A1C8` | TrackedController::FindWhereIAm (signature and full body unresolved) | [src/game_core/indep/src/trackedcontroller.cpp:25](../src/game_core/indep/src/trackedcontroller.cpp:25) |
| `0x0023A2F8` | TrackedController::JustReset (signature and full body unresolved) | [src/game_core/indep/src/trackedcontroller.cpp:30](../src/game_core/indep/src/trackedcontroller.cpp:30) |
| `0x0023A300` | TrackedController::GetAmountOutOfBounds (signature and full body unresolved) | [src/game_core/indep/src/trackedcontroller.cpp:35](../src/game_core/indep/src/trackedcontroller.cpp:35) |
| `0x0023A3D0` | TrackedController::GetCurrentRoutePoint (signature and full body unresolved) | [src/game_core/indep/src/trackedcontroller.cpp:40](../src/game_core/indep/src/trackedcontroller.cpp:40) |
| `0x0023A3D8` | RealController::Sub_0023A3D8 (signature and full body unresolved) | [src/game_core/indep/src/realcontroller.cpp:37](../src/game_core/indep/src/realcontroller.cpp:37) |
| `0x0023A4D8` | RealController::Sub_0023A4D8 (signature and full body unresolved) | [src/game_core/indep/src/realcontroller.cpp:42](../src/game_core/indep/src/realcontroller.cpp:42) |
| `0x0023A560` | RealController::DoSnapshot (signature and full body unresolved) | [src/game_core/indep/src/realcontroller.cpp:48](../src/game_core/indep/src/realcontroller.cpp:48) |
| `0x0023AA50` | RealController::Think (signature and full body unresolved) | [src/game_core/indep/src/realcontroller.cpp:54](../src/game_core/indep/src/realcontroller.cpp:54) |
| `0x0023B0F8` | HudAnimator::Sub_0023B0F8 (signature and full body unresolved) | [src/game_core/indep/src/hudanim.cpp:13](../src/game_core/indep/src/hudanim.cpp:13) |
| `0x0023D368` | QuickSpline::FixupEndpoints (signature and full body unresolved) | [src/game_core/indep/src/quickspline.cpp:60](../src/game_core/indep/src/quickspline.cpp:60) |
| `0x0023D600` | QuickSpline::GenerateExtrapolatedControlPoints (signature and full body unresolved) | [src/game_core/indep/src/quickspline.cpp:65](../src/game_core/indep/src/quickspline.cpp:65) |
| `0x0023D868` | QuickSpline::FindClosestControlPoint (signature and full body unresolved) | [src/game_core/indep/src/quickspline.cpp:70](../src/game_core/indep/src/quickspline.cpp:70) |
| `0x0023F300` | JoystickCalibrationScreen::ResetJoystickCalibration (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/joystickconfiguration.cpp:55](../src/game_core/indep/src/frontendscreens/joystickconfiguration.cpp:55) |
| `0x002400C0` | JoystickPreconfig::Sub_002400C0 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/joystickconfiguration.cpp:31](../src/game_core/indep/src/frontendscreens/joystickconfiguration.cpp:31) |
| `0x00240230` | JoystickPreconfig::Sub_00240230 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/joystickconfiguration.cpp:36](../src/game_core/indep/src/frontendscreens/joystickconfiguration.cpp:36) |
| `0x002402C0` | JoystickPreconfig::Sub_002402C0 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/joystickconfiguration.cpp:41](../src/game_core/indep/src/frontendscreens/joystickconfiguration.cpp:41) |
| `0x002402C0` | controller profile comparison missing | [src/game_core/indep/src/frontendscreens/joystickconfiguration.cpp:16](../src/game_core/indep/src/frontendscreens/joystickconfiguration.cpp:16) |
| `0x00241960` | tSpikeStrip::GetRenderOrigin (signature and full body unresolved) | [src/game_core/indep/src/spikestrip.cpp:13](../src/game_core/indep/src/spikestrip.cpp:13) |
| `0x00241CF8` | DrivingChoice::SetChoice (signature and full body unresolved) | [src/game_core/indep/src/drivingchoice.cpp:33](../src/game_core/indep/src/drivingchoice.cpp:33) |
| `0x00243988` | DrivingChoice::ReduceCurrentL (signature and full body unresolved) | [src/game_core/indep/src/drivingchoice.cpp:38](../src/game_core/indep/src/drivingchoice.cpp:38) |
| `0x00244768` | FuturePoints::AddPoint (signature and full body unresolved) | [src/game_core/indep/src/drivingchoice.cpp:45](../src/game_core/indep/src/drivingchoice.cpp:45) |
| `0x002452B0` | FuturePoints::Dump (signature and full body unresolved) | [src/game_core/indep/src/drivingchoice.cpp:50](../src/game_core/indep/src/drivingchoice.cpp:50) |
| `0x002455B0` | TrackRoutePoint::GetAmountOutOfBounds (signature and full body unresolved) | [src/game_core/indep/src/drivingchoice.cpp:59](../src/game_core/indep/src/drivingchoice.cpp:59) |
| `0x00246BD8` | MemcardLoader::Sub_00246BD8 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/memcardscreens.cpp:17](../src/game_core/indep/src/frontendscreens/memcardscreens.cpp:17) |
| `0x00247530` | load/save operation Back polling dependencies missing | [src/game_core/indep/src/frontendscreens/options.cpp:264](../src/game_core/indep/src/frontendscreens/options.cpp:264) |
| `0x00248568` | AnimationGroup::Sub_00248568 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/feanim.cpp:43](../src/game_core/indep/src/frontendscreens/feanim.cpp:43) |
| `0x002486E8` | AnimationGroup::SetupToAnimate (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/feanim.cpp:36](../src/game_core/indep/src/frontendscreens/feanim.cpp:36) |
| `0x00248BE8` | tSlideAnimation::Sub_00248BE8 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/feanim.cpp:134](../src/game_core/indep/src/frontendscreens/feanim.cpp:134) |
| `0x002490D8` | tExitScreenAnimation::Sub_002490D8 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/feanim.cpp:59](../src/game_core/indep/src/frontendscreens/feanim.cpp:59) |
| `0x002491B0` | screen scrolling-text attachment not recovered | [src/port/runtime/frontend_runtime.cpp:869](../src/port/runtime/frontend_runtime.cpp:869) |
| `0x002491B0` | tAnimatedBlurb::Sub_002491B0 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/feanim.cpp:52](../src/game_core/indep/src/frontendscreens/feanim.cpp:52) |
| `0x00249348` | scrolling-text update requires the bound tAnimatedBlurb screen state | [src/game_core/indep/src/fmenus.cpp:1100](../src/game_core/indep/src/fmenus.cpp:1100) |
| `0x002495F0` | tGlobalBackgroundAnimation::Sub_002495F0 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/feanim.cpp:66](../src/game_core/indep/src/frontendscreens/feanim.cpp:66) |
| `0x00249790` | tGlobalBackgroundAnimation::RemoveTextureHash (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/feanim.cpp:71](../src/game_core/indep/src/frontendscreens/feanim.cpp:71) |
| `0x002497B8` | tGlobalBackgroundAnimation::GetNumTextures (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/feanim.cpp:76](../src/game_core/indep/src/frontendscreens/feanim.cpp:76) |
| `0x002497F0` | tGlobalBackgroundAnimation::GetNthTexture (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/feanim.cpp:81](../src/game_core/indep/src/frontendscreens/feanim.cpp:81) |
| `0x00249C78` | tMenuAnimElement::Sub_00249C78 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/feanim.cpp:95](../src/game_core/indep/src/frontendscreens/feanim.cpp:95) |
| `0x00249D70` | tMenuAnimElement::ApplyDeltaY (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/feanim.cpp:100](../src/game_core/indep/src/frontendscreens/feanim.cpp:100) |
| `0x00249DE0` | tScrollingMenuList::Sub_00249DE0 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/feanim.cpp:107](../src/game_core/indep/src/frontendscreens/feanim.cpp:107) |
| `0x00249FF8` | scrolling screen not bound | [src/game_core/indep/src/fmenus.cpp:982](../src/game_core/indep/src/fmenus.cpp:982) |
| `0x0024A050` | tScrollingMenuList::FindMenuElement (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/feanim.cpp:112](../src/game_core/indep/src/frontendscreens/feanim.cpp:112) |
| `0x0024A090` | selected widget not in scrolling list | [src/game_core/indep/src/fmenus.cpp:1016](../src/game_core/indep/src/fmenus.cpp:1016) |
| `0x0024A248` | tScrollingMenuList::Highlight (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/feanim.cpp:117](../src/game_core/indep/src/frontendscreens/feanim.cpp:117) |
| `0x0024A340` | Original performance-counter service is absent | [src/game_core/indep/src/componentphysics.cpp:36](../src/game_core/indep/src/componentphysics.cpp:36) |
| `0x0024A350` | Original performance-counter delta service is absent | [src/game_core/indep/src/componentphysics.cpp:44](../src/game_core/indep/src/componentphysics.cpp:44) |
| `0x0024A368` | uninitialized viewport slot | [src/game_core/indep/src/fmenus.cpp:1062](../src/game_core/indep/src/fmenus.cpp:1062) |
| `0x0024A3E0` | tScrollingMenuList::ShowUpArrow (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/feanim.cpp:122](../src/game_core/indep/src/frontendscreens/feanim.cpp:122) |
| `0x0024A430` | tScrollingMenuList::ShowDownArrow (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/feanim.cpp:127](../src/game_core/indep/src/frontendscreens/feanim.cpp:127) |
| `0x0024A690` | tLeftRightArrowScroll::Sub_0024A690 (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/feanim.cpp:88](../src/game_core/indep/src/frontendscreens/feanim.cpp:88) |
| `0x0024D048` | tKeyboard::PlayerRight (signature and full body unresolved) | [src/game_core/indep/src/frontendscreens/nameentry.cpp:17](../src/game_core/indep/src/frontendscreens/nameentry.cpp:17) |
| `0x0024D870` | NameEntry keyboard state missing | [src/game_core/indep/src/frontendscreens/options.cpp:199](../src/game_core/indep/src/frontendscreens/options.cpp:199) |
| `0x0024E4D0` | tTrackGraph::GetVertexNumber (signature and full body unresolved) | [src/game_core/indep/src/trackgraph.cpp:13](../src/game_core/indep/src/trackgraph.cpp:13) |
| `0x002500B0` | tWordQueue::Store (signature and full body unresolved) | [src/game_core/indep/src/roadblock.cpp:13](../src/game_core/indep/src/roadblock.cpp:13) |
| `0x00250100` | tWordQueue::Remove (signature and full body unresolved) | [src/game_core/indep/src/roadblock.cpp:18](../src/game_core/indep/src/roadblock.cpp:18) |
| `0x002503F8` | bList::GetNode (signature and full body unresolved) | [src/game_core/src/bList.cpp:13](../src/game_core/src/bList.cpp:13) |
| `0x00250428` | bList::TraversebList (signature and full body unresolved) | [src/game_core/src/bList.cpp:18](../src/game_core/src/bList.cpp:18) |
| `0x00250458` | bList::AddTail (signature and full body unresolved) | [src/game_core/src/bList.cpp:23](../src/game_core/src/bList.cpp:23) |
| `0x00251398` | event localized format conversion not implemented | [src/game_core/indep/src/eventtree.cpp:63](../src/game_core/indep/src/eventtree.cpp:63) |
| `0x002537AC` | Original divide-by-zero BREAK7 | [src/game_core/src/bMath.cpp:17](../src/game_core/src/bMath.cpp:17) |
| `0x002538B8` | AsyncEntry::Sub_002538B8 (signature and full body unresolved) | [src/game_core/src/bFile.cpp:28](../src/game_core/src/bFile.cpp:28) |
| `0x00253A40` | AsyncChunkPri::Sub_00253A40 (signature and full body unresolved) | [src/game_core/src/bFile.cpp:21](../src/game_core/src/bFile.cpp:21) |
| `0x002572D8` | MemoryPool::GetAmountFree (signature and full body unresolved) | [src/game_core/src/bMemory.cpp:13](../src/game_core/src/bMemory.cpp:13) |
| `0x00257310` | MemoryPool::GetLargestFreeBlock (signature and full body unresolved) | [src/game_core/src/bMemory.cpp:18](../src/game_core/src/bMemory.cpp:18) |
| `0x00257DF8` | SlotPool::FlushSlotPool (signature and full body unresolved) | [src/game_core/src/bSlotPool.cpp:47](../src/game_core/src/bSlotPool.cpp:47) |
| `0x00257F28` | SlotPool::CleanupExpandedSlotPools (signature and full body unresolved) | [src/game_core/src/bSlotPool.cpp:16](../src/game_core/src/bSlotPool.cpp:16) |
| `0x00258200` | SlotPool::FastMalloc (signature and full body unresolved) | [src/game_core/src/bSlotPool.cpp:52](../src/game_core/src/bSlotPool.cpp:52) |
| `0x002584B0` | SlotPoolManager::NewSlotPool (signature and full body unresolved) | [src/game_core/src/bSlotPool.cpp:35](../src/game_core/src/bSlotPool.cpp:35) |
| `0x00258548` | SlotPoolManager::DeleteSlotPool (signature and full body unresolved) | [src/game_core/src/bSlotPool.cpp:40](../src/game_core/src/bSlotPool.cpp:40) |
| `0x0025A3D0` | original zero-bound trap | [src/game_core/indep/src/fmenus.cpp:1163](../src/game_core/indep/src/fmenus.cpp:1163) |
| `0x0025A488` | missing shared RNG input | [src/game_core/indep/src/fmenus.cpp:1196](../src/game_core/indep/src/fmenus.cpp:1196) |
| `0x00266FB0` | E-0016-A02: non-VAG sample chunk | [src/game_core/indep/src/sound.cpp:101](../src/game_core/indep/src/sound.cpp:101) |
| `0x00266FB0` | E-0016-A02: sound sample chunk variant | [src/game_core/indep/src/sound.cpp:97](../src/game_core/indep/src/sound.cpp:97) |
| `0x00266FB0` | unobserved VAGx channel layout | [src/game_core/indep/src/sound.cpp:36](../src/game_core/indep/src/sound.cpp:36) |
| `0x002705C0` | bEffect::SetParameter (signature and full body unresolved) | [src/game_core/src/bEffect.cpp:13](../src/game_core/src/bEffect.cpp:13) |
| `0x002719B8` | E-0016-A01: requested bSound event absent in Guest asset bank | [src/game_core/indep/src/sound.cpp:172](../src/game_core/indep/src/sound.cpp:172) |
| `0x00273280` | bSample::Initialize (signature and full body unresolved) | [src/game_core/src/bSample.cpp:13](../src/game_core/src/bSample.cpp:13) |
| `0x002735A8` | bSample::GetName (signature and full body unresolved) | [src/game_core/src/bSample.cpp:18](../src/game_core/src/bSample.cpp:18) |
| `0x002735E0` | bSample::GetBankName (signature and full body unresolved) | [src/game_core/src/bSample.cpp:23](../src/game_core/src/bSample.cpp:23) |
| `0x00273618` | bSample::SetCategory (signature and full body unresolved) | [src/game_core/src/bSample.cpp:28](../src/game_core/src/bSample.cpp:28) |
| `0x00275B70` | bSample::GetType (signature and full body unresolved) | [src/game_core/src/bSample.cpp:33](../src/game_core/src/bSample.cpp:33) |
| `0x00275BA8` | bSample::IsVoiceLockedPlaying (signature and full body unresolved) | [src/game_core/src/bSample.cpp:38](../src/game_core/src/bSample.cpp:38) |
| `0x00275BE0` | bSample::IsVoiceLocked (signature and full body unresolved) | [src/game_core/src/bSample.cpp:43](../src/game_core/src/bSample.cpp:43) |
| `0x00275C18` | bSample::ReleaseVoice (signature and full body unresolved) | [src/game_core/src/bSample.cpp:48](../src/game_core/src/bSample.cpp:48) |
| `0x00275D40` | bSample::GetFlags (signature and full body unresolved) | [src/game_core/src/bSample.cpp:53](../src/game_core/src/bSample.cpp:53) |
| `0x00275D70` | bSample::GetPitch (signature and full body unresolved) | [src/game_core/src/bSample.cpp:58](../src/game_core/src/bSample.cpp:58) |
| `0x00275DA8` | bSample::GetPan (signature and full body unresolved) | [src/game_core/src/bSample.cpp:63](../src/game_core/src/bSample.cpp:63) |
| `0x00275DE0` | bSample::GetSampleVolume (signature and full body unresolved) | [src/game_core/src/bSample.cpp:68](../src/game_core/src/bSample.cpp:68) |
| `0x00275E50` | bSample::GetCallback (signature and full body unresolved) | [src/game_core/src/bSample.cpp:73](../src/game_core/src/bSample.cpp:73) |
| `0x00275E88` | bSample::SetCallback (signature and full body unresolved) | [src/game_core/src/bSample.cpp:78](../src/game_core/src/bSample.cpp:78) |
| `0x00275EB8` | bSample::GetCategory (signature and full body unresolved) | [src/game_core/src/bSample.cpp:83](../src/game_core/src/bSample.cpp:83) |
| `0x00275EF0` | bSample::IsPaused (signature and full body unresolved) | [src/game_core/src/bSample.cpp:88](../src/game_core/src/bSample.cpp:88) |
| `0x00276500` | bSegment::GetOffset (signature and full body unresolved) | [src/game_core/src/bSegment.cpp:13](../src/game_core/src/bSegment.cpp:13) |
| `0x00276538` | bSegment::Sub_00276538 (signature and full body unresolved) | [src/game_core/src/bSegment.cpp:18](../src/game_core/src/bSegment.cpp:18) |
| `0x00276570` | bSegment::Sub_00276570 (signature and full body unresolved) | [src/game_core/src/bSegment.cpp:23](../src/game_core/src/bSegment.cpp:23) |
| `0x002765B0` | bSegment::GetDataSize (signature and full body unresolved) | [src/game_core/src/bSegment.cpp:28](../src/game_core/src/bSegment.cpp:28) |
| `0x002765E8` | bSegment::GetHeaderSize (signature and full body unresolved) | [src/game_core/src/bSegment.cpp:33](../src/game_core/src/bSegment.cpp:33) |
| `0x00277DC0` | bSound::GetCategoryVolume (signature and full body unresolved) | [src/game_core/src/bSound.cpp:13](../src/game_core/src/bSound.cpp:13) |
| `0x00277EA0` | bSound::GetStatus (signature and full body unresolved) | [src/game_core/src/bSound.cpp:18](../src/game_core/src/bSound.cpp:18) |
| `0x00278320` | bSound::EnablePrintf (signature and full body unresolved) | [src/game_core/src/bSound.cpp:23](../src/game_core/src/bSound.cpp:23) |
| `0x00278368` | bSound::DisablePrintf (signature and full body unresolved) | [src/game_core/src/bSound.cpp:28](../src/game_core/src/bSound.cpp:28) |
| `0x0027A6D8` | bSound::getPendingRegion (signature and full body unresolved) | [src/game_core/src/bSound.cpp:33](../src/game_core/src/bSound.cpp:33) |
| `0x0027A720` | bSound::getPendingRegionsCount (signature and full body unresolved) | [src/game_core/src/bSound.cpp:38](../src/game_core/src/bSound.cpp:38) |
| `0x0027C5C0` | bVoice::Release (signature and full body unresolved) | [src/game_core/src/bVoice.cpp:13](../src/game_core/src/bVoice.cpp:13) |
| `0x0027C5E8` | bVoice::Set (signature and full body unresolved) | [src/game_core/src/bVoice.cpp:18](../src/game_core/src/bVoice.cpp:18) |
| `0x0027C6F8` | bVoice::Sub_0027C6F8 (signature and full body unresolved) | [src/game_core/src/bVoice.cpp:23](../src/game_core/src/bVoice.cpp:23) |
| `0x0027CA00` | jukebox synchronous prelude not implemented | [src/game_core/bSound/src/bJukebox.cpp:64](../src/game_core/bSound/src/bJukebox.cpp:64) |
| `0x0027D6A8` | Jukebox preview category/song operands missing | [src/game_core/indep/src/frontendscreens/options.cpp:226](../src/game_core/indep/src/frontendscreens/options.cpp:226) |
| `0x0027DAC8` | unobserved jukebox cartridge format | [src/game_core/bSound/src/bJukebox.cpp:34](../src/game_core/bSound/src/bJukebox.cpp:34) |
| `0x002800E0` | cJukeboxPlayer::IsDonePlaying (signature and full body unresolved) | [src/game_core/src/bJukebox.cpp:31](../src/game_core/src/bJukebox.cpp:31) |
| `0x00280130` | cJukeboxPlayer::IsDoneFading (signature and full body unresolved) | [src/game_core/src/bJukebox.cpp:36](../src/game_core/src/bJukebox.cpp:36) |
| `0x00280260` | cJukeboxPlayer::GetSampleRate (signature and full body unresolved) | [src/game_core/src/bJukebox.cpp:41](../src/game_core/src/bJukebox.cpp:41) |
| `0x00280298` | bSound::GetPath (signature and full body unresolved) | [src/game_core/src/bJukebox.cpp:22](../src/game_core/src/bJukebox.cpp:22) |
| `0x00280340` | bSampleStream::SetSampleRate (signature and full body unresolved) | [src/game_core/src/bJukebox.cpp:13](../src/game_core/src/bJukebox.cpp:13) |
| `0x002A0550` | CopDriver::Sub_002A0550 (signature and full body unresolved) | [src/game_core/port/police/cop_control.cpp:202](../src/game_core/port/police/cop_control.cpp:202) |
| `0x002A0550` | NullController::Sub_002A0550 (signature and full body unresolved) | [src/game_core/indep/src/car.cpp:108](../src/game_core/indep/src/car.cpp:108) |
| `0x002A0550` | PsychoDriver::Sub_002A0550 (signature and full body unresolved) | [src/game_core/indep/src/psychodriver.cpp:148](../src/game_core/indep/src/psychodriver.cpp:148) |
| `0x002A0550` | RealDriver::Sub_002A0550 (signature and full body unresolved) | [src/game_core/indep/src/realdriver.cpp:237](../src/game_core/indep/src/realdriver.cpp:237) |
| `0x002A0550` | kAITireSound::Sub_002A0550 (signature and full body unresolved) | [src/game_core/indep/src/carsoundtires.cpp:93](../src/game_core/indep/src/carsoundtires.cpp:93) |
| `0x002A7860` | Track GPU depth format outside original normal targets | [src/game_core/port/track/effects.hpp:47](../src/game_core/port/track/effects.hpp:47) |
| `0x002A7860` | Track GS depth format outside original normal targets | [src/game_core/port/track/effects.hpp:39](../src/game_core/port/track/effects.hpp:39) |
| `0x002A8308` | Viewer VU flag0x40 material route | [src/platform/godot_bridge/ps2_vehicle_renderer.cpp:276](../src/platform/godot_bridge/ps2_vehicle_renderer.cpp:276) |
| `0x0032C9F0` | Debug global operand has not been audited | [src/game_core/port/driving/debug_globals.hpp:161](../src/game_core/port/driving/debug_globals.hpp:161) |
| `LANGUAGES/FE_*.BIN` | localised string table format not parsed | [src/game_core/indep/src/fmenus.cpp:728](../src/game_core/indep/src/fmenus.cpp:728) |

</details>

<!-- generated:boundaries:end -->

## Current focus and next action

| | |
|---|---|
| **Focus** | TASK-0050 checks all-track/car unit consistency and corrects outside camera projection to original horizontal bAngle0x32DC; geometry and physics scale are preserved. See TRACK_VIEWER.md and reverse/task0050. |
| **Last session** | Elise visual/CV/wheel units and actual Godot unit scales agree. All30-track ordinary sign bounds audit passed1388 instances. Actual4:3/16:9/portrait projection checks and driving-return restoration passed. Original follow camera remains tool behavior, without matched original-frame certification. |
| **Exact next action** | Original numerical certification remains in TASK-0038/0023: boot the exact Debug ELF in a known runnable scene, establish PINE/MCP connectivity and owned Move00137D18/component hooks, build a Debug oracle map, then compare equal initial state/input/time/material/contact. Do not reuse Retail hooks. |
| **Tooling note** | Read-only Ghidra client: `from tools.ghidra import Client`, service `http://127.0.0.1:8080/mcp`. Select `NFSHP2.ELF` explicitly for driving; `SLUS_203.62` remains the unrelated Retail target. Check live identity/availability each session. No database mutations. |

## Blockers

TASK-0039 current frontend work: read-only Ghidra MCP recovered and corrected event-number
centering, font height/pairs, text alignment/icon/fixed-cell paths, quad corner colours/UVs and
clip submission. All41 authored documents were render-smoked; original equivalence remains
unverified. See [FRONTEND_RENDER_AUDIT.md](FRONTEND_RENDER_AUDIT.md) and
[TASK-0039](../tasks/active/TASK-0039-frontend-rendering.md). Exact next action: original matched
Event1/3/Options captures, FCR31/sampling comparison, then special/rotation draw dependencies.
Historical CarViewer parse/shared-build blockers below no longer reproduce: TASK-0045 built the current extension and passed its host checks. Original frontend numerical parity remains separate.
TASK-0039 validation: native48/48 and final focused 4/4 passed; earlier41-document render smoke
and routed event3/Options captures inspected. Final Godot rerun was blocked by an external
concurrent cleanup/rebuild of shared outputs. Let it finish, then build and run frontend host
checks sequentially. Full rendering parity remains active; no Verified promotion or commit.

Ordered by how much they hold up. Each is a real dependency, not a decision waiting to be made.

| # | Blocker | Blocks | Tracked in |
|---|---|---|---|
| 1 | No synchronized native-versus-original comparison oracle. TASK-0045 PINE probe timed out with no runnable Debug scene; prior TASK-0038 observations are not same-state/time/contact numerical comparisons. Nothing is Verified. | every equivalence claim in the repository | [TASK-0023](../tasks/active/TASK-0023-pcsx2-comparison.md), [TASK-0004](../tasks/backlog/TASK-0004-original-comparison-setup.md) |
| 2 | `00183830`, the full profile and game reset, is untranslated. | `Language Options Pre FE` activation and Back | [TASK-0025](../tasks/active/TASK-0025-language-options-screen.md), [TASK-0021](../tasks/active/TASK-0021-memorycard-profile.md) |
| 3 | `001EE6C0`, the display-settings snapshot, is untranslated. | entering Display Options at all | [TASK-0017](../tasks/active/TASK-0017-submenu-back-overflow.md) |
| 4 | Event selection and reward preview (`001F6A28`, `001F6440`) need race, car and model dependencies that do not exist yet. | choosing an event from the tree | [TASK-0022](../tasks/active/TASK-0022-eventtree-completion.md) |
| 5 | Whole-binary recovery covers a small fraction of the ELF, and the structure records are thinner still. | everything outside the front end | [TASK-0009](../tasks/active/TASK-0009-full-recovery.md) |
| 6 | Classes built through the unlabelled allocator `00257690` have no recoverable name, and non-virtual members cannot be attributed to a class at all. | naming the rest of the classes above | [TASK-0027](../tasks/active/TASK-0027-class-recovery.md) |

## Validation snapshot

TASK-0038 accepted third-person redesign: Godot Debug54/54 passed(52.01s), affected
native 4/4 passed(13.79s), real owned-window Win32 input22/22 passed. Test drive starts
from a rear camera with captured mouse/WASD; only Return/hints remain. F8/focus resumes
automatically; errors are terminal and explicit. Camera/input changes are port-side,
native algorithms unchanged. See TASK-0038 and DRIVING_LAB.md for current behavior.

TASK-0038 WASD follow-up: Test drive starts immediately; keyboard gating distinguishes
actual keyboard capture from mouse hover. Normal Godot input events now verify W movement,
A/D steering, S braking and release clearing over ImGui. Affected host 4/4 passed(5.89s),
windowed native driving/replay smoke passed. Physics unchanged; details in TASK-0038.

TASK-0038 user corrections: affected native 4/4 passed(16.46s), Godot Debug full53/53
passed(55.52s). One CarViewer scene/model/skin/camera, Test drive/Return to preview ImGui
clicks, no new simulation shortcuts, original steering sign, shared original wheel basis,
540-step native drive/replay and preview/camera restoration are covered. Windowed preview/
rest/drive captures inspected. No original numerical equivalence. Console-off/Release/GPU
numeric oracle not repeated in this correction; prior checks are in TASK-0038.

Last host-adapter run: TASK-0036. No original-output equivalence established.

| Check | Command | Result |
|---|---|---|
| Native build/tests | `cmake --build --preset native-debug -j6`; `ctest --preset native-debug` | **36/36 passed** |
| Godot Debug with GPU oracle | console ON, GPU tests ON; build and full CTest (TASK-0036) | **41/41 passed** |
| Console-off Debug | last run TASK-0035; not repeated for host-only TASK-0036 | **4/4 passed** (prior scope) |
| Release | build and full CTest (TASK-0036) | **40/40 passed**, plus common material/reflection GPU oracle |
| Vehicle visuals | ELISE complete batched upload; windowed62-ID batch regression | inspected; ignored local/task0036 captures |
| Generated tables | `py -3 reverse/scripts/generate_progress.py --check` | passed |
| Original comparison | none | **not performed** |

Toolchain: CMake3.29.2, Ninja1.12.0, GCC13.2.0 (Strawberry), Python3.14.4, Godot4.4.1.
Default delivery is Debug console ON / GPU test option OFF. Debug and Release share the
extension output path; force relink before switching configuration tests. See TASK-0035.
Do not assume the previous machine's environment; check availability before relying on it
(`AGENTS.md` startup protocol step 4).

## Where the detail lives

Rules and process:

| Document | What it holds |
|---|---|
| [../AGENT.md](../AGENT.md) | **Binding** naming and evidence rules. Stricter than `AGENTS.md` wherever they overlap. |
| [../AGENTS.md](../AGENTS.md) | Session contract: startup and completion protocol. |
| [DECISIONS.md](DECISIONS.md) | Architecture decisions, D001 onward, with the reasoning kept. |
| [DEVELOPMENT.md](DEVELOPMENT.md) | How to build, run and test. |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Module boundaries and what may depend on what. |
| [ROADMAP.md](ROADMAP.md) | Milestones beyond the active tasks. |

Recovery knowledge, by subsystem:

| Document | What it holds |
|---|---|
| [REVERSE_NOTES.md](REVERSE_NOTES.md) | Cross-cutting findings and method notes. |
| [CLASS_RECOVERY.md](CLASS_RECOVERY.md) | How the class names and member functions in the table above were recovered, and what that method cannot see. |
| [ORIGINAL_SOURCE_TREE.md](ORIGINAL_SOURCE_TREE.md) | The original source layout recovered from `__FILE__` strings, and what is *not* pinned to a file. |
| [MAIN_LOOP_STRUCTURE.md](MAIN_LOOP_STRUCTURE.md) | The main loop and the `GAMEFLOW_STATE_*` dispatcher. |
| [SAVE_SYSTEM.md](SAVE_SYSTEM.md) | `memorycard.cpp`, `settings.db` and the global player profile. |
| [EVENT_TREE.md](EVENT_TREE.md) | The event tree, its requirement table and its open questions. |
| [FRONTEND_MENU.md](FRONTEND_MENU.md) | The menu framework: screens, lists, items, messages. |
| [FRONTEND_MENU_INTERACTION.md](FRONTEND_MENU_INTERACTION.md) | Navigation, selection and the sound events. |
| [FRONTEND_MENU_LANGUAGE.md](FRONTEND_MENU_LANGUAGE.md) | The Language Options screen, start to finish. |
| [FRONTEND_BACK.md](FRONTEND_BACK.md) | Per-screen Back (`0x11`) behaviour across the whole family. |
| [FRONTEND_LAYOUT.md](FRONTEND_LAYOUT.md) | Authored layout, widgets and bitmap fonts. |
| [FRONTEND_MEDIA.md](FRONTEND_MEDIA.md) | Startup media, movie player and audio. |
| [FRONTEND_MODE_SELECT.md](FRONTEND_MODE_SELECT.md) | The Cops On / Cops Off mode selects. |
| [FRONTEND_SCROLL_BACKGROUND.md](FRONTEND_SCROLL_BACKGROUND.md) | Scrolling text and background animation. |
| [FRONTEND_VIEWPORT.md](FRONTEND_VIEWPORT.md) | Overflow clipping and the viewport. |
| [ASSET_PIPELINE.md](ASSET_PIPELINE.md) | The eLab container and TPK texture formats. |
| [SOUND_RECOVERY.md](SOUND_RECOVERY.md) | `bSound`, the jukebox and the sample banks. |
| [POLICE_AI_MAP.md](POLICE_AI_MAP.md) | The police AI call map. |
| [FUNCTION_NAMING.md](FUNCTION_NAMING.md), [FUNCTION_NAME_RECOVERY.md](FUNCTION_NAME_RECOVERY.md), [NAMING_AUDIT.md](NAMING_AUDIT.md) | Identifier recovery and the audits against AGENT.md R2. |

Machine-readable records â€” these are the source of the tables above, and the thing to update
when a finding changes (AGENT.md R6):

| Path | What it holds |
|---|---|
| `reverse/symbols.csv` | One row per function with a record: address, status, confidence, evidence IDs. |
| `reverse/classes.csv` | One row per recovered class: name, object size, constructor, vtable, member functions, per-class progress. |
| `reverse/classes/` | The raw passes behind it: allocator call sites, confirmed vtables, name bindings. |
| `reverse/functions/<binary-id>/<subsystem>/<address>.md` | Per-function findings, hypotheses and open questions, kept separate. |
| `reverse/structures/<binary-id>/` | Per-structure layout records. |
| `reverse/binaries.csv` | Binary identity: SHA-256, region, version provenance. |
| `docs/assessment/FUNCTION_INVENTORY.csv` | Every function in the ELF â€” the denominator above. |
| `tasks/{active,backlog,completed}/` | The task files. `AGENTS.md`'s startup protocol selects from these; they are not summarised here. |

## House rules that keep catching people out

Four that are easy to violate by accident, each one the subject of a real correction in the log:

1. **An unimplemented recovered path throws.** `hp2::not_implemented(elf_address, what)` â€” never
   a default return, a plausible approximation or a silent no-op. A silent fallback is
   indistinguishable from recovered behaviour at runtime, which is the exact failure these rules
   exist to prevent (AGENT.md R3).
2. **Identifiers come from the binary or carry its address.** Recovered names verbatim,
   otherwise `Sub_<ADDR>` / `Type_<ADDR>` / `Field_0x<off>`, otherwise `port::`-namespaced and
   documented as port-side. Never an invented descriptive English name for recovered code
   (AGENT.md R2, with the known debt listed in R8).
3. **Unknown is a required answer.** Say so where a reader would otherwise assume recovery
   happened. A plausible reconstruction is a hypothesis and must be labelled one (AGENT.md R1).
4. **Ghidra access is read-only, and no proprietary material enters Git.** No annotations or
   renames in the database; no ELF, ISO, asset, save file or Ghidra project in the repository
   (AGENT.md R5, R7).

TASK-0031 debug tooling is complete. To recheck it: `cmake --preset godot-debug
-DHP2_ENABLE_DEBUG_CONSOLE=ON`, build that preset, then `ctest --preset godot-debug`.
Run the asset-free `res://scenes/debug_console_smoke.tscn` for a window check.
Build Godot configurations sequentially because they share the extension output path.

TASK-0032 is complete: open F8, run `carviewer` or `trackviewer`; `viewer.close` returns
to a fresh frontend. TASK-0048 now provides original track loading through the TrackViewer panel. See DEBUG_VIEWERS.md and tasks/completed/TASK-0032-debug-viewers.md. Recheck
with `ctest --preset godot-debug -R 'debug_viewers_host|debug_console_host'`.


TASK-0040 global naming audit: 562 source address identifiers checked; 357 have
existing class attribution, but no new exact method spelling. 169 addresses lack
a Ghidra symbol object. See the task0040 global-symbol-provenance record and ledger
in reverse/naming. Reproduce with `py -3 reverse/scripts/audit_symbol_provenance.py --live`
using the configured SHA-bound ELF and read-only Ghidra connection. Further name
recovery requires an actual diagnostic/allocator/RTTI naming bridge; no behavior
or verification status changed.


TASK-0041 reference naming: 940 retail addresses have supported names; the reference
catalog contains 8715 functions and 1284 named aggregate types. All 11 former Type_
class/struct declarations have reviewed names or an explicit two-class split.
The original 562-address source audit retains 175 candidate-only and 196 ambiguous/
unresolved entries. Read DEBUG_REFERENCE_SYMBOLS.md and task0041-source-coverage.csv.
Next: review those candidate rows using named caller/table relationships and exact
instruction differences; preserve reference/retail addresses and layout differences.
Reproduce using the commands in DEBUG_REFERENCE_SYMBOLS.md and the ignored SHA-bound
Ghidra cache. This is naming recovery, not additional implemented or Verified behavior.


TASK-0042 continues the historical TASK-0041 baseline: all 371 remaining rows
were revisited; 64 candidate and 21 unresolved names gained independent evidence.
See [DEBUG_CALLCHAIN_RECOVERY.md](DEBUG_CALLCHAIN_RECOVERY.md), TASK-0042 ledgers
and the active task for source layout changes, validation and remaining bounds.
Names do not promote behavior status. Next: independently anchor the remaining
111 candidates and 175 unresolved rows; preserve folded ambiguity until caller
context disambiguates it.


TASK-0043 completes the supported class-method mapping/template pass: 822 rows
accounted for, 650 missing whole-entry gaps added, 372 classes/1,203 attributions.
42 callable identifiers, EventInfo and 11 fields use confirmed names; 4,999
unmapped Debug methods stay in a per-class catalogue. See
[CLASS_MAPPING_COMPLETION.md](CLASS_MAPPING_COMPLETION.md). Remaining names
require independent entity/field evidence; the extracted TitleStringId helper
retains its address identifier. No new recovered behavior or verification claim.


TASK-0044 uses the user-authorized Debug binary for the supported driving closure. All 151
legacy projection entries are audited, 207 Debug function records preserve version-specific
addresses, and Test drive calls the canonical original PhysicsCar/component implementations.
See [DEBUG_DRIVING_AUDIT.md](DEBUG_DRIVING_AUDIT.md). Native Debug-image trajectories and
23 real Windows interactions passed. Original Debug numerical certification remains blocked
by PINE/MCP probe timeout/no runnable matching scenario; no Verified promotion. Next action:
connect the exact Debug game in a running driving scene, implement the Debug oracle address
map, then capture equal-state/input/time/contact component traces at Move 00137D18. Do not
reuse Retail oracle hooks or denominator statistics for Debug records.


TASK-0045 adds five original-data materials and per-wheel synthetic zones to Test drive,
with a collapsed read-only grip panel. All33 terrain rows and31 bounded functions were
audited; five additional Debug records are Decompiled, no Verified promotion. The source
review chain retains frozen TASK0044 pins and separately reviews11-before/17-after port
integration changes plus the Debug VRSQRT diagnostic anchor. Native material/zone/replay
checks and31 actual Windows interactions passed; original PINE comparison remains blocked
by timeout/no runnable Debug state. See [DRIVING_GRIP.md](DRIVING_GRIP.md). Next certification
action: same-state/input/time/contact Debug traces at Move00137D18 and Wheel entries.

TASK-0046 adds original RealDriver player control, manual/automatic shifting, neutral,
reverse, handbrake and original skid generation/render commands to the single chain.
Godot driving bindings now live in InputMap plus one semantic adapter. Actual-state
HUD and original textures are exercised by69 real OS checks and visual inspection.
Full Godot regression75/76 plus corrected historical count check1/1 passed; native
targets and required structural checks passed. Strict25-before/81-after source review
retains TASK0044/45 pins. See [PLAYER_DRIVING.md](PLAYER_DRIVING.md) and completed
TASK-0046 for exact scope, attempts and limits. Debug records now258 unique addresses
(226 Implemented/32 Decompiled), no Verified. Remaining certification action stays
TASK-0038: attach a runnable synchronized Debug scene and equal-state input/time/contact
trace; PCSX2 probe still times out. Whole world/race, full DMA/GS and degenerate VU
domains remain explicit boundaries rather than invented behavior.


## TASK-0047 extraction result

Alpha archive extraction completed outside the repository: 405 records, 81 decompressed COMP members, all output hashes checked. 92 original paths remain unresolved and are preserved by hash. See completed TASK-0047 and reverse/structures/alpha-zzdata-task0047.md. No blocker for extraction; optional next action is recovering unresolved names from Alpha-specific evidence. Existing game recovery tasks remain unchanged.
