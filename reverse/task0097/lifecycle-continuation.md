# TASK-0096 downstream lifecycle audit continuation

E-0096-C01, investigation only; no TASK-0099..0102 implementation or completion
claim. Authority Debug NFSHP2.ELF SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`,
revision/date unknown. Fresh read-only MCP captures and hashed manifest remain
ignored under `local/task0096/career-continuation/`. Existing E-0097-G01/P01
remains the starting inventory; this adds precise finish/save dependencies.

## Confirmed police and World prerequisites

World0014CA28 calls InitCops00222490; creation002225B0 ->CreateCop00221B28
->tAICop002281A8. World timestep0014D9A8 invokes GlobalCopThink00222530 before
car Think/Drive/movement/collision and later Race::Update. Manager0021EF58 orders
speech, supervisor, pursuits, speed-trap/patroller management, spikes, pullovers,
cop dispatch and BeACop. Pursuit00223B08 depends on original barricades/landmarks/
routes/targets/primary cop/state/stoppability/roadblock/chopper systems.
CopDriver0019F980/CopMover0019E8A8 and MovementModeTransitions00228BC8 require
the actual hybrid COP/MELLOW movement, not the single-car tool backend.

CreatePullover002217A0 checks prior tickets before increment; EndPullover002200F0
either releases/reset routes or calls genuine Race::FinishCar, with distinct
BeACop outcomes. ShouldPursuitBeCalledOff002269A0 reads tires, point-to-point,
free-run, laps and remaining distance. No generic pursuit-distance substitute is
evidenced. Current source has predominantly Retail throwing shells and only two
bounded Retail police-control leaves, not a native Debug manager/pursuit runtime.

## Confirmed real outcome chain

Race::FinishCar001483F8 derives rank/time/reason from actual participants, calls
RaceEvent::RecordResults001FC6F8 and changes player control/finish behavior.
Coordinator0014BAF4 calls FrontendDatabase::TheRaceIsOver001858B8, which snapshots
options, records scores, awards real Player points, constructs TransientStats,
posts results and checks rewards. Original bodies include00185750/00185690 points,
00184E60 records,00185320 transient stats,002443C8 task rewards and00244D00 rewards.

PostResults001FCA30 separates rank0/2, packed time1/5/6, KO3 and tournament4.
Tournament uses its original rank-points table; KO stores elimination ranks.
ResultShouldBeDiscarded001FCCE8/MaybeDiscardResult001FD0B8 preserve better prior
state. Restart001FCBD0 restores aPrevState with distinct multitrack behavior;
quit001859F0, leave001859C0 and restart00185A50 are not interchangeable.
A port outcome-delivery guard must surround genuine state, not generate a won flag.

## Confirmed results and private save prerequisites

EndGameScreens::BuildScreenList001D9550 conditionally appends points/task/unlocked-
event/event-reward/BeACop/race/tournament/record/autosave/end-menu screens in order.
Special31/32 conditions are in ShouldSeeEventRewardScreen001D9208. Debug
Autosaver00241220 owns SavegameData and saver/tester/prompter; HandleEvent00241338
finishes only after device completion and dispatches its callback on SCREEN_TICK.
Endgame callback001D9AD8 advances the screen;001D9B10 installs it.

Current private store uses direct truncating writes; existing autosave is a
bounded Retail successful-device projection. Neither supplies Debug race outcome
idempotence or transactional replacement. Review semantic progression conversion,
especially Debug KO12 versus Retail16 bytes, before saving real results. No actual
outcome was fabricated or private profile modified during this investigation.

## Remaining implementation boundary

Required sequence remains selection/render/loading -> shared World/participants/
Race -> original police/pursuit/hybrid movement -> genuine finish/results ->
rewards/records/progression conversion -> ordered result/autosave UI and private
transactional persistence. Whole entries and virtual dependencies above remain
untranslated. No original numerical/visual comparison or Verified promotion.
