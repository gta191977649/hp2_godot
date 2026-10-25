# TASK-0098 locked-car presentation dependency

E-0098-L02 implements the bounded native CarLockedDisplayer state and its grouped
AnimationGroup in port-owned typed storage. Authority: Debug `NFSHP2.ELF`, SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`;
executable revision/date unknown. No Retail ABI, original declaration file,
complete selector or original-runtime equivalence is asserted.

## Confirmed evidence

- `00192770` saves authored floating EndPosition, signed-byte MaxAlpha and a
  grouped slide starting at640. `00241C38` saves the master's integer Y and sets
  original spline duration. The background's authored200 alpha is therefore
  cached as -56, not clamped to128 or interpreted as positive200.
- `00192800` gates all visibility, description formatting, slide reset and timer
  reset on VisibleCar pointer identity. Changed unlock flags on the same pointer
  do not refresh this state. Description formatting also runs for unlocked cars.
  Selection changes restart at650 (`0032559C` bits44228000), distinct from the
  constructor's640. Its formatter can bind the recovered priority projection below.
- `001928F8` listing establishes SUBU32 followed by signed CVT.S.W, scaled by
  `003255A0` bits3983126F. Delay is strict `elapsed > .4`, duration `.5`;
  `00327AE0..E7` contains3F000000/3ECCCCCD. The slide advances with supplied
  RealTimeElapsed only after the delay. Overlay fade instead uses total timer
  elapsed and restores signed MaxAlpha once its factor reaches1.
- Grouped `00241DB8/00241EB8` retargets and publishes original cPoint::SplineSeek
  output after integer conversion. `001450F8` captures the current master's
  position before backwards traversal, preserving each group object's relative
  offset. `001452D8` group visibility writes Visible without marking dirty.
- `00143E60/00143F30/001446A0` return zero for missing position/alpha widgets;
  `001446D8` ignores a missing alpha widget. Existing reviewed frontend storage
  setters carry the bounded semantic widget mutations.
- `001929D0` keeps authored text2A67E2D5 hidden and explicitly submits the formatted
  blurb through rectangle2A65DAF9 at scale1 only when the current car is locked
  and unlock-all is zero. Its frame allocation/rectangle rendering is a required
  service, not an ordinary visible text-widget replacement.
- `00244A68` prioritizes task reward, game7/player0 diagnostic, event reward,
  points reward, then diagnostic fallback. `00244490/002444E8` searches all ten
  task rows00314510 at56-byte stride, with UnlockedInGameType=-1 wildcard and
  first-match order. `002446C0/00244700` searches nineteen points rows00314740
  at12-byte stride. Typed native decoding reads only the original accessed fields
  from caller-supplied SHA-bound Debug memory; it embeds no proprietary table.
- `001FA990/001FAA58/001FAE10/001FAED0` establishes forward tree list, event and
  reward order. Reward type0 and unsigned byte1 select the car; first match wins.
  The full `00244A68` listing, unlike its decompiler's incomplete varargs display,
  passes **signed** EventInfo nNumber to C030 (tree0) or C031 (tree1), and passes
  RequiredPoints to BFB8. A first matching unknown tree yields its original
  diagnostic without continuing into another matching tree or points reward.
  Task descriptions have no numeric argument. Original localized printf dialect
  remains an explicit required service receiving the original ID and optional
  signed numeric argument; no guessed localization or host format is inserted.

Source captures: existing ignored `local/task0096/selectcar-agent/00192770.json`,
`00192800.json`, `001928f8.json`, `001929d0.json`, `00241c38.json`,
`00241db8.json`, `00241eb8.json`, `0032559c.memory.json`,
`00327ae0.memory.json`; fresh read-only MCP captures under
`local/task0098/locked/` include type, helper decompilations and full Update listing.
The initial memory query used the wrong `size` argument; it returned
`Missing: length` and supplied no evidence. The corrected query and existing
hex capture retain the actual constants. No Ghidra mutation was performed.
Additional fresh event lookup/type captures are `001faa58.json`, `001fa990.json`,
`type-EventReward.json` and `type-eTreeType.json` under the same ignored directory.

## Implementation and limitations

`src/game_core/port/frontend/selection_locked.{hpp,cpp}` is explicitly a
port-owned projection, not a claimed original file/ABI. It uses original
VisibleCar identities, existing authored FeScreen storage and recovered cPoint
spline arithmetic, without Godot dependencies. FormatUnlockBlurbForCar now has
its own table/priority projection; the host must provide the original ordered
event list, SHA-bound table decode, localized formatting and owning frame's
original rectangle render service. Missing required services throw at their original addresses.
Formatting capacity is bounded at the original256 bytes. This dependency has
not been attached to production selection or an original frame-stage consumer.

No new hypothesis is used. Actual localized formatting and original text
wrapping/rendering, original destructor restoration, production
route, visual comparison and full loading/reentry acceptance remain unresolved
outside this projection. PC tests do not establish Verified.

## Validation 2026-10-26

- `cmake --build --preset native-debug --target selection_locked_tests --parallel 4`:
  passed with CMake3.29.2 and existing GCC13.2.0 configuration.
- `ctest --preset native-debug -R '^selection_locked$' --output-on-failure`:
  1/1 passed0.46s after formatter additions. Covers constructor640 versus reset650, fractional authored
  positions/group offsets, signed alpha wrap, strict delay equality, signed
  wrapping packed-time subtraction, pointer gating, unlocked formatting,
  render suppression/rectangle arguments, missing widgets and missing services.
  Also covers both original table strides/final rows/signed wildcard, all
  description priority branches, duplicate task/event order, signed event
  nNumber, points values, unknown-tree early return, noncar reward exclusion,
  missing localization and explicit over-capacity formatting rejection.

No original-runtime or window comparison, performance or Verified promotion.
The exact next integration action is to bind LockedDisplay::Update to the
original selector Tick position and Render to its reviewed SCREEN_RENDER stage,
with real description formatting and frame-owned rectangle renderer services.
