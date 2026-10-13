# TASK-0022 detail text recovery

Binary: SLUS_203.62, boot VER 1.00, executable revision unknown, SHA-256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
Read-only GhidraMCP / Ghidra 12.0.4, 2026-10-13. No database mutations or original
execution comparison. Per-function records under reverse/functions/<binary-id>/frontend
retain E-0022-D01 through D07; raw outputs remain ignored local/task0022/detail-core.

## Confirmed static findings

- FUN_001F6B70 dispatches input; it does not generate Status or Reward text.
  Message 1 checks status, emits sound 0x16 for unlocked or 0x17 for locked, and
  calls FUN_001F6A28 only for the unlocked arm. Messages 6 through 9 emit sound zero.
- FUN_001F6770 builds document 17 text from localized record+0xB4, expanded by
  FUN_001F5F88. It also sets the title hash 0xF601F2D4 to the track's localized ID,
  using the current track row or first-row fallback, low byte track ID, bit 9 direction.
  Text is drawn against hashes 0x7A31036B and 0x7AD2801E by FUN_001451E0.
- FUN_001F8890 populates overlay hashes 0x87CFD6C6 (Status), 0x35B77247 (Reward),
  and 0xCEA3B7C2 (localized 0x111AF format with event number). These are screen 39
  overlay texts; they are separate from the document 17 producer.
- FUN_001F60E8 uses IDs 0xF230..0xF234. Status 0/1 copies text; status 2 formats
  progress and total; status 3/4 formats the placing or timer result string.
- FUN_001F6220 uses first reward row in chunk 0x34A04. No row uses localized
  0x101D8. Kind 0 resolves car index byte 1; kind 1 resolves track ID byte 1 and
  direction byte 2. Names come from original databases through injected callbacks.
- Goal 3 slice progress counts eight nonzero nibbles; its placing is the low nibble.
  Goal 4 counts completed rounds by total eight-byte points divided by the sum of
  0A 06 04 03 02 01 00 00 through opponent-count index; its placing ranks byte zero
  against the other seven, keeping earlier index first on ties. Both status arms are
  translated, replacing the earlier NotImplemented gap.
- Result localization table has nine IDs 0xEE48..0xEE50, for placing 0..8.
- Timer ticks are 1/4000 second. Original formatting preserves locale-specific
  separators, leading spaces, fixed-width fields, zero and INT_MAX sentinels, and
  sign shift/truncation. Event callers pass flags 0. Timer functions are colocated
  with the event helpers as a port-side dependency aggregation; their source path
  is not established by this investigation.
- The detail-token formatter really contains placeholder strings in this ELF:
  `zz:zz`, `ff:ggg`, `events x,y,z are Unlocked!`, `event x is unlocked`, and
  `TRACK_NAME_HERE`. Preserve them where the original emits them. `$NUM_LAPS`,
  `$NUM_OPPONENTS`, `$NUM_RACES`, and time thresholds use the recovered operands.

## Native scope and limitations

The Godot-free core exposes Sub_001F60E8, Sub_001F6220, Sub_001F5F88, Sub_00200E78,
Sub_001A3B98, and the tree's Sub_002008F8, Sub_00200938, Sub_002012D8. New record
fields +0x3C/+0x3E/+0x3F/+0x6C/+0x8C/+0xB4 and table chunks 0x34A02/0x34A04 are
parsed from original assets. Callback seams allow the runtime to supply the original
localization/car/track databases without coupling recovered code to Godot.

The localized format seam supports `%d`, `%i`, `%s`, and `%%`; other reached
conversions throw hp2::NotImplemented. Out-of-range proprietary table indices are
host validation errors or explicit missing-scope throws, not asserted original behavior.
INT_MIN timer negation remains explicit NotImplemented. The original unchecked
memory behavior of malformed data is not reproduced as a host memory error.

Event 31's completion globals and event 32's cross-tree completion gate remain the
prior external dependency gaps. No guessed gate values were added. Race settings
and launch side effects in FUN_001F6770/FUN_001F6A28 require separate implementation.

## Validation

`cmake --build --preset native-debug --parallel 4` passed.
`ctest --preset native-debug -R eventtree --output-on-failure` passed 1/1.
GCC 13.2.0, CMake 3.29.2; Windows MinGW native-debug preset.
The eventtree test runs synthetic checks before optional original-asset checks.
This machine also parsed both shipped trees successfully in the same test.

Synthetic cases cover nibble progress/empty/completed/threshold boundaries, points
round count/rank, localized status operands and result IDs, token recognition and
512-byte bound, timer punctuation/padding/sign/sentinels/hours, track/reward accessor
count bounds and table indexing, reward dependency failure. Tests establish native
implementation consistency only. None of these functions is Verified.

## Continuation

Read TASK-0022 and this report, integrate these APIs into the actual four-document
cluster, supply original car/track catalogs, test real Status/Reward/title output,
and compare a defined original-execution scope under TASK-0023.
