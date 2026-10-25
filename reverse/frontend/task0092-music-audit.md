# TASK-0092 corrective music audit -2026-10-26

Authority: Retail SLUS_203.62 US PS2, revision unknown; configured ELF SHA-256
rechecked as7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
Fresh read-only Ghidra MCP captures are ignored in local/task0092-audit.
No Ghidra mutations, computer-use or original-runtime comparison.

## Confirmed - E-0092-J08

0016F000 calls001AEAB0 only in its intro-bypass branch. In the normal movie
branch it starts the movie and does not start BGM.0016F1F0 calls001AEAB0 when
the last movie finishes naturally, provided DAT_002EFFE0==0 (non-demo).
0016EEF8 manually skipping the second movie transitions to static Title and
resets animation/timer WITHOUT calling001AEAB0.00216460 main-menu constructor
explicitly calls001AEAB0. Thus static Title is not universally silent: natural
movie completion/bypass and manual second-movie skip have different music states.

001AEAB0 explicitly selects a tune only if playback/pending state is inactive;
001AEA40 gives the first frontend preference5.0027DFF8 services transport and
advances a naturally finished tune only when auto-mode+2278==1.0027D868 clears
that auto mode.001F0CC8 selects a Jukebox preview with auto0.001F0D10 tick
001F112C..001F11FC requests delayed preview after strictly0.5 seconds and, after
strictly30 seconds without active/pending playback, requests preview again.
That30-second rule is original code, not a generic frontend music restart policy.
Jukebox base constructor001F4C98 and concrete001F02D8 do not call001AEAB0.
Opening Jukebox therefore must not invent a new background-music start event.

## Confirmed defect and correction

TASK-0092 added per-frame Godot music polling for live gain/output/preview
requests. FrontEndRuntime::frontend_music(false) also called scheduler.enter on
an idle automatic scheduler. This turned a device poll into an unsolicited
001AEAB0 call: intro and manually skipped Title could start BGM. Existing tests
incorrectly expected a fresh direct-Jukebox poll to start song5; they did not test
silence before an explicit original start call. Earlier PC pass reports do not
establish this lifecycle was correct; that implicit-start assumption is retired.

Polls now only drain pending requests and report gain/output. Explicit start uses
a separate bound start_frontend_music call from the existing original startup/
main-menu call sites. Unsolicited completion while stopped or replacing a pending
track cannot advance the playlist. Preview-stop and auto0 completion stay silent
under subsequent polls. Existing original30-second Jukebox retry is retained.

## Validation and limits

Native build targets frontend_jukebox_tests/frontend_runtime_tests/movieplayer_tests
and Godot-debug ps2_port build passed. Native Jukebox/startup/runtime regressions
record exact results in TASK-0092 and PROGRESS_LOG. Added intro/skipped-Title/idle
completion/direct-Jukebox silence and repeated-poll-after-stop checks. Existing
movieplayer tests separately cover natural-end versus manual-skip start flags.

Godot4.7.2 headless `--path godot --script D:/dev/hp2_godot/reverse/scripts/check_frontend_music_gate.gd`
passed FRONTEND_MUSIC_GATE_OK: silence before explicit start, song5 then completion7.
Windowed same command without--headless and with `-- --host-audio --screen=22 --scale=1`
passed FRONTEND_HOST_MUSIC_GATE_OK: actual host AudioStreamPlayer stays stopped
through Title polling, then plays on explicit start. Both exit0 without errors.
These are port regressions, not PCSX2 equivalence. Full SPU async/error behavior
and whole-Jukebox parity remain unverified; no Verified promotion.
