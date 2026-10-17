# TASK-0046 skid chain audit

Binary: Debug NFSHP2.ELF, SHA-256 `815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`.

## Confirmed implementation

E-0046-S01 restores original SkidSegment/SkidSet/SkidMaker bodies in indep/src/skids.cpp with logical-memory host bindings. E-0046-S03 restores Wheel::MaybeMakeSkids and bNormalize. E-0046-S02 restores ePoly constructor/render-command slice, eView::GetPixelSize and AABB classification in the original EcstasyE translation unit. E-0046-S04 records fresh Debug texture packet/test-context dependencies; no Retail layout delta is assumed. The function ledger and individual records contain exact ranges and listing hashes.

The single algorithm source is Wheel -> SkidMaker -> SkidSet/SkidSegment -> ePoly. The explicit port arena supplies pool memory and a single synthetic clan; a host render sink performs no stripe geometry calculation. Actual resource lookup is injected. Original null texture pointers remain null, and missing required test resources must be reported by resource preparation. Geometry tests deliberately bind a named synthetic test texture only in tests.

Original capacity8/128, compressed signed bytes, original thresholds, material width, independent endpoint height queries, puddle-zero intensity, pixel-size fade and render-dependent LRU remain intact. There is no new time fade. AddSegment passes travel distance, not tire width, to bExpandBoundingBox: Debug001A8304/001A8324 independently confirm this surprising operand. Original constant bits are read from the selected ELF at runtime.

## Explicit limits

No original-world Clan/GetClan routing, streaming/clan eviction or full topology is claimed; one clan covers the synthetic test scene. Slot allocation is a bounded host arena rather than recovered bSlotPool ABI. Rendering emits original ePoly vertex/color/UV semantics but does not emulate DMA/VU/GS execution. Camera H/clip matrix are tool-camera inputs, not a recovered original camera. Degenerate AddSegment VRSQRT and unsupported conversion ranges throw NotImplemented. Source integrity and PC tests do not establish PS2 equivalence.

## Validation

`driving_skid_tests` covers signed segment compression, endpoint geometry, height/width queries, eligibility, contact loss, blown tire and force threshold, puddle zero, merge/terrain bridge, eight-point rollover,128-slot tail replacement, original UV/rawalpha/Z offset, integer pixel culling/fade, NIS and AABB classification. Root integration records the exact final build/test commands and results. No PS2/PCSX2 synchronized comparison has passed.

## Debug rendering counterevidence

Fresh Debug AttachSolidPolys0010BEF0 and AttachAlphaPolys0010BBF8 invalidate reusing the Retail header+50 depth-write projection for skids. Solid context is3 for GlossMapInAlpha, otherwise2 for opaque, otherwise3 for signed RenderingOrder<3 and1 for later semitransparent. Alpha route omits the gloss override. The global Z-write gate clears bit1. Canonical selection slices now publish context/depth/alpha test/reference in SkidQuad, and Godot consumes them without reimplementing this choice. Original normal target defaults in00106320 and packet generation001058D8 establish even-context TEST5030D / odd-context5000B and context0/1 versus2/3 depth masks. Initial investigation wording about context4/5 was a decompiler-array-index misreading and is retired; there are four contexts0..3.

CalculateH001042E8 and CreateViewMatricies0010EA18 independently establish horizontal original FOV and ScreenWidth/2 tangent scaling. The tool-camera pixel-focal conversion is explicit and is not an original camera reconstruction.
