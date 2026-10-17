# E-0050-R01: track render scale investigation

Read-only investigation of the reported Lotus/large-road-sign screenshot.
No renderer, geometry decoder, physics or asset values were changed.
Status: static binary/asset corroboration, not original-runtime Verified.

Retail ELF SHA-256:
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
Debug ELF SHA-256:
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
Raw read-only Ghidra captures and private byte probes: local/task0050.

## Observed coordinate and instance behavior

Retail0011C2B8 reads XYZ pointers from0011BF98 and copies the three words.
0011BF98 computes packed float-lane addresses only; neither introduces a
coordinate scale. This matches the current float VIF geometry decoder.

Fresh Retail DrawAScenery0014FE18 independently confirms the current instance
projection: signed rotation halfwords at1C..2C are multiplied by6.1035156e-05
(exact1/16384), position10..18 is copied, homogeneous W becomes1. Flag200
passes a null world matrix instead. This corroborates earlier Debug0014FA68
without relying on its version as the sole Retail authority.

For Track11,8306 of8407 stored instance matrices have all three basis lengths
rounding to1.0000. Minor quantized deviations and explicit authored scales such
as1.6 and0.75 are retained; no common4/128 factor appears. The host axis
permutation(x,y,z)->(y,z,x) is length-preserving and its basis transformation
implements P*M*P^-1. No uniform scale is introduced there.

## Independent render/topology alignment

Original TRACKB11.LZC packed SHA-256:
8f254c9415e980d5f7dedb4ff5d0d4c69eff0e21a07163a1dedfa2cccdcee4a2.
Decoded SHA-256:
0be2ebbb6a11b23e3cc4f0a2f748d15457562999d63ff6ecd0209465ad12b397.

The original ForwardStartingPosition marker is(-98.0809173584,-732.541687012).
Nearby RD_SECTION70_CHOP1 and RDDRT_SECTION70_CHOP1 instances carry flag200
and zero instance positions. Raw road XYZ values therefore enter world space
unchanged. A road vertex(-130.4987640381,-732.9916992188,-89.6064910889)
is within0.00887397964 three-dimensional native units of independently decoded
topology vertex(-130.5,-733,-89.609375). The dirt-road candidate has a nearest
corresponding topology vertex distance0.021290469. These support a shared native
coordinate scale; they are not a full surface-coincidence or numerical-physics
certification. Multiplying only rendered track geometry by4,1/4 or128 would
break these measured coordinate correspondences.

## Diamond-sign family dimensions and independent original bounds

The screenshot shows the back of a diamond sign with a smaller rectangular
panel, consistent with the candidate XS_CURVE35/50 sign family. Its exact
instance cannot be identified from the screenshot alone; no identity is asserted.

Track11 XS_CURVE35L_S1_00 (hash6425D644) at decoded solid header7E630 has
raw decoded mesh dimensions(2.47108553,0.18857556,4.97074020). Original header
AABBMin is(-1.21620059,-0.100674756,0.0476432815), AABBMax is
(1.27488494,0.107900806,5.03838348). These independently preserve approximately
five native units of vertical extent rather than an omitted small divisor.
The analogous35R/50L/50R S1/S2 meshes have the same size within float variation.

Original eSolid's imported header names+60 as PivotMatrix. Every candidate
XS_CURVE35/50 sign PivotMatrix has an exact identity upper3x3 and only an
authoring translation. Applying or omitting it cannot supply a missing shrink
factor for this family. All candidate animation hash/pointer fields28,2C,A8
are zero in the original resource headers.

Independent original SceneryInstance bounds agree with this tall extent:

- Decoded instance6FA940, XS_CURVE35L_S1_00: original integer AABB
  (-605,244,-63)..(-601,247,-57); origin(-603.05859375,245.73918152,-62.46289825),
  Z basis(0,0,16384), SceneryInfo.Radius5.029284954.
- Decoded instance742130, XS_CURVE50R_S2_00: original integer AABB
  (-169,-631,-83)..(-165,-629,-77); origin(-167.04772949,-630.07543945,-82.14265442),
  Z basis(0,0,16384), SceneryInfo.Radius4.924538612.

These are original scene-space culling bounds, separate from our vertex decoder.
They refute shrinking the candidate sign geometry as a decoding correction.
They do not establish which sign the screenshot contains or realistic road-sign
dimensions; no real-world-size analogy is used as original-game evidence.

## Static draw path and PivotMatrix concern

Read-only Retail eModel::Render0010DC80 passes its supplied world matrix
unchanged to RenderStrips0010DD58 on the ordinary nonanimated branch.
0010DD58 retains that matrix in uStack_f0 and passes it to00104B88.
SetLocalWorldMatrix00104B88 stores the same pointer and queues its four vectors
at VU348. No eSolid+60 PivotMatrix multiplication appears in this static path.
The Debug functions0010E048/0010E120 independently name and corroborate the
same model-render/strip-render behavior. Animated paths are separate and are
not used to infer ordinary sign scale.

## Conclusion and remaining unknowns

No track-wide render scale bug was found in the inspected path. Raw render XYZ,
topology units, Retail instance quantization, sign scene bounds and static draw
matrix flow agree with the current adapter. The screenshot alone is insufficient
to justify changing those values. A separately measured vehicle/camera audit
and identification of the exact screenshot location remain necessary before
choosing any corrective source change. Original frame comparison remains open.
