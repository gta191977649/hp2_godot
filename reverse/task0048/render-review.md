# E-0048-A07: primary track rendering review

Review only; static byte/source checks, not original-runtime verification.

All60 original TRACKS/TEX* archives and GLOBALB/INGAMEB/DYNTEX were scanned
at their original TPK entry-table and aligned data-pool offsets. Across12200
entries, no raw palette alpha or direct32 pixel alpha exceeds127. This upper
bound covers unused palette entries as well as every potentially selected index.
ALPHA44 and48 source textures therefore do not themselves require factors>1.

Every geometry strip in all30 decompressed TRACKB bundles was checked against
its texture hash metadata: no strip with render_flags80 has flag4 or a nonzero
TextureInfo74. Current original world-identity primary lighting has ambient
alpha128 and all directional/point colour alpha0. Therefore primary lit vertex
alpha is128, prelit alpha is128, and modulated source alpha is at most127.
These supplied primary track passes do not trigger the known source-over GS
alpha>128/destination-read host limitation. This is not a bound for a future
environment second pass or modified assets.

The native-to-host axis permutation is(x,y,z)->(y,z,x). host_matrix's basis
permutation and world-normal conversion consistently implement P*M*P^-1.
Track11 has no duplicated geometry hashes and all10369 strip UV-scale pairs
are32768/32768, so existing FixStripSTs crop correction would be identity there.

Actionable source findings sent to the root implementation:

- DirectorySource::probe rejects zero-byte files, while every legitimate TRACKA
  is empty. Catalogue availability must validate file presence independently.
- TEX1 L/K bias was originally applied only after the raw derivative lambda
  magnification branch. The host projection must apply its intended bias before
  choosing that branch, especially observed K4016(-5) and K4(.25).
- The stale native gap text claimed facade orientation was unavailable after
  original CreateFacadeMatrix was wired. It must describe the current boundary.
- Original flag100 environment/second-context passes are not covered by primary
  material/light binding. A missing second pass must stay explicit.

Ghidra proves TEX1 register construction and original mip source data; it does
not execute GS sampling hardware. The Godot derivative LOD/filtering, transparent
sorting and rasterization remain host projections, not PS2 numerical equivalence.
