# TASK-0083 tire skid, drift squall and burnout audio

Evidence E-0083-T01. Debug NFSHP2.ELF SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b;
version/build independently unknown. Read-only captures local/task0083/tires.
Bounded implemented port-side producer; not Verified against original execution.

## Confirmed sources and assets

PlayerTireSound constructor0020D660 selects original InGame samples RoadNoise,
GravelNoise, LongitudinalSkid, GravelSkid, SquallSkid, TireFlat, CrashMetalScrape,
BurnOutPeel and GravelSkid again for DirtSquall. The current user-requested
skid/drift slice loads the four unique LongitudinalSkid/GravelSkid/SquallSkid/
BurnOutPeel samples through the shared InGame MSH/MSB decoder. ADPCM loop flags
remain authoritative. These sounds are independent of SkidMaker geometry gates.

CarSide has aliases FRONT=RIGHT=0 and LEFT=REAR=1. This is actual type evidence;
never infer axle/side symmetry from the enum wording. Car.State is at40;
WheelTouchingGround310, WheelTerrain314, WheelSlip334, WheelTractionUsage354,
VelocityMagnitude384, Throttle38C are offsets within State.

- CalcTireSkidVolumeAndPitch0020DBC8 uses contact-qualified WheelSlip length,
  zeros values below.25, then averages wheels0/1 for side0 and2/3 for side1.
  Side1 has a threshold and locks its selected pitch until the slip falls below
  threshold. All tuning constants are read from the bound Debug image325E4C..60.
- CalcTireSquallVolume0020DF80 requires absolute speed>=11, uses contact-qualified
  traction usage pairs0/3 for side0 and1/2 for side1, averages absolute values,
  applies325E74/78 threshold/gain and clamps volume at1.
- CalcTireSquallPitch0020E088 uses speed*325E7C plus325E80 for side0.
- CalcTireSquallVolumeAndPitch0020DAF8 smooths volume with distinct per-call
  rise/fall constants325E48/44; this is not timestep-based smoothing.
- Update0020E1E8 determines road/dirt from terrain pairs1/2 for side0 and0/3
  for side1 using TerrainTypeInfo.ShouldTireMakeRoadSound. Puddles preserve the
  previous road/dirt selection. Longitudinal volumes clamp at3.5road/2.75dirt.
- UpdateSquall0020E860 chooses SquallSkid on road and GravelSkid on dirt, stops
  the alternative, applies original smoothing325E98 using smooth001365F0 and
  forwards raw octave pitch (calculatedPitch-1) to bSample.
- BurnOutPeel in0020E1E8 requires road, throttle>325E90, summed longitudinal
  wheel slip>3 and speed<30. Volume is min(slip*.0625,3)*distanceVolume.
  Play pitch side0 uses325E94, side1 uses0. SoundEnabled326F40 and
  CarSoundEnabled327980 gate voice starts; stop/update gates remain as decompiled.

## Bounded implementation and limits

TireAudioMixer in original carsoundtires.cpp stores both original logical tire
sound states and produces ten voice slots: road skid, dirt skid, road squall,
dirt squall, burnout for each of the two aliased CarSide values. Slots sharing
GravelSkid remain independent voices. Raw volume/octave pitch become native
PCM gain/pitch through the TASK0082 helpers. No Godot headers enter this code.
The full original PlayerTireSound::Update ABI remains unimplemented. This slice
omits road/gravel rolling noise, track leaves, metal/pothole one-shots, tire
impact/bottom-out, thumper/damaged-tire logic, spatial azimuth and doppler services.
It does not manufacture those omitted paths or claim complete tire sound recovery.
Distance volume is an explicit host binding supplied by the caller.

## Validation

Driving tire audio tests decode original nonzero PCM/repeat intervals; assert
no-slip, longitudinal skid, traction squall, road/dirt switch, rear pitch lock,
airborne release and disabled-sound gates; then run480native driving frames
with actual steering and handbrake and require nonzero tire voice requests.
PC regression does not establish timing/numerical/audio equivalence to PS2.
