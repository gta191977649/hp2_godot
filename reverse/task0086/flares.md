# TASK-0086 vehicle light flare native recovery

Evidence **E-0086-F01**. Debug `NFSHP2.ELF`, SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`;
version/build independently unknown. Read-only Ghidra MCP inspection, 2026-10-21.
Raw responses remain ignored under `local/task0086/`; prior full vehicle Render
and Create captures remain under `local/task0082/lights/`. No original runtime
comparison is available; nothing here is Verified.

## Confirmed original chain

- `CarRenderInfo::CreateCarLightFlares` Debug `0x00120558` enumerates original
  position markers matching `bStringHash(BaseModelName)`, ignores unrecognized
  marker hashes, and uses marker position minus Body0 `PivotPosition`. Param byte
  1 selects `CarLightFlareNormals`, six aligned 16-byte entries at `0x002D7AF8`.
  Flare Enabled is one; red and blue cop types set Flags bit 0. Other lamp types
  clear it. Port allocation/storage does not reconstruct original slot-pool ABI.
- `CarRenderInfo::Render` Debug `0x001210F0` has an independent intensity slice.
  Night TimeOfDay 2 or 3 starts heads at 0.75 and brakes at the original scalar
  `0x00324B84` (0.7). HeadlightsState adds one. Brake strictly above the original
  `0x00324B88` threshold (0.05) adds Brake*16; force brake adds one instead.
  Gear REVERSE=0 enables reverse. Force flags 1/2/4/8 apply head/brake/reverse/cop;
  original globals `0x00326C80..0x00326C8C` OR those bits. Active intensity is capped
  at one before submission.
- Broken bits 1/2 gate individual head sides, 4/8 gate brake/reverse sides, and
  16 gates centre brake. Cop head strobes also honor broken head-side bits.
  Cop state nonzero enables cop flares. FAST=2 uses original eight-entry red/blue/
  white arrays at `0x00326C98`, `0x00326CA0`, `0x00326CA8`, indexed by phase&7.
  Other active states use original slow arrays at `0x002D7AB8`, `0x002D7AC8`,
  `0x002D7AD8`, indexed by (phase>>1)&15. Head strobes use `0x002D7AE8`, phase&15.
  Phase is CarTimebaseStart + WorldTimeFrames. The native producer reads original
  state; a new cop activation control/AI is not claimed. Unit tests exercise both
  states by writing the fixture memory, distinct from actual scene activation.
- `eRenderLightFlare` Debug `0x001168E8` transforms position and normal, rejects
  disabled or centre-clipped flares, normalizes camera-position minus world lamp
  position, and computes its dot with the transformed normal. Flags bit 0 takes
  absolute dot. The ten type rows at `LightFlareParameterIndicies` `0x002CD998`
  each select up to two original parameter pointers.
- Scale is camera-space Z divided by **eView::H**, not distance or viewport height.
  Assembly `0x00116ADC..0x00116AF0` extracts the transformed Z lane and divides H.
  `CalculateViewMatricies` `0x00106F18` copies Camera.CurrentKey.Matrix into
  WorldViewMatrix. `CreateViewMatricies` `0x0010EA18` stores `CalculateH`
  `0x001042E8`: (ScreenWidth>>1)/(sin(FOV>>1)/cos(FOV>>1)); tan zero returns 10.
- `RenderSingleFlare` Debug `0x00116510` reads 60-byte eLightFlareParams entries:
  MinSize/MaxSize at 0/4, four float MinColour/MaxColour at 8/24, Power 40,
  ZBias 44, MinScale/MaxScale 48/52 and TextureID 56. Ten entries begin
  `0x002CD740`. The producer reads original Debug memory, without embedding the
  proprietary parameter table into source.

## Angular calculation is not pow

The scalar assembly at `0x00116574..0x001165BC` confirms an IEEE bit approximation:

1. max(angle dot, original minimum at `0x00324A4C`, 0.01).
2. Interpret those float bits as signed int32, then numerically convert int to float.
3. Multiply by Power, add `1065353216.f - Power*1065353216.f` with separate float ops.
4. Convert float to int32 using the existing finite `cvt_w_s` port adapter, then
   reinterpret the resulting bits as float; multiply by caller intensity.

For angle 0.3 and Power 2 the recovered sequence returns 0.08749961853027344,
not mathematical 0.09. The test guards this difference. Ordinary finite scalar
behavior is reconstructed; exceptional FPU/VU rounding/denormals and original
runtime numeric equivalence remain unverified.

Size is MinSize+(MaxSize-MinSize)*angular intensity; nonpositive size submits no
quad. Then size multiplies cameraZ/H clamped between original MinScale/MaxScale.
Centre shifts toward camera by ZBias. WorldView first/second rows supply billboard
axes. Original corners are centre-X+Y, centre+X+Y, centre+X-Y, centre-X-Y. UVs
are (0,0),(1,0),(1,1),(0,1). Each RGBA component independently interpolates original
float bounds, converts to int and stores its low byte; no invented colour curve.

## Textures and ePoly consumer

`eLightUpdateTextures` `0x00116880` loops exactly three original hashes from
`0x002CD730`, calling GetTextureInfo(hash,0,0). Asset/model integration resolves
these hashes against original GLOBAL textures and reports missing resources.

`ePoly::Render3D` `0x00110368` sends vertices/colours/UVs in strip order 0,1,3,2,
sets identity local-world, uses the texture's active DMA list and emits
AddData(type=5,qwsize=8,parameter=(IsAnySemitransparencyFlag!=0)|0x200).
It does not read ePoly's AlphaA/B/C/D override fields. ApplyAlphaSorting adds an
original alpha key with Z from WorldView*corner0. `eCreateActiveTexture`
`0x00109F78` selects AlphaTextureList or SolidTextureList by ApplyAlphaSorting.

`AttachAlphaPolys` `0x0010BBF8` selects matrix pack using TextureInfo.BiasLevel,
uses the original RenderingOrder and semitransparency target state, and attaches
the texture environment. `CreateTextureContext` `0x00108AE0` obtains ALPHA from
TextureInfo.AlphaBits and AlphaFix<<32, TEX0/TEX1 from original texture/mipmap fields,
and CLAMP from TilableUV unless the original global override applies. The flare
call adds no generic alpha or depth override. `AttachDmaDataListMain` `0x0010A2D0`
passes type-5 parameter to the VIF packet; 0x200 is not a recovered standalone
blend mode. Existing recovered texture/render-target rules remain required.

## Port implementation and validation

`src/game_core/port/driving/vehicle_flares.hpp/.cpp` is a Godot-independent port
projection, not replacement definitions of original callable signatures. It
publishes original-state intensities and fixed billboard descriptions; it does
not alter physics or invent particles/lifetimes/road beams. Explicit gaps throw
NotImplemented for unknown normal indices, original table pointers/types or
exceptional camera normalization. Ordinary nonlamp markers remain ignored.

Commands on 2026-10-21, CMake 3.29.2 / MinGW-w64 GCC 13.2.0:

- `cmake --build --preset native-debug --target vehicle_flare_tests -j 6`: passed.
- `ctest --preset native-debug -R '^vehicle_flares$' --output-on-failure`: 1/1 passed,
  final run 2.48 seconds, original Debug data available (not skipped).
- `cmake --build --preset native-debug --target driving_backend_tests driving_player_tests -j 6`:
  passed.
- `ctest --preset native-debug -R '^(driving_backend|driving_player)$' --output-on-failure`:
  2/2 passed, 15.89 seconds.

Tests cover decoded original hashes/parameter pointers/normals, marker pivot/type,
unknown normal rejection, day/night/brake equality/scale/reverse/damage/force,
fast/slow cop phases, nonmathematical power, angular floor/two-sided flag,
two-layer headlight textures, original colour byte, corners, ZBias, scale and clip.
These PC checks do not establish original PS2 frame equivalence. Godot scene,
texture/depth occlusion and attached-instance lifecycle validation are recorded by
the TASK-0086 host integration owner separately.

## Index and continuation

Four address records were added for 00116510,00116880,001168E8,00120558. Existing
001210F0 record/index is extended for the intensity slice only, retaining prior
shadow reconstruction and original provenance. No original whole-entry stub
transition occurred. Remaining uncertainty includes actual original runtime visual
comparison, complete CarRenderInfo damage ownership and cop activation/AI services.
Use the commands above with configured original Debug ELF for native continuation;
raw captures and originals stay ignored. TASK-0086 remains unverified regardless
of host test results.

## Original cop activation continuation

The existing TASK-0082 implementation already supports actual player cop activation:
`RealDriver::JoyHandler_HonkHorn` Debug `0x0013BF58` checks original CarConfig+0x538
CarClass==1. When TheRaceParameters.BeACop is zero, horn press toggles CopLights
between OFF=0 and FAST=2 and SirenMode between 0/1; release retains cop state.
Noncop horn press/release controls HeadlightsState instead. Fresh Ghidra capture
`local/task0086/0x0013bf58.json` agrees with existing
`src/game_core/indep/src/realdriver.cpp` and NativeBackend's horn-edge dispatch.
No new production cop-memory setter is needed or introduced.

Read-only instruction search also located `tPlayerCop::Think` 0x00227C50 setting
FAST during pursuit and `tPlayerCop::ChangeChaseTarget` 0x00227D88 toggling OFF/SLOW
without a pursuit. Those full police AI/manager services are dependency evidence
only, outside this implementation; BeACop manager ownership remains explicit.

The final native test now additionally loads the real original BMWM5COP config,
advances NativeBackend for 32 held-horn frames, requires both red and blue phase
outputs, then releases and presses horn again and requires all cop phase outputs
off. The final `vehicle_flares` build and test passed 1/1 in 2.79 seconds. This is
actual reconstructed driving/input publication, distinct from the isolated slow/
fast table fixture and from original PS2 runtime verification. Prerequisites now
include configured original GLOBAL assets in addition to the original Debug ELF.
