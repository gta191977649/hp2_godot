# TASK-0097 - Ultimate Racer authority and dependency inventory

## Authority and evidence

Debug NFSHP2.ELF SHA815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b;
Retail SLUS_203.62 SHA7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
Both configured files rehashed2026-10-26. Read-only MCP available; original assets
resolve through tools.ps2_assets/platform resolver. No database mutation/computer-use.
Raw evidence ignored local/task0096/{root,selectcar-agent,race-agent,police-agent}.
No scope is Verified. Source fields and function prototypes are version-specific.

## E-0097-D01 - All-event data and semantics

Debug LoaderEventTree001FB138 maps chunks34A01 cars/4bytes,34A02 tracks/4,
34A03 goals/8,34A04 rewards/8,34A05 events/188,34A06 opponents/4.
SetEvents001FB458 divides by188. Imported EventInfo has fields:
number28,type29,class2A,opponentSelect2B,nCops2C,copStrength30,roadblocks34,
spikes35,barricades36,tickets37,chopper38,BeACop allowances39/3A,
counts3B..3F,zone40,objective42,cars44,tracks6C,requirements7C,rewards8C,
opponents9C,boostAC,titleB0,descriptionB4,damageB8. Getter instructions corroborate
array bases and signed count/index handling. Unknown padding and runtime pointers
are not rebound. Non-E3 tree names are case-insensitive in001F9450; the current
port adapter accepts the shipped non-E3 names case-insensitively and throws for variants.

Original Retail asset FRONTB contains two32-record tables. Hot Pursuit has types
0:8,1:2,2:4,3:5,4:4,5:4,6:5. Debug SetRace001FA378 maps these to single(default),
time trial,lap knockout,race knockout,tournament,point-to-point,and BeACop.
All32 nodes, including31/32, remain in the matrix. Special-node unlock semantics
must be recovered in TASK-0102; their presence is not completed implementation.

Derived event-coverage.csv contains counts, objective and police capability inputs,
not a copied asset payload. Full exact car/track/prerequisite/reward/opponent bindings
stay ignored in root/asset-event-bindings.json. Reproduce with
`py -3 reverse/scripts/audit_ultimate_racer.py`; `--check` verifies the durable matrix.
All references for both64-record tables pass native checks. Stored integer flag
bBarricadeSmokeables is18 in these assets; never silently normalize it to bool.

## E-0097-D02/D03 - First portable recovered implementations

New hp2::debug namespace isolates Debug EventInfo/EventTrack names from existing
Retail projections. Standard containers and explicit ByteView/service arguments
are port ownership, not reconstructed PS2 ABI or whole EventTree lifetime.
Code remains in original eventtree.cpp and original Debug header EventTree.hpp.
GetCar001FBBC8/GetGoal001FBC08/GetTrack001FBC48/GetReward001FBC88/
GetOpponent001FBCC8 reject caller indices outside count by returning null as original;
GetObjective001FBD08 uses nObjective, independently of prerequisite list.
Malformed loaded-table indices fail explicitly instead of resolving different data.
GetStateSize001FC028 is fully translated including original default arm.

EventTrack::SetRace001FA178 binds a packed track to Debug RaceParameters0x310:
number bits0..7,laps14..17,weather10..11,time12..13,direction9,
traffic18..24,oncoming25..31. Constants00325D10/14 bothbits3C23D70A.
Alternate bit8 is not consumed by this function. Original GetTrackInfo callback
supplies Point2Point; missing TrackInfo preserves prior value. No track index is
substituted for its original number. Unknown providers throw before mutation.
Other RaceParameters fields remain unchanged; this is not a complete race launch.

## E-0097-V01 - Version boundary and counterevidence

Common event record offsets and table indexing are corroborated by original
Retail and Debug consumers and real-asset parser cross-checks. This does NOT mean
progress buffers or runtime object layouts are compatible. Debug race-KO state
is12bytes (001FC028), Retail16 (00200DE8); five Hot Pursuit nodes require explicit
semantic conversion. Debug CarSelector3F0 vs Retail400; FrontendDatabase also
differs. Existing Retail save input must never be rebound to Debug pState.
Outcome/profile conversion remains TASK-0102 and must reject unsupported fields.
The user's 'basically same' observation is not used as equality evidence.

## E-0097-G01 - True launch and World ownership

StartEvent001F2BD8 -> EventTree::SetRace001FA378 -> BuildUnlockList ->
MakeEventCarsAvailable001822F0 -> Select Car. Championship uses GameType8,
Hot Pursuit uses9. In-progress multitrack uses its distinct continue path.
Challenge SetupRaceParametersAfterRaceSetup00184640 is NOT this event mapper.

Selector MaybeContinue001965C0 -> SetSelectedCars00196198 ->
FrontendDatabase::StartRace00183378. StartRace picks event track by raceIndex%nTracks,
adjusts knockout/tournament placement, caps AI by original maximum/eight racers,
SetupCarsForRace00183A58, copies original player/display/name/record state, then
requests UNLOADING_FRONTEND or LOADING_TRACK_COMMON according to current state.
No direct fixture scene substitution is valid here.

Loading-specific00168540 ->001684F8->001684E8->001684B8->00168438->00168418->
001683C8; last requests RACING. EndGameFlowLoadingTrackSpecific00168608 initializes
collision/topology/physics/routes/world/AI resources, then RaceCoordinator ctor001495B0.
InitForRace00149800 owns PlayerAA0 each,ReplayManager50B4,Race6B4,World1730;
InitCars0014A128 -> InitPlayers00149B28 -> Race00147768 -> coordinator state.
Cleanup001499F8 owns destruction. Coordinator transition sends UNACTIVATE,ACTIVATE,
thenTICK; message queue is FIFO. All these whole entries still need translation.

## E-0097-S01 - Original selection closure

SelectCar callback001963E8,registered00196688; documentB5074AB4 for1P.
MakeEventCarsAvailable001822F0 flushes64 statuses per player to locked-invisible3,
then resolves EventCar indices. EventCar byte2=type,highbyte low4=skin,
bits4..5=usage,bit6=dashboard. Skin0 means all skins via -1.
VisibleCarSet001921D0 and sort00192410 use status,police usage,HP/class/fullname.
Car types must not be sorted by numeric ID. CarStatus24bytes,RideInfo8,
FrontendPlayerInfo6D0 with statuses0/playerSettings644/rides674/points6CC.

States SELECT0,TRANSMISSION1,HANDLING2,READY3. 1P ready is HANDLING2 (NOT3).
Cars move without wrapping; skins wrap. Transmission results1/2/3=auto/manual/sport;
handling4/5=classic/extreme. Back also commits selection. Loading alpha advances
+7/frame to82 before replacement and-3/frame after load; do not replace with
frame-rate-independent guessed durations. Orbit/camera/light/thumbnail/loader/help/
speech effects remain mandatory TASK-0098 dependencies, not an excuse for a generic list.

## E-0097-P01 - Police closure and dependencies

World0014CA28 -> InitCops00222490 (only maxCops>0) -> TrackGraph002480D0.
InitCars0014A128 constructs police DriverInfo categories3..5 through002225B0,
CreateCop00221B28,tAICop002281A8; one shared Chopper001E4018 if police entries exist.
World timestep0014D9A8: TrafficTeleporter,GlobalCopThink00222530,car Think ranges,
wheel forces,Drive,ChopperFly,BeginMovement,move,collisions,FinishMovement,
events,player state,Race update. ManagerThink0021EF58 orders speech/supervisor/
pursuit management/spawn-or-reclaim/spikes/pullovers/cop think/BeACop.

Manager0021DE80 owns6 pursuits640bytes,6AIcops5C,10spikes900,cop pointers7,
pullover entries8. Pursuit ctor00223850 owns drive targets/roadblock/barricades;
Update00223B08 manages state/capture/roadblocks/chopper; destructor00223A18 releases.
CopDriver0019F980 -> CopMover0019E8A8 plus MovementModeTransitions00228BC8:
police uses original hybrid COP/MELLOW movement, not generic PhysicsCar-only chasing.
LoaderCops002225F8 accepts80034500 children34510/34520/34530 for traps/blocks/landmarks.

CreatePullover002217A0 compares existing tickets against configured arrest limit
before increment. EndPullover002200F0 repairs/releases or calls Race::FinishCar
with RACING_CAR_ARRESTED. Escape depends on ShouldPursuitBeCalledOff002269A0,
not only distance. Track graph, route widths/clearance, teleport safety, speech/NIS,
munition and all original pursuit tactics remain TASK-0100 dependencies.

## Performance baseline and external prerequisites

Current race_scene is explicitly single-car; native_backend.cpp deep-copies Memory,
collision and audio state each candidate. Memory owns map<address,vector<byte>>;
mutable ByteViews bypass a future write barrier unless view acquisition is covered.
Do not multiply single-car backends for multi-car races. Old TASK0055 benchmark
paused physics and moved camera: it does not establish driven multi-car60FPS.
Original race density/pursuit creates additional native, draw, reflection, particle,
audio and allocation load. Record per-stage timings in subsequent tasks; no new
FPS/performance claim from this data-only work. Renderer owns persistent resources;
retain its accepted caches and do not enable previously rejected MultiMesh by default.

Tools detected CMake3.29.2,GCC13.2.0,Python3.14.4,Godot4.7.2; godot-cpp
pin e4b7c25e721ce3435a029087e3917a30aa73f06b. Existing local tools/config and
running-process checks expose no PCSX2 oracle; original Debug traces are unperformed.
Raw originals/captures/saves remain ignored; no commits/staging.

## Validation and continuation

`cmake --build --preset native-debug --target ultimate_event_tests --parallel 4` passed.
`ctest --preset native-debug -R '^(ultimate_event|eventtree)$' --output-on-failure`:
2/2 passed1.36s.64 records/all references, state-size version difference, getter
bounds, original nonboolean flag and packed track/missing TrackInfo behavior tested.
Next TASK-0098 original car availability/list/state/loader/render integration.
This inventory is not a playable race, a migrated save or a Verified closure.
