# TASK-0016 — original menu sound data and host playback

Target: US PS2 SLUS_203.62, ELF SHA-256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2;
executable revision unknown. No original comparison; nothing here is Verified.

## Confirmed ELF routing (E-0016-A01)

FUN_0016DED8 binds event names from the menu table to bSound event objects.
FUN_0016E038 ignores disabled sound / missing object, suppresses the same object
when elapsed time is below 34 milliseconds, then assigns category 3 and submits
FUN_00274080(1.0, 0, 0, object). FUN_00250F48 establishes the time unit.
IDs 1/2 share MenuLeftRight, 3/4 share MenuUpDown. Existing event names remain
the authoritative mapping; no new mapping is inferred from menu appearance.

FUN_00273C80 gets event amplitude from sound descriptor +0x24, multiplies by
category gain, float bits 0x38800200, and object +0x1c. Category gains and object
factor start at 1 in constructors (002776C0 / 00272FF0), but 001AE608 immediately
calls 001AFAB8, applying 001AFA28's initialized defaults. Menu category 3 becomes
0.31 (bits 3e9eb852 at 0032E750), so playback multiplies descriptor gain by 0.31.
Later user volume writes and platform master volume remain unreconstructed.

## Confirmed original asset layout (E-0016-A02 / A03)

The configured player's __Unknown/E2F0CC8A (6025080 bytes; SHA-256
a482196881ca0b50ceee20f0fa56183668f213f6fde1641d23c8705a7f697541)
contains 382 named Guest sample chunks. Tags 0x80014103 hold version 1, padded
name byte count, name, then tag 0x14204 VAGp payload. VAGx at header+36 and
big-endian channel count at +44 show stereo data interleaved in 16-byte blocks.
The initial mono hypothesis was retired after inspecting the extension and the
original hardware bank; decoding it as mono doubles duration and loses stereo.

Common runtime descriptors are __Unknown/84CBF303 (768 bytes, SHA-256
d6fd274a2b8f1aa28f198e2406d6fa76b15353d614b02a201cc6912aec5356cf),
and PS2 ADPCM data is __Unknown/84CBF2FD (75744 bytes, SHA-256
b7de542239df28eedfc3d4c51ddcc7003f306812a22898916727585db89168f2).
Descriptor header is 12 bytes with count 14 at +8. Records are 52 bytes,
event name +0 (32 bytes), rate +0x22, amplitude +0x24, sample offset +0x28.
Paired names are left/right sample planes. Plane size follows the next offset;
rates are 22050 Hz. Samples include a leading zero block, while Guest VAG
contains the same nonterminal blocks and a final terminator instead.

| Event | Original amplitude | Normalized gain |
|---|---:|---:|
| MenuStart | 6553 | about 0.4000 |
| MenuLeftRight | 8191 | about 0.5000 |
| MenuUpDown | 5406 | about 0.3300 |
| MenuSelect | 6061 | about 0.3700 |
| MenuCancel | 9174 | about 0.5600 |
| MenuWrong | 10321 | about 0.6300 |
| MenuAdjustVolume | 10321 | about 0.6300 |

## Port implementation and limits

src/game_core/audio/sound.* mirrors the recovered original sound.cpp filename;
everything added lives under hp2::port::audio. Original assets are opened only
through a root supplied by platform::resolve_zzdata_root. Common event playback
uses the original planar hardware sample bank and original amplitudes. The Guest
bank remains available for named sample inspection. No proprietary bytes, WAVs,
or synthetic replacement sounds are committed.

The host PCM decoder is independently written standard PSX ADPCM infrastructure.
Its public predictor reference is [FFmpeg n7.1 ADPCM_PSX](https://raw.githubusercontent.com/FFmpeg/FFmpeg/n7.1/libavcodec/adpcm.c).
The later user-authorized map_tools_ps2 reference supplies the rounded prediction
and clipped-history convention used by the current host decoder (E-0016-A06). That reference
does not establish HP2 behavior and is not evidence for an ELF claim. Decoder
outputs interleaved signed 16-bit stereo PCM for the host's AudioStreamWAV.
Godot category volume, missing banks and codec errors must remain explicit.

Validation: g++ 13.2.0, `g++ -std=c++17 -Wall -Wextra -Wpedantic -Isrc src/tests/sound_asset_tests.cpp src/game_core/audio/sound.cpp src/platform/ps2_asset_root.cpp -o local/task0016/sound_asset_tests.exe`,
then `local/task0016/sound_asset_tests.exe`: PASS synthetic nibble/sign/flag and
bad predictor checks; all seven original common stereo events decode nonzero
PCM at 22050 Hz with descriptor gain. This does not establish SPU equivalence.

Next: recover common bank loading filenames/lifecycle (00210010 references common),
user category 3 gain writes, jukebox scheduling, and compare output with PS2 capture.

## Frontend jukebox (E-0016-A04 / A05)

Recovered original startup001AE608 loads SOUND/JukeboxM.JBC. The shipped cartridge
is193167360 bytes; its32-byte header reports16-bit,2-channel,32000 Hz,1 category,
and4096 metadata bytes. Category InGame contains23 tune records; its name does
not determine which screens use those tracks. Categories are64 bytes and tunes
96 bytes. Tunes hold offset,size,syncsize,84-byte name. Every current tune has
syncsize0 and a VAGp header at its stated offset. The VAGx marker is+36 and
channels+44; the earlier+32 hypothesis was corrected through actual cartridge
decoding. Samples decode directly at runtime without converted files.

Frontend sound load001AE9A0 sets category5 and calls001AF050(5). Its original
matrix002B6300 permits song type4/5 in frontend (column0); song type is field4
of the original hyphen-delimited name, parsed by001AED90/001AEFD0. Fresh
AudioOptions001EF1C8 initializes genre mask+0xaf to0xff. Saved profile genres and
track editing remain unimplemented. Category5 default gain is0.45, bits3ee66666,
read from0032E758 via001AFA28.

Retail intro phase2 in0016F1F0 does not start jukebox music until all movies
finish and phase1 begins. Intro-bypass constructor0016F000 calls music immediately.
The initialized gp b610 flag at00330F00 is1;001AEA40 consumes it for category5
and prefers AudioOptions row5. Original enumeration001EEF60 maps that row to
category0,tune5: Hot Action Cop-Fever for the Flava--Rock-4. Entering Main Menu
calls001AEAB0 again; a playing track continues.

Natural tune completion0027DFF8 in auto-mode1 invokes0027E6C8, whose selector
0027ECA8 advances through enabled tunes in authored order with wrap. With fresh
frontend flags,tune5 advances to7. Natural completion does not apply random
startup skipping. The one-time random fallback in001AEAB0 uses original frame
counter when no tune is playing and the preferred path cannot be used.0025A3D0
returns old seed modulo count and advances seed by XOR/shift. Listing0025A3E4
is DIVU; the decompiler's signed remainder is retired counterevidence. High-bit
seed regression verifies unsigned remainder.

Port implementation: src/game_core/audio/bJukebox.* contains cartridge I/O and
a fresh-profile frontend scheduling projection, all under hp2::port::audio.
src/tests/jukebox_tests.cpp tests mode filtering, enabled-order skip/wrap,
unchanged failed selection, unsigned RNG,preferred initial5 and next7,continued
track on screen reentry,and actual JBC stereo decoding. Standalone command:
`g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc src/tests/jukebox_tests.cpp src/game_core/audio/bJukebox.cpp src/game_core/audio/sound.cpp src/platform/ps2_asset_root.cpp -o local/task0016/jukebox_tests.exe`
then executable: PASS,23 cartridge tunes,tune1 decoded15457904 signed16 samples,
2 channels at32000 Hz. Host clock/frame count,stream buffering,ducking/fades,
saved user settings,racing audio,and original comparison remain limitations.

## Playback correction and differential (E-0016-A06)

The user reported incorrect playback speed and explicitly authorized reference
inspection of D:/dev/eagl-godot/map_tools_ps2. Its sound_extract.py declares JBC
interleave 0x8000; its tests separately cover16-byte menu VAG interleave and the
JBC0x8000 layout with a shorter final stereo group. This was a lead to checking
the original cartridge, not evidence that an arbitrary external port matches PS2.

The original tune5 VAG header directly establishes32000 Hz,2 channels and payload
size. Payload-derived frame count is7880320 (28 samples per16-byte ADPCM block per
channel), so its duration is246.26 seconds. The old decoder incorrectly alternated
channels every16 bytes in JBC too. That changed which predictor history decoded
each channel and changed temporal ordering within every original0x8000 plane.
Its total duration stayed the same; merely changing playback rate cannot fix it.

Current decode_vag takes an explicit interleave argument: common menu/default16,
JBC32768. Channel histories persist independently, the final shorter group divides
remaining bytes equally between channels, and output remains interleaved PCM.
Prediction adds32 before floor division by64 and retains clipped16-bit history,
matching the authorized reference. The former FFmpeg-style unrounded/unclipped
history convention is retired for this host decoder. This is a codec adapter
choice validated against a reference, not recovered ELF SPU software.

`python src/tests/sound_reference_differential.py --native-runner local/task0016/jukebox_tests.exe --reference-source D:/dev/eagl-godot/map_tools_ps2/src`
PASS: actual tune5 native PCM equals the requested reference byte-for-byte,
31521280 bytes, SHA-256
d3aad200958cbdc416597101f17078d2101022b743bc5ad777a0960cc3fcd5c9.
Header-derived duration246.26 seconds is checked independently of PCM equality.
Raw PCM/WAV/result files stay under ignored local/task0016/audio-reference.
The synthetic regression checks independently recognizable left/right patterns,
large interleave and a short final group, predictor rounding, clipped history and
malformed input. Original common7 and JBC tests also pass. No proprietary samples
were added to tracked files. No PS2/SPU capture comparison, so status is unverified.

## Movie hardware audio and neutral pitch (E-0016-A07; 2026-10-13)

The original AVI `auds` carrier reports 8-bit mono PCM and rates such as54549 and
51077. Those fields do not describe the bytes sent to the game's sound hardware.
Movie002380A8 constructs bSound type3, bank `_STREAMS`, and event `_STREAMS`;
assigns basefrequency0xAA66; category0xffffffff; and installs callback00237AB8.
00238DB0/00269D00 bind movie userdata at0033211C.002684F8 detects `_MOVIE` and
calls the bound callback;00238088/00239AB8/00232380 copy raw audio ring bytes
without a PCM conversion. Source01wb payloads assemble into ordinary header-first
PSX ADPCM blocks. The temporary reversed16-byte hypothesis was rejected: all20480
EALOGO block controls are valid at offset0; only15617 are valid at offset15.
No reversal is implemented and the initial zero block is retained.

Original001AE608 listing passes a ninth initializer argument65536.00277820 forwards
it to0025E690, which writes gpC7CC streambuffer size. Stereo002611C0 checks the
seventh character of `_STREAMS` to allocate2 channels, then0026DCE0 sets the half
buffer32768 and0026DDB0 binds the paired channel. This establishes the0x8000
channel-plane size from the ELF, corroborating the earlier reference-led layout.

Basefrequency is not final playback rate.00273850 at pitch0 reads table[129]=4140
at00327B82, scales basefrequency by4140/4096 and converts to integer through
0025E530.0026D258 then computes `(frequency << 12) / 48000` for SPU pitch.
Moviebase43622 produces frequency44090 and pitch3762, yielding exact hardware
rate44085.9375 Hz. The port makes WAV signed16 stereo at48000 and applies
pitch_scale3762/4096, reproducing the recovered ratio. Moviecategory-1 obtains
gain1 via00277DC0; it does not use category3 menu volume0.31.

The same neutral-pitch table applies to other sounds: JBC metadata32000 produces
SPU pitch2759, rate32332.03125 Hz and host factor1.0103759765625. Menu metadata22050
produces pitch1901 and rate22277.34375 Hz. PCM metadata and byte differential remain
at their original decoded rates; host pitch is a separate parameter.0025E530 uses
CVT.W.S; FPU rounding-control initialization was not captured. For these exact
rates, nearest versus truncation produces the same final SPU pitch, so the recorded
hardware ratios do not depend on that unresolved detail.

Validation: movie_audio_tests tests separate recognizable32768-byte stereo planes,
short final planes and exact pitch fraction. Actual EALOGO327680 bytes produces
286720 stereo frames; NFSINTRO4128768 bytes produces3612672 frames. NFS raw padded
stream duration81.9461308 seconds is not stretched to video duration. Original
002380A8 also adds initialized0.16 seconds to the video starting clock after audio
starts. Elapsed video time is currentclock minus that startingclock, clamped at zero;
audio starts immediately, and video advancement lags by160 ms. The first frame may
already be displayed and held during that interval. Host EOF stopping and synchronization
must preserve that direction; neither sample-rate stretching nor delayed audio reproduces it.

`python src/tests/movie_audio_reference_differential.py --native-runner local/task0016/movie_audio_tests.exe --reference-source D:/dev/eagl-godot/map_tools_ps2/src`
PASS: native EALOGO PCM equals authorized reference byte-for-byte,1146880 bytes,
SHA-25684b776d61bd95808ba857e2a0c2e6a78d2ac9ce202a4f8bc3f915fceccfa297c.
The reference validates codec/channel decoding only; its rate was supplied from
the recovered driver parameters. Outputs remain ignored local/task0016/movie-audio-reference.
Original SPU capture comparison remains outstanding. The host movie parser uses
audio_data to make its raw encoded nature explicit.
