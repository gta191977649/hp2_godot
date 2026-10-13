# The original source tree and class names (recovered directly from ELF strings)

> **TASK-0020 update (2026-10-13).** The scan was repeated without Ghidra, by parsing the ELF's
> PT_LOAD program headers directly and mapping every printable-ASCII run's file offset to its
> virtual address (`reverse/naming/task0020-original-source-tree.csv`, E-0020-N01..N03). Result:
> 8,153 strings, **296 source-path hits / 54 distinct**, 74 distinct `Class::Method` strings and
> 23 `t`-prefixed pool names. That matches the TASK-0016 rescan (298/54) and confirms the
> original 93-hit figure on this page was truncated. The recovered root prefix is `..\indep\src\`,
> with the leading `..\`, not `indep\src\` as written below.
>
> `src/game_core` was restructured onto these paths in TASK-0020: the seven files whose original
> path is recovered verbatim now sit at it, the bWare `bJukebox` pair sits under `bSound/`, and
> everything with no recovered path moved under `src/game_core/port/` so no file's location
> implies provenance it does not have. **No identifier was renamed**: every `Class::Method` and
> pool string here is still `string_only`, and pinning them needs the cross-references only
> Ghidra MCP provides, which was not connected for that task.

> TASK-0016 correction (E-0016-N01): the original count of 93 below was not a complete scan — the
> 50 `.hpp` hits were a truncated page. A proper rescan of the canonical address space through
> Ghidra MCP on 2026-10-12 found 43 `.cpp`, 246 `.hpp` and 9 `.h` strings, 298 path matches in all
> (54 distinct byte strings). Rechecking all 43 `.cpp` addresses, 40 do have PARAM references, so
> this page's original claim that "none of them has a cross-reference" is disproved.
> `fmenus.cpp`, `lzcompress.cpp` and `movieplayer.cpp` still report no references. The method text
> also yielded a fuller catalogue; the presence of a string is not confirmation of a function's
> identity. Exact counts, addresses and the per-file recheck are in
> [NAMING_AUDIT.md](NAMING_AUDIT.md) and
> [the original identifier catalogue](../reverse/naming/task0016-original-identifiers.csv). The
> historical text on this page is kept, but where its counts or its blanket "no references"
> conclusion conflict with the new evidence, this correction wins.

Completed 2026-10-12, Australia/Sydney. This is not the output of one subsystem's task — it is
**project-level** evidence, because it covers several subsystems at once (the bWare middleware,
the front-end menus, cameras, sound, replay, AI and the movie player). Any later task touching
those subsystems should reference this file and prefer the real original identifiers recovered
here over inventing more port-side names.

## Evidence and method

A read-only string search over the authorized ELF (`SLUS_203.62`, SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`) through GhidraMCP
(`memory search`, search_type=string) for `.cpp` and `.hpp` returned 43 + 50 = 93 hits. These
strings are almost certainly `__FILE__` expansions inside `assert()`, debug logging or profiling
hooks — a common pattern with the PS2 SN Systems toolchain. **This session found no
cross-references** to them (`references_to` came back empty for every exact address checked),
which most likely means the corresponding assert/profiler code was compiled out of the retail
build while the string data still linked into the final ELF. **So: we have file paths and nearby
class/method name strings, but have not pinned them to specific call-site addresses** — an
explicit open item, not a confirmed "function X lives in file Y at line Z".

## The recovered original source tree

The compile-time paths carry no drive prefix, so recovery stops at this granularity (the build
system probably invoked the compiler with relative or mapped paths):

```text
indep\src\
├── fmenus.cpp                          # the front-end menu system (see below; lines up closely with TASK-0011)
├── frontendscreens\
│   ├── selectcar.cpp                   # car select screen
│   ├── racesetup.cpp                   # race setup (Race Setup / Knockout Setup / Lap Knockout / Free Run / Select Car2P)
│   ├── options.cpp                     # options screen
│   └── joystickconfiguration.cpp       # controller configuration screen
├── car.cpp                             # vehicles (CloseCars/CloseCops/CarSlidingSideways/CarCollision/FlattenFern…)
├── racecoordinator.cpp                 # the RaceCoordinator class
├── cameramover.cpp                     # cameras (holds the CM_CUBIC_DAMPER enum and MenuCarDispl… strings; overlaps the front end)
├── zonecameras.cpp                     # ZonePreview
├── player.cpp                          # Player (ScorePoints and friends)
├── carinfo.cpp                         # car info (texture name templates such as %s_REVERSELIGHT_ON)
├── spline.cpp / quickspline.cpp        # splines
├── carsounds.cpp                       # car audio (CopSiren0/TirePop1/SirenSound/HornSound3…)
├── table.cpp                           # (PidError…)
├── animateddriver.cpp                  # the AnimatedDriver class
├── sound.cpp                           # the sound subsystem
├── lzcompress.cpp                      # LZ compression, i.e. where the .LZC pack/unpack lives
├── trackroute.cpp                      # track routing
├── eventtree.cpp                       # the event tree (the subsystem behind the "View Event Tree" string TASK-0012 saw)
├── beltmover.cpp                       # the BeltMover class
├── trackpath.cpp                       # TrackPathC… (track pathfinding)
├── speech\sedwedge.cpp
├── movieplayer\
│   ├── cyclicbuffer.cpp
│   ├── dctimage.cpp
│   ├── movieplayer.cpp                 # MOVIE_%d_%d_Y / UA / VA / US / VS (movie decoding, including DCT images)
│   └── ringbuffer.cpp
└── bDebug.cpp and friends — see the bWare middleware section below
```

A second group sits directly under `src\` / `src/` with no `indep\` prefix, and is almost
certainly the **bWare middleware** itself. This is consistent with the debug paths TASK-0012
already confirmed (`bWare/inc/bWare.hpp`, `bSound/include/bFragmentedVector2.hpp`), adds the
middleware's `.cpp` file names, and carries real `assert()` expression text, which makes the
evidence stronger:

```text
src\bDebug.cpp             -- assert: "return_code < max_chars"; log prefix "**** Assertio[n...]"
src\bFile.cpp              -- assert: "bStrLen(filename) + 1 < (int)sizeof(bFile::Filename)"
src\bFileE.cpp             -- "WARNING:  Too many CD errors.  Will now call bFatalCDErrorFunction() at 0x%08x"; assert: "num_read_chain_tags > 0"
src\bMemory.cpp            -- assert: "(size >= (int)sizeof(FreeBlock)) && ((size & 3) == 0)"
src\EspClient.cpp          -- assert: "bfBufferCurrent <= bfBufferEnd"; "EspressoS[erver?]"
src/bDriverPS2.cpp         -- "SOUND_Memory_EE", "bSound : EE Mem Usage: %d bytes", "bSound Multistream Error: Can't init IOP"
src/bEffect.cpp            -- assert: "!m_OnlyOne"
src/bRegion.cpp            -- "bSoundRegions"
src/bSample.cpp            -- assert: "voiceId != BSND_CHANNEL_BUSY", "m_pVoice != NUL[L]"
src/bSegment.cpp           -- "bSound: API not initialized!"
src/bSound.cpp             -- assert: "pRegion != 0", "cannot load more than one region simultaneously!"
src/bVoice.cpp             -- assert: "VoiceId < BSND_PS2_NUM_VOICES"
src/bJukebox.cpp           -- assert: "nCatNum < m_nNumCategories", references include/bJukebox.h
src\care.cpp               -- CARFX_BREAK_WINDOW_RIGHT_FRONT/REAR, CarEmitterPositionSlotPool, CarRenderInfo ("care" may be short for "car effects"; unconfirmed)
src\acidfxe.cpp            -- XE_WaterFall_Drops_01 (effects-related; "acid fx e[ngine]"? unconfirmed)
src\memorycard.cpp         -- sceMcErrSemapho/sceMcErrUnBind/sceMcErrNullStr (the PS2 memory card API wrapper)
```

## Recovered original class and method names (partial; all from adjacent strings, none pinned to an address)

These are C++ symbol texts read directly out of the same string block — most likely class-name
prefixes in assert messages, or `"Class::Method()"` format strings in debug prints. **All of them
are real original identifiers, not names this project made up:**

| Original identifier | Context / likely meaning |
|---|---|
| `World::DoTimestep` | a profiling category string, immediately followed by the sub-entries `- UpdatePlayersGameState()` and `- Race::Update()`, repeated 21 times (it reads like a per-subsystem performance counter name table). **Confirms the "World" class name TASK-0010 had been assuming without evidence.** |
| `Race::Update()` | same source; confirms the `Race` class. |
| `RaceCoordinator` | see also the warning text "RaceCoordinator.Memory Leak - Track Specific" and the separate file `racecoordinator.cpp`. |
| `ReplayManager` / `ReplayManager::HandleSnapshot()` | the replay system; nearby are `REPLAY_MANAGER_STATE`, `DEMO_MODE_SNAPSHOT`, `CHECKSUM0..3` and `REPLAY0.BIN`. |
| `TrafficCar::Move()` | confirms the `TrafficCar` class and its `Move()` method — corroborating evidence that TASK-0008's CopMover/CopDriver "Move" assumption in `POLICE_AI_MAP.md` follows the same virtual-function naming convention. |
| `RenderManager::RenderLayer()` | the render manager. |
| `FlailerRenderManager::RenderScreenPrintf()` | "Flailer" is of unknown meaning; possibly a debug-only render manager subclass. |
| `GrandSceneryCullInfo::CullView` / `::DoCulling()` | the distant scenery culling system. |
| `List::Sort()` | a generic container class (possibly matching the `HP2_SortListByAdjacentSwaps` family already in reverse/functions; not cross-checked). |
| `AnimatedDriver` | a class name appearing right after the identically named string — the classic RTTI/factory registration pattern, matching TASK-0011's "class dispatch function takes a name string and constructs the object". |
| `PsychoDriver`, `RealDriver` | two candidate AI driver class names ("PsychoDriver" suggests stunt/crazy AI behaviour; unconfirmed). |
| `NisCameraCoordinator` | the NIS (cutscene) camera coordinator. |
| `QuickSpline` | matches `quickspline.cpp`. |
| `BeltMover` | matches `beltmover.cpp`. |
| `EspressoServer` (inferred; the string is truncated to "EspressoS") | matches `EspClient.cpp`; guessed to be a debug/logging server component of the PS2 toolchain, unconfirmed. |
| `TrackPathC[...]` | matches `trackpath.cpp`; the class name is truncated. |
| `JR2Server` | an unknown system, appearing near the camera/traffic strings. |
| `TexturePack` / `TextureAnimPack` | **the original class names for what we have been calling the "TPK" format.** The abbreviation "TPK" almost certainly comes from the real "TexturePack" file/class name, matching the community tool naming (`TPK_HP2_*`) cited in docs/ASSET_PIPELINE.md rather than being invented by the community. |
| `LoadedTextureData` | a very frequently repeated string, most likely the fixed prefix of a texture-load failure or debug log line rather than a class name — treat with care. |

## Direct link to TASK-0011 (the front-end menus)

The strings immediately around `indep\src\fmenus.cpp` are `MENUITEM_%d`, `RECORD_LABEL`,
`ARTIST_NAME`, `SONG_TITLE`, `RECORD_TITLE`, `CHYRON_BG_%d`, `LOADING_TRACK`, `tHelpMenu`,
`tPopupMenu` and `GlobalBackgroundAnimat[ion]`. Those line up closely with the widget name strings
TASK-0012 found independently inside `FRONTEND/FRONTB.BUN` — `UwEMENUITEM_*`, `MENUITEM_*`,
`UwEPOPUP_TITLE` (see docs/ASSET_PIPELINE.md §3) — which strongly suggests:

- **`fmenus.cpp` is the real original source file of the functions TASK-0011 traced**:
  `FUN_0016eef8` (the TitleScreen instance message handler), `FUN_0016f440` / `FUN_0016f810`
  (class dispatch functions) and `FUN_0016c4c0` / `FUN_0014cc78` (the screen switch manager).
  This session could not pin any specific line of code to an exact address (see the open item
  above), but the file-level correspondence is strongly evidenced.
- `frontendscreens\selectcar.cpp`, `racesetup.cpp`, `options.cpp` and
  `joystickconfiguration.cpp` are separate source files for individual **menu pages**, showing the
  original architecture is "fmenus.cpp provides the generic menu framework, with one file per page
  under frontendscreens\". That matches **structurally** what we built in
  `src/game_core/frontend/screen_manager.cpp` as "a generic ScreenManager plus concrete pages" —
  not a coincidental copy, since the structural analysis came first, but this is the first time
  original source-tree evidence directly supports that layering.

## What this means for naming in this project

- If TASK-0011's functions are later given `HP2_`-prefixed working names, those names should match
  the `fmenus.cpp` / `frontendscreens\*.cpp` context rather than a generic `HP2_GameFlowXxx`
  style. But until a specific function is pinned to a specific file, `fmenus.cpp` must not be
  written into the `original_identifier` column of `reverse/symbols.csv`, because that would pass
  "very likely" off as "confirmed".
- `TexturePack` / `TextureAnimPack` should be recorded in docs/ASSET_PIPELINE.md as the original
  class names behind the "TPK" format (recorded here already; a cross-reference should be added
  the next time that document is updated).
- `World`, `Race`, `RaceCoordinator`, `ReplayManager` and `TrafficCar` now have string-level
  evidence behind them, which is far better than the earlier "guessed from a variable name", so
  naming work in TASK-0009 / TASK-0010 should use these real class names instead of continuing
  with speculative `HP2_` prefixes.

## Open questions

1. None of these 93 strings has a code location in the ELF that references it directly
   (`references_to` returns empty). It was not established whether that is because the retail
   build compiled the assert/profiler calls out (leaving the string data orphaned) or because
   Ghidra's static cross-reference analysis misses an indirect or computed reference.
2. Whether the file paths have a longer drive/project prefix before `\indep\` (something like
   `X:\NFSHP2\indep\...`) is unconfirmed; no further prefix characters were visible in the context
   window read this session.
3. The class and method names listed here are "adjacent string" evidence, not evidence of which
   concrete function or address they belong to. Pinning them down needs finer cross-reference or
   pattern-matching work per address, which was not performed.
4. Some strings were not investigated at all this session (what `JR2Server`, `EspClient.cpp`,
   `care.cpp` and `acidfxe.cpp` actually are), left as leads for later tasks.

Continuation hint: to tie a concrete address such as `FUN_0016eef8` to a specific line of
`fmenus.cpp`, the next step would be to look for the "string table" structure these `.cpp`/`.hpp`
strings live in — is it a uniform array of `{file, line, expr}` assert records? If that table's
memory layout can be found, it may be possible to work backwards to which functions were compiled
within which file's range, and so batch-classify many more functions into their correct original
source files.
