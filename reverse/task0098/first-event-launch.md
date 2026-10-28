# TASK-0098 first-event native launch boundary

Authority: Debug NFSHP2.ELF SHA-256 `815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`, revision unknown. Existing Retail assets/profile remain separately identified by `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`. Readonly Ghidra MCP and original configured assets only. Captures remain ignored under local/task0098/first-event-launch and local/task0096/first-event-launch; hashes are in evidence.json.

## Implemented native scope

E-0098-RP01: RaceParameters::InitWithDefaults001E9968, player defaults001EA330, display defaults001EA440; fourteen-slot DriverInfo add/set/get/find/eliminate/restore/count/start-position methods001E9DE0..001EA0B8; location001EA158/178; free-run001EA7A8; integer bRandom00253858. Native Memory storage remains independent of Godot. Original TheRaceParameters is00311F78; DriverInfoTable00312288 spans150hex bytes. Defaults clear310hex bytes, retain original default fields, initialize both settings blocks/display and consume shared bDefaultSeed for colors. Event entry does not re-zero them. SkipFE/language values are explicit port bindings. Player reset does not overwrite name bytes after copied NUL.

AddDriverInfo invokes GetSkinInfo(type,SIGNED skin,1) before copying24 bytes and reloads count afterward; full14 is a true no-op with no lookup. Padding and character pointer are copied, not reconstructed from selected visible fields. SetDriverInfo does not append/look up skins. Signed ranks/numbers and first-match lookup are retained, including negative rank behavior. Port bounds checks reject addresses outside decoded valid storage; they are not asserted original failure behavior.

E-0098-RP02: RideInfo::Init00185FA0, DriverInfo001867C0, GetRandomSkin00186610, GenerateRandomRideInfo001863C8, PickCharacterNumberThatIsntRacing00182610, PickCopCharacterNumber00183A08 and first-delivery SetupCarsForRace00183A58. Other events/AI and unassigned starting-position sorting00184390 retain explicit exceptions. Actual64-car geometry/class/skin/location fields are decoded; generation follows original two-pass cumulative location weights. Every non-cop skin draw still occurs even if SetupCars later overwrites traffic skin to-1. Body gender selection is deterministic; cop class ties retain first encountered type, and cop skin is fixed1. bSetRandomSeed00253790 writes then ADVANCES RNG once with bRandom(1); assigning the seed alone is wrong.

E-0098-RP03: port::race::LaunchCatalog adapts real original GLOBAL catalogs. Debug TrackInfo strideA0 differs from RetailA4; only reviewed fields are decoded, never a whole-row memcpy to Debug. Retail Loader00153608 and GetTrackInfo00153420 use one-based indexing/name presence and clear track54. TrackNumber+41, Point2Point+43 and CopCharacterNameHash+54 are confirmed by Retail consumers001846A0/00184F68/00185640. Character chunk34300 is10 rows of44hex bytes; GetCharacterInfo001D61C8 uses array INDEX, hash lookup001D6140 returns the record's CharacterNumber. First track13 maps to FatCop through the real hash, without hardcoded participant choices. GetSkinInfo00186330 defaults to FIRST skin table row, not the selected car's default. Provider parsing/reserving happens before one persistent arena allocation; callbacks and Memory must outlive consumers.

E-0098-RP04: first-delivery EventTree::SetRace001FA378, native RaceEvent metadata/snapshots/results, EventResult::Reset001FC648, ResetResults001FC678, EventTimeState Reset001FB540/GetStatus001FB560, scoped ResetState001FD708/GetRaceIndex001FD118 and ordinary-event prediction001FC1E8. Actual32-byte name bytes are preserved. Old progress is captured before StartRace resets the live time state; prior snapshot tail/previous-state buffers are not fabricated. Time0/7FFFFFFF is UNTRIED, comparisons are signed32. Special31/KO/tournament/rank dependencies remain explicit gaps. Event31 unlock prediction is not approximated.

E-0098-RP05: first-delivery FrontendDatabase::StartRace00183378 now executes state reset -> indexed track -> bonus prediction -> original AI caps/track correction -> SetupCars -> profile settings -> records for actual players -> original empty ChooseHudTextures001EA7A0 -> first-type5 multitrack no-write behavior -> EventTreeRace -> GameFlow request -> aborted flag clear. It requests UNLOADING_FRONTEND when already in frontend, otherwise LOADING_TRACK_COMMON, with NONE continuation. It never directly launches the existing single-car scene. Missing profile/record/flow services throw before this slice begins; runtime service exceptions still require an owning candidate/session rollback policy.

## Real first event and tested boundary

Hot Pursuit tableindex0 is eventnumber1, Elise Delivery, type5/class5. It allows car41 and original available skins; packed trackA190B00D selects13, forward/clear/time3/laps2/traffic100percent/oncoming80percent. Packed objective1200000 is not seconds. Zero AI racers coexists with6 cop driver entries and6 traffic entries. Tickets1, roadblocks0, smokeables18, spikes1, helicopter0 and original damage/boost fields are retained. First reward is one CAR reward(type0,car41,skin1), not zero rewards.

race_parameters tests read the SHA-bound ELF and real GLOBAL/FRONTB, use the original global parameter address, event availability and selected ride, run native SetRace/StartRace, verify13 original-input driver records and World table consumption, and check30 native startup reentries. Each reentry retains13 records/old-state snapshot and releases temporary24-byte logical scratch storage. The synthetic single-traffic-type test requires1003 shared RNG advances: one seed warm-up plus two draws per attempt over1+5*100 traffic attempts. This catches both skipped-skin draws and premature duplicate acceptance.

Localization/formatting, profile/record values and GameFlow execution in this test are CLEARLY LABELLED HOST FIXTURES. Car prefixes used for shared World table integration are also fixtures, not constructed racing cars. These tests do not prove that a production page, original loading coordinator, running World, pursuit, finish, results/save,30 full retries or full resource lifetime acceptance works. No win/loss field was used to certify a race.

## Counterevidence and corrections

- An initial bounded first-event SetRace guard assumed no reward. Real asset integration failed; existing coverage matrix proves one reward, and reward metadata/car-name translation was implemented. No fake reward was removed.
- Decompiler rendered minimum car-class comparison as unsigned64. Fresh0018420C listing is signed SLT; code uses signed classes. Decompiler types are not authority over instructions.
- Seed assignment alone would omit bSetRandomSeed's original warm-up; fresh00253790 confirms the draw and tests check exact advancement.
- Initial new test compile errors (missing namespace qualification) were fixed; an incidental ctest run against the preceding executable after that failed build is not counted as validation.
- A logical-memory reentry test initially forgot the existing16-byte unmapped guard after allocations. Corrected expected address gap32; production allocator was unchanged.
- Initial missing bMath declaration was fixed by including the existing canonical header. Existing unrelated warning sites were not changed.

## Validation and continuation

CMake3.29.2, GCC13.2.0, Python3.14.4, Godot4.7.2 and godot-cpp e4b7c25e721ce3435a029087e3917a30aa73f06b checked on2026-10-28. Readonly MCP works; no PCSX2 process or21512/28011 listener was found. This blocks original comparison, not independent implementation.

- `cmake --build --preset native-debug --target race_parameters_tests world_participants_tests ultimate_event_tests ultimate_selection_tests ultimate_selection_camera_tests ultimate_selection_widgets_tests ultimate_selection_lifecycle_tests selection_locked_tests selection_page_tests eventtree_tests driving_init_tests driving_solver_tests --parallel 4`: passed.
- `ctest --preset native-debug -R '^(race_parameters|world_participants|ultimate_event|ultimate_selection|ultimate_selection_camera|ultimate_selection_widgets|ultimate_selection_lifecycle|selection_locked|selection_page|eventtree|driving_init|driving_solver)$' --output-on-failure`:12/12 passed12.73s, configured original ELF/assets exercised.
- `cmake --build --preset godot-debug --target ps2_port --parallel 4`: passed latest normal DLL.
- `py -3 reverse/scripts/audit_ultimate_racer.py --check`: passed64 event records/32 Hot Pursuit; five KO version-storage mismatches remain isolated.

Current implementation proceeds to TASK-0099 shared World constructors/real cars and scheduling under the user's first-event priority. Formal TASK-0098 frontend/compositor, actual private-profile settings/record adaptation and production transition bindings remain open. No Verified, running-race, original comparison or1080p60 performance claim. No commit/staging/raw proprietary export.

## Source review follow-up

Independent readonly source/listing review found two live-read order differences in the bounded SetupCarsForRace projection: selected ride was resolved before character selection, and location was cached outside traffic generation. Both are corrected to original order. Callback probes now change GameType during character lookup and TrackNumber after the first traffic skin lookup, asserting that later ride/location/cop queries observe the correct live values. Latest race_parameters test passed1/1 in1.72s after the changes; final rebuild/regression result is recorded in TASK-0099. No behavior inference from immutable fixture equivalence is substituted for original read order.

Final asset identity audit compares ALL64 original serialized car codes/types to the verbatim Debug CarType enum from MCP. All64 enum names exactly equal CARTYPE_ plus the corresponding asset code, and serialized type IDs match those identities. This is explicit semantic correspondence, not ordinal-only compatibility. The private car-identity-audit.json is hash-indexed. Final real-participant tests also assert generated traffic/cop types belong to their original usage categories.
