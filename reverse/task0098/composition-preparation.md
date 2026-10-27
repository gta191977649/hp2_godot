# TASK-0098 shared alpha and Flailer depth preparation

E-0098-F04. Authority: Debug NFSHP2.ELF, SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`;
revision/date unknown. Readonly original captures in ignored compositor and
compositor-runtime directories, hashes in evidence.json. This adds native command
preparation, not a GPU compositor or original-runtime verification.

## Confirmed and implemented bounded slices

RadixSortAlphaKeys00104FE0 performs byte-radix passes on float bit representations,
a signed high-byte reversal for negative keys, rendering-order buckets (outside0..7
maps7), followed by SortAlphaKeys001053B8's reverse-list rebuild. Native port indices
preserve that order, including equal nonnegative keys reversing input traversal and
equal negative keys retaining traversal. No numeric float comparator replaces it.
Original counters are16-bit; native preparation supports at most65535 keys and
explicitly rejects the unimplemented overflow domain. Original FrameMalloc/DMA and
failure-return behavior are not emulated. An independent bit-order comparator
checks randomized inputs through65535 keys, signed zero and nonfinite bit patterns.

Flailer ePoly::Render2D0010FE48 reads table002C1530. Layers20..24 use table depth
directly; other supported frontend layers use ZMax-table. GS packet Z is integer
conversion then left shift4. Alpha sorting always uses table-ZMax, including the
background layers. The native helper takes the resolved original16/24-bit ZMax
explicitly, reads the caller's SHA-bound Debug memory and rejects unsupported layer/
conversion domains. It retains the existing host CVT.W.S projection; original FCSR
rounding remains unverified. No proprietary table is baked into the port.

FrameBuilder now retains authored frontend_layer on every emitted quad, including
expanded glyphs and startup overrides. Existing stage ranges, flat order and clip
behavior remain intact. Packet concatenation preserves each layer so a compositor
can honor per-layer original flushes and derive native depth later. Legacy port
quads retain layer-1; depth preparation rejects that missing provenance.

## Required next implementation

E-0098-F03's corrected flush order remains binding. Stages2/4 do not flush their
pending alpha; stage5 sorts their shared domain. UI target0/car target1 resolve to
the same physical colour AND depth in frontend FULLHEIGHT. Runtime capture audit:
SetScreenBuffers00106320, eInitViews001079B8, MaybeChangeViewMode00107BD0,
eSwapScreenBuffers00107E68, UpdateGifTags001058D8, CreateViewMatricies0010EA18,
CalculateViewMatricies00106F18 and SetupMatrixPacks00105668.

A GPU executor still needs explicit native ordered submissions into shared colour/
depth, target/scissor/pixel-offset/Z-mode/masks/tests, separate solid/alpha matrix
packs, per-strip transformed AABB keys0010CFF0 and original texture context/blend.
The ordinary Node3D plus CanvasItem queues cannot establish this contract.
apply_frontend_pose still does not consume Reflection's intrinsic0x11 flags. No
flattened viewport substitution or full original compositor claim is made.
