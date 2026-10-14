# E-0033-B16 — Viewer swatch provenance boundary

Target SLUS_203.62, revision unknown, SHA-256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
Read-only Ghidra MCP inspect/decompile00188180 on2026-10-14 confirms a0x40-byte row search
matching car at+0 and skin at+4, with optional first-row fallback. No color interpretation
is established by this function. Viewer membership remains strict and never takes that fallback.

Player GLOBAL/GLOBALB.BUN34601 payload0x9C398, first three rows: car7/skin2 Silver
Field28=0x20,material2C=0x93C0EAF6,Field30=0xC2FA99BC; car7/skin1 Red Field28=4,
material2C=0xFC329CF4,Field30=0x95CDDF80; car7/skin3 Black Field28=1,
material2C=0xD22FF4B6,Field30=0x42C5E784. Field28/30 remain unknown; do not treat them
as packed RGB or infer a name-to-color table.

The compact viewer instead computes a representative tile using actual bound body skin
textures, interpolated recovered VU RGBA and MODULATE. Upper-facing triangle-centroid sampling
and brightest-quartile averaging are explicitly port-side display heuristics. They are not an
original frontend algorithm or a new recovered class/function. Missing preview data retains
an explicit reason and no invented RGB. Original frontend swatch behavior remains unknown.
