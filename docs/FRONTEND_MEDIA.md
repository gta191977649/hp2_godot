# TASK-0016 — Startup and original movie playback

Target: US PS2 `SLUS_203.62`, executable revision unknown, SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
Read-only Ghidra MCP, 2026-10-12. Static evidence is not original execution verification.

## Confirmed startup behavior (E-0016-V01)

Godot's previous Run target was `main.tscn`, the arithmetic scaffold. Its behavior
did not reflect original startup. The title class constructor `0016F000` has
two branches controlled by `gp-0x59FC` (`0032FEF4`, initial value zero): zero
starts movie state 2; nonzero starts title state 1. Runtime producers of that
flag remain unresolved; its initial value does not establish every boot configuration.

`0016EE90` selects an eight-byte movie record from `002FB750` in normal mode,
or `002FB7D0` in demo mode. Normal records have two movies per language:
EA logo followed by NFS intro. Demo has one EA logo. Language indices 0..5
select logo suffixes EN, FR, GE, IT, SP, SW. Intro suffix is FR for index 1,
EN for the remaining indices. Each record's second word controls which movie
image widget is shown; the constructor and update hide/show those authored widgets.

`0016F1F0` waits for `0016EE18`'s completion seam, increments movie index and
opens the next movie, or enters title state when index equals `0016EEE0`'s count
(2 normal, 1 demo). Message 1 in `0016EEF8` rejects activation when instance
+0x08 (movie index) equals zero: the initial EA logo cannot be skipped.
During the second movie, activation skips directly to title state.
Counterevidence correction: this pass initially misread that +0x08 guard as
movie-widget presence and permitted skipping EA; the constructor/update both
identify it as movie index. That interpretation and implementation were retired,
and a regression test now enforces the first-movie guard.
Title activation calls `0016F4F8`: timestamp must be neither zero nor
`0x7FFFFFFF`, and elapsed time must be strictly greater than four seconds.
Normal-mode activation then requests `New Main Menu` with history push enabled.
Demo mode blocks that transition. The seconds multiplier is raw float
`0x3983126F` at `0032E1D0` (=0.00025, nominal 4000 Hz).

Music callsites are separate from showing title: bypass-movie constructor
`0016F000` calls `001AEAB0(1)` unconditionally; natural sequence completion
`0016F1F0` calls it only in normal mode. The second-movie skip handler
`0016EEF8` stops the decoder and resets pulse time, but has no music-start call.
`StartupResult.start_music` projects those exact callsites; entering ShowTitle
alone is not a reason to start background music.

`0016F950` is a conditional history-driven startup selector, not an unconditional
fixed page order. Its initial gate `00330664` is 1. It pushes Title Screen
for gate 1 or 2; the legal-screen gate `0016F938` returns zero in this binary.
BootAutoload and Language Options Pre FE are conditional. `0016F8C8`'s external
`001A3390` dependency is unresolved. Full boot language/save-card workflow is not
reconstructed merely by implementing the title movie sequence.

## Confirmed background movies (E-0016-V02)

| Constructor/path | Requested original asset |
|---|---|
| Main menu `00216460` | `MOVIES/mainmenu_movie.AVI` |
| Cops On `00216E80` | `MOVIES/hotpursuit_movie.AVI` |
| Cops Off `00217538` | `MOVIES/worldracing_ movie.AVI` (embedded space is real) |
| Generic Options `0016D100` → `0016D0F0` | `MOVIES/options_movie.AVI` |
| Credits `001F1F38`, shared Options base `001F4C98` (Audio/Controller/Display/Jukebox) | `MOVIES/options_movie_new.AVI` |

`0016CE30` returns without restarting if the requested filename matches the
active movie. It binds original `PANELMOVIE` and `MOVIE_256_384_Y/UA/VA/US/VS`
textures, clears the previous decoder and opens the requested filename through
`00237AD8`. `0016D028` checks `002380A8`; while the active panel exists and
the stop gate is clear, completion reopens the same filename. This is the
background loop. `0016D0A0` stops playback and clears the bound plane pointers.
The authored panel widget supplies display geometry; the video's 256×384
encoded dimensions alone do not prescribe a screen-space rectangle.

Authored geometry (E-0016-V04, FRONTB.BUN): PANELMOVIE texture hash is
`0xC3F0CAEF`, 256×384. Main/Cops On/Cops Off use a `NONAME` quad at (141,73),
scale (1.94900000095,0.69800001383), layer 5; Options uses the same transform
at layer 6. Replacement must preserve that quad's rectangle and layer.
Screen 22's TITLE_FULLSCREEN (name hash `0x3ACF2D73`) has SPLASH_SCREEN
512×256, position (-10,0), scale (1.25,1.875), yielding 640×480.
TITLE_LETTERBOX (`0xBFAEBF79`) uses the same texture at (-1,65), scale
(1.25,1.18799996376), yielding 640×304.1279907. The movie table second word
selects the full-screen widget for EA logo and the letterbox widget for NFS intro.
The tiny negative X offsets are authored values, not a reason to recenter either.

Title visibility (E-0016-V05): `0016F1F0` toggles the `TITLE_IMAGE` class group,
hash `0xB73C8D23`, by calling `001454E8` with `state != 2`. That function uses
`00143DD8` to match widget class hash at +0x0C and writes visible byte +0x29.
Screen 22's group contains COPYRIGHT_TEXT and two NONAME quads (texture hashes
`0xAB0E817D` and `0xC5A905AF`); it does not contain either movie surface.
The static title therefore uses those authored group quads. It must not leave
TITLE_FULLSCREEN or TITLE_LETTERBOX enabled after stopping the movie.
PRESS_START_1 is set visible only when `0016F4F8`'s strict four-second gate passes.
No separate on/off blinking was observed in this slice; common ghost-highlight
animation is constructed and bound to PRESS_START_1. Its constructor retains
the default period; main-menu helper `0016FBB0`'s two-second override is not called
by the title constructor. Movie skip resets pulse timestamp via `00248AD0`.

## Container and decoder (E-0016-V03)

2026-10-13 orientation correction (E-0016-V06): the original `00237AD8`
negates output BITMAPINFOHEADER height at movie +0xC0 after format creation.
`00239148` detects that negative height and sets flags bit 0. `002363A8`
then begins at `output + width*4*(height-1)` and uses stride `-width*4`;
the inner pixel writes still advance positively by eight bytes per two pixels.
This establishes vertical row reversal without horizontal pixel reversal.
FFmpeg MV30 writes YUV planes with positive linesizes and does not apply the
original output-header orientation. The host now reverses returned RGBA row
order after conversion, keeping prediction buffers unchanged.

The user's reported upside-down playback and root's inspected private EA capture
`local/shots/task0016-ea-before.png` agree with this correction; they are host
diagnostics, not an original hardware comparison. PANELMOVIE authored UV data
at +0x70..+0x8C is (0,0.006),(1,0.006),(1,1),(0,1); +0x40..+0x4C are zero.
Those observations provide no support for a 90/180 degree rotation or horizontal
flip. Complete GS plane/shader equivalence and UV edge cropping remain unresolved.
An asymmetric 2×3 synthetic RGBA test verifies exact row reversal, channel/pixel
order, unchanged middle row, involution and invalid dimensions; native movieplayer
build and CTest passed 1/1 on 2026-10-13.

All 18 local movie assets inspected are RIFF AVI with `vids/mv30` stream header
and BITMAPINFOHEADER compression `MV30`. Main/background movies contain 900 frames,
256×384, scale/rate 1/30, no audio. The EN EA logo is 512×256, 180 frames;
EN NFS intro is 512×256, 2425 frames, scale/rate 33333/1000000.
Counterevidence E-0016-A07: the AVI carrier declares PCM format 1, mono 8-bit,
but its bytes are raw stereo PS2 ADPCM. Playback uses the original driver pitch
3762/4096 at 48000 Hz and a 0.16-second video clock bias; see SOUND_RECOVERY.md.
`00237760` reads AVI video headers and conditionally the following audio stream.
`00239148` reads the four-byte compressed packet prefix and calls `00234B38`.
Its closure includes entropy (`00237428`), fixed-point IDCT, quantization,
motion compensation and color conversion; these original internals are not translated.

The port's `read_avi` walks bounded RIFF chunks, preserves packet order and PCM
bytes, and rejects unsupported codecs/formats. `MovieDecoder` uses FFmpeg's
MV30 CPU decoder in-process, preserving inter-frame state, then libswscale to
produce RGBA. No subprocess, pre-export or converted asset is required. It is
an explicitly port-side media backend, not a recovered original codec implementation.
FFmpeg output and color conversion have not been compared to PS2 GS output.

Primary host dependency sources:
[FFmpeg MV30 decoder](https://github.com/FFmpeg/FFmpeg/blob/n8.1/libavcodec/mv30.c),
[AVI codec tags](https://github.com/FFmpeg/FFmpeg/blob/n8.1/libavformat/riff.c).
These justify host decoder availability, not game behavior.

Ignored local package: `local/tools/ffmpeg/ffmpeg-n8.1-latest-win64-lgpl-shared-8.1`.
Downloaded package URL:
`https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-n8.1-latest-win64-lgpl-shared-8.1.zip`.
Version `n8.1.3-17-gc389840894-20261011`; archive SHA-256
`ec30bde63b8e36ebdc7ba5b37140a43a2d2afe884650269fadeb033a57098cfc`.
The URL is mutable: reinstall must verify that exact hash, not silently accept
new bytes. Build is shared LGPL v3 (enable-version3, no enable-gpl/nonfree).
Headers, import libraries and `LICENSE.txt` accompany DLLs.

## Validation and limitations

`src/tests/movieplayer_tests.cpp` tests normal/demo counts, language lookup,
non-skippable EA logo, second-intro skip without music, natural/bypass music
callsites, strict four-second boundary, timestamp sentinels,
32-bit timer wrap, malformed AVI bounds and synthetic chunk padding/demux order.
Optional arguments supply private AVI paths through the configured asset resolver.
2026-10-12 direct GCC 13.2.0 compilation with `-std=c++17 -Wall -Wextra` passed.
The private check decoded ten consecutive frames each from main menu, EN EA logo
and EN NFS intro: dimensions/opaque RGBA validated, no decoder error.
Native tests do not establish original equivalence. Other OS FFmpeg packaging,
PCSX2 comparison, exact GS YUV reconstruction, movie-widget timing and full
BootAutoload/language/attract state closure remain explicit limits.

## Evidence ledger

Raw JSON exports are ignored under `local/task0016/`; no Ghidra mutations.

- E-0016-V01: decompiles `0016F000`, `0016F1F0` (requested interior `0016F290`
  correctly reported entry), `0016EEF8`, `0016EE90`, `0016EEE0`, `0016F4F8`,
  `0016F950`, `0016F938`, `0016F940`, `0016F8A0`, `0016F8C8`; memory table
  `002FB750`, strings `002B2E00`, gp initial data `0032FEF4/00330664/0032E1D0`.
- E-0016-V02: `0016CE30`, `0016D028`, `0016D0A0`, `0016D0F0`, `0016D100`,
  `00216460`, `00216E80`, `00217538`, `001F1F38`, `001F4C98`; literal and
  plane names at `002BC430/002BC508/002BC5C0/002BA010/002B2850`.
- E-0016-V03: `00237760`, `00237AD8`, `00238FB0`, `00239148`, `00234B38`,
  `00237428`, asset SHA metadata `movie-assets-metadata.json`. Representative
  SHA-256: MAINMENU `217699602d8130253ac2fddb2011cff9a718659e53d6c65ff2dfaff306d556d0`,
  EALOGO_EN `d7078f75e1a5844ffa5f22f7edd190946200608f280028e75ac0729e4d1a1ea3`,
  NFSINTRO_EN `e88ec73bc0be3235d8713c7b22e10f225374b9fc85593dbf5220e19d65e89812`.
- E-0016-V04: FRONTB.BUN quad records and matching TexturePack header dimensions;
  private derived metadata `movie-widget-geometry.json`, no decoded image stored.
- E-0016-V05: `00144968`, `00140728`, `001454E8`, `00143DD8`, `00248870`,
  `002488D0`, `00248AD0`; title screen widget class hashes, and their callers
  `0016F000/0016F1F0/0016EEF8`. Static group versus movie-surface distinction
  cross-confirmed between asset bytes and code.
- E-0016-V06: private decompiles `00237AD8`, `00239148`, `002363A8` under
  `local/task0016/`; PANELMOVIE original widget byte fields and FFmpeg primary
  MV30 source in `local/task0016/ffmpeg_mv30.c`. Port-only orientation correction,
  no recovered GS rasterizer or original execution verification claimed.

TASK-0017 reinspection001F4C98 and Display001F33B8/Jukebox001F02D8 confirms
common base requests002BA010. Host background mapping now includes Display26
and Jukebox27. Prior table mislabeled001F1F38 asAudio; it isCredits constructor.
