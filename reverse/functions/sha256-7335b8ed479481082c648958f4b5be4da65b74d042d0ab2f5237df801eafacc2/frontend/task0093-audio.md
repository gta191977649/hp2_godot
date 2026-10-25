# TASK-0093 - Audio Options and scalar animation

Retail SLUS_203.62 US PS2, revision unknown, SHA-256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
Read-only Ghidra MCP2026-10-25; raw captures ignored in
local/task0091/options-agent,popup-agent and001DA*.json. No database changes.

## Confirmed - E-0093-A01

001EF3C8 constructs seven0016DC20 sliders for menu items1..7, snapshot volume
order/categories5,4,0,1,2,3,6. Conversion factor is0.7/category default.0016DD38
adds raw delta(+/-0.05 from00331688), multiplies factor, clamps normalized0..1,
divides factor, writes raw amplitude, applies category gain and requests changed
feedback via001AF760(normalized,"MenuAdjustVolume", string002B2B48).
Raw maximum is default/0.7, not1. Constructor snaps current/target normalized
amplitude; path duration0.25 seconds. Retargeting preserves current derivative.

001DA780 coefficients are a=(v0+v1)-2*(target-current),
b=(3*(target-current)-v1)-2*v0,c=v0,d=current.001DA7D0 uses Horner position;
001DA800 uses(a*t*3+b+b)*t+c for derivative.001DAEC8 resets normalized time on
phase2, builds coefficients, advances delta/duration, and publishes target value
AND derivative when reaching limit1. This is a scalar-only host projection of the
zero external limit arguments used by0016DE30; no general constrained path claim.

## Confirmed - E-0093-A02

0016DE30 measures full track rectangle and sets marker X to
int(left+normalized*(right-left)), preserving integer endpoints and truncation.
Track hashesA29F7A16..1C, marker hashesC866642B/686C/6CAD/70EE/752F/7970/7DB1.
001444A0->00140A20 changes X, stamps dirty4, retains Y/scale.

001EFAA0 cycles output0..2;001EFB38 sets MONOSTEREO2932DFCA IDs1ADB9/1ADB8/
1ADBB, original FE strings Mono/Stereo/Dolby Pro Logic II.001EFB80 handles
restore-volumes AEE77883 via001AFAB8, restore-music AEE77884 via001AF1E0
(authored modes,zeroA0/A4/A8/AE,maskFF), Jukebox52E039FB and conditional Back
snapshot commit/Autosave. Existing viewport, blurb and nonwrapping guards remain.

## Tests

frontend_audio covers nonzero-derivative retargeting, endpoints, raw and normalized
bounds, marker-X geometry, original volume feedback, live music gain, output cycle,
restore defaults and Back. frontend_jukebox route/persistence regression also passes.
Exact host build/capture results are recorded in TASK-0093.

## Hypotheses and unresolved

No hypotheses feed implementation. PC scalar tests do not certify PS2 exceptional
FPU or SPU behavior. Speaker-mode hardware encoding, full PS2 sound-category/device
state and original-runtime comparison remain unresolved. Native host keeps seven
volume settings; only categories used by frontend music/menu samples have live
audio consumers here. This is not a claim that unavailable racing channels play.
