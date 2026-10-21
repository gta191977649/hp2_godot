# TASK-0083 collision sound evidence

E-0083-A01. Behavior authority: Debug NFSHP2.ELF SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
Version/build independently unknown. Read-only Ghidra captures remain ignored
under local/task0083/audio; public capture/asset hashes are in
collision-audio-captures.json. Original Retail sound assets remain a separate
source. Status: bounded reconstruction, not Verified against original runtime.

## Confirmed selection and samples

GetInGameSample001AB900 obtains bSound region InGame and resolves named samples.
CollisionSounds::LoadSampleTable001E0D68 loads these exact names, assigns category0
and initializes each three-sample library's CurrSampleIndex to0:

- CrashMetalSmall01/02/03, CrashMetalMedium01/02/03, CrashMetalHard01/02/03.
- CrashWood01/02/03, CrashFoilage01/02/03 (original spelling retained).
- CrashMetalScrape, CrashWoodScrape and CrashGlass01.

The base names concatenate literal01 at002B1080, then replace the final digit.
SOUND\INGAME.MSH and SOUND\INGAME.MSB uppercase bStringHash names resolve to
__Unknown/FDFBE4AB and __Unknown/FDFBE4A5. The original header has68 descriptors;
all18 above are present. Descriptor layout and raw ADPCM framing follow the
previous bank evidence E-0082-A01. Only the two scrape patches loop; other impact
patches are one-shots. Glass has32000Hz, all other listed patches22050Hz. Gains
come from each original descriptor, not a substitute normalization.

GetCollisionSoundType001E0C18 uses terrain15or16 (strict14<type<17) for the wood
library. All other observed branches select metal level
clamp(trunc(TotalImpulseMagnitude * float00325B78 *3),0,2). It increments the
chosen library index modulo3 before selecting; initial selection is variant02.
Foilage samples are loaded but this inspected function does not select them;
this is not permission to alias general scenery contact to foliage.

Collision::GetTerrainType00219698 calls each witness body's virtual terrain getter
and selects the higher TerrainTypeInfo.CollisionPriority; equal priorities select
body1. TerrainTypeInfoTable003102E8 has stride0x90, CollisionPriority float+0x2C.
Renderer materials alone cannot substitute for this original body-priority rule.

## Confirmed event and gain rules

CollisionSounds::CalcCollisionVolume001E11E8 uses distance from initial contact
point to camera. If distance>=35, result is0. Otherwise impulse multiplies
00325B80 then00325B84, caps above1, and multiplies min(35/distance,1).
The strict35 gate makes attenuation1 throughout the accepted positive-distance
range; preserving this apparently redundant expression avoids inventing a fade.

collision_sound::CreateSample001E1838 waits until
float(WorldTimer.PackedTime - CollisionTime.PackedTime)*00325BA4 >=00325BA8.
After that it selects a sample even when the eventual volume is too quiet.
Only volume>00325BAC allocates a CollisionSample. Allocation occurs even when
SoundEnabled326F40 is0; disabled playback does not consume randomness. Enabled
playback calls sound::Random001ACFA8 with00325BB0 and uses pitch
(random+0.5)-1; volume is calculated volume*00325BB4. sound::Random calls original
bRandom(float,unsigned*)002537E0 with SoundRandomSeed00327EEC.

CollisionSounds::Update001E1468 traverses world CollisionList and calls CreateSample
only while collision.Sample is null; each live collision owns at most one impact
sample. ReleaseSample001E1970 retains an unfinished impact on FreeSampleList after
the collision expires; completed samples are deleted. Host transport should let
accepted impact one-shots finish, rather than repeatedly starting them each frame.
The original pool Register001E0D00 has40 slots of0xC4. Full PS2 sample allocation,
virtual dispatch, pool exhaustion, spatial azimuth and lifecycle ABI are not claimed
by a bounded host request projection.

## Exact original constants

| Address | Raw float bits | Value / use |
|---|---|---|
|00325B78|3D088889|1/30 metal strength|
|00325B80|3D088889|1/30 volume impulse|
|00325B84|3F266666|0.65 volume|
|00325BA4|3983126F|0.00025 packed timer seconds|
|00325BA8|3DCCCCCD|0.1 delay|
|00325BAC|3E4CCCCD|0.2 strict minimum volume|
|00325BB0|3F19999A|0.6 random range|
|00325BB4|3FF33333|1.9 playback gain|

## Scraping and unresolved scope

PlayScrape001E1D80 requires SoundEnabled and null Scrape. StartScraping001E1288
waits0.1s, chooses wood for terrain15/16, metal for SOLID_WALL, then calculates
CalcCollisionScrapeVolume001E10C8 = min(CalcCollisionVolume * CurrentScrapeSpeed
*00325B7C,1). Gain multiplier00325B90 is1.9; random pitch range00325B94 is0.36,
offset00325B98 is-0.18. UpdateSample001E13C0 combines ComputeVolume(current
contact,200), scrape volume and1.9; release fades to0 in50ms (001E1970).
The bounded native scrape projection restores these start/update rules and original
fade volume steps. The caller owns persistent contact/voice state and invokes start
only for a fresh AddImpulse. Full SPU ownership and camera azimuth remain unbound.
Glass, smokeable, roadblock and explosion event producers are not implicitly
restored by loading their samples. No full game audio or PS2 equivalence claim.

## Native implementation and checks

Port-side collision_sound_state.hpp carries no Godot or audio decoder types.
CollisionSoundSelector preserves library index state; collision_sound_request uses
candidate Memory for RNG and returns an explicit playing=false allocation when
SoundEnabled is disabled. The caller owns original collision lifetime aggregation
and retries. collision_audio.hpp exposes original sample decoding and the shared
load_ingame_sound_samples helper for other InGame audio consumers.

New driving_collision_audio_tests checks exact timer399/400 boundary, strict35
camera cutoff, variant order, metal strength, wood sharing, low-volume selection,
disabled allocations without RNG, candidate-copy replay and all18 decoded nonzero
sample/loop contracts. These are PC regression checks, not original certification.

Validation2026-10-21: `cmake --build --preset native-debug --target
 driving_collision_audio_tests -j6` passed (GCC13.2.0/CMake3.29.2/Ninja1.12.0).
`ctest --preset native-debug -R '^driving_collision_audio$'
 --output-on-failure` passed1/1 in0.57s with configured original assets.
No original execution comparison was performed.


## Persistent scrape and fade extension

Collision::AddImpulse00219400 permits PlayScrape only when the selected terrain's
CollisionScrapeMinSpeed is strictly less than CurrentScrapeSpeed. The recovered
TerrainTypeInfo layout fixes that threshold at table003102E8 + terrain*0x90 +0x88.
Terrain enum evidence fixes SOLID_WALL=17. StartScraping001E1288 compares timer
using325B88/B8C, then plays at scrape volume*325B90 and random pitch
bRandom(SoundRandomSeed,325B94)+325B98. Updates do not add an invented SoundEnabled
check: UpdateSample001E13C0 updates an already allocated scrape's raw volume.

Sound3d::ComputeVolume00203E20 with200 sphere yields
1-sqrt((distance+distance)*(1/200)-(distance*(1/200))^2) while distance<=200,
otherwise0. Start uses initial-point camera distance; update also uses current
contact distance for this envelope. Both positions must be explicit caller inputs.

bSample::Fade0026C910 sets m_fFadeUpdateAdjust=(target-current)/(length*3262D4)
for lengths>=33, while shorter fades apply volume directly. ReleaseSample requests
length50 andtarget0. Float3262D4 raw3D75B5FB is0.059988003224134445. Each original
bSample::Update0026CB40 adds the fixed adjustment and clamps an overshoot to target.
The collision free list stops/deletes a sample at volume<=325BA0 (0.05). Port-side
helpers preserve these per-service-update steps; dispatch once per committed host
frame is a host cadence binding, not verified original SPU/service scheduling.
A continuous Godot Tween is not substituted for this recovered step progression.

The native test now also covers strict scrape speed and timer gates, disabled RNG,
metal/wood sample selection, random pitch range, nonlinear envelope endpoint,
and all three fade updates through zero. Rebuild/testpassed1/1 in0.59s.
