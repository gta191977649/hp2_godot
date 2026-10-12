# TASK-0017 Audio Options viewport

US PS2 SLUS_203.62 revision unknown, ELF SHA-256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
Read-only Ghidra MCP Client, 2026-10-13. Raw exports under ignored
local/task0017/scroll; no original-runtime comparison, not Verified.

## E-0017-S01 — scrolling state and exact widget membership

Audio registration wrapper001F0278 constructs001EF3C8. Its 00249DE0
object uses first/last authored row hashes MENUITEM_1/7 for top/bottom Y,
UP_ARROW EDF2E4EE, DOWN_ARROW20D73D01, clip hashC67CFB4A.
Audio constructor copies26 hashes from initialized002B9C30..002B9C97:
MENUITEM_1..9, MENUITEM_10/11 hashesAEE77883/84, AUDIO_BAR1..7,
AUDIO_BAR1_V..7_V, MONOSTEREO. SONGNAME, fixed MI masks, under/overscore,
footer, title, background and arrows are not moved by this list.
001EF3C8 sets total row count11. Widget paths00249C78 snapshot authoredY,
current velocity0, endpoint velocity0, duration0.5, normalized limit1.
00249D70 adds a target offset to each path, preserving its current velocity.
00249DA0 updates001DAEC8 and writes floatY through00140A48.
This is actual original half-second cubic Hermite motion, not an invented tween.

0024A480 handles8/9 before menu navigation, decrementing/incrementing a fixed
viewport slot, clamped0..min(7,total)-1. Initial message4 assigns slot0..6
from selected MENUITEM_1..7, all later rows map6. 0024A120 compares selected
widget's currentY with authored top/bottomY. If out of bounds,0024A090 shifts
all26 targetY values by boundaryY minus selected targetY. This distinction
matters during rapid navigation before the previous animation finishes.
0024A368 returns the fixed MI_1_MASK..MI_7_MASK for the current viewport slot;
table0031CFA0 is populated by0024A888 from exact strings002BF018..002BF087.
Do not place bars against MI_8..11_MASK while those rows scroll.

Arrow visibility uses any targetY above/below bounds (0024A3E0/0024A430),
only when totalcount>7. 0024A1A8 advances arrow alpha sine phase with angular
rate10, wraps once at6.283 using strictgreater-than, maps[-1,1] to[0,128].
00249DE0 initially sets both alphas64. std::sin matches the mathematical
mapping but does not reproduce original0025A5E0 table quantization.

## E-0017-S02 — original clipping and asset viewport

00249F70->00140A70 assigns each listed widget's+0x4C clip-widget name hash.
For text, it sets+0x58 bit0x10 via00140C68. FeWidget now preserves this field
from assets and copies it in001410A0 ghost transform duplication.

Renderer00145F30 finds the clip widget with00143470. A missing/zero clip
hash disables clipping, matching the original. For text bit0x10 is required:
rectleft=maskX+4, right=maskX+integer-width, top=maskY,
bottom=maskY+integer-height. 00142260->00141A98 uses00141808 to reject
outside glyphs and intersect surviving glyph geometry while adjustingUVs.
For quads the renderer sets the temporary scissor to maskX/Y plus float
width/height via00141798, then restores default(0,0,644,480)001417D8.
Thus clipping is real geometry/UV clipping, not hiding entire menu rows.

Configured FRONTB.BUN screen28 has authored SCROLLING_MENU_CLIP_REG
(hashC67CFB4A), X20,Y73, scale43.125x16.25, textureE2A5C626.
The host must measure its runtime original texture instead of hardcoding
viewport dimensions. GetWidgetClip_00145F30 exposes the original bounds
through the existing WidgetMeasure seam. The default644 edge vs640host
canvas remains an explicit host equivalence limitation.

## E-0017-S03 — Audio selection bars

001EFB80 message4 obtains mask0024A368 and invokes001F4E68.
001F4E68 puts underline at maskbottom and overscore at masktop-6.
Main-menu00216880 instead puts underline at maskbottom-3; do not reuse
that offset for Audio. Native PlaceSelectionBars_001F4E68 implements the
common screen helper geometry; other side effects remain host seams.

## Integration and checks

Construct Type_00249DE0 with ConstructAudio_001EF3C8 before menu's initial
highlight callback. Forward messages8/9 and4 to Handle_0024A480 in their
original callback order. Tick Update_0024A1A8(delta) before rendering;
use Sub_0024A368 for Audio selection bar mask. Export per-widget clip rect
on each frame and intersect glyph/quad destination plus UV as the renderer did.
No generic screen-wide group offset is warranted by this evidence.

Synthetic test src/tests/frontend_viewport_tests.cpp passed FRONTEND_VIEWPORT_OK:
`g++ -std=c++17 -Isrc src/tests/frontend_viewport_tests.cpp src/game_core/frontend/fmenus.cpp src/game_core/assets/elab_chunks.cpp -o local/task0017/scroll/frontend_viewport_tests.exe`
then `& local/task0017/scroll/frontend_viewport_tests.exe`.
Compiler g++13.2.0 MinGW-W64 x86_64-ucrt-posix-seh. Covers membership,
clip-bit and textleft inset, midpoint/final row shift, unrelated widget stability,
arrow visibility, fixed mask slot and copied ghost clip binding.
Root owns CMake, bridge/Godot render, screenshot and shared index integration.
Other screen uses of00249DE0, exact sine-table quantization, arbitrary text/quad
rendering differences and original comparison remain unresolved.

## E-0017-S04 — selector arrows and Audio navigation boundaries

Audio constructor001EF3C8 also constructs0024A690 with LEFT_ARROW hash
D1CDA594 and RIGHT_ARROW9345E4A7. 0024A690 initializes phase0 and immediately
binds target0 through0024A860, hiding both arrows. 001EFB80 message4 binds
MONOSTEREO2932DFCA only for MENUITEM_8 hash52E039FA; all other items bind0.
0024A730 ticks sine phase10rad/s with the same0..128 alpha wave as up/down.
When target is nonzero,00144860->001447E0 gets the target's bounding box and
computes integer vertical centreY0+(Y1-Y0)/2. 00140930 centres each arrow by
subtracting its measured height/2 and writes floatY. X remains authored.
Arrow target is the actual moving MONOSTEREO widget, not the fixed viewport
row mask or the original footerY. Tick after widget scrolling so it follows
current positions; Bind immediately changes visibility.

Audio001EFB80 messages8/9 intercept navigation boundaries: message8 with
currentMENUITEM_1 sets event+8 to1; message9 with currentMENUITEM_11
(AEE77884) also sets event+8 to1. MenuList0016A458 honors this result and
skips Previous/Next. Audio does not wrap at its ends. A hypothetical odd
fixed-slot underline after wrap is therefore not a reachable original path.
The handler's raw decompile is retained at local/task0016/scroll/001efb80.json;
new selector and geometry exports at local/task0017/scroll/0024a690.json,
0024a730.json,0024a860.json,00144860-more.json,001447e0.json.

Native Type_0024A690 implements constructor, bind and update with the existing
measurement/time seams. Added synthetic checks for immediate visibility,
integer target centre, moving-target tracking, authoredX preservation and hide.
The standalone compiler/run command above again passed FRONTEND_VIEWPORT_OK.
No settings activation logic was broadened by this correction.

## Host integration validation

Audio viewport and selector are constructed before initial menu refresh. Screen
messages4/8/9 retain001EFB80's fixed masks, selector target and end guards. Paths
tick before selector positioning. Godot intersects destination and UV rectangles
against native widget clipping bounds; partial glyphs are retained. Centre alignment
tests bit4, independently of clipbit16. Measurement completion places selection
bars using the current fixedviewport mask.

Actual DX12 initial/scrolled/AudioMode captures inspected in ignored
local/shots/task0017-audio-*.png. SubmenuSmoke tests clip, fixedfooter, selector
and both nonwrapends. Native/bridge CTest18/18PASS. OriginalGS and sinetable
byte-equivalence remain unverified. Quote comma-containing MenuEvents arguments
in PowerShell; unquoted commas may be parsed as arrays.

## E-0017-S05 � other Options submenu usage

Read-only references to00249DE0 yield exactly three direct callsites:
001B844C (race-setup base001B8390),001EF4BC (Audio001EF3C8), and
001F3414 (Display001F33B8). This is a bounded direct-call result; it does
not imply that all other menu behavior is implemented.

| Inspected class/constructor | Same viewport object |
|---|---|
| Audio001EF3C8 | 26-widget list, total11, original first/seventh row bounds |
| Display001F33B8 | 15-widget list, total9, same bounds/arrows/clip hashes |
| JukeBox Music001F02D8 | Does not construct00249DE0; initializes six song-row storage entries |
| Joystick PreConfig002400C0 | No00249DE0 constructor call |
| NameEntry0024D408 | No00249DE0 constructor call |
| Username Options001F1280 | No00249DE0 constructor call |
| Phils Load Save00247530 handler | No00249DE0 call; separate memory-card state machine |
| Race Setup001B8390 | Constructs00249DE0; list population/count mutation not recovered here |

Display initialized table002BA178 supplies the exact15 hashes:
52E039F3..52E039FB (nine items),55CB09F2,2E931F1B,03D90172,
5610F2F4,8FE4086B,422313CB. Its constructor separately assigns cliphash
C67CFB4A to LEFT_ARROW/RIGHT_ARROW with00144358. The generic native
Construct_00249DE0/Add_00249FF8 now support original argument seams and
public originalfield_0x34 totalcount. ConstructDisplay_001F33B8 supplies
these exact arguments/list/count9; Audio wrapper retains26/count11.

Display001F3A28 message4 binds hintIDs24CAC,24CAD,24CAE,24CAF,
24CB1,24CB2,24CB3,24CB4,24CB5 in item order. Selector targets for the
firstsix items are55CB09F2,2E931F1B,03D90172,5610F2F4,422313CB,
8FE4086B; lastthree bind0. It uses the viewport fixed-mask slot for bars.
Messages8/9 block navigation atMENUITEM_1/MENUITEM_9 respectively.
Settings snapshot/source semantics remain separate unresolved work.

Synthetic Display checks cover all15 clip assignments, separately clipped
selector arrows, rowcount9 and last-row displacement with value widgets.
Standalone compile/run again passesFRONTEND_VIEWPORT_OK. Root owns any
additional Display/Juke routes; no arbitrary short-menu viewport was added.
Raw evidence:00249de0-refs.json,002ba178.json,001f3a28.json,
001f02d8.json,001f19e8.json,001f1280.json underlocal/task0017/scroll;
Display/base prior constructors remainlocal/task0016/scroll/001f3454.json
and001b848c.json. Juke song pagination/list semantics are unresolved and
must not be replaced by Audio scrolling.
