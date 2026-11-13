# TASK-0125 - Godot frontend and race handoff

- Status: active
- Scope: run the donor startup movies and frontend menu state in Godot, then enter the supported race through the original menu flow.
- Depends on TASK-0124 for the Godot race scene.

## Acceptance

- [x] Open the authored title video and title screen in a Windows Release Forward+ window.
- [x] Use keyboard input in the donor frontend and enter the Track 12 race through the menu.
- [ ] Preserve the explicit Track 12 diagnostic entry and trace unsupported race handoffs.
- [x] Record the commands, screenshots and outstanding frontend coverage.


## Original AVI playback

The Godot VideoStreamPlayer uses a VideoStreamPlayback extension backed by
FFmpeg 8 libavformat and libavcodec. The input is a seekable FileRange from
the original game source. Video decoding advances with playback. Godot
uploads the decoded planes and draws them with a canvas shader. The title
audio is read from the original AVI, decoded with the existing PS2 ADPCM
decoder, and submitted to the Godot mixer. The menu movie boundary carries
only the resource name, request ID, and completion. Playback creates no
intermediate media or cache. The previous Ogg cache, scratch AVI copies,
audio dumps and local media conversion probes were removed.

Windows Release, Godot 4.7.2 and Forward+ on Intel Arc B390. A full menu
smoke reached Championship, selected the first event and a car, entered
Track 12 and drove to World step 84. With the source ISO, the run
measured first-frame initialization at 1 ms for EA Logo and under 1 ms for
the NFS intro. The Godot mixer accepted 277,147 and 431,799 stereo frames
respectively before the clips ended or were skipped. This checks submission
to the mixer. Physical output and PS2 audio/video equivalence were not
measured. The original NFS intro has black opening frames. The window
test samples the actual pursuit footage after those frames and rejects a
black screenshot.

```powershell
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 12
# Set HP2_PS2_ASSET_ROOT to the complete original game source.
$env:HP2_PROFILE_DIR = "$PWD/local/task0125-direct-avi-profile"
$env:HP2_MVP_SCREENSHOT_DIR = "$PWD/local/task0125-direct-avi"
$env:HP2_MVP_CHAMPIONSHIP = '1'
New-Item -ItemType Directory -Force $env:HP2_MVP_SCREENSHOT_DIR
$engine = (Resolve-Path local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe).Path
& $engine --path build/godot-mvp-release/mvp-project --script (Resolve-Path tools/test_frontend_mvp.gd).Path
py -3 reverse/scripts/generate_progress.py --check
git -c core.safecrlf=false diff --check
```

Evidence includes local/task0125-direct-avi-final.log and the pursuit frame at
local/task0125-direct-avi/nfs-intro.png. Keep the task active for broader
frontend branches and presentation parity.
