# TASK-0047 - Alpha ZZDATA extraction

- Status: completed
- Objective: extract the user-supplied Alpha ZDIR.BIN / ZZDATA.BIN into the requested external GAME directory.
- Scope: offline archive analysis, safe extraction, byte/hash validation; no game behavior changes.
- Original PS2 addresses: not applicable to asset-only analysis; executable revision unknown.
- Dependencies: Python 3.14.4, original archive and index supplied by user.
- Preserve all pre-existing working-tree changes.

## Acceptance criteria

- [x] Validate every index range and preserve entry identity.
- [x] Extract all entries, decompress supported wrappers and report unresolved paths explicitly.
- [x] Validate extracted sizes/hashes and retain an external manifest.
- [x] Record evidence, commands and continuation.

## Current findings

E-0047-001: ZDIR.BIN is 4860 bytes, exactly 405 little-endian triples. Interpreting each as hash, 2048-byte sector, byte size places all entries within the 3116437504-byte ZZDATA.BIN. Names are absent from the index. First nonempty entry starts with COMP.

## Unresolved questions

Compression framing and original paths remain under investigation. No executable behavior or equivalence claim.

## Next steps

Inspect compression and establish hash/name correspondences from original data.

## Related Git commits

None.

## Completion evidence - 2026-10-17

405 records extracted, 81 COMP wrappers decoded, 3251095824 output bytes; 92 unresolved paths retained without guessing, 3 duplicate-path records preserved. All written files reread and SHA-256 checked. Input hashes and decoding/name evidence E-0047-001..004: reverse/structures/alpha-zzdata-task0047.md. Manifest: user-requested external GAME/extraction_manifest.json. Exact successful command: `py -3 -u tools/asset_extractor/zzdata_archive.py --source D:/ps2_game/GAME/ALPHA_VER/NFSHP2 --output D:/ps2_game/GAME/ALPHA_VER/GAME --names-root D:/ps2_game/GAME/ZZDATA --resume` (exit 0). First run exposed duplicate paths and stopped safely; ordinal duplicate preservation and byte-identical resume added. Synthetic overlapping-copy, malformed-size/distance, duplicate-entry and existing-output protection tests: `py -3 -m unittest tools.asset_extractor.test_zzdata_archive`, 2 passed on Python 3.14.4. No runtime build needed; no game source change. No commit or Verified claim.

Continuation: extraction objective complete. Optional TASK-0047 follow-up is recovering the 92 original paths using Alpha evidence; consult the external manifest and retained hash names first. Earlier exploratory findings/questions above are superseded by the linked completion record.
