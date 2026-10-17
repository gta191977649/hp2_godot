# Alpha ZDIR / ZZDATA extraction - TASK-0047

## Confirmed observations

- E-0047-001: the user-supplied Alpha index contains 405 12-byte little-endian records (hash, sector, size). Sector units of 2048 place every record within the 3116437504-byte archive. All nonempty ranges are read exactly; zero-length entries are preserved.
- E-0047-002: read-only Ghidra Debug NFSHP2.ELF, SHA-256 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b, LZDecompress(unsigned char *, unsigned char *) at 0x001B1658 (404 bytes) supplies the COMP decoding rule. Header +0x0C is packed length; +6 selects uncompressed-copy mode 1. Otherwise flags are little-endian 16-bit, consumed low bit first; zero means literal, one means two-byte distance/length. Distance is ((a & 0xF0) << 4) | b; length is (a & 15) + 3; overlapping copies proceed forward. All 81 Alpha COMP members consumed their packed extent and produced their +8 declared size. This confirms applicability to these members, not general Alpha executable equivalence. Raw decompile/listing queries are ignored under local/task0047/.
- E-0047-003: Debug bStringHash(char const *) at 0x0024C8B0 starts at 0xFFFFFFFF and multiplies by 33 before adding each byte. Uppercase backslash-separated paths from the existing original asset tree match 310 distinct Alpha index hashes. 313 records use unique reference-tree path candidates; 92 remain unresolved. Hash matching alone does not establish original Alpha path spelling. Three repeated-path records are preserved under ordinal _DUPLICATES paths. No existing files were overwritten; the resumed first partial run reused only identical bytes.
- E-0047-004: 405 output members total 3251095824 bytes. Every output was read back and SHA-256 checked. Manifest includes each record offset, stored/decoded size and SHA-256, candidate path basis, and whole-input digests. Output remains outside Git at the user-selected location.

## Input identity

- Version: user-labelled ALPHA_VER; exact executable revision/date unknown. No Alpha ELF was analyzed or modified.
- ZDIR.BIN SHA-256: 6efeb466c07f41007d18dc3ddbc0a74ddca590a5b26102c3d0017da05096c040
- ZZDATA.BIN SHA-256: 2751eb1b5b76fd117d3b672ed8d26d76ad7c37c79f1ae5e4350cf22c9eba80ff

## Limits and continuation

Original Alpha paths for 92 records remain unknown. No original runtime comparison; no Verified promotion. This is offline port-side tooling, not a recovered game implementation; no symbols index changes. COMP wrappers are removed even when the candidate filename retains .LZC. Archive-level extraction does not convert texture, geometry or audio formats.

Reproduce into a fresh external directory with `py -3 tools/asset_extractor/zzdata_archive.py --source <original-NFSHP2-dir> --output <new-output-dir> --names-root <original-reference-ZZDATA-dir>`. Existing outputs are refused unless --resume is requested; resume requires byte identity. Validation: `py -3 -m unittest tools.asset_extractor.test_zzdata_archive` (2 passed, Python 3.14.4). Native/Godot builds omitted because no runtime source changed.
