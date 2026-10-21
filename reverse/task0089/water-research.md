# TASK-0089 research handoff

Research and bounded native implementation, 2026-10-21. No original-runtime
verification is claimed. Authority: Debug `NFSHP2.ELF`, SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`,
version independently unknown; player's configured original ZZDATA assets.
Research preceded TASK-0088 strict completion; native work began only after
the parent activated TASK-0089 following that completion.

## Evidence manifest

E0089W01: `Car::TriggerCarTerrainEffects` 00133988, capture
`local/task0089/0x00133988.json`; terrain rows 00310528, 003105B8, 00310E28,
speed conversion float 00324D2C (2.236999988555908), corresponding `*-bytes.json`.
E0089W02: timed wipers 00133590; Render tail 00123580..0012368C
(`wiper-render-listing.json`); constructor 0011EA88.
E0089W03: splash trigger/update/sample 0018B490/0018B4A8/0018B958,
BaseCarSound update 0018B348, sound Random 001ACF88.
E0089W04: animation loader 00103588, GetRigidAnimation 0010CCE8,
GetPivotMatrix 0010CE58, GetRigidAnimationTime 0010DEB0,
SetupRigidAnimation 0010DF98, GetMatrixAtTime 001A4CF8,
eModel Render 0010E048, bMulMatrix 002541C0.
E0089W05: TriggerEffect 0011E4E0, InitRenderInfo 0011DEA8;
original-asset metadata `water-asset-audit.json`, `wiper-splash-asset-audit.json`;
read-only Track12 audit `audit_track12_particles.cpp` and executable.
E0089W06: ReplayManager RegisterInterestingEvent 0013E070.
All named captures are ignored private files under `local/task0089/`; no ELF,
asset payload, original matrices or decompilation is committed here.

## Confirmed water branch

E0089W01 loops grounded wheels, identifies terrain PUDDLE=4, DEEP_WATER=5,
MUD_PUDDLE=20, and builds a wheel mask and grounded-deep-water flag. With
distance-to-player <60, a nonzero mask, and speed >10 it requests timed wipers
at WorldTimeFrames+60 through +180. If pBaseCarSound is nonnull it requests
one splash, side `(mask & 6) != 0`, intensity deep ? 3 : 1. This is one request,
not one sound per wheel. The deep-water latch clears whenever no grounded
wheel is deep, independent of this distance/speed gate.

With player nonnull, `(WorldLoopCounter+3)&3 == 0`, and speed >30, shallow
water requests replay weight 1.25 with label `InShallowWater`; deep water
requests 10 with `InDeepWater` only on the first latched encounter.
0013E070 only changes the INSTANT_REPLAY snapshot weight while CapturingFlag
is enabled; its label argument is unused in the recovered body. Port input
recording is not evidence of original ReplayManager capture ownership.

## Visible spray versus audio

The visible spray is authored CARFX_DRIVE_OVER1/2, not TriggerSplash and not
the separate RainSplash rain-system feature. With >2 grounded wheels,
00133988 queries body terrain at State.Position. Intensities use speed times
00324D2C, then clamp `(speed-min)/(max-min)`. If their sum exceeds 1, twice
the excess is subtracted from the lower-minimum channel (first when min1<min2,
otherwise second), clamped at zero. Positive channels invoke effect 1/2.

| Terrain | DriveOver1 group, min..max | DriveOver2 group, min..max | Skid min..max | Spew min..max |
| --- | --- | --- | --- | --- |
| 4 | XETT_PUDDLE_DRSL, 3..30 | XETT_PUDDLE_DR, 35..180 | 5..20 | 1..100 |
| 5 | XETT_DEEPWATER_DRSL, 3..30 | XETT_DEEPWATER_DR, 35..80 | 5..20 | 1..100 |
| 20 | XETT_MUDPUDDLE_DR, 50..150 | empty | 5..20 | 2..100 |

Skid/spew names use the respective terrain prefix with `_SK`/`_SP`.
GLOBAL has 20 matching water DriveOver definitions/prototypes: deep DR=6,
deep DRSL=8, puddle DR=2, puddle DRSL=4. Their original textures are DUSTA_CS
and SMOKEB_CS. GLOBAL has no matching water SK/SP or MUDPUDDLE definitions.
Actual Track12 decoded 20 definitions/40 emitters, with no water groups.
Consequently the current wheel-only effects 3/4 consumer does not supply
this spray. Empty authored lists must remain empty; no new splash emitter
or replacement texture is justified.

All audited water prototypes use original CarPosition 1..4: front/rear/left/
right wheel-pair midpoints from InitRenderInfo. Definition offset88 is the
raw enum bits (currently controls[14]), not a numeric float. TriggerEffect
uses State.GeometryPosition and BodyMatrix for these effect1/2 positions;
the body terrain query instead uses State.Position. Preserve this difference.

## Timed state, audio and visible wipers

00133590 writes Car WiperOnTime+838, WiperOffTime+83C only if WiperState+840
is zero. It merges intervals using the original current-frame comparisons,
then rounds off time to `((off-on+81)/82)*82+on`. Preserve boundary comparisons
and integer division; the 82-frame schedule is not the per-car animation cycle.
The full native test must cover now=0 and overlapping/expired intervals.

TriggerSplash stores left/right intensity. UpdateSplash first decrements a
nonzero channel timer. A nonzero intensity at timer zero chooses Splash1/2/3
through sound::Random(3), plays volume*intensity with arguments -1 and 0,
then sets timer10. Selection/play requires SoundEnabled, CarSoundEnabled and
RoadNoiseEnabled, but timer reset still occurs when disabled. Intensity clears
even during cooldown. Left updates before right. Random consumes the original
SoundRandomSeed00327EEC only when the enable gate allows playback.
The existing original InGame bank contains all three samples (22050Hz,
patch volume8682); reuse its VAG decoding and sample gain/pitch helpers.

Render001210F0 tail requires pixel size >40 and a nonnull car. Wipers render
when WiperState!=0 or On<=WorldTimeFrames<Off. Each original wiper eModel
with a rigid animation receives start `(On&3fff)|c000`; rendering multiplies
the original base/pivot/animation matrices. Inactive separate wiper models are
not drawn. Rigid animation uses discrete frames, no interpolation:
elapsed=(WorldTimeFrames-start)&3fff; looping start4000 selects elapsed modulo
(NumFrames*FrameRateScale); frame=(time/FrameRateScale)%NumFrames.
Chunk34012 has a 48-byte header followed by 64-byte matrices. Solid animation
may refer to an external animation hash, which requires explicit resolution.
911TURBO and CROWNVIC have left/right 41-frame scale2 animations; ELISE has
only right, 61 frames scale2; BMWM5COP has no WIPER solids. Missing original
models are legitimate absence, not permission to fabricate geometry.

## Implementation contract proposed after TASK-0088

All following API names are port-side proposals, not recovered identifiers.

Native producer owner: water state, original timed scheduling, body DriveOver
mapping and native splash selector. Model owner: camera_recovery, original
rigid-animation decoding, selected wiper assets and model consumer. Root:
Godot dictionary/audio transport, original bank loading and shared integration.

Publish `Snapshot.wipers` with signed `state`, `on_time`, `off_time`, and
unsigned `world_frame`; preserve raw original times, not an invented phase.
Model consumer computes the exact original active interval and frame from
those values and applies original pixel-size visibility. Publish a vector
`Snapshot.splash_audio` of port-side requests `{sample_index, volume,
pitch_octaves=0}` after original enable/RNG selection; root transports each
committed request once and resolves the original InGame sample.
The original Play argument -1 uses the original default/centered host binding;
no unverified positional interpretation is added to the native request.
Native selector state owns left/right pending intensity and cooldown timers.
The fixture allocates an explicit BaseCarSound activation proxy at Car+850,
not a constructed original audio object; the producer preserves the null-pointer
guard. Mixer ownership is explicit, transactional and reset with the session.
Use original sample gain helpers at the audio-bank boundary.

Water state/audio advance once per committed native world step in the native
backend candidate, never during rendering or environment capture. Candidate
rollback must restore memory, cooldowns, SoundRandomSeed and pending requests.
DriveOver emissions join the existing visual particle transaction once per
visual epoch, retaining original ordering, RNG, capacity and GPU/native paths.
Extend original position binding for the audited CarPosition1..4; do not
route them through per-wheel effect3/4 overrides. Paused/repeated presents
must neither reschedule wipers nor consume RNG nor replay audio.

Replay remains an explicit ownership decision: preserve original water latch
and request conditions; do not silently map ReplayManager to input tapes or
pretend that original capture is restored. An entered unsupported original
capture path must follow the repository's NotImplemented contract.

Acceptance: native interval-boundary, wheel-mask/deep-latch, threshold/channel
overlap, audio cooldown/flags/RNG/order and original frame tests; actual assets
and Track12 integration; paused/retry/reset/unload transport checks. Original
runtime comparison remains absent. Damage/nitro/flat-tire branches and the
rain-screen RainSplash system are outside this bounded water recovery.

## TASK-0089 native implementation and checks

After TASK-0088 strict completion, the bounded producer was implemented in
`water_effects.cpp`; `car.cpp` contains the DriveOver intensity slice and
`care.cpp`/`car_effects.cpp` bind authored groups and original CarPosition0..8.
NativeBackend owns candidate mixer state and publishes optional WiperSnapshot
and splash requests. Existing own-car listener distance0/volume1 is an explicit
port binding, consistent with existing native tire audio; this is not recovery
of original world distance lookup or complete Sound3d envelope ownership.
The existing disabled-capture ReplayManager service is reused. The full
Car::TriggerCarTerrainEffects entry still throws after its old wheel slice;
it is not claimed complete. Unknown DriveOver marker lists beyond8 throw.

Read-only World::DoTimestep0014D9A8 capture `world-order.json` confirms movement/
FinishMovement, WorldEffects update, terrain effects, then FakeEngineTask order.
The bounded native state/audio producer runs after movement, before snapshot
publication/AdvanceWorldTime; visual DriveOver joins the existing post-update
visual particle transaction. Complete World sound-task scheduling remains
outside the fixture; neither repeated renders nor captures run native audio.

Build: `cmake --build build/native-debug --target water_effects_tests
driving_particle_tests gpu_particle_emission_tests driving_terrain_effects_tests
driving_backend_tests driving_camera_tests driving_player_tests driving_skid_tests
water_contact_tests -j 4` passed. Regression command:
`ctest --test-dir build/native-debug -R '^(water_effects|driving_particles|driving_particles_original|driving_wheel_particles_assets|gpu_particle_emission|driving_terrain_effects|driving_backend|driving_camera|driving_player|driving_skids|water_contacts)$' --output-on-failure`
passed 11/11 (65.01s). Native synthetic checks cover interval boundaries,
manual scheduling bypass, frame0, audio flags/cooldown/RNG/left-right ordering,
deep latch/replay gates, two-channel intensity overlap and original pair positions
with GeometryPosition distinct from Position.

Actual original911TURBO config/geometry +GLOBAL particle library, controlled
native water topology, 480frames/terrain, two independent native backends:

| Terrain | Max native speed | Splash requests | Scheduled frames | Peak particles |
| --- | --- | --- | --- | --- |
| 4 | 47.6327 | 44 | 432 | 1000 |
| 5 | 47.0749 | 43 | 425 | 1000 |
| 20 | 47.6327 | 44 | 432 | 0 |

This checks real authored water emission, capacity, raw wiper scheduling, sound
sample RNG replay, subsequent physics equality after visual mutation, and reset.
MUDPUDDLE remains authored-empty. No per-frame asset loading, benchmark or
original-runtime verification was added. A final null-sound guard review added
the original pointer gate; its synthetic test supplies an explicit proxy.

Final native null-sound-guard source rebuild and focused `water_effects` rerun
passed 1/1 (31.55s test duration). Tools: CMake3.29.2; MinGW-W64 UCRT g++13.2.0.
`git diff --check` over the owned native modules/test passed. Build warnings
reported pre-existing unrelated care.cpp shadow and Polyhedra indentation.
