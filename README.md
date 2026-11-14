# HP2 Godot MVP

The repository contains the Windows C++ game core from `D:/dev/hp2_ps2_rev`
and a Godot 4.7.2 project for the race and frontend integration.

## Build and launch

Use CMake 3.24+, Ninja, a Windows x64 C++17 toolchain. Place the Godot 4.7.2 console executable under
`local/tools/godot-4.7.2/`.

```powershell
./build.ps1 --run
```

The default launch opens the original title sequence in Godot. Leave the
Start Game prompt idle for 30 seconds to start the original attract demo.
Press a menu button to stop the demo and return to the main menu.

Under **Options → Credits**, select **The Porting of HP2** for the port credits.
Edit `src/mvp/assets/port_credit.hpp` and rebuild to update the text.
Press **Space** or the controller's **X / Square** to pause or resume scrolling.
See [third-party notices](third_party/THIRD_PARTY_NOTICES.md) for licenses
and source references. The staged project includes these notices.

With `./build.ps1 --run --mvp --debug --jobs 16`, select **Debug** just before
**Exit** to open **Track Viewer**, see the **Car Viewer** placeholder, or start
the attract demo immediately with **View Demo Scene**.
The build uses the shared FFmpeg 8 SDK under `local/tools/ffmpeg`.
For another installation, configure CMake with `-DHP2_FFMPEG_SDK=<sdk-directory>`.
Godot plays the original AVI directly from the game asset source through
the FFmpeg MV30 decoder. Playback needs no generated media files.

Use the arrow keys and Enter in the menus. World Racing leads to Quick
Race or the Championship event tree (Track 12). Hot Pursuit → Ultimate
Racer leads to the first event, Lotus Elise Delivery (Track 13).
Use WASD or the arrow keys to drive, Space for handbrake, and Esc or Backspace to pause.
Use H for the horn or police signal, Q/E to shift, Shift to look back,
C to change camera, Tab to recover the car and N for nitrous where available.
While driving, move the mouse to orbit the car. After 1.5 seconds without
mouse movement, the view blends back to the selected HP2 camera.
In Be the Cop, H changes the pursuit target. Hold Space and press E to
request a roadblock or Q to request helicopter support. R and T are
shortcuts for those two requests.
In the frontend, Backspace goes back and Esc exits the application. Edit the
game's keyboard bindings in Godot under **Project Settings → Input Map**.
Click menu items to select them, scroll to navigate lists, and right-click
to go back. The race HUD uses its existing driving controls.
Under **Options → Player Name**, open the name editor and type directly.
Use Left/Right to move the caret, Backspace or Delete to remove characters,
and Enter or Esc to commit the name.

On Windows, `./build.ps1 --run --rendering-driver d3d12` starts Forward+
with Direct3D 12 and SDR output. Windows 11 can apply Auto HDR to eligible
games when HDR and Auto HDR are enabled in Windows Display Settings.
The default launcher uses Vulkan.
The default race minimap rotates with the player's car. Single-player
fixed-map choices are available in Display Options.
`./build.ps1 --run --race-track=33` opens the diagnostic race directly.
Use an authored scenario ID to choose a track for this single-player Quick Race.
Use `./build.ps1 --race-event=ultimate-racer:2` to play the authored Track 33
career event directly. The other tree is `--race-event=championship:<id>`.
The diagnostic uses an isolated profile by default.

For race integration, use a complete extracted game directory, a PS2 disc image, or a
`ZZDATA.BIN` archive with its matching directory file. The extracted tree
must include the original sound banks. The asset path can also come from
`config/ps2_assets.local.json` (see `config/ps2_assets.example.json`).
Use `HP2_PROFILE_DIR` to select a profile directory. By default the game
uses the Godot project's `user://profile` directory.
The Nitro action is bound to **N**. To enable the donor PC racer option,
add `racer_nitro=1` to `NFSHP2_pc.ini` in that profile directory.

`./build.ps1` stages the project in `build/godot-mvp-release/mvp-project`.
`./build.ps1 --run --mvp --debug --jobs 16` builds and runs the Debug
configuration and enables the native race debug tools. When launching the
staged Godot project directly, pass `-- --debug`. Explicit debug launches
use a fresh test profile unless `HP2_PROFILE_DIR` is set.
In a regular single-player race, hold **Start / Menu**, then press
**Select / Back / View** (keyboard: hold **Esc** or **Backspace**, then press
**Tab**). Release all controls before using the camera. Short Select cycles
the cameras; repeat the chord to restore the previous view and resume driving.
Press and release Start by itself to pause, then choose **Debug** for the native
pause debug menu.
In the debug menu, press **Backspace**, **Esc**, or **B / Circle**, or
right-click, to return to the Pause Menu.

The grave/tilde key toggles the ImGui Console and gives the game its keyboard
input back. The live frame-time graph stays visible throughout the frontend and race.
Enter `effect` in the console to open the live race effect checkboxes.
The window stays available after closing the console. Checked effects
follow the game's triggers, including forest fire smoke and sandstorms.
Enter `trackviewer` in the Debug Console to browse the original tracks.
Choose a track from the **Track** dropdown in the top bar. Expand the track
bundles, sections and models in **Hierarchy**. Left-click a model in the
viewport to select it in red, then click it again or click empty space
to clear the selection. Double-click an instance in
the tree to fly the camera to it. The selected item's properties appear below
the tree.
Hold the right mouse button in the viewport to fly and release it to stop.
Use WASD and Q/E to move, Shift to boost, and the wheel to change speed.
The **Main Menu**
button returns to the game frontend and reloads the player profile.
The window title shows the active graphics API and live FPS, for example
`hp2 - (Vulkan) 120 FPS`. Debug launches append ` | Debug Mode`.
Godot uses VSync to pace the entire game to the current display refresh.
The donor simulation runs on its own clock.
See [TASK-0125](tasks/active/TASK-0125-godot-frontend.md) for frontend validation and open coverage.
See [TASK-0126](tasks/active/TASK-0126-authored-frontend-and-race-ui.md)
for original font, texture and in-race UI coverage.
