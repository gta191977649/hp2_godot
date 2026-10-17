# TASK-0048 - Original track viewer and driving integration

- Status: completed (bounded PC implementation; original equivalence unverified)
- Authority: Retail SLUS_203.62 track assets and loaders; existing Debug NFSHP2.ELF driving chain, explicitly adapted.
- Scope: original track catalogue, geometry and texture loading, topology/elevation/material queries, real-track driving entry, static walls/obstacles when recovered.
- Preserve all pre-existing dirty work. Ghidra read-only. No invented physics, proprietary tracked assets, or Verified claims from host tests.

## Acceptance

- [x] Catalogue and resource selection recovered from original binary/data.
- [x] Original geometry/textures displayed in TrackViewer; load/switch/clear/failure lifecycle tested.
- [x] Typed recovered methods placed in their original declaring classes with address/evidence records.
- [x] Real per-wheel elevation/normal/terrain feeds existing Debug driving chain through an explicit version adapter.
- [x] Original start placement; static wall/obstacle interaction; unresolved water/destructibles reported.
- [x] Native, host and real-window checks recorded; all catalogue resources audited.
- [x] Durable evidence, progress and continuation updated.

## Evidence and work allocation

REA skill staged investigation step 6: topology worker owns recovered track classes and contact queries; loader worker owns catalogue/render decoding; driving worker owns native driving adapter; root owns host, UI, build and integration. Evidence uses E-0048 IDs, with private captures under local/task0048. This task does not broaden TASK-0038 or rewrite TASK-0044..46 history.


## Implementation and evidence

- `reverse/task0048/assets.md`, `render-review.md`: 30-entry catalogue, resource reuse,
  original scene geometry/textures/mips, bounded lighting and facade math.
- `topology-evidence.md`: original TopologyTree, TrackPolygon and TopologyCoordinate
  operations, owned bounds-checked loading and original path/start selection.
- `driving-integration.md`, `terrain-mapping.json`: explicit Retail-to-Debug terrain
  adapter; four-wheel topology contacts feed the existing tire/load/noise chain.
- `collision-geometry-evidence.md`, `static-collision-evidence.md`, `collision-impulse.md`:
  original CV/wafer/VClip, contact-time backup, sorted impulses and native re-move.
- `model-alias-correction.md`: catalogue resource aliases and BaseModelName CV lookup.
- `host-integration.md`: asynchronous load/switch/failure lifecycle, native ground plane
  diagnostics, real-track entry and return. `source-review*.json` preserve historical pins.

## Validation and boundaries

Godot debug native build and scoped source audit passed. Actual GL Compatibility windows
were inspected at the real Track11 start and in driving mode; captures/logs are ignored
under `local/task0048/`. The uploaded Track11 scene contains 569,057 triangles, 339 textures,
1,114 render nodes and 7,692 batched opaque instances. No stable FPS claim is made.

Native acceptance covers all 30 original starting positions, 120 settling steps with
four-wheel contact, three real terrain samples, 15-step replay, synthetic return,
complete original-car wall and corner impulses, and exact retry after rejected-step
rollback. Full-suite and all-track render sweep final results are recorded below.

Static collision scope is immutable track polygons (including walls and topology
obstacles), not arbitrary rendered props. Retail anchored Smokeables belong to the
explicitly excluded breakaway/dynamic lifecycle (T07). Water, dynamic interactions,
AI and race orchestration remain excluded. Environment reflection and original world
camera LOD/culling are explicit render gaps. Host derivative mip LOD is a projection,
not GS raster equivalence. No function receives Verified from these PC checks.


### Final command records (2026-10-17)

- `cmake --build --preset godot-debug -j 6`: PASS (native core, tests and GDExtension).
- `build/godot-debug/track_asset_tests.exe D:/ps2_game/GAME/ZZDATA`: PASS all30 catalogue
  entries, original geometry, instance, texture, mip and origin-light references.
- `python reverse/scripts/audit_task0048_review.py`: PASS frozen predecessor chain,
  TASK0048 17-before/156-after and independent alias correction 3-before/4-after.
- `python reverse/scripts/generate_class_templates.py --check`: PASS372 classes,
  1,203 member attributions and23 catalogue-only labels.
- Initial integrated84-test run:82PASS, two stale test expectations failed. Corrected
  alias availability counting and canonical Move wrapper assertion; both targeted
  reruns PASS. Production source was unchanged by these test corrections.
- `static_collisions`: PASS21 declared model aliases,20 available decoded CVs; missing
  DIABLOHP base resource is reported unavailable. Full wall/corner/rollback tests PASS.
- Actual-window captures: local/task0048/track11-start.png, track11-driving.png and
  track11-final.png. Screenshot inspection checks original road/material display and
  driving presentation. Cross-material response and collision assertions are native
  tests, not claimed as manual original-game or hardware comparison.

- Final clean `ctest --preset godot-debug --output-on-failure`: **84/84 PASS**, zero
  failures,124.21 seconds. Log: local/task0048/final-ctest-confirmed.log.
- Final generated-progress, class-template, source-review and diff-whitespace checks PASS.

## Continuation

For certification, attach a runnable matching original version and collect equal-state
input/time/contact traces plus matched camera images. Recover environment reflection
and original camera visibility/LOD as separately scoped render work. Extend original
Smokeable/damage/effect ownership only with their lifecycle dependencies; do not add
stand-in collisions or friction. Original binaries/assets remain external.
