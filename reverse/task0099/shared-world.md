# TASK-0099 shared World implementation start

Behavior authority: Debug NFSHP2.ELF SHA-256 `815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`, revision unknown. Readonly captures and types/listings are SHA-indexed in evidence.json. No original assets or database edits enter Git.

## E-0099-W01 participant table and scalar state

World::BuildCarTable0014D028 reads the original intrusive CarList and Car.MyDriver(+14hex)->DriverType(+8), preserving list order in four passes: player1, AI racer2, police4/supercop5, traffic3. AICarPos is the end of player segment, CopCarPos end of AI racers, TrafficCarPos end of police. Unknown types are omitted. It writes contiguous CarIndex shorts at Car+8; DriverNumber at+A is independent. Table trailing slots and omitted cars' prior index are not cleared. GetCarByNumber0014D1F8 compares signed16 DriverNumber and returns first match/null. ResetTimeScale0014D328 changes five original fields and preserves RNG, PreviousTimeToWaste, Teleporter and other state.

All participants share ONE logical Memory. No per-car NativeBackend instances or memory-image copies are introduced. Table rebuild has no host/logical allocations: four native category traversals plus port-only constant-storage malformed-ring guards. Port validation rejects null/cycles/table overflow; these are outside-original-valid-domain safeguards, not original failure behavior. The host World(memory,address) view is NOT World0014CA28 construction.

## E-0099-W02 first constructor dependencies

DriveTarget00216528 zeroes210hex bytes then invokes Clear002167C0. LastIndexToCheckForDrivability at204 becomes signed-1; constructor is not equivalent to plain zero-fill. Empty/zero/negative-distance clear paths reset length/timers/index while populated positive-distance interpolation remains an explicit exception.

TrafficTeleporter0021CFF0 requires actual loaded-route lookup. FillInTotals0021D778 accumulates NumPoints for50..59 and60..69, retaining totals across genuinely absent routes. SetThinkTime0021D700 reads the shared original TheRaceParameters TrafficDensity and computes(1-density)*15+1. Constructor clamps below20 and clears LastCar; later setter does not clamp. Native type capture places timer0,LastCar4,ForwardTotals8,BackwardTotals30hex; no offsets were inferred from field order.

Tests exercise stable mixed categories, signed lookup, omitted/trailing state, malformed lists,14-entry overflow, scalar preservation, zero allocations across30 table rebuilds, DriveTarget clear gaps, route calls/cumulative missing counts and constructor/setter clamp distinction. The real first-event driver data test uses explicitly labelled minimal Car-header fixtures. It is NOT a real Car/World construction or gameplay acceptance.

## E-0099-W03 required real construction, investigation only

RaceCoordinator001495B0/InitForRace00149800 creates Players -> ReplayManager -> Race001473F0 -> World0014CA28 -> InitCars0014A128 -> InitPlayers00149B28 -> Race::Init00147768 -> ghost/driver mode/audio/speech -> publish coordinator/change state. Race constructor already needs actual start/split/finish boundaries00147928. NativeBackend's fixture state4/camera/single car must not replace this chain.

World0014CA28 initializes three lists,40 OBBs,three DriveTargets, publishes pCurrentWorld, resets clock/consumes RNG, initializes counts/IDs/time scale/player positions, constructs TrafficTeleporter then effects/clans/skids/munitions/fern/smokeables/terrain emitters/cops/track graph. Required dependencies are not no-ops:

- OBB00218850 writes WorldObj only; do not zero geometry as an alleged original default. Its individual source ownership remains to be bound before implementation.
- InitClans001A8C98 lazily owns50hex-byte x40 pool; InitSkids001A8798 owns110hex x128, terrain texture bindings and debug state.
- InitMunitions001BB7A0 owns handler and SmokeBomb001BBDB0(480hex x14), Rocket001BC560(490hex x3), DroppedSpikes001BC108(490hex x8), creators and ROCKETEXPLD group.
- InitFernFlatteners001D6698 is confirmed empty; InitFernShakers001D6D30 owns pool1Chex x32, two infos/animations/groups.
- SpawnSmokeables0021A4E8 depends on loaded scenery; only actual zero spawners is naturally empty.
- InitTerrainEffectEmitters0021CE28 owns738hex object with7x33 groups; InitCops00222490 owns2914hex manager when nMaxCops>0. First event requires it.
- InitTrackGraph002480D0 always creates5470hex graph from racing routes/signposts via00247218 (100 vertices/400 edges plus roadblock/speedtrap pools); it is not optional without police.

These constructor/pool/graph dependencies are captured, not implemented by the registration tests. No World or race may be advertised ready before these required resources and real Car/controller/movement/collision/update paths exist.

## Validation and exact next action

Latest scoped build and12-test command passed12.73s as recorded in ../task0098/first-event-launch.md. Normal Godot DLL build passed. Tools/pins unchanged. No Debug original-runtime, full scene/lifetime or FPS acceptance; current PCSX2 process/listener probes were empty. Current task stays active.

Continue TASK-0099 by recovering real routes/TrackGraph and original World construction/resource ownership, then Car/Player/controller initialization and original ordered World timestep. Reuse existing physics/contact/render assets through one session, never a separate world per car. Production frontend/private-profile/high-score/flow services remain unbound from TASK-0098. A session must own parsed tree/state/catalog/memory throughout callbacks and cancel/fence resources before disposal; a RaceEvent ByteView must not outlive menu-local progress storage. Never certify finish by modifying outcome fields.

Port memory clone baseline E-0099-M01 is measured separately in memory-baseline.md. It is not full-frame/FPS acceptance. Read-only footprint assertions cover zero persistent growth during table rebuild; write-barrier/dirty-page replacement and live World allocation/upload measurements are still open.
