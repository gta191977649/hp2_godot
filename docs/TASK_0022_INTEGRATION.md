# TASK-0022 continuation: native event UI integration

Binary: SLUS_203.62, SHA-256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
SYSTEM.CNF VER 1.00; executable revision unknown. Read-only Ghidra MCP and the
player's configured ZZDATA are the only original-behavior sources. No Ghidra
annotations or database changes were made. The ELF hash was checked again with
`Get-FileHash -Algorithm SHA256 D:/ps2_game/SLUS_203.62`.

## Confirmed findings and implemented scope

- E-0022-R01: 0x002491B0 accepts document, text-position hash, alpha-target hash,
  and bar hash. 0x001F96B8 supplies EVENT_TITLE_BAR for both text operands and
  WHITEOUT_BAR for the bar. The existing scrolling-text class now accepts these
  operands and is reused by screen 39; its former hardcoded SCROLL_TEXT hash was
  inappropriate for this caller.
- E-0022-R02: original FRONTB.BUN document hashes match NEW_EVENT_TREE (screen 39,
  0x147F8A59), TREE_DETAIL_VIEW (17, 0xA744BA7B), and TREE_REWARD_VIEW (19,
  0xF1EED4CD) under StringHash_002534A8. These are hash-matched names, not recovered
  debug symbols. The remaining document 18 name is unknown.
- E-0022-R03: 0x001F8890 writes OVERLAY_STATUSTITLE (0x87CFD6C6),
  OVERLAY_REWARDTITLE (0x35B77247), and SMALL_EVENTNUMBER (0xCEA3B7C2).
  Status text comes from 0x001F60E8; Reward text from 0x001F6220; the event-number
  format is localized ID 0x111AF. 0x001F8938 fades the overlay using 0.15 seconds,
  updates its text after that interval, and slides its anchor from 640 to 382
  through 0.35 seconds. Constants at 0x0033174C..0x00331758 are float bits
  3E19999A, 3E19999A, 3EB33333, 43BF0000. 0x001F8A98 backdates initial time
  by -0.15 seconds. FE_ENGLISH.BIN resolves title IDs 0x272A/0x272D to
  Championship/Ultimate Racer. Earlier prose calling 0x272A World Racing is superseded.
- E-0022-R04: 0x001F6770 draws document 17 and passes a separate 512-byte detail
  buffer to 0x001451E0. That function explicitly draws TRACK_LINE_1 inside
  BLURB_BOX through 0x00144ED0, even though the template's authored visibility is
  zero. Its authored text capacity is not the paragraph output bound. Whitespace
  lookahead wraps words; '@' starts another row. The row step is the original
  font line height. The native frame builder has an explicit text-box projection.
  Original special inline controls and full kerning are not all reproduced by
  this projection; original equivalence remains unverified.

Core text/data recovery is detailed in [TASK_0022_DETAIL_TEXT.md](TASK_0022_DETAIL_TEXT.md).
Car and track names are supplied by EventNameCatalog: original GLOBALB.BUN track
rows and the ELF's metadata mapping, with localization from FE_*.BIN. No asset or
decoded texture is copied into Git.

## Input and transitions

One guarded native input dispatch now handles the event tree and reports missing
dependencies through MenuState. The script no longer bypasses audio/error handling
through an unguarded separate bool call. Back uses JOY_EVENT_MENU_BACK, as confirmed
by manager 0x0016CAB0; MENU_CANCEL is not substituted for it.

0x001F6B70's locked selection answers sound 0x17. Positive status answers 0x16 and
reaches 0x001F6A28, whose unrecovered car/race settings dependencies throw. Square
from detail requires record+0x3E >= 1, then enters reward preview with sound 0x18;
the missing model renderer is explicitly reported at 0x001F6440. Preview Back/Square
enters state 6 and returns to detail after the shared cubic finishes. State 0 Back
stores the cursor in the live profile and returns through existing history.

The transition predicates, state writes, and fade operands come from
0x001FA458/0x001FA698 and E-0022-T01..T12. The original detail zoom is 3.0, separate
from the browsing zoom levels 0.4/1.0. State 1/3/4/6 lasts 0.5 seconds, using the
same zero-velocity cubic as the existing screen-exit animation. No Godot Tween is used.

## Geometry and effects

E-0022-G01..G07 remove the unsupported panel fit and selected-node size multiplier.
The actual leaf draw uses authored widget scale and the current matrix. Connectors
use original HLINE/VLINE textures, recovered colors, shared horizontal runs,
panel-edge vertical endpoints, and BOTTOM_MASK clipping. Event numbers use their
original authored widget and font. Selection corners use the original 1.05 bounds,
15% runs, and recovered polynomial pulse, advancing phase by 0.2 per render.
Original FCSR rounding mode and original floating-point equivalence remain unknown.

Unknown special-node gates are preserved as diagnostics with status/visibility -1
(port-side unknown sentinels), not silently labeled Locked. Their artwork is skipped.
The cross-tree event 32 gate can be rejected from a known non-winning earlier event;
its fully-completed path still depends on unresolved event 31 globals.

## Validation and continuation

Native and Godot validation results are recorded in TASK-0022 and PROGRESS.md.
Private DX12 captures are under ignored local/task0022/. They prove native output
was rendered, not original-game equivalence. No function is Verified.

Reproducible capture trace (from the project root, configured assets required):

```powershell
& ./local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe --path godot res://scenes/frontend_native.tscn --quit-after 600 -- --screen=2 --scale=1 --menu-events=4,4,9,wait:1.2,9 --capture-after=0.8 --capture=D:/dev/hp2_godot/local/task0022/championship-detail.png
```

The wait operands are explicitly port-side test infrastructure. They let normal
runtime ticks finish recovered animations between input events.

TASK-0022 remains active until all acceptance criteria have evidence. Continue with
0x001F6440's car/track model renderer dependencies, 0x001F6A28's selection/race settings,
the event 31 completion globals, and TASK-0023 original comparisons. None of these
gaps may be replaced by invented output or described as complete.
