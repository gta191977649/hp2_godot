# TASK-0151 - Issue #5 split-screen investigation

- Status: complete (investigation only; split-screen remains unimplemented).
- Date: 2026-11-20, Australia/Sydney.
- Issue: https://github.com/gta191977649/hp2_godot/issues/5
- Source baseline: `9c2d388e2aaa4994cc523eae47d7e76d9d9536ea`.
- Dependencies: TASK-0141 single-player races and controller integration,
  TASK-0124 presentation ownership, TASK-0126 authored UI.

## Finding

The current Godot host does not assign a second local player. It can detect
two controllers independently, but either controller drives P1. The earliest
source-level blocker is `RaceHost::ReadInput`: only port 0 receives input;
port 1 returns disconnected. The original Challenge screen consequently
keeps the number of players at one. A controller driver/mapping adjustment
alone cannot enable split-screen races: race admission and presentation also
explicitly support only one player.

This explains the reported symptom for the inspected implementation. The
reporter's build, OS, controller models, connection methods and failing
screen were not supplied. Their exact hardware failure remains unverified.
The issue was open, labelled enhancement, and had no comments when read
through the GitHub connector on 2026-11-20. No issue comment was posted.

## Evidence and data flow

Source paths and line numbers below refer to the baseline above. This is
port-source inspection, not an original-game equivalence claim.

| Boundary | Evidence | Result |
| --- | --- | --- |
| Godot discovery | `src/mvp/presentation/race_presentation.cpp:110`, `RaceHost::PollEvents` enumerates all joypads | Runtime verified: devices 0 and 1 have independent button and axis state. |
| Device ownership | Same file, `RaceHost::use_joypad` at 138 and the single `racePad_` | A second pad replaces P1's active pad; no per-player assignment. Runtime verified for button and analog takeover. |
| Native input | Same file, `RaceHost::ReadInput` at 277 | Only `port == 0` populates `ControllerState`; other ports return disconnected. Source verified; no direct port-1 telemetry was added. |
| Original scanner | `src/game_core/indep/src/main.cpp`, `ReadApplicationInput` and `GenerateInput`; `joystick.cpp`, `joystickGetType` / `joystickIsPluggedIn` | Existing host interface already accepts a port. Disconnected port 1 cannot satisfy the menu's native controller check. |
| Player-count admission | `src/game_core/indep/src/frontendscreens/racesetup.cpp:43`, `BothControllers`, `MaybeChangeNumPlayers`, Challenge tick/scroll | Both ports are required for arrows; missing port 1 clamps the desired count to one. This is the first downstream user-visible blocker by source inspection. |
| Player/profile and car selection | Same file, `ChallengeScreen::OnSelect`; `frontendscreens/selectcar.cpp:944`, `ActivateCarSelector` | Native flow already chooses `Select Car2P` for two players and routes selectors by original joystick port and profile. P2 activation cannot be reached normally through the current input adapter. |
| Race handoff | `src/game_core/indep/src/main.cpp:730` | State-only entry/continuation throws `NotImplemented("Godot race handoff", "Local multiplayer presentation and input")` for more than one player or mode greater than one. |
| Committed bridge | `src/mvp/bridge/entity_state_bridge.cpp:65`, 77, 140 | Publishes primary camera, marks only `playersByNumber[0]`, and asserts exactly one player vehicle. |
| View interface | `src/game_core/indep/inc/Flags.hpp`, `RaceStateView`; `src/mvp/bridge/presentation_snapshot.hpp`, `EntityFrame` | One primary camera plus optional rear camera; vehicle has a boolean player flag, not a local seat identity. Rear view is not P2. |
| Presentation and controls | `src/mvp/presentation/race_presentation.cpp`, P1 settings at 948 and P1 visual/camera at 1062 | Camera, controls settings, orbit and effects orchestration assume P1. Existing frontend preview rendering supports multiple view keys, but that does not establish race split-screen support. |
| Rumble | Same file, `SetRumble` at 241; native runtime's two-port motor loop | Core already sends port-specific motor updates; Godot ignores all ports except 0. |

### Release comparison

`beta-v6` points to `ebacec4a3481827cf242695d40b6b8cd7e2a0502`
(2026-11-17); `x64-beta-v5` points to
`58a67ba30ab7b0b789de66c4ead5e85c311e60f3` (2026-11-16).
Read-only `git show` checks found the P1-only input path in both tags, the
multiplayer handoff guard in beta-v6, and the single-player bridge assertion
in v5. `git diff beta-v6 HEAD` confirms the relevant guard and bridge
restriction remain; later clock/physics changes do not provide P2 support.
This is not evidence that the reporter ran either release. Exported release
executables were not run in this investigation.

### Lifecycle gap assessment

| Area | Current evidence | Follow-up required |
| --- | --- | --- |
| Pause/disconnect | Native pause menu checks each player's controller port; host exposes only P1 | Verify independent pause ownership, reconnection prompts and released inputs with two real seats. |
| Restart/menu return | Existing generation retirement covers single-player; multiplayer handoff still rejects | Verify both seats retire/rebuild without stale inputs, HUDs or view resources. |
| Replay | Native ReplayScreen contains a second-player path; bridge exposes primary/rear camera only | Establish intended original two-player replay behavior and verify seek epochs for both views. |
| Results/profiles | Native two-player result and profile consumers exist | Exercise both racers' results, progression/save ownership and return flow. |
| Per-view rendering | Current primary view drives scene visibility and effects | Separate camera visibility, hidden cars, HUD/minimap placement, depth/post-effect resources and rear views for each seat. Profile with Godot Visual Profiler. |
| Audio/rumble | Native scheduling remains authoritative; host rumble accepts P1 only | Verify native two-player spatial audio policy and independent motor routing. |

None of those two-player lifecycle paths was force-enabled or claimed tested.
The multiplayer guard remains necessary and unchanged.

## Runtime reproduction and validation

Added `tools/test_mvp_second_controller.py` and
`tools/test_mvp_second_controller.gd`. These are an explicit investigation
probe for current single-player takeover behavior, not an acceptance test
for future split-screen. Future independent seat assignment should replace
the takeover expectations. No production interface or game_core file changed.

The runner uses the existing local `vgamepad` package and installed ViGEmBus
to send OS-level input through two virtual Xbox 360 controllers. It does not
install packages/drivers. It runs a visible Godot Vulkan race on Track 33,
enables the existing controls telemetry and isolated diagnostic profile,
and stores IPC/logs under ignored `local/issue-5-*` directories. It requires
no other connected controllers, because device enumeration order identifies
the two test pads. `--reverse` reverses the creation order of the two logical
virtual pads; both emulate the same controller model.

Environment: Windows x64, Godot 4.7.2 `ed1daf0bf`, Vulkan 1.4.359,
Intel Arc B390. Release `stage_mvp` completed successfully before the runs.
The tested `hp2_blackbox.dll` SHA-256 was
`fd90e9d6e53c3545fa7ce87677a667faf7c80324d9a9a534c5d02c566e033077`.

```powershell
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 6
py -3 tools/test_mvp_second_controller.py --vgamepad-dir local/issue-1-vgamepad
py -3 tools/test_mvp_second_controller.py --vgamepad-dir local/issue-1-vgamepad --reverse
py -3 tools/test_mvp_second_controller.py --vgamepad-dir local/issue-1-vgamepad --startup
py -3 tools/test_mvp_second_controller.py --vgamepad-dir local/issue-1-vgamepad --startup --reverse
py -3 reverse/scripts/generate_progress.py
py -3 reverse/scripts/generate_progress.py --check
git diff --check
```

All four final probe variants exited 0 with
`SECOND_CONTROLLER_INVESTIGATION_PASS`. Each verified independent OS button
states, P1 throttle from pad 0, P1 brake and steering takeover from pad 1,
keyboard acceleration with both pads connected, release to neutral, remaining
pad operation after disconnecting pad 0, all-input release on disconnect,
and neutral/reusable input after reconnect. Devices enumerated as `[0, 1]`,
then `[1]`, then `[]`, then `[0]`, exercising a remaining nonzero device ID.
`py -3 -m py_compile tools/test_mvp_second_controller.py` also passed.
Progress regeneration updated the session-log index; the subsequent
`--check` reported the compact progress, report and log index current.
`git diff --check` passed (only the existing Windows LF/CRLF advisory).

Final evidence directories (each contains `probe.log`):

| Connection timing | Creation order | Local directory |
| --- | --- | --- |
| During race | Normal | `local/issue-5-20261120-182021-59120` |
| During race | Reversed | `local/issue-5-20261120-181912-57236` |
| Before launch | Normal | `local/issue-5-20261120-181936-3564` |
| Before launch | Reversed | `local/issue-5-20261120-181955-13568` |

Earlier exploratory runs predated the axis check and are not counted above.
The runner does not inject Godot joypad events: controller button/axis checks
use ViGEm OS input. Only the keyboard takeover check injects a Godot key event.
Physical USB/Bluetooth pads, Linux/macOS backends, reporter-specific mappings,
the actual two-player menu UI and two-player races remain unverified.
No original ELF/ISO comparison was performed; PAL1B5C90 is an existing donor
annotation for `MaybeChangeNumPlayers`, with original ELF revision/hash
unresolved. No recovered behavior or equivalence status was changed.

## Recommended implementation sequence

1. Add explicit per-seat host assignment and independent input/edge/rumble
   state using existing `ReadInput(port, ...)` and `SetRumble(port, ...)`.
   Keep single-player keyboard/gamepad takeover. In two-player mode, a pad
   must not steal the other seat; global `device=-1` menu actions must not
   merge P2 input into P1. Define keyboard-plus-pad admission and reconnect
   ownership before implementing that mode. Validate the native Challenge
   player count, two profiles and both selectors without bypassing checks.
2. Extend the smallest Godot-independent committed view boundary for stable
   seat IDs, each player's camera/hidden-car/render operands and HUD ownership.
   Carry generation/step/replay epoch through both views. Share world entities
   while Godot owns separate viewports, per-view visibility/depth/effects and
   authored split-screen UI. Record the integration dependency and validation
   for any required core boundary change; preserve native rules and clocks.
3. Enable multiplayer race admission only after both input and presentation
   can consume the original two-player setup. Cover two pads and the chosen
   keyboard policy, disconnect/reconnect, pause/resume, restart, replay,
   results/save and return to menu. Retain explicit NotImplemented behavior
   for paths that remain unsupported. Validate Vulkan/D3D12 and per-seat
   authored aspect ratios; use the Godot Visual Profiler for two-view costs.

Do not remove the handoff guard as a standalone controller fix. Full feature
implementation needs a separate task; the present investigation is complete.

## Proposed issue response (not posted)

Thanks for reporting this. The current Godot port only connects input to
player 1; split-screen is not implemented yet. I tested two virtual Xbox
controllers: both are detected, but the second controller takes over player 1
rather than becoming player 2. There are also race-camera and presentation
changes needed before two-player races can work.

Which release/version and operating system are you using, what are the two
controller models, and are they connected over USB or Bluetooth? Please also
describe the menu where player 2 stops working and whether each controller
works as player 1 when used alone. That will help distinguish the current
multiplayer limitation from an additional device-detection problem.
