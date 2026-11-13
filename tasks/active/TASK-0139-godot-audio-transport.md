# TASK-0139 - Godot game audio transport

- Status: active
- Scope: connect original game audio, including police radio, to Godot playback and mixing. Keep the donor's request ordering, voice lifecycle, gains, loop points and completion.
- Current boundary: `NativeApplicationHost` supplies OpenAudio, PumpAudio and CloseAudio. The Windows consumer delegates sample-type 1 commands and observations to the optional Godot speech output. Other game audio uses XAudio2. Original AVI audio uses Godot's video playback.
- Codec: Godot AudioStreamWAV supports IMA ADPCM. Original game banks and speech use PS2 ADPCM. Retain the source decoder and move bounded read/decode work to the Godot-side transport workers, then send PCM to Godot's audio mixer.
- Acceptance: verify menu sound, engine loops, pursuit radio, music, pause, return and shutdown. Compare radio playback order and frame spikes on Track 13.
- Next: compare the Godot speech path against XAudio2 before migrating other audio categories. The previous asynchronous XAudio2 experiment was reverted after an inconclusive full-route comparison.

## Implementation and checks

- `HP2_MVP_AUDIO_BACKEND=godot` enables the speech-only Godot path. Unset the variable or use `xaudio2` for the default. The donor request pump retains ordering and completion, and its speech producer supplies the ranges after removing each 48-byte header. One Godot-side worker reads 1 KiB encoded blocks and reuses the donor decoder and predictor history. Godot reads from a pool of three 16,384-frame rings, with a 1,024-source-frame prebuffer or a fully decoded shorter clip.
- Debug and Release staged with `cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 8` and the corresponding Release preset. Godot 4.7.2 Forward+ Release frontend smoke reached Track 12, paused and returned to the main menu during implementation.
- The Debug Track 13 run from the frontend with the donor racer AI opened and played five sentences, observed four completions before shutdown, consumed 257,224 source frames, and reported zero underrun blocks. Maximum submission-to-prebuffer time was 16,603 us, including worker scheduling. See `local/task0137-visual/audio-godot-pursuit/stderr.log`. Godot reported null-mesh errors during teardown.
- Acceptance stays open. The capture did not verify a natural finish, a matched XAudio2 A/B, displayed-frame intervals, isolated audio-related spikes, or radio ordering/pause/BUSTED by listening. Engine, siren, menu and music migration remain pending. Compare authored effects and gain envelopes before switching the default.

To resume, set `$env:HP2_MVP_AUDIO_BACKEND='godot'` and launch with
`./build.ps1 --run --mvp --debug --jobs 16`. Repeat with `xaudio2` and the
same Track 13 AI, resolution and Godot Profiler capture settings.
