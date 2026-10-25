# TASK-0091 - Expanded frontend inventory and corrections

Retail SLUS_203.62 US PS2, executable revision unknown, SHA-256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
Read-only Ghidra MCP and original asset bytes,2026-10-25. Not Verified.

## E-0091-G01 - Registration/document coverage

The initial38 registrations omit Select Car/Select Car2P/Select Track, pause,
endgame and lap-time callbacks. Coverage now has63 registration rows, including
two pad-specific Select Car2P documents. This is an evidence inventory, not a
claim that63 screens are implemented or exhaustive.

Car/track registration operands:00198BC8/00198DC0/00198E20 and001A09E8..001A0A5C.
Pause:00174B20..00174CB8. Endgame:001E1EEC and001E1F88..001E217C; most endgame
classes share dispatcher001DD278. Existing class-template destructor addresses
must not be mistaken for registration callbacks.

FRONTEND/FRONTB.BUN has41 authored documents; GLOBAL/INGAMEB.BUN adds35, including
pause, rewards/results/standings/best-times, objectives and replay. Raw captures
remain ignored under local/task0091/{race-agent,endgame-agent}. CSV operands are
registration/data evidence. In-game documents are not yet loaded in production.

## E-0091-G02 - Correct gameflow table origin

The structure begins at002FAEA0, with name/enter/exit/tick at offsets0/4/8/C.
Earlier naming started at enter002FAEA4 and shifted labels by one. Fresh memory
and pointed-string reads establish original GAMEFLOW_STATE_ values:

0 NONE,1 LOADING_FRONTEND,2 UNLOADING_FRONTEND,3 IN_FRONTEND,
4 LOADING_TRACK_COMMON,5 LOADING_TRACK_SPECIFIC,6 RACING,
7 UNLOADING_TRACK_SPECIFIC,8 UNLOADING_TRACK_COMMON,9 EXIT_DEMO_DISC.

0016CCA8's state3 gate permits original blueprint updates in frontend; the earlier
runtime freeze is corrected.001EE148 modifies primary Display map-style byte+2
in frontend state3. A fake Player count is unnecessary.00330738 is the Player
counter:001751D8 increments and00175BF0 decrements. Its construction is owned by
World/restart, not frontend startup.001EE148/001F3730 listings confirm the branch
and subsequent derivation of secondary map fields. Original English tip24CAC
and labels22345 etc disprove an earlier camera/POV interpretation of this option.

Captures: local/task0091/display-agent/002faea0.correct-state-table.json,
*.correct-state-name.json,001ee148.startup6.json,0016926c.startup6.json.
No original runtime comparison or Verified promotion.

## Confirmed implementation leads and open dependencies

Race Setup row/default/change evidence is in race-agent. Its shared Select exit
pushes NO history; Challenge pushes history. Car selector/render/unlocks and full
race parameter construction remain unrecovered. Preserve00184F68 as launch boundary.

NameEntry uses an authored3x14 grid and seven-character overwrite buffer; Back
commits. Legacy Username Options uses separate rotating-character logic and true
cancel rollback. Help uses per-screen original localized arrays and0.5s slide paths.
Pause/replay/endgame require explicit fixture inputs, not fabricated race results.
Raw NameEntry/Help evidence is under ignored local/task0091/name-agent.
