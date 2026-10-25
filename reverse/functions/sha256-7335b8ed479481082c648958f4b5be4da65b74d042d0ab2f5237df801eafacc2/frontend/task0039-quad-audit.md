# TASK-0039 shared frontend quad audit

Date: 2026-10-17. Evidence: read-only Ghidra MCP `inspect` decompilation and bounded listings, compared with the current native parser/frame builder. No database edits and no original-runtime comparison. Status: static findings only, not Verified.

Binary: SLUS_203.62 US; SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`; exact executable revision unknown. Ghidra project program version 5 is database metadata, not a game revision. Program reports `r5900:LE:32:default`, compiler spec `default`, MIPS-R5900, little endian, image base `00100000`. Ghidra application/MCP build version was not supplied by the inspected resource. The SHA is the task/registered binary identity; this sub-audit did not independently rehash the executable.

Raw tool responses are private under ignored `local/task0039/quad-*.json`; this record contains analysis rather than a full proprietary code export. Repeat with `inspect(action=decompile,file_name=SLUS_203.62,address=<entry>)`, or `listing` with the stated address/end_address. Range responses paginate at approximately 100 instructions; do not regard the first page as a complete requested range.

## Confirmed findings

### E-0039-Q01: actual ordinary quad submission is 001428F0

`00143078` is a wrapper which normally delegates to `001428F0`. A global switch and two specific texture identifiers select a separate path, gated by five texture pointers plus another global; that path submits up to five replacements at different scales, modifies V coordinates with half-texel terms, then restores texture, UVs, scale and render state. It is not a universal single-quad draw primitive. Names and semantic purpose of those special textures are unresolved here. Do not apply its UV inversion or double scale to all widgets.

### E-0039-Q02: four colours are independent

At `00142F24..00142FE8`, widget words `+0x58`, `+0x5C`, `+0x60`, `+0x64` are copied respectively into packet colours `+0x80`, `+0x84`, `+0x88`, `+0x8C`. Their four alpha bytes are independently multiplied by the caller fade scalar and converted with `CVT.W.S` before byte stores. The packet positions establish corner order TL, TR, BR, BL (`00142E90..00142ECC`). This confirms a parser/frame-builder mismatch: `FeWidget` currently retains only the first colour, and `render::Quad` provides only one modulation colour. Gradient bars cannot be reproduced through that contract.

The conversion instruction is `CVT.W.S`, not `TRUNC.W.S`. FCR31 rounding configuration and byte wrap/saturation equivalence remain outside this audit; a C++ cast is not independent proof of rounding behavior.

### E-0039-Q03: all authored corner UVs are consumed

`00142C0C..00142CF4` reads eight floats at widget `+0x70..+0x8C` and fills four packet UV pairs in TL, TR, BR, BL order. For texture dimensions W/H and allocation dimensions PW/PH returned by `0013FF60`, the pairs are:

| Corner | U | V |
|---|---|---|
| TL | W*u0/PW + 0.5/PW | H*v0/PH + 0.5/PH |
| TR | W*u1/PW - 0.5/PW | H*v1/PH + 0.5/PH |
| BR | W*u2/PW - 0.5/PW | H*v2/PH - 0.5/PH |
| BL | W*u3/PW + 0.5/PW | H*v3/PH - 0.5/PH |

An optional animated texture path adds time-dependent U/V offsets before these terms (`001428F0` decompilation). `0013FF60` computes the next power of two for positive dimensions (including 1 -> 2 in its actual loop); zero returns zero. Preserve the actual helper rather than assume a generic library definition.

Current parser discards these eight floats; frame_builder samples the entire image using a single axis-aligned source rectangle. This is a concrete mismatch even before sampler/half-texel equivalence is resolved. A source rectangle alone cannot represent all four independent authored pairs. Applying half-texel shrink blindly to the current Godot path also needs sampler/rasterizer validation; the packet's normalized coordinates target padded original textures, whereas the port resource may expose a cropped bitmap.

### E-0039-Q04: scaled destination dimensions pass through integer conversion

`00142B40..00142B4C` multiplies texture width by widget X scale, executes `CVT.W.S`, and moves the result into an integer register; the corresponding Y path does the same. The non-rotated packet uses floating authored X/Y plus these converted dimensions (`00142E90..00142ECC`). Current frame_builder keeps unconverted float width/height. Rounding mode must be established before claiming exact reproduction. This is not evidence that authored X/Y themselves are truncated.

### E-0039-Q05: rotation is supported and changes clipping

`00142CFC..00142D08` chooses rotation only when widget `+0x44 == 2` and signed integer `+0x40 != 0`. `00142D18..00142D58` converts the angle through float multiply by 65536, `CVT.W.S`, signed integer division by 360, then masks to 16 bits before `0011BA18`. Four local corners are centred on half of the converted destination width/height and transformed around `(x + width/2, y + height/2)` (`00142D5C..00142E8C`). `0011BA18 -> 0011B7D8` constructs an in-plane rotation matrix. Exact table-trig precision and FCR31 conversion remain unresolved; generic host sine/cosine is not established equivalent.

Rotation clears the flag at `00142E4C`; `00142FEC` then bypasses CPU clip helper `00141808`. A replacement must not crop a rotated quad's axis-aligned bounding rectangle as though that were original behavior. `FeWidget` currently drops both rotation fields; `render::Quad` cannot express four transformed corners.

### E-0039-Q06: ordinary axis-aligned clipping adjusts positions and UVs

`00141808` rejects outside bounds and interpolates the clipped packet positions and UV edge values. It leaves the four packet colour words unchanged. Ordinary quad submission calls it at `00142FFC` and releases rejected packets through `00145900`; rotated quads bypass it as described above. The current `crop` has broadly similar positive-area rectangle behavior, but cannot retain arbitrary UV corners or colours, and rejects nonpositive dimensions before any original equivalent has been established.

### E-0039-Q07: signed layers and original traversal

`00146600..00146610` compares two widget layer bytes using signed `LB`, `SLT`, `XORI`: predicate true when first layer <= second layer. `00145F30` calls `00146510` (existing thunk to `00250550`) with this predicate. `00250550` traverses next links and swaps only a false predicate, yielding stable ascending signed layers along next links. `00145F30` draws through previous links (`+4`), yielding descending signed layers. `00145D38` also addresses an eight-byte layer bucket by signed layer.

The current stable descending sort uses unsigned `uint8_t`. It agrees on ordinary 0..127 layer values, but equal-layer equivalence depends on how authored nodes were inserted and traversed; this audit did not establish the construction order. Do not change equal-layer order based on appearance alone. Existing purpose label `HP2_SortListByAdjacentSwaps` in Ghidra is prior analysis naming, not an original recovered identifier.

### E-0039-Q08: draw visibility is exact 1

`00145F30` draws a widget only when its visibility byte is exactly 1; current frame_builder draws every nonzero byte. Original assets may contain only 0/1, so no visible regression is established for the current corpus. Preserve exact comparison in a bounded parity fix rather than infer other flag meanings.

## Recommended bounded implementation and checks

1. Preserve authored four-corner colours and UVs plus `+0x40/+0x44` in the port-side record, with length checks against the complete quad payload; add parser fixtures with distinct values at every corner.
2. Extend the backend-neutral packet with optional independent positions/UVs/colours, retaining existing rectangle callers. Verify host corner order and gradient/UV submission with deterministic fixtures. Preserve original clip's unchanged corner-colour behavior.
3. Separate packet geometry from original sampling equivalence. Confirm padded versus cropped textures and host sampler conventions before introducing the half-texel terms globally.
4. Establish original FCR31 setup and rotation lookup constants before labeling dimension conversion or rotation translation exact. Until supported, retain these as explicit remaining limitations rather than invent rounding semantics.
5. Regression fixtures should cover nonuniform colours, UV crops, negative or flipped UV direction, exact visibility, rotated clipping bypass, and equal-layer construction order once established.

## Bounded implementation and validation

After the evidence pass, TASK-0039 integration requested a backend-neutral packet extension. `render::Quad` now has optional TL/TR/BR/BL `positions`, `source_points` in bitmap pixels, and `colors`, enabled by `has_vertex_data`; `bypass_clip` retains the rotated-quad CPU clip bypass. Existing rectangle fields/callers remain supported. `crop` follows the observed component-wise original UV clipping, leaving corner colours unchanged. The Godot submission adapter uses per-corner polygons and normalizes pixel UVs by the actual uploaded bitmap dimensions. This is a port-side rendering contract, not a recovered ABI or a claim of Godot/GS rasterizer equivalence. The builder owns authored UV conversion and geometry generation. No rotation trigonometry was added here.

`src/tests/frontend_quad_tests.cpp` exercises partial clipping, distinct unchanged corner colours, outside rejection, flipped UV direction and arbitrary rotated positions with CPU clip bypass. Commands on 2026-10-17:

```powershell
g++ -std=c++17 -Wall -Wextra -Isrc src/tests/frontend_quad_tests.cpp src/port/render/frame.cpp -o local/task0039/frontend_quad_tests.exe
./local/task0039/frontend_quad_tests.exe
C:/Strawberry/c/bin/ninja.exe -C build/godot-debug CMakeFiles/ps2_port.dir/src/platform/godot_bridge/ps2_render_host.cpp.obj
```

Standalone compile passed, then `FRONTEND_QUAD_TESTS_OK`. GCC 13.2.0 (MinGW-W64 x86_64-ucrt-posix-seh, r8). The initial fixture used exact float equality and failed on normal interpolation rounding; it was corrected to absolute tolerance 0.0001. Ninja reported the host object up to date (a concurrent parent build may have compiled it); this invocation is not independent proof of a fresh host compile. The generated Godot 4.4.1 binding declaration confirms the selected `canvas_item_add_polygon` signature. Main TASK-0039 owns full build/integration tests, symbols index, task status and progress updates.

## Remaining unknowns and coverage boundary

This is the quad dependency slice only. It does not establish complete frontend fidelity, text measurement/kerning, font bearings, event-number placement, special-texture semantics, all render-state bytes, triangle diagonal/interpolation behavior, sampler equivalence, or hardware comparison. Those require the other TASK-0039 slices and/or original-runtime evidence. No behavior is Verified.


## Parent integration results

The packet/backend extension and native frontend quad fixtures were subsequently implemented by TASK-0039. Parent integrated authored parser fields, draw colours/UVs, alpha dispatch, signed layers and exact visibility. Nonzero mode2 rotation remains a throwing gap. A containing-clip regression corrected right/bottom branch guards to test the original clip bound, preserving arbitrary uncut UV corners. See docs/FRONTEND_RENDER_AUDIT.md and the active task for final validation and original-equivalence limitations.

## TASK-0095 continuation - E-0095-N04

The Q07 construction-order unknown is now resolved by0013FBA0 tail insertion and
001402A0 non-reordering postload: equal-layer CPU submission reverses authored
order. Crucially final visual order also depends on00112858 bucket phases:
24..20,15..0,19..16,28..25. Universal numeric descending is retired. See
[NameEntry render evidence](task0095-name-entry.md) for original call sites,
captures and bounded native/Godot validation. No original equivalence promotion.
