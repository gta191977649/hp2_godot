# TASK-0017 — Original submenu Back handling

2026-10-13, read-only Ghidra MCP. US PS2 SLUS_203.62, revision unknown,
SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
No Ghidra edits or original execution comparison. Native behavior is a partial
message-0x11 projection, not complete screen reconstruction.

## Manager contract

`0016C670` initializes a 16-byte callback payload with +8 permission=1 and
+12 sound=6. When callback enabled, it sends message0x11 to current class and
plays the resulting sound even if permission is zero. Only permission exactly
one allows its subsequent history pop and class-name request. `00170320(1)`
calls this manager with callback disabled, preventing callback recursion.
History entries are class identities, not layout indices; shared-layout classes
must retain their original identity.

## Confirmed local handler slices

| Class handler | Real callback | Back behavior |
|---|---|---|
| Audio `001F0278` | generic `001F5060` → `001F4F88` → vtable `001EFB80` | Changed0xB0 settings snapshot copied to DAT002FC0E4; apply001EF120, then push Autosave if DAT002FC054!=0; default permission1/sound6 |
| Display `001F3EC0` | same generic wrapper → `001F3A28` | Always copy instance+0x64 eight bytes to DAT002FC19C; aggregate comparison001EE878 determines optional Autosave push |
| Controller `00240060` | same generic wrapper → `002402C0` | Compare cached+0x78/+0x7C against00240B50 player0/1 enabled values; changed and autosave set pushes Autosave |
| NameEntry `0024DBD8` | `0024D870` | Keyboard-active branch0024D088 sets permission0/sound29, commits name, closes editor and calls0016D448(0); inactive branch compares profile fields0024D370 and optionally pushes Autosave |
| Language `001F1DD8` | `001F1B08` | Always permission0; unknown language=-1 returns; otherwise current selected hash chooses language, calls0021E0D0, optionally resets profiles00183830, then direct00170320(1) pop |
| Credits `001F21B0` | `001F2070` | No0x11 permission/sound write;00248AE0 is called but ignores0x11 |
| Cops On/Off `002174A0` / `00217AF8` | own handlers | No0x11 write while no exit animation; active exit callback00249128 denies permission and clears sound |
| Jukebox `001F1220` | generic wrapper → `001F0D10` | Changed snapshot optionally pushes Autosave; always copies0xB0 snapshot, applies001AF050(5), then resumes jukebox0027D640(1) or previews selected enabled/loaded song0027D6A8 |
| User names `001F19E8` | `001F1568` | Compare both widget strings and0x30 profile snapshots; changed commits both names/profiles then optional Autosave |
| Load/save `00247830` | `00247530` | Idle0x54=0 leaves payload unchanged; non-idle polls/dispatches asynchronous save/load/editor callbacks and is unresolved |

The generic exit branch `001F4F88` routes all messages to `00249128` when
instance+0x5C is nonzero. That callback writes sound0 for all messages and
permission0 for0x11. It is also used by the two mode-select screen exit paths
via their +0x270 pointer. Audio/Display/Controller/Jukebox may therefore block
Back while exiting. There is no blanket allow-all policy.

The Autosave history literal is exactly `Autosave`, confirmed independently
at002B9C98,002BF2B8 and002BE338. A callback pushes that class onto history so
the manager's subsequent pop enters it; dropping this side effect would lose
the original automatic-save path.

## Constructor state and language distinction

NameEntry constructor0024D408 explicitly writes instance+0=0, making keyboard
inactive on entry. Display constructor001F33B8 captures DAT002FC19C at+0x64.
Controller002400C0 captures enabled values at+0x78/+0x7C. Audio001EF3C8 copies
the global settings block then reads live bSound group gains into its snapshot.
Equal snapshots must be established by runtime state; an absent comparison
must never be silently treated as unchanged.

Language wrapper001F1DD8 compares current class-name strings. Normal
`Language Options` uses001F1D70 forwarding flag1 to001F1A78, whose third argument
is stored at instance+8. `Language Options Pre FE` uses001F1A78 directly with
flag0. The MIPS listing confirms forwarding a2 unchanged. Consequently normal
Language Back avoids00183830; Pre FE Back may perform that full profile/game
reset. It is not a harmless settings refresh. The locale loader0021E0D0 itself
returns immediately if requested language equals the current one; otherwise it
unloads/loads locale resources, triggers callbacks and invalidates widgets.

## Portable interface and limits

`port::frontend::query_back(handler_address, BackContext&, BackServices&)` routes
to address-anchored message slices. BackEvent represents the manager's permission
and sound words. BackContext holds explicit host branch inputs; optional missing
settings/keyboard/language state throws NotImplemented. BackServices supplies
original side-effect seams and class history pushes. The helpers remain free of
Godot types. They use `Sub_<ADDR>` names, with the partial message scope in comments.

Audio/user-name `invoke` calls at the handler address represent only their
snapshot/profile commit closure, not recursively executing the full handler.
Jukebox `invoke(001F0D10,0x11)` similarly represents only the0xB0 snapshot copy;
eligibility restore001AF050(5) and resume0027D640(1) are separate calls in original
order. A settings comparison proving equal bytes makes the copy idempotent,
but does not justify discarding the music calls. Unsupported selected-song
preview operands and non-idle save/load paths throw at their missing boundaries.
Callback fixtures test ordering; they do not implement the proprietary profile
or bSound services themselves.

Controller helper follows the recovered original `frontendscreens/joystickconfiguration.cpp`
source tail. Options-family helpers live in `frontendscreens/options.cpp`.
NameEntry and mode wrappers have no confirmed original TU name; their portable
routing is explicit rather than creating invented original source filenames.

## Evidence and tests

Raw exports are ignored under `local/task0017/`. Ghidra cannot decompile the
uncreated entries001F0D10/002402C0: their vtable pointers, message jump tables
and existing disassembly establish the0x11 slices without modifying the database.
Some initial instructions are displayed as raw bytes; immediate/table operands
and branch targets were decoded from those bytes. No missing full function body
is presented as a complete decompile.

- E-0017-B01:001F0278/001F5060/001F4F88/001EF3C8/001EFB80/001EF120, vtable002BA558.
- E-0017-B02:001F3EC0/001F33B8/001F3A28/001EE878, vtable002BA4E8.
- E-0017-B03:00240060/002400C0/002402C0 listing/00240B50, vtable002BE5B8, jump table002BE510.
- E-0017-B04:0024DBD8/0024D408/0024D870/0024D088/0024D370/0016D448.
- E-0017-B05:001F1DD8/001F1A78/001F1D70/001F1B08/0021E0D0/00183830/00170320; language class literals002B9B98/002B9EB8.
- E-0017-B06:001F1220/001F02D8/001F0D10 listing/001AF050, vtable002BA538, jump table002B9D50.
- E-0017-B07:001F19E8/001F1568.
- E-0017-B08:0016C670/00249128/002174A0/00217AF8/001F21B0/001F2070/00248AE0/00247830/00247530/001F5C28 and Autosave literal reads.

Synthetic tests: unchanged/changed/exit gates; sound defaults; exact Autosave
order; display copy; keyboard cancellation; normal versus Pre FE language reset
and direct-pop distinction; jukebox copy/eligibility/resume order; missing-state,
missing-service, unknown-handler and busy-save loud failures. Run
`cmake --build --preset native-debug --target frontend_back_tests --parallel 4`
and `ctest --preset native-debug -R '^frontend_back$' --output-on-failure` passed
on2026-10-13 (1/1). CMake3.29.2, Ninja1.12.0, GCC13.2.0; host tests only.
No original equivalence status promotion.

## Host integration and practical limits

The bridge routes Backspace through the same KEY_BACKSPACE→JOY_EVENT_MENU_BACK11
input path. History now retains class names, including shared layout29 NameEntry.
Normal reachable Audio/Controller/NameEntry/Credits/Language/idleMemoryCard and
CopsOn/CopsOff paths pass integration checks. Languageflag1 applies selected FE
locale then performs its own history pop before the outer sound6 playback; it
does not run PreFEflag0 fullprofile reset. Same-language loader is idempotent as
0021E0D0's explicit equality guard.

Current bridge has no settings/editor mutation handlers. Entry snapshots therefore
remain unchanged; this is the represented host state, not a claim about original
profile byte values. Changed-setting commit, active name keyboard, fullprofile
reset, Jukebox preview/resume dependencies and busycard operations remain explicit
NotImplemented boundaries. Display entry is still blocked by prior001EE6C0 profile
snapshot dependency; its portable Back-copy slice is tested with synthetic state.
Pages selected directly bydebugPgUp/PgDn or--screen bypass navigation history;
empty history leaves the current page alone, matching0016B438/0016C670.

## Host integration and practical limits

Bridge routes KEY_BACKSPACE through JOY_EVENT_MENU_BACK11; release and keyrepeat
are rejected by the keyboard input adapter. Class-name history preserves NameEntry
identity on shared layout29. Audio/Controller/NameEntry/Credits/Language/idlecard
and two mode selectors pass traversal and Back checks. NormalLanguageflag1 applies
selected locale then pops before outer sound6; PreFEflag0 fullprofile reset remains
unimplemented. Same-language0021E0D0 equality guard is preserved.

No settings/editor mutation handlers are enabled in the bridge; represented entry
snapshots remain unchanged. This is not evidence of original profile byte values.
Changed-setting commits, active keyboards, Jukebox preview/resume and busycard
operations remain explicit NotImplemented dependencies. Display entry retains
its prior001EE6C0 snapshot blocker; portable Back-copy slice is tested separately.
DirectdebugPgUp/PgDn/--screen bypass history; an empty history leaves current
screen alone as0016B438/0016C670 specify.
