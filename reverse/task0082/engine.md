# TASK-0082 engine and vehicle sound bank evidence

Status: recovered bounded slices; host regression tested. Not Verified against
original runtime audio. Debug authority: `NFSHP2.ELF`, SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`;
build/revision independently unknown. Retail sound assets are a separate input.
Raw read-only Ghidra captures: ignored `local/task0082/engine/*.json`.

## E-0082-A01: original sample selection and decoding

`PlayerEngineSound::Init` 0020AE48 selects Sound.BaseName and Sound.Prefix.
The suffix pointer array 00328930 selects, in order, `lelo`, `lemd`, `lehi`,
`lxlo`, `lxmd`, `lxhi`, `ceid`, `cemd`, `cehi`, `cxid`, `cxmd`, `cxhi`.
BasePlayerCarSound::GetHornSample 0018BDA8 selects Prefix + `Horn` in the same
bank. `SoundInfo` type evidence fixes BaseName at configuration330, HornPitch350,
Prefix356, IdleOffset35E, POVTimbre360, BendRange364, BaseAdd366, Detune368,
Volume380, RedLineOffset3B0 and TopGearOffset3B2 (hexadecimal offsets).

Original loader 00260560 builds `SOUND\\<BaseName>.MSH` and its MSB body.
The extracted hashed MSH entry is found using original uppercase bStringHash
(initial FFFFFFFF, multiply33/add byte). No filename search or sample fallback
is used. SOUND_PATCH_INFO is52 bytes after a12-byte header: Name0[32],
Pan32, LoopChannel33, Pitch34(u16), Volume36(u16), EffectVolume38(u16),
Offset40(u32), ADSR44, Reserved48. Offsets determine each raw ADPCM extent;
Pitch is the PCM source rate. GetChannelNum00260BE0 supplies the channel field.
The port currently accepts mono patches only, and rejects other layouts.

The existing VAG decoder decodes the original ADPCM bytes. ADPCM flag4 marks
the repeat-start frame, flag1 the terminal frame and flag2 terminal repeat.
Each16-byte block produces28 PCM frames. Loop endpoints preserve those flags;
no whole-file looping is inferred. Terminal padding is excluded. Descriptor
volume remains explicit. No proprietary sample bytes are committed.

ENGINE.CTB/LTB are unresolved version2 `CRDl` EngineDef tables. ResolveTables
0020B2E0 establishes pointer-relative relocation: each pvoltable word at
296+4*slot is relative to that word. The port keeps the original files immutable,
validates offsets and reads signed512-byte xlate tables. Their patch numbers
select the actual12 samples; they are not replacement RPM thresholds.

## E-0082-A02: original engine control and sample parameters

Read-only decompilation/listing captures cover:

| Entry | Recovered identifier / role |
|---|---|
|0020AB30|PlayerEngineSound constructor: raw RPM initialization|
|0020B3D8|RampThrottle: rise half-difference, fall eighth-difference, original endpoint snaps|
|0020B498|UpdateGearShiftEffect: requested/current gear, clutch state, smoothed gain|
|0020B590|UpdateToRedLineRPM: idle offset, gear shift RPM/redline ratio and top-gear offset|
|0020B858|UpdateEngineDucker: music-category and five-second throttle gate|
|0020B908|UpdateEngine: CTB/LTB crossfade, timbre, detune, bend, mono gain and voice transitions|
|0020C0A0|Update: RPM smoothing +270/-constant, maximum controllerlevel440 and service gates|
|0020C3E8|Set: throttle ratio, POV timbre and base volume|
|0020A990|VoiceAttr::Play: clamp initial volume and play sample|
|0020AA58|VoiceAttr::SetVolume: signed-short smoothing by15|
|0026BA98|bSample::CalcPitch: original octave lookup and integer-frequency quantization|
|002578D8|bFloatToInt: MIPS CVT.W.S listing|
|0025C7A8|bSnd_SetVoicePitch: hardware frequency conversion|
|0026BEC8|bSample::GetScaledVolume: descriptor/category volume|
|0026C120|bSample::CalcVolume: original integer voice-volume conversion|

The port EngineAudioMixer in original source carsoundengine.cpp binds the
ordinary physics-car, non-replay mono branch. It reads engine constants,
CrossFade00313350, octave tables0031EAF8/0031EB40, TestRPM0032818C and enabled
gates from the Debug image. No generated waveforms, guessed octave powers,
new RPM bands or copied synthetic loops are used. PCM pitch output represents
the original quantized SPU rate relative to decoded source rate; host PCM gain
represents original integer voice volume. Category volume is an explicit
host argument, currently unity. Music-category volume defaults to zero for
the current playerless laboratory.

Explicit limitations: this is not the complete PlayerEngineSound::Update ABI.
Original camera/player/RaceCoordinator services, replay direction timbre,
stereo delayed second voices, nitro, backfire, damage, SPU ADSR/reverb and
spatial mixing remain unresolved/unbound. The port returns RedLineSyncRPM as
inspection data without mutating the original CarState field. Unsupported
view/distance inputs and non-mono layouts throw instead of fabricating behavior.

## Validation and asset availability

Commands: `cmake --build --preset native-debug --target driving_audio_tests -j 4`
and `build/native-debug/driving_audio_tests.exe`. GCC (Strawberry MinGW) host;
this is PC validation, not original-runtime equivalence.

Passed:49 original car rows load all12 engine patches and horn, including exact
ADPCM repeat intervals. All64 row references inspected. Fifteen traffic rows
reference unavailable hashed MSH47CB8E5A: PARCELVAN, PICKUP,4DR_SEDAN, BUS,
SUV, MINIVAN, HATCHBACK, HATCHBACK02, TAXI,4DR_SEDAN02, AMBULANCE, TAXI02,
PANELVAN, COUPE, FIRETRUCK. Those loads report the missing original bank.
No audio substitute is supplied.

The asset-backed911TURBO test exercises real NativeBackend state at idle,
180 full-throttle steps and90 release steps, requiring audible original table
selection, rising controllerlevel, endpoint throttle ramps, finite pitch/gain
and the CarSoundEnabled gate. Missing bank input is rejected. Host audible
playback and perceptual/original-SPU comparisons are separate validation.

Next action: exercise live host audio under acceleration, shifts, pause/reset
and horn, then compare captured original SPU output before promoting equivalence.
