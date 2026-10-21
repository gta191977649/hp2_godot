# TASK-0071 host renderer migration evidence

## E0071H01 - Port-side Vulkan + Forward+ adapter changes

User authorization: migrate the renderer and global ProfileDebug to Vulkan +
Forward+, maximize measured FPS while preserving recovered behavior and quality.
This record authorizes exact host-adapter identities and auxiliary source pins
listed in review-scope.json; it does not authorize recovered algorithm changes.

Confirmed port-side scope: framebuffer colour-domain transport, Godot shader
sampling/output, viewport configuration, depth projection, material binding,
launch/backend validation, profiler metadata and deterministic benchmark tooling.
The reviewed identity inventory must preserve every other full identity and stub
classification. Frozen historical review/scope JSON bytes remain immutable.

Retail target remains SLUS_203.62 SHA-256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
Debug driving remains NFSHP2.ELF SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
Read-only Ghidra rereads at Retail 00108CC0 and 0010AC50 reconfirm the existing
texture/material state boundary; they establish no new original algorithm.
Keep their original records and evidence rather than renaming recovered code.

Known adapter limitation: HDR accumulation clamps on resolve; original per-primitive
GS COLCLAMP is not reproduced. This review does not waive that difference.

Unknown: complete original runtime visual/numeric equivalence. Host tests and
benchmark gains cannot promote any recovered function to Verified. PCSX2 current
PINE28011 and JSON21512 probes timed out; no matched Track11 state is established.

## Review generation

After code settles, run:
`py -3 reverse/scripts/audit_task0071_review.py --write`

Generation rejects non-allowlisted identity changes and compares the preserved
local/task0071/baseline/source-identities.json with the frozen predecessor state.
It records current exact identities and LF-normalized auxiliary fingerprints.
A later source change must fail checking until explicitly reviewed again.
