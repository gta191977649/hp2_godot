# TASK-0092 - Jukebox state projection

Target: Retail SLUS_203.62 US PS2, executable revision unknown, SHA-256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
Read-only Ghidra MCP, 2026-10-25 (Australia/Sydney). No database edits.
Status: static recovery with bounded PC implementation; not Verified.

## Evidence and reproduction

Run `py -3 reverse/scripts/inspect_jukebox_screen.py --elf
(Get-Content config/ps2_elf.local.json | ConvertFrom-Json).elf` in PowerShell.
Ignored `local/task0091/capture/manifest.json` records queries and response hashes.
Three MCP memory ranges match file-backed bytes in the SHA-bound ELF: the complete
Jukebox code region 001F02D8..001F121F, static table 002B9D30..002B9D97 and
widget hashes 0031AE00..0031AE77. No raw code or original assets are checked in.

## Confirmed

- **E-0092-J01**: 001F1220 allocates 0x240 and calls 001F02D8. Constructor copies
  the 0xB0 AudioOptions block at 002FC0E4 to instance+70. It clears list focus+6C,
  cursor+110 and viewport+120, then calls 001EF240 and 001F0478. 001EF240 groups
  nonempty metadata field3 by exact comparison, in first-seen order, preserving
  original song order; it writes each song's group byte at AudioOptions+1F+4*i.
  Original storage has six 0x2C group records (20-byte label, count, 20 indices).
  001EEF60 enumerates at most 32 songs into AudioOptions+1C, reading the initial
  per-song mode through 001AEFD0. The caller supplies the complete audio snapshot.
- **E-0092-J02**: 001F0478 advances/decrements first row until cursor fits the
  five-row window. Row mode is forced to zero when its category bit is disabled.
  Only the focused song has row arrows; category arrows require category focus.
  Up requires list focus and nonzero first row; Down requires list focus, count>4
  and first row != count-5. Missing rows are hidden. The host projection exports
  these decisions; it does not yet mutate production widgets or bind colours.
- **E-0092-J03**: jump table 002B9D50 has 18 entries indexed by message-1.
  Messages1/10/13 clear sound; 6/7 adjust left/right (sound1/2), cycling category
  or song mode0..5. Category changes reset cursor. Message15 toggles category bit
  (sound5); disabling clears list focus, cursor and viewport. Down from category
  focus enters an enabled list and schedules preview, or reports sound6 when
  disabled. Song navigation is bounded, with sound3/4 on movement and zero at
  the last row. Up from the first song leaves list focus, calls0027D868 and
  preserves the incoming sound. Movement schedules preview at the current tick.
  Message18 uses strict elapsed>0.5 for pending preview and elapsed>30 with no
  active playback for retry. 001F0CC8 resolves the grouped song and invokes
  0027D6A8 with its cartridge/category pair and auto-mode0. Message17 is Back.

## Implementation boundaries

The free address-anchored overloads in options.cpp model grouping, navigation,
row decisions and preview requests with explicit port-owned state/services.
Existing throwing whole-entry class templates remain unchanged. Missing preview,
stop or Back services throw at their original dependency addresses. Invalid
inputs exceeding original array extents fail explicitly; this is a host boundary,
not a claim about original malformed-input handling.

Back's existing E-0017-B06 chain remains separate: conditional Autosave history,
snapshot commit, mode5 filter, selected enabled/nonzero song resume with auto-mode1,
otherwise0027D640(1). The new projection delegates that chain to an explicit owner;
it does not claim to implement persistence or resume merely by calling a callback.

## Counterevidence and hypotheses

The initial MCP failure was not an unavailable Retail program. Current database
marks the initial 001F0D10 bytes undefined, but contains interior instructions,
other Jukebox functions and the jump table. Read-only memory/listing access is
sufficient for this bounded recovery. No function creation or analysis was run.

No new behavioral hypotheses are used. Port field names describe host state and
do not claim recovered original identifiers or PS2 ABI layouts.

## Unresolved

Production widget/localization/colour binding, metadata adaptation, full audio
global initialization, preview audio transport and natural completion, current-song
publication, Back filter/resume/Autosave sequencing, production menu routing and
matched original-runtime captures remain open under TASK-0092. Caller-provided
elapsed time/playback-active inputs are not a recovered whole timer/audio driver.
PC tests do not establish original equivalence or complete frontend functionality.

## Production continuation - E-0092-J04..J07

Read-only MCP captures under local/task0091/{audio-agent,popup-agent,options-agent}
and direct responses00143E70/00143F88/00140CA0 establish:

- **J04**: original metadata fields0/1/3 feed artist/title/category; row hash blocks
  are0031AE00/18/30/48/60, statuses002B9D30, formats00331690 (`%s`) and00331698
  (`%s (%s)`). Existing recovered highlight/disabled colours are reused. Text setter
  00140E20 preserves its capacity/truncation and clears string ID on change;00140CA0
  preserves alpha. Runtime Jukebox document95B2B627 is current FRONTB index34.
  The prior27 claim in media notes is counterevidence, not a binding.
- **J05**:001EF1C8/001EEF60 initialize the0xB0 block, seven volumes in category
  order5,4,0,1,2,3,6,32 signed-byte song pairs, preferred+A4=5,output+AC=1,
  genre+AE=0,mask+AF=FF.001AFAB8/001AFA28 exact default bits by category0..6:
  3EC28F5C,3F0CCCCD,3EA8F5C3,3E9EB852,3F59999A,3EE66666,3F400000.
  The frontend filter is column0 of six three-byte rows at002B6300 and permits
  modes3/4/5. Earlier exclusion of3 is corrected. Original initial data has no3,
  so the existing natural selection5->7 fixture remains valid.
- **J06**: Audio001EFB80 item52E039FB commits/applies the full audio snapshot
  then exits to JukeBox Music. Back001F0D5C..0ED8 compares/commits the full block,
  conditionally pushes Autosave, applies mode5 filter and resumes snapshot+A4
  with auto-mode1 if genre enabled and mode nonzero. Otherwise0027D640(1) stops
  current audio and selects NEXT enabled, not current preview.0027D6A8(...,0)
  previews without natural advance;0027D868 stops and clears auto-mode.0027DFF8
  advances on completion only in auto-mode1. Preferred+A4 is not the current
  preview;0027E530 publishes current cartridge/song at00313854/58 separately.
- **J07**: Autosave00247D00/00247B48/00247C68 delegates successful save polling
  to00246138. Strict probe>1s, saving>2s, success>2s, with body IDs24DE5/24DDC/
  24DDD and final history pop00170320(1). Disk I/O is the existing port adapter at
  00245B00, retaining original profile stamping/checksum and protecting read input.
  00245E30 uses authored memory-card popup hash0CE3F7CE (FRONTB35),title249F4,
  zero buttons,flags9,pad-1. All input is consumed.00144BA0/00144AE8 wrap class
  5D6DD123 by character capacity, spaces/tabs and forced`@`, preserving scanner
  restart behavior; trailing lines hide. No font-width wrapper substitutes it.

The earlier unresolved production-binding items above describe the pre-integration
state. Runtime metadata/widgets, preview/stop/completion, Back/filter/resume and
successful isolated-host Autosave are now implemented and tested. Missing PS2 async
hardware/error-device states and original comparison remain unresolved. Card-device
selection, capacity and overwrite branches are explicitly replaced by the existing
private host-save adapter's successful path, not claimed as PS2 card emulation.

Validation: frontend_jukebox native real-asset test passes, including real Audio
route, mode3 edit, preview0, Back preferred5, completion7, isolated save and return.
Godot4.7.2 Vulkan screen34 capture was inspected under ignored local/task0091/.
