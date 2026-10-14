# Original car and skin catalog projection

- TASK-0033; stable evidence E-0033-B15;2026-10-14.
- SLUS_203.62, revision unknown; SHA-2567335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
- Sources: authorized GLOBAL/GLOBALB.BUN bytes and read-only00187F98/001B0560 captures.
- PC API is port::vehicle::Catalog; no original ABI/type name claimed.

## Confirmed layout

GLOBALB chunk34600 header0x86B80 size0x15808; aligned car rows begin0x86B90,
64 rows of0x560 bytes. Each retains original ordinal ID, code+0x00, prefix+0x20,
geometry resource text+0x60.001B0560 consumes+0x60 for the indexed request.
Original resource names have LZC extension; this viewer explicitly chooses BIN sibling.

Chunk34601 header0x9C390 size0x3AC0; unaligned payload begins0x9C398,
235 rows of0x40 bytes. Car ID+0, skin ID+4, original name+8 (32-byte field),
unknown words+0x28/+0x30 and original material association+0x2C are preserved.
Membership/order and same-name records are retained by original IDs; no custom RGB.

Car22 SPIDERHP retains prefixCHALLENGE and CARS/CHALLENGE/GEOMETRY.LZC resource.
Cars39/40 (911TURBO/911TURBOHP) share the original geometry path. Cars45/46 retain
missing BIN diagnostics; their7 original skin associations remain listed.

## Validation and limits

Catalog64/235 and all memberships pass native tests. Full available-skin load probe:
228 success,7 unavailable,0 failed. Each binds original geometry, texture hashes,
material descriptors and finite adjusted UVs. Reproduce with vehicle_catalog_tests
--all-skins using the global asset-root configuration. Source and decoded bytes stay
outside Git. Host tests do not establish original-device equivalence.
