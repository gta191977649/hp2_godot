# TASK-0012: the PS2 original asset pipeline (the eLab container format and the .tpk texture format)

**2026-10-12 update**: the user pointed out that a mature, validated project of the same kind
already exists on this machine — `D:\dev\eagl-godot\map_tools_ps2`, the same user's existing HP2
PS2 asset export toolchain covering track meshes, textures, audio and environment — and that its
`src/map_tools_ps2/textures.py` is a proven texture decode reference. Comparing against it showed
this task's original pixel decode was missing a step: **the start offset of the texture data pool
(chunk4) has to be rounded up to a 0x80 byte boundary**
(`data_base = align(data_chunk.data_offset, 0x80)`). The original implementation used the raw
unaligned offset, which left every decoded texture with a layer of fine speckle noise (recorded as
"unresolved" in §4.4 at the time). After porting the alignment plus the full Modelulator legacy
PS2 RWBuffer decode path (instead of the earlier simplified 8bpp-only path), the problem
disappeared completely, and textures that had been skipped for being 4bpp indexed (`LOGO_NFS`,
`CARLOGO_*` and friends) now decode correctly too — the success count across
`FRONTEND/FRONTB.BUN`'s 365 texture records went from 210 to 357. The text below reflects the
fixed state; the original "unresolved" record is kept at the end of §4.4 as a process record and
must not be deleted.

Completed 2026-10-12, Australia/Sydney. The scope is **read-only parsing of the container format
and the P8/RGBA texture pixel formats**, which is now enough to decode a large body of real
original UI/HUD artwork out of the user's authorized PS2 disc asset directory. **Semantic parsing
of the menu layout and widget tree — which texture belongs on which menu, at which pixel
coordinates — is not done**, so the current results must not be presented as "a fully
reconstructed menu screen". 4bpp textures, DXT compressed textures and 32-bit textures with the
swizzle flag are not decoded; they are explicitly refused rather than guessed at.

## Background and the user's requirements

The user supplied an authorized copy of the PS2 original disc assets at
`D:\ps2_game\GAME\ZZDATA` and asked for:

1. no hardcoded path anywhere in the code, and a single global configuration entry point in the
   repository;
2. the reverse engineering to be able to draw the original UI directly, behaving as the PS2 does;
3. project file names and function name structure to reflect the real structure of what is being
   reversed, rather than arbitrary naming.

## 1. The global asset configuration entry point

`tools/ps2_assets.py` is the **only** place in the repository allowed to resolve the ZZDATA path.
Any script that needs to read original assets must `import ps2_assets` and call
`ps2_assets.zzdata()` / `ps2_assets.path(*parts)`; hardcoding a path anywhere else is forbidden.
Resolution order:

1. the environment variable `HP2_PS2_ASSET_ROOT`;
2. `config/ps2_assets.local.json` (in `.gitignore`; machine-specific and never committed).

What is committed is `config/ps2_assets.example.json` (a placeholder path plus usage notes); the
real path exists only in the local `config/ps2_assets.local.json`, which on this machine holds
`{"root": "D:/ps2_game/GAME/ZZDATA"}`.

## 2. The container format: eLab nested chunks

Evidence: the string literal `Global\Pipeline\eLabFrontEnd.bin` embedded in
`FRONTEND/FRONTB.BUN` (nothing to do with ELF addresses — it is a debug path string inside the
asset file) identifies the engine toolchain as **eLab**, an EA internal pipeline tool from the
same era and ecosystem as the bWare middleware (`bWare/inc/bWare.hpp` debug path strings appear
repeatedly in the same asset region; the relationship between the two was not investigated
further).

The format hypothesis, confirmed by manual byte analysis plus whole-file consumption checking
(below):

```c
struct Chunk {
    uint32_le tag;    // bit 0x80000000 set = container (the following `size` bytes are nested Chunks)
                       // clear = leaf (the following `size` bytes are opaque: name strings, floats, texture bytes, ...)
                       // (tag & 0x7fffffff) increments by one per chunk in pre-order within a file,
                       //   so it reads as a file-level chunk index
    uint32_le size;    // payload bytes that follow
    uint8 data[size];
};
```

Implementation: `tools/asset_extractor/elab_chunks.py`, with `parse_chunks()` /
`parse_chunks_top()` at its core, purely read-only. Validation: recursively parse the whole file
and check `consumed_bytes == total_bytes`, i.e. the file is consumed **exactly**, with nothing
left over and nothing out of bounds. That is stronger evidence than "it ran without an error",
because wrong chunk boundaries would almost never land random data precisely on the end of file.

| File | Size (bytes) | fully_consumed | Top-level chunks | Total chunks |
|---|---|---|---|---|
| FRONTEND/FRONTB.BUN | 14,385,132 | true | 21 | 1,760 |
| GLOBAL/HUDTEX.BIN | 1,540,664 | true | 3 | 9 |

Two files from different sources — a front-end pipeline bundle and a texture pack — both consumed
exactly by **the same generic parser** is strong evidence for the hypothesis (E-0012-01).

## 3. Real original text (evidence, not invention)

After `elab_chunks.py` string-scans the leaf chunks, the last ~300 KB of `FRONTEND/FRONTB.BUN`
(from offset ~14,080,000) turns out to hold a large block of clean, regularly laid out asset and
widget name strings, including developer debug/placeholder text — which proves this is
uncleaned original data rather than anything invented here:

- Development placeholders and jokes: `"You need at least one menu item"`,
  `"Phil is a loser, so I have to make this fake menu item"`,
  `"This is The Title of the Event Phil is still poo poo. Greg will always be the greatest"`,
  `"DummyMenuItem"`, `"Silly menuitem"`.
- Real UI text: `"PRESS START"` (twice), `"Championship"`, `"Options"`, `"Credits"`,
  `"Controller"`, `"Language Select"`, `"Player Options"`, `"Setup"`, `"Race Type"`,
  `"Next Track:"`, `"Select Laps"`, `"Look Back"`, `"Menu Music"`, `"Sound Effects"`.
- Many `UwE`-prefixed widget type names: `UwEMENUITEM_n`, `UwEANIMATING_MENUITEM`,
  `UwEPOPUP_TITLE`, `UwEBACKGROUND_FADE`, `UwESCROLLING_MENU_CLIP_REG`, `UwECONTROLLER_BITMAP`.

Evidence ID: E-0012-02. **None of these strings has a confirmed correspondence to a specific menu
or page yet** (for instance, which group of `UwEMENUITEM_*` belongs to "New Main Menu"). They form
an evidence chain entirely separate from TASK-0011's "New Main Menu" / "TitleScreen" evidence
decompiled from the ELF code (`docs/FRONTEND_MENU.md`), and the two have not been cross-linked.

## 4. The texture format: HP2's TPK container plus PS2 GS P8/RGBA pixel formats

### 4.1 Chunk structure (a community reference implementation, cited and checked)

`GLOBAL/HUDTEX.BIN`, `FRONTEND/TSELECT.BIN` and a 12.49 MB sub-container embedded in
`FRONTEND/FRONTB.BUN` all carry a `0xB0300001` container chunk at the top, whose children are
`0x30300002` (a package name / source path leaf), `0x30300003` (the texture header array) and
`0x30300004` (the shared pixel/palette data pool). Those tag values match
[`xan1242/xnfstpktool`](https://github.com/xan1242/xnfstpktool) — the
`TPK_HP2_MAIN_CHUNKID` / `TPK_HP2_CHUNK2/3/4_CHUNKID` constants in `TPKTool/TPKTool.h` —
**byte for byte**. That tool's source explicitly labels them "HP2 chunks" and
`TPKTOOL_READINGMODE_HP2`, confirming this is the reading mode meant for this game (the PC/PS2
NFS:HP2 that SLUS-203.62 corresponds to).

The texture header structure (that tool's `AncientTextureInfo`, 164 bytes):

```c
struct AncientTextureInfo {
    uint32_t Unknown[2];          // +0x00, meaning unresolved
    char DebugName[24];            // +0x08
    uint32_t NameHash;              // +0x20
    uint16_t Width;                 // +0x24
    uint16_t Height;                // +0x26
    uint8_t ImageCompressionType;   // +0x28  0x08=P8, 0x20=RGBA, 0x22/24/26=DXT1/3/5; anything with bit 0x4 is a 4bpp variant
    uint8_t RenderingOrder;         // +0x29
    uint8_t unk1..unk5, AlphaUsageType; // +0x2A..+0x2F, meaning unresolved
    int32_t ImagePlacement;         // +0x30  pixel data offset relative to the start of the chunk4 pool
    int32_t PalettePlacement;       // +0x34  palette offset relative to the same
    int32_t ImageSize;              // +0x38
    int32_t PaletteSize;            // +0x3C
    uint32_t unk6, unk7;             // +0x40..+0x47, meaning unresolved
    uint8_t ShiftWidth, ShiftHeight; // +0x48,+0x49
    uint8_t UnkData[0xB];            // +0x4A..+0x54, meaning unresolved
    uint8_t swizzled;                // +0x55  non-zero = the pixel data needs PS2 GS block unswizzling
    uint8_t RestOfData[0x4E];        // +0x56..+0xA3, meaning unresolved
};
```

### 4.2 What this repository verified independently (rather than trusting the citation blindly)

Taking `GLOBAL/HUDTEX.BIN`'s first texture record, `"VIPERGTS_TACH"`, as the baseline and checking
byte by byte: `DebugName` matches exactly at +0x08; `NameHash` at +0x20 is `0x08a0f7f2`;
`Width` / `Height` at +0x24 / +0x26 are both 128; `ImageCompressionType` at +0x28 is `0x08` (P8);
`ImageSize` (+0x38) is 0x4000 = 16384, exactly 128x128x1; and `PaletteSize` (+0x3C) is 0x400 =
1024, exactly 256x4, the standard P8 palette size. The texture header array's total size of
14,596 bytes divided by 164 bytes per record is **exactly** 89 — matching the 89 textures
HUDTEX.BIN actually contains (E-0012-03). The same bulk check over the 365 texture headers in
`FRONTEND/FRONTB.BUN` (strictly contiguous `image_placement` / `palette_placement`, no overlaps,
no out-of-bounds) passes for all 365 records.

### 4.3 Pixel format decoding (now a port of this machine's existing authoritative implementation)

Implementation: `tools/asset_extractor/hp2_tpk_textures.py`. The pixel decode part was **not
derived independently in this repository**; it is a function-by-function port of the user's
existing project at
`D:\dev\eagl-godot\map_tools_ps2\src\map_tools_ps2\textures.py` (`decode_indexed_texture`,
`_decode_modelulator_indexed_texture`, `_legacy_ps2_rw_plan`, `_legacy_ps2_rw_buffer`,
`_legacy_ps2_swizzle_psmt4/8/16/32`, `_legacy_ps2_block_address`, `_decode_palette`,
`_decode_ps2_alpha`, with the logic entirely unmodified). That project's own documentation,
`D:\dev\eagl-godot\docs\hp2_ps2_texture_decoding.md`, in turn states its swizzle/RWBuffer
algorithms come from Modelulator (a well-known NFS modding tool), `eTexturePack.lua` / `PSX2.lua`.
The rest of this section is a historical record of the public community tools this task consulted
when it first implemented decoding itself; it is kept for traceability. **The current code no
longer uses the simplified algorithms below** and uses the port described above instead — see
§4.4 for why, and how it was verified.

- **Historical record (no longer used) — the early P8 (8-bit indexed) implementation:**
  - The pixel bytes are stored in the file using the PS2 GS PSMT8 block/column swizzle (when
    `swizzled=1`, which every texture checked in HUDTEX.BIN is). The restoration algorithm came
    from the `UnSwizzle8` function of a Silent Hill 2 (PS2) PIC.IMG extraction tool
    ([gist](https://gist.github.com/infval/b23380076d77341ccc379a62ef9f23b3)), which itself cites
    a 2012 XeNTaX forum post. A self-consistency test with random data ("swizzle forward, then
    unswizzle back to the original bytes") round-trips identically (E-0012-05 — which does not
    mean "correct", only that the port has no transcription error). Three further independent open
    source implementations (the documentation comments in `xnfstpktool`,
    [`PS2HomeDeveloper/ps2-tim2-tool`](https://github.com/PS2HomeDeveloper/ps2-tim2-tool) and
    [`leeao/PS2Textures`](https://github.com/leeao/PS2Textures)) list the same shape of algorithm
    as the standard 8bpp method, corroborating one another.
  - The palette (256 x RGBA32) needs the PS2 CSM1 reorder: within each group of 32 colours, the
    four 8-colour "stripes" change order from `0,1,2,3` to `0,2,1,3`, and that permutation is its
    own inverse. Source: `_unswizzle_clut8` in `PS2HomeDeveloper/ps2-tim2-tool`. It was
    independently cross-checked against `unswizzlePalette` in `leeao/PS2Textures` (a bitwise
    implementation that swaps bits 3 and 4 of the index); the two are mathematically equivalent
    (the same permutation table), confirming the port is not a mis-transcription (E-0012-05).
  - Alpha channel: the PS2 GS native alpha range is 0..0x80, not 0..0xFF, so the restoration
    formula is `a_255 = min(round(a_ps2 * 255 / 128), 255)`, also from `ps2-tim2-tool`.
- **RGBA (32-bit direct colour)**: decoded only when `swizzled=0` (read 4 bytes per pixel row by
  row, with the same alpha restoration). RGBA textures with `swizzled=1` are **explicitly refused**
  (an error rather than a wrong image), because although the 32-bit GS block swizzle algorithm can
  be found in `_gs_swizzle_32bit` in `ps2_tim2_tool.py`, this session had no confirmed
  `swizzled=1` RGBA texture to validate against and would not apply it on spec.
- 4bpp indexed textures (`ImageCompressionType & 0x4`, e.g. `CARLOGO_*`, `BIG_ARROW_*`) and
  DXT1/3/5 compressed textures are **not implemented at all**; at runtime they are explicitly
  skipped with a printed reason, never silently ignored or guessed at.

### 4.4 Decode results (fixed)

**Root cause and fix**: the original implementation used chunk4's raw `data_offset` as `data_base`
without rounding up to 0x80. Looking at `read_ps2_tpk_bytes` in
`D:\dev\eagl-godot\map_tools_ps2` showed the correct form is
`data_base = align(data_chunk.data_offset, 0x80)`. For `GLOBAL/HUDTEX.BIN`, chunk4's raw offset is
14712 and the aligned value is 14720 — a difference of 8 bytes, which exactly explains the
symptom of "the composition is completely correct but carries a fine, regular speckle" (every
index was systematically displaced by a few bytes rather than being scrambled wholesale). At the
same time the pixel decode moved from the earlier simplified "8bpp only" path to the reference
implementation's full Modelulator legacy PS2 RWBuffer path (supporting both 4bpp and 8bpp, keyed
on the real type in the `pixel_storage_mode` field rather than a guessed compression-type table),
plus the reference implementation's extra vertical flip for export purposes
(`frontend_textures.py` does exactly this for front-end textures, because the internal decode path
follows 3D mesh UV conventions and has already flipped once).

**Verification after the fix**: bulk decoding `GLOBAL/HUDTEX.BIN` (89 records) and
`FRONTEND/FRONTB.BUN` (365 records) succeeds 89/89 and 357/365 respectively (the remaining 8 are
placeholder records with empty names/sizes and are explicitly skipped rather than erroring).
Visual inspection confirms no artifacts at all: `VIPERGTS_TACH`'s tachometer legibly reads
"RPM X1000" with its graduations; `DISPLAYSETUP_BG` is a perfectly clean track scene (red sports
car, number plate, roadside trees, barriers and distant clouds, all free of noise); `LOGO_NFS`
clearly shows the official red "NFS" logo with its trailing effect; and `LOGO_NFS`, `CARLOGO_*`
and the other textures previously skipped for being 4bpp now decode correctly. Evidence ID:
E-0012-06.

## 5. Relationship to TASK-0011

TASK-0011 (`docs/FRONTEND_MENU.md`) confirmed the `TitleScreen -> New Main Menu` edge and the
manager-level API by decompiling ELF code; this task (TASK-0012) confirmed the container and
texture formats and decoded a large body of real original artwork out of the asset files. **The
two evidence chains are currently independent and not cross-linked** — in other words, we do not
yet know which of the decoded textures, strings and widget records the "New Main Menu" screen
actually uses. Linking them (parsing the menu state machine `FUN_0016fa80` exposes together with
the `UwEMENUITEM_*` widget records in the asset files, and finding how they reference texture
names/hashes) is the next step towards Godot actually drawing a New Main Menu that matches the
original, and it has not been started.

## 5a. Addendum 2026-10-12: a native C++ runtime loader (replacing the manual export flow)

The user asked explicitly: "do not manually export textures and feed them to Godot; the reversed
code should load everything from the original PS2 assets at runtime." So the decode logic in
`hp2_tpk_textures.py` (the alignment fix plus the full Modelulator legacy RWBuffer path) was
ported function by function into Godot-free native C++:

- `src/game_core/assets/elab_chunks.{hpp,cpp}` — the C++ version of the chunk container parser.
- `src/game_core/assets/hp2_tpk_texture.{hpp,cpp}` — the `Hp2TpkArchive` class: texture header
  array parsing plus full pixel/palette decoding (P8/4bpp, including the PS2 legacy RWBuffer
  swizzle). **Validated by a byte-level comparison test** (`src/tests/hp2_tpk_texture_tests.cpp`,
  CTest case `hp2_tpk_texture`): for the same `VIPERGTS_TACH` texture, the C++ decode output is
  byte-for-byte identical to the Python reference implementation.
- `src/platform/ps2_asset_root.{hpp,cpp}` — runtime ZZDATA path resolution (the same order as
  `tools/ps2_assets.py`: the `HP2_PS2_ASSET_ROOT` environment variable first, otherwise walking up
  from the executable's directory looking for `config/ps2_assets.local.json`), Godot-free and
  shared by the bridge layer and the native tests.
- `src/platform/godot_bridge/ps2_texture_loader.{hpp,cpp}` — the Godot `RefCounted` wrapper
  `Ps2TextureLoader`, exposing `load_texture(relative_path, texture_name) -> ImageTexture` to
  GDScript and calling `Hp2TpkArchive` directly, **without going through any pre-exported PNG**.

`godot/scripts/frontend_menu_observer.gd` now instantiates `Ps2TextureLoader` in `_ready()` and
calls `load_texture("FRONTEND/FRONTB.BUN", "LOGO_NFS")` and the like at runtime, rather than
`load()`ing any pre-exported texture resource; the manually exported PNGs that used to live in
`godot/assets/extracted/frontend/` have been deleted. The Python
`tools/asset_extractor/hp2_tpk_textures.py` is kept as an offline analysis and quick-look tool but
is **no longer part of any runtime path**.

On whether the original really contains this CPU decode code: a read-only string and hex search
over the ELF found that the chunk tag constants (`0xB0300001` and friends) appear neither as code
immediates nor as data words, and that the strings "TPK", ".tpk" and "eLab" are entirely absent
from the ELF and present only in the asset files. That suggests the tag values are purely asset
file data and that the original code most likely reads children in a fixed order rather than
dispatching on literal tag values. Going further: the PS2's GS hardware natively supports
addressing swizzled and indexed textures during sampling, so the original code most likely just
configured the GS registers (TEX0/TBW/PSM/CBP) over the raw bytes and DMA'd them to video memory,
never needing a CPU-side conversion to linear RGBA at all — meaning this "decode to linear RGBA"
step is probably something only this porting task needs (because the target is a modern GPU and
Godot, not real PS2 hardware), and the original may well have no one-to-one counterpart function.
That is reasoning, not a confirmed conclusion, and is recorded as an open item; see the
`TexturePack` / `TextureAnimPack` original class names in
[docs/ORIGINAL_SOURCE_TREE.md](ORIGINAL_SOURCE_TREE.md) (the class names were found; the address
of a specific decode function was not).

## 6. Artifacts integrated into the repository

- `tools/ps2_assets.py`: the global asset path resolution entry point (the only place allowed to
  touch the ZZDATA path).
- `config/ps2_assets.example.json` (committed) and `config/ps2_assets.local.json` (local only,
  ignored).
- `tools/asset_extractor/elab_chunks.py`: the generic chunk tree parser plus a CLI.
- `tools/asset_extractor/hp2_tpk_textures.py`: HP2 TPK texture header parsing, the P8/RGBA decoder
  and a CLI (offline analysis; no longer part of any runtime path, see §5a).
- `reverse/assets.csv`: SHA-256 provenance records for the analyzed asset files.
- `tools/asset_extractor/output/` (ignored, never committed): decoded PNG samples and chunk tree
  JSON dumps.
- `src/game_core/assets/elab_chunks.{hpp,cpp}` and `hp2_tpk_texture.{hpp,cpp}`: the native runtime
  decoder (see §5a), byte-compared against the Python reference by
  `src/tests/hp2_tpk_texture_tests.cpp`.
- `src/platform/ps2_asset_root.{hpp,cpp}`: runtime ZZDATA path resolution.
- `src/platform/godot_bridge/ps2_texture_loader.{hpp,cpp}`: the Godot `Ps2TextureLoader` bridge.
- `godot/scenes/frontend_menu_observer.tscn` + `godot/scripts/frontend_menu_observer.gd`: the
  operable menu loop demo that loads textures from the original assets at runtime (see
  docs/FRONTEND_MENU.md).

No original asset bytes, decoded texture PNGs or any other potentially copyrighted file have been
committed to Git — all of them land under `tools/asset_extractor/output/`, which `.gitignore`
covers.

## 7. Open questions and next steps

1. ~~The residual fine artifacts in P8/RGBA decoding~~ — resolved (§4.4, E-0012-06).
2. DXT1/3/5 compressed textures are still not implemented (this task met no real sample that
   needed them; 4bpp and 8bpp indexed were solved along with the §4.3 port). RGBA
   (non-indexed) texture decoding has not been ported from the reference implementation either —
   the current `decode_texture` only handles the indexed path — so the few RGBA entries in
   FRONTB.BUN (such as `ARROW_L`) are skipped.
3. The binary layout of the `UwEMENUITEM_*` and similar widget records at the end of `FRONTB.BUN`
   (position, size, associated texture reference) is completely unparsed, and that is the
   prerequisite for "placing textures the way the original lays them out". Nothing has been done
   on it.
4. `FRONTEND/TSELECT.BIN` (the track select texture pack, 20.64 MB), `GLOBAL/DYNTEX.BIN`,
   `GLOBAL/CHARACTERSA.BIN` and `LANGUAGES/FE_*.BIN` (localised text, very likely the
   authoritative source of the menu copy — this session only looked at the file header and did not
   parse its record format) are all unhandled.
5. The TASK-0011 and TASK-0012 evidence chains are not cross-linked (see §5). That is the current
   highest priority — see the continuation hint below. The user asked explicitly for the original
   menu source to be reversed properly with Ghidra MCP, so the focus of the work moves to the ELF
   side next.

Continuation hint: read this document together with `docs/FRONTEND_MENU.md`. To get Godot drawing
a New Main Menu that genuinely resembles the original, the suggested order is: first parse
`LANGUAGES/FE_ENGLISH.BIN` for authoritative menu text (more trustworthy than the scattered
strings in the asset packs; the file header already shows the beginnings of a string table
structure); then parse the binary layout of the `UwEMENUITEM_*` widget records; and finally solve
4bpp/DXT texture decoding and the residual artifacts. Every step should be verified before it is
used, and placeholder images must never be passed off as decoded original artwork.
