# VAGx — port-side reader evidence E-0016-A06

Target ELF: US PS2 SLUS_203.62, SHA-256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2;
executable revision unknown. Audio data is the configured player's original files.
This describes asset parsing and host codec infrastructure, not a recovered C++ type.

## Observed original bytes

SOUND/JUKEBOXM.JBC tune 5 (first frontend preferred tune) has VAGp header at its
index-table offset, big-endian payload byte count at +12, rate32000 at +16,
VAGx marker +36 and channel count2 at +44. Header length48. Payload count gives
7880320 PCM frames and246.26 seconds at32000 Hz, independently of implementation.

## Reference-led interpretation and port validation

The user authorized D:/dev/eagl-godot/map_tools_ps2 as the audio reference.
Its sound_extract.py uses fixed0x8000-byte channel planes for JBC and independently
tests shorter final groups. The previous host16-byte-only interpretation was wrong
for music. Native decoder now accepts an explicit plane size; common menu samples
retain16, JBC uses32768, tails divide remaining bytes equally between channels.
Rounded predictor arithmetic and clipped sample history match the reference.

Actual tune5 byte differential passes:31521280 PCM bytes,
SHA-256 d3aad200958cbdc416597101f17078d2101022b743bc5ad777a0960cc3fcd5c9.
Reproduce with src/tests/sound_reference_differential.py and the built jukebox_tests
runner; exact command in docs/SOUND_RECOVERY.md. Original samples and reference
WAV/PCM remain ignored under local/task0016/audio-reference, never tracked.

## Limits

The reference identifies a format interpretation; it does not constitute original
SPU playback comparison. No hardware/PCSX2 audio capture was compared, so no Verified
status. Prior tests that checked only rate/channel/count failed to detect this error;
byte/channel-order regression now protects the repaired path.
