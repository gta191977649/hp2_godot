# TASK-0098 production entry prerequisites,2026-10-28

E-0098-E01. Behavior authority: Debug NFSHP2.ELF, SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`;
revision/date unknown. Retail frontend registration/assets remain separately bound
to SLUS_203.62 SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
Only read-only Ghidra MCP and configured original assets were used.

## Confirmed entry orchestration

Debug tTreeDetails::StartEvent001F2BD8 first writes
Fred.ChallengeScreenData.mRaceMode=1, TheRaceParameters.NFSRaceMode=1 and
Fred.nCurrentEvent=LastKnownEvent->nNumber, then calls EventTree::SetRace and
current_leaf->BuildUnlockList. Listing001F2C10/20/38 fixes the store order;
RaceParameters type information confirms NFSRaceMode offset254hex. The original
race-mode enum identifies1 as RACE_MODE_NFS_EXTREME.

IsMultiTrack001FC5C8 accepts only event types3/4. For those events, StartEvent
reads GetStatus AGAIN after SetRace/BuildUnlockList. INPROGRESS2 takes a separate
continuation route: Port0 pushes View Event Tree, then ContinueMultiTrackEvent;
other ports return without changing mode, availability or page. It must not
reuse the status read by the initial selection gate. Other routes set GameType8
for tree0 or9 otherwise, MakeEventCarsAvailable for both players, then
ChangeToScreen("Select Car",1). An ordinary event's entry has no Port0 gate.

HandleMessages001F2D20 only starts on MENUITEM_SELECT1 when signed status>0,
writing sound16hex before StartEvent; locked/nonpositive status writes17hex.
Messages6..9 write sound0; other messages leave it unchanged. In particular,
SCREEN_START_BUTTON13 does not alias SELECT here. Listing confirms signed
comparisons, immediate sounds and the tail call. The existing Retail runtime's
START alias is NOT changed on this evidence alone; the production Debug route
must use its own message contract and reviewed version adapter.

The C++ implementation lives in original eventtree.cpp as hp2::debug::tTreeDetails.
The port declaration header event_entry.hpp and EventEntryServices explicitly
bind globals/current leaf without claiming PS2 layout. ScreenCallbackData.Port
is a decoded carrier field; its host position is not the original+4 ABI.

## Inventory and validation baseline

E-0091-G01's25 missing car/track/pause/endgame registration operands are now in
the63-row runtime registry. An exact class/document lookup disambiguates the two
Select Car2P documents. Registration does not implement any page lifecycle.

`py -3 reverse/scripts/audit_frontend_closure.py --check` checks the runtime
operands against the independent existing registration ledger and regenerates
the expected public closure-routes.csv in memory. It reads original FRONTB and
INGAMEB screen headers through the configured asset root:63 registrations,
76 documents,90 route/unresolved-document rows. All registered document hashes
resolve;27 authored documents still lack a registration/caller classification.
This is not exhaustive reachable-state coverage. All final window/original
acceptance fields deliberately remain pending. Retail handler addresses in this
matrix are not substituted for Debug behavior addresses.

ultimate_event tests cover all seven event types, both tree modes and ports,
nonselect messages, locked/negative status, original call/store order, both
multitrack continuation routes and a status that changes between reads.
race_parameters tests bind the new entry to actual original first-event
SetRace/availability/launch data. Unlock-list and page/history callbacks remain
labelled host fixtures, as do existing text/profile/flow test services.

## Unknowns and required continuation

Production FrontEndRuntime still throws at Retail001F6A28. No actual page,
matched original window/media, running race, persistence, performance or Verified
acceptance follows from this checkpoint. BuildUnlockList, original profile-state
and unlock semantics, full multitrack continuation, real page loader/input/audio
leases and the shared physical colour/depth executor still require production
bindings/recovery. A service failure can leave the original-order writes applied;
the future owning candidate/session must provide rollback before publication.

Current task remains TASK-0098. Next implement the real shared colour/depth
submission executor from E-0098-F03/F04 and bind SelectionPage resources; then
wire this Debug entry through a reviewed Retail event/profile adapter and accept
Event -> Select Car -> cancel -> Event -> reentry in the actual Godot window.
Do not replace these services with the callbacks used in unit tests. TASK-0099
resumes only after that production roundtrip milestone.

Read-only MCP responded on this machine; no running PCSX2 or listener21512/28011
was observed. A same-SHA Debug scenario and working paused-state/trace endpoint
remain prerequisites for original comparison. Their absence does not establish
equivalence and does not block independent implementation.

Raw captures remain ignored under local/task0098/production-entry, with hashes
in evidence.json. No proprietary data, raw listings or images are exported here.

Final16 regressions passed29.15s. Timer declaration global namespace qualification
is also covered by include-order compilation. Normal Godot output relink was
blocked by an in-use DLL; an isolated link from the exact newly built objects and
generated linker arguments passed. No running-user DLL replacement or window pass.
