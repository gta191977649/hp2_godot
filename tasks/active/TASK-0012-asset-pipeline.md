# TASK-0012 — PS2 original asset pipeline (eLab container / TPK texture format)

- Status: active (container format + P8/RGBA texture decode delivered; layout/text semantic
  decode not started)
- Objective: Provide a non-hardcoded, globally-configured entry point to the user's authorized
  local PS2 ZZDATA copy, and reverse engineer enough of its resource formats to decode real
  original UI/HUD art, as a prerequisite for eventually rendering the actual original menu
  layout (not placeholders) in Godot.
- Scope: Included — global asset-root config (env var + local config file), a generic eLab
  nested-chunk container parser, the HP2 TPK texture-header struct and P8/RGBA pixel decode,
  evidence records and citations for every borrowed algorithm. Excluded — 4bpp/DXT texture
  decode, the swizzled-RGBA case, widget/layout record parsing (position, size, which texture
  belongs to which menu), localization string table parsing, any Godot rendering of decoded
  assets, any write/compression path.
- Original PS2 addresses: not applicable (this task is asset-file format RE, not ELF code RE).
- Dependencies: user-provided local ZZDATA copy (not part of this repo); TASK-0011
  (`docs/FRONTEND_MENU.md`) for the ELF-side screen-manager evidence this will eventually need
  to cross-reference.
- Relevant files: tools/ps2_assets.py, config/ps2_assets.example.json,
  config/ps2_assets.local.json (gitignored), tools/asset_extractor/elab_chunks.py,
  tools/asset_extractor/hp2_tpk_textures.py, reverse/assets.csv, docs/ASSET_PIPELINE.md.
- GitHub issue: not created.

## Acceptance criteria

- [x] No ZZDATA path is hardcoded anywhere in the repo; a single module (`tools/ps2_assets.py`)
      resolves it via environment variable or a gitignored local config file, with a tracked
      example/template config.
- [x] A generic container-format parser that fully consumes (byte-exact) at least two
      independently-sourced ZZDATA files, proving the chunk-format hypothesis.
- [x] At least one real, named, original texture decoded from the user's asset dump and visually
      confirmed recognizable (not a placeholder, not fabricated).
- [x] The decode algorithm and struct layout are either independently derived with strong
      in-repo verification, or cited from external sources with the verification performed
      documented — never presented as "known correct" without evidence.
- [x] Every file this session wrote decoded images or chunk dumps into is gitignored; no
      proprietary asset bytes or derived images committed to the repo.
- [x] Residual decode artifact resolved: root cause was a missing 0x80-byte alignment on the
      pixel/palette data-pool base offset, found by cross-checking against the user's existing
      D:\dev\eagl-godot\map_tools_ps2 project; the pixel decode path was replaced with a verbatim
      port of that project's validated Modelulator-derived decoder (357/365 FRONTB.BUN textures
      now decode cleanly, up from 210/365 with artifacts).
- [x] Native runtime loader: the decode path is ported into Godot-independent C++
      (src/game_core/assets/elab_chunks.*, hp2_tpk_texture.*) with a Godot bridge
      (Ps2TextureLoader) that loads textures directly from the user's configured ZZDATA copy at
      runtime -- no pre-extracted image files are shipped. Verified byte-for-byte identical to
      the Python reference via CTest `hp2_tpk_texture`. A standalone exported .exe
      (out/windows-debug/port.exe) was built and run successfully with this loader.
- [ ] Widget/layout records parsed well enough to know which decoded texture/text belongs to
      which named screen (needed before any Godot rendering of real original layout).
- [ ] TASK-0011's ELF-side screen-manager evidence cross-referenced with this task's asset-side
      findings.

## Verification procedure

```bash
py -3 tools/ps2_assets.py                                  # prints ZZDATA root + top-level dirs
py -3 tools/asset_extractor/elab_chunks.py FRONTEND FRONTB.BUN --max-tree-depth 2
py -3 tools/asset_extractor/elab_chunks.py GLOBAL HUDTEX.BIN --max-tree-depth 2
py -3 tools/asset_extractor/hp2_tpk_textures.py GLOBAL HUDTEX.BIN --limit 8
py -3 tools/asset_extractor/hp2_tpk_textures.py FRONTEND FRONTB.BUN
```
Expect `fully_consumed: true` in both `elab_chunks.py` summaries, and `decoded N, skipped M`
with zero `FAILED` lines from `hp2_tpk_textures.py` (skips are expected for 4bpp/DXT/unsupported
compression types; those are not failures). No original behavior/visual comparison against the
real PS2/PCSX2 has been performed; decoded images were only checked by human visual inspection
for recognizability, not pixel-exact correctness.

## Current findings

See `docs/ASSET_PIPELINE.md` for the full write-up (evidence E-0012-01..05). Summary: the eLab
chunk container format is solved and validated by exact full-file consumption on two unrelated
files. The HP2 TPK texture header (`AncientTextureInfo`, 164 bytes) is solved and validated
byte-for-byte against a real texture record plus a 365-record batch consistency check. P8 pixel
decode (PSMT8 block swizzle + CSM1 CLUT reorder + PS2 alpha range) produces clearly recognizable
real game art (tachometer gauges, car badges, full background scenes) but with an unresolved
fine speckle artifact that is most likely authentic PS2-era dithering, not yet confirmed.

## Unresolved questions

- Is the residual speckle pattern authentic original dithering, or a remaining decode bug?
  (docs/ASSET_PIPELINE.md §4.4)
- Binary layout of the `UwEMENUITEM_*`/widget-style records found in FRONTB.BUN's string region
  (position, size, texture/text references) — not attempted this session.
- 4bpp indexed and DXT1/3/5 decode — algorithms are known from the same reference tools but not
  yet implemented here.
- Swizzled (32-bit block-swizzle) RGBA decode — algorithm known (`ps2_tim2_tool.py`'s
  `_gs_swizzle_32bit`, needs inverting) but not implemented or tested (no confirmed swizzled
  RGBA sample found this session).
- Whether/how TASK-0011's ELF-confirmed screen names ("New Main Menu", "TitleScreen") map onto
  specific decoded textures and widget records.

## Next steps

1. Parse `LANGUAGES/FE_ENGLISH.BIN` for an authoritative menu text table (more reliable than the
   scattered strings found inside FRONTB.BUN).
2. Reverse the `UwEMENUITEM_*`-style widget record layout to recover per-screen composition
   (which textures/text appear where).
3. Resolve the residual P8 decode artifact, ideally by comparing against a real PCSX2 screenshot
   the user can provide.
4. Implement 4bpp and DXT decode to cover the remaining ~40% of FRONTB.BUN's textures.
5. Only after 1-4: attempt an actual Godot rendering of the real New Main Menu layout, cross
   referencing TASK-0011's confirmed code-level evidence.

## Related Git commits

None yet; this task's files are uncommitted as of its last update (see TASK-0011 for the note
about the user's independent scaffold commit earlier in this session).

## Completion evidence

2026-10-12 (Australia/Sydney): `elab_chunks.py` reports `fully_consumed: true` for both
FRONTEND/FRONTB.BUN (14,385,132 bytes, 1,760 chunks) and GLOBAL/HUDTEX.BIN (1,540,664 bytes, 9
chunks). `hp2_tpk_textures.py` decoded 89/89 HUDTEX.BIN headers without error (P8 textures
decoded, non-P8 would have been skipped but none present) and 210/365 FRONTB.BUN headers
(remainder skipped for unsupported compression, zero failures). Visual inspection of 11+ decoded
PNGs (VIPERGTS_TACH, 911TURBO_TACH, BMWM5_TACH, CROWNVIC_TACH, DIABLO_TACH, F50_TACH,
911TURBO_MPH, F50_KMH, BMWM5, ASTON_BADGE, DISPLAYSETUP_BG, PURSUIT_TREE_BG) confirmed
recognizable real content. Remaining limits: see Acceptance criteria unchecked items and
Unresolved questions above. This task stays in `active/`, not `completed/`: the broader "render
the original UI" request is only partially addressed (real textures decodable, but not yet
placed into any layout).
