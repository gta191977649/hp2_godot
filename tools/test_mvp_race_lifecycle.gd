extends "test_frontend_mvp.gd"

const RESUME := 0x52e039f3
const RESTART := 0x52e039f4
const REPLAY := 0x52e039f6
const QUIT_RACE := 0x52e039f9

var direct := false
var expected_track := 33
var first_generation := 0
var race_phase := 0
var original_scene: Node
var original_atlas: SubViewport
var start_position := Vector3.ZERO
var selected_mode := 0
var mode_changes := 0
var check_replay := false
var check_mouse_replay := OS.get_environment("HP2_MVP_MOUSE_REPLAY") == "1"
var replay_present_position := Vector3.ZERO
var seek_epoch := 0
var seek_at := 0
var seek_sent := false
var check_reflections := OS.get_environment("HP2_MVP_CHECK_REFLECTION_LIFECYCLE") == "1"
var mode_screen := ""
var picker_direction := 0
var campaign_moves := 0
var check_shortcuts := OS.get_environment("HP2_MVP_TEST_RACE_SHORTCUTS") == "1"
var checking_shortcuts := false

func _initialize() -> void:
    if check_shortcuts:
        OS.set_environment("HP2_MVP_TEST_CONTROLS", "1")
    direct = not OS.get_cmdline_user_args().is_empty()
    check_replay = OS.get_environment("HP2_MVP_CHECK_REPLAY") == "1"
    if direct:
        expected_track = OS.get_cmdline_user_args()[0].trim_prefix("--race-track=").to_int()
    else:
        var track_setting := OS.get_environment("HP2_MVP_EXPECTED_TRACK")
        if not track_setting.is_empty():
            expected_track = track_setting.to_int()
        selected_mode = OS.get_environment("HP2_MVP_CHALLENGE_MODE").to_int()
        mode_changes = selected_mode - 3
        mode_screen = "ChallengeBeACop" if selected_mode == 7 else "Challenge"
        picker_direction = OS.get_environment("HP2_MVP_PICKER_DIRECTION").to_int()
        campaign_moves = OS.get_environment("HP2_MVP_CAMPAIGN_NEXT").to_int()
    super._initialize()
    if selected_mode == 7:
        pursuit = true

func _process(delta: float) -> bool:
    if not direct and stage != 8:
        if is_instance_valid(race) and stage in [2, 10]:
            var title: Dictionary = race.telemetry()
            if title.get("video_request", 0) == 2:
                stage = 11
                next_input = 0
        if selected_mode >= 3 and _challenge_setup():
            return false
        if campaign_moves > 0 and stage == 15 and Time.get_ticks_msec() >= next_input:
            _send(KEY_DOWN)
            campaign_moves -= 1
            return false
        if (championship or pursuit) and selected_mode == 0 and stage in [7, 19] and is_instance_valid(race):
            if race.telemetry().get("race_state") == 3:
                stage = 8
                return false
        return super._process(delta)
    if Time.get_ticks_msec() > deadline:
        return _fail("race_lifecycle_%d" % race_phase)
    if not is_instance_valid(race):
        race = root.get_node_or_null("NativeGame/RacePresentation")
        _dismiss_debug_console()
        return false
    var sample: Dictionary = race.telemetry()
    var now := Time.get_ticks_msec()
    if now < next_input:
        return false
    var active: bool = sample.get("race_state", 0) == 3
    var paused: bool = sample.get("screen", "") == "Pause Menu"
    var selected: int = sample.get("selected_item", 0)
    match race_phase:
        0:
            if active:
                if check_shortcuts:
                    if not checking_shortcuts:
                        checking_shortcuts = true
                        _check_race_shortcuts.call_deferred(sample.controls_camera_view)
                    return false
                original_scene = race.race_scene()
                if sample.get("track") != expected_track or not original_scene or \
                        sample.get("placements", 0) <= 0 or sample.get("vehicles", 0) <= 0 or \
                        race.race_scene().get_viewport().get_camera_3d() == null:
                    return _fail("expected_committed_scene")
                if selected_mode >= 3 and sample.get("game_type") != selected_mode:
                    return _fail("expected_challenge_mode")
                if selected_mode == 7 and (not sample.get("player_is_pursuing_officer", false) or not sample.get("police_hud", false)):
                    return _fail("expected_police_role_and_hud")
                if selected_mode >= 3 and sample.has("reverse") and sample["reverse"] != (picker_direction == 1):
                    return _fail("expected_challenge_direction")
                if OS.get_environment("HP2_MVP_TEST_TRACK_REVERSE") == "1" and not sample.get("reverse", false):
                    return _fail("expected_reverse_track")
                if check_reflections:
                    original_atlas = original_scene.get_node_or_null("CarReflectionAtlas") as SubViewport
                    if not _reflection_world_valid(original_scene):
                        return _fail("reflection_and_rear_view_worlds")
                first_generation = sample["generation"]
                start_position = sample["position"]
                _key(KEY_UP, true)
                race_phase = 1
        1:
            var drove: bool = sample.get("speed", 0) >= 12.0 and start_position.distance_to(sample["position"]) >= 10.0
            var replay_start_step := 660 if check_mouse_replay else 360
            if active and ((check_replay and sample.get("step", 0) >= replay_start_step) or (not check_replay and drove)):
                _key(KEY_UP, false)
                _send(KEY_ESCAPE)
                race_phase = 2
        2:
            if paused:
                var target := REPLAY if check_replay else RESUME
                _select_item(selected, target)
                if selected == target:
                    if check_mouse_replay:
                        replay_present_position = sample["position"]
                    race_phase = 12 if check_replay else 3
        3:
            if active:
                _send(KEY_ESCAPE)
                race_phase = 4
        4:
            if paused:
                _select_item(selected, RESTART)
                if selected == RESTART:
                    race_phase = 5
            elif active:
                _send(KEY_ESCAPE)
        5, 9:
            if paused and sample.get("modal", false):
                _send(KEY_LEFT)
                race_phase += 1
        6, 10:
            if paused and sample.get("modal", false):
                _send(KEY_ENTER)
                race_phase += 1
        7:
            if sample.get("generation", 0) > first_generation and sample.get("screen", "").begins_with("Pre Race"):
                _send(KEY_ENTER)
            elif sample.get("generation", 0) > first_generation and active:
                if is_instance_valid(original_scene) or sample.get("track") != expected_track:
                    return _fail("restart_scene_retirement")
                if check_reflections and (is_instance_valid(original_atlas) or not _reflection_world_valid(race.race_scene())):
                    return _fail("restart_reflection_retirement")
                if check_reflections:
                    original_atlas = race.race_scene().get_node("CarReflectionAtlas") as SubViewport
                _send(KEY_ESCAPE)
                race_phase = 8
        8:
            if paused:
                _select_item(selected, QUIT_RACE)
                if selected == QUIT_RACE:
                    race_phase = 9
            elif active:
                _send(KEY_ESCAPE)
        11:
            var page: String = sample.get("screen", "")
            var returned := page == "New Main Menu" or page == "View Event Tree"
            if returned or (direct and not sample.has("generation")):
                if race.race_scene() != null:
                    return _fail("return_scene_retirement")
                if check_reflections and is_instance_valid(original_atlas):
                    return _fail("return_reflection_retirement")
                print("HP2_MVP_LIFECYCLE_PASS track=", expected_track, " direct=", direct)
                quit(0)
        12:
            if sample.get("race_state") == 4 and sample.get("replay_epoch", 0) > 0:
                if check_mouse_replay:
                    race_phase = 14
                    _replay_mouse_smoke.call_deferred()
                    return false
                if OS.get_environment("HP2_MVP_CHECK_REPLAY_SEEK") == "1":
                    if seek_epoch == 0:
                        seek_epoch = sample["replay_epoch"]
                        seek_at = now + 2500
                    if not seek_sent and now >= seek_at:
                        var wheel := InputEventMouseButton.new()
                        wheel.button_index = MOUSE_BUTTON_WHEEL_UP
                        wheel.position = Vector2(root.size) * 0.5
                        wheel.pressed = true
                        Input.parse_input_event(wheel)
                        seek_sent = true
                    if sample["replay_epoch"] <= seek_epoch:
                        return false
                    if sample.get("scenery_sections", 0) <= 0:
                        return _fail("missing_sectors_after_replay_seek")
                if check_reflections:
                    if not is_instance_valid(original_atlas):
                        return _fail("replay_reflection_atlas_lifetime")
                    _check_replay_atlas.call_deferred(sample["replay_epoch"])
                    race_phase = 13
                    return false
                print("HP2_MVP_REPLAY_PASS track=", expected_track, " epoch=", sample["replay_epoch"])
                quit(0)
    return false

func _replay_mouse_button(point: Vector2, down: bool, button := MOUSE_BUTTON_LEFT) -> void:
    var event := InputEventMouseButton.new()
    event.position = (race.get_node("FrontendCanvas") as Node2D).to_global(point)
    event.global_position = event.position
    event.button_index = button
    event.pressed = down
    root.push_input(event, true)

func _replay_click(point: Vector2) -> void:
    _replay_mouse_button(point, true)
    _replay_mouse_button(point, false)

func _replay_wait(label: String, matches: Callable) -> bool:
    var until := Time.get_ticks_msec() + 8000
    while Time.get_ticks_msec() < until:
        await process_frame
        if matches.call(race.telemetry()):
            return true
    print("REPLAY_MOUSE_STATE ", race.telemetry())
    _fail(label)
    return false

func _replay_motion(point: Vector2) -> void:
    var motion := InputEventMouseMotion.new()
    motion.position = (race.get_node("FrontendCanvas") as Node2D).to_global(point)
    motion.global_position = motion.position
    motion.button_mask = MOUSE_BUTTON_MASK_LEFT
    root.push_input(motion, true)

func _replay_at_fraction(state: Dictionary, fraction: float) -> bool:
    var expected := lerpf(state.get("replay_begin_ticks", 0), state.get("replay_end_ticks", 0), clampf(fraction, 0, 1))
    return not state.get("replay_seeking", true) and absf(state.get("replay_ticks", 0) - expected) <= 75

func _replay_seek(fraction: float, drag: bool) -> bool:
    var state: Dictionary = race.telemetry()
    var bar: Rect2 = state.get("replay_bar", Rect2())
    if not bar.has_area():
        _fail("missing_replay_bar")
        return false
    var epoch: int = state["replay_epoch"]
    var target := Vector2(bar.position.x + fraction * bar.size.x, bar.get_center().y)
    if drag:
        var midpoint := lerpf(state["replay_begin_ticks"], state["replay_end_ticks"], 0.5)
        var start_fraction := 0.1 if state["replay_ticks"] > midpoint else 0.9
        _replay_mouse_button(Vector2(bar.position.x + start_fraction * bar.size.x, target.y), true)
        var intermediate := 0.4
        _replay_motion(Vector2(bar.position.x + intermediate * bar.size.x, target.y))
        if not await _replay_wait("replay_live_seek_intermediate", func(s: Dictionary) -> bool:
            return s.get("replay_epoch", 0) > epoch and _replay_at_fraction(s, intermediate)):
            return false
        _replay_motion(target)
        if not await _replay_wait("replay_live_seek_%s" % fraction, func(s: Dictionary) -> bool:
            return _replay_at_fraction(s, fraction)):
            return false
        _replay_mouse_button(target, false)
    else:
        _replay_click(target)
    return await _replay_wait("replay_seek_%s" % fraction, func(s: Dictionary) -> bool:
        return s.get("replay_epoch", 0) > epoch and _replay_at_fraction(s, fraction))

func _replay_mouse_smoke() -> void:
    var play := Vector2(165, 438)
    var camera := Vector2(207, 438)
    var help := Vector2(252, 438)
    var hud := Vector2(340, 438)
    if not await _replay_wait("replay_mouse_entry", func(s: Dictionary) -> bool: return s.has("replay_bar")):
        return
    _replay_click(play)
    if not await _replay_wait("replay_mouse_pause", func(s: Dictionary) -> bool: return s.get("replay_paused", false)):
        return
    var start: Dictionary = race.telemetry()
    var bar: Rect2 = start["replay_bar"]
    _replay_mouse_button(bar.get_center(), true)
    race.notification(Node.NOTIFICATION_WM_WINDOW_FOCUS_OUT)
    _replay_mouse_button(bar.get_center(), false)
    await create_timer(0.2).timeout
    if race.telemetry()["replay_epoch"] != start["replay_epoch"]:
        _fail("replay_seek_after_focus_loss")
        return
    if start.has("debug_console_open"):
        _replay_mouse_button(bar.get_center(), true)
        await _tap(KEY_QUOTELEFT)
        if not race.telemetry()["debug_console_open"]:
            _fail("replay_console_did_not_open")
            return
        await _tap(KEY_QUOTELEFT)
        _replay_mouse_button(bar.get_center(), false)
        await create_timer(0.2).timeout
        if race.telemetry()["replay_epoch"] != start["replay_epoch"]:
            _fail("replay_seek_after_console_capture")
            return
    var before_camera: Vector3 = race.telemetry()["stock_camera_eye"]
    _replay_click(camera)
    if not await _replay_wait("replay_mouse_camera", func(s: Dictionary) -> bool:
        return before_camera.distance_to(s["stock_camera_eye"]) > 0.1):
        return
    if not await _replay_seek(0.75, true) or not await _replay_seek(0.2, false):
        return
    if not race.telemetry()["replay_paused"]:
        _fail("replay_seek_lost_pause")
        return
    if not await _replay_seek(-0.05, true):
        return
    var epoch: int = race.telemetry()["replay_epoch"]
    _replay_click(Vector2(296, 438))
    if not await _replay_wait("replay_mouse_next", func(s: Dictionary) -> bool: return s["replay_epoch"] > epoch):
        return
    epoch = race.telemetry()["replay_epoch"]
    _replay_click(Vector2(75, 438))
    if not await _replay_wait("replay_mouse_previous", func(s: Dictionary) -> bool: return s["replay_epoch"] > epoch):
        return
    _replay_click(Vector2(119, 438))
    if not await _replay_wait("replay_mouse_slow", func(s: Dictionary) -> bool: return s["replay_slow"]):
        return
    if not await _replay_seek(0.55, true):
        return
    if not await _replay_wait("replay_seek_resume_slow_motion", func(s: Dictionary) -> bool:
        return s["replay_slow"] and not s["replay_paused"]):
        _fail("replay_seek_lost_slow_motion")
        return
    await _tap(KEY_ENTER)
    if not await _replay_wait("replay_keyboard_pause", func(s: Dictionary) -> bool: return s["replay_paused"]):
        return
    _replay_click(play)
    if not await _replay_wait("replay_mouse_resume", func(s: Dictionary) -> bool: return not s["replay_paused"]):
        return
    _replay_click(help)
    if not await _replay_wait("replay_mouse_help", func(s: Dictionary) -> bool: return s["replay_help"] and s["replay_paused"]):
        return
    _replay_click(help)
    if not await _replay_wait("replay_mouse_dismiss_help", func(s: Dictionary) -> bool:
        return not s["replay_help"] and not s["replay_paused"]):
        return
    _replay_click(hud)
    if not await _replay_wait("replay_mouse_hide", func(s: Dictionary) -> bool: return not s["replay_hud"]):
        return
    _replay_click(hud)
    if not await _replay_wait("replay_mouse_show", func(s: Dictionary) -> bool: return s["replay_hud"]):
        return
    root.size = Vector2i(1120, 630)
    await create_timer(0.25).timeout
    if not await _replay_seek(1.05, true):
        return
    if not race.telemetry()["replay_paused"]:
        _fail("replay_endpoint_did_not_pause")
        return
    var end: Dictionary = race.telemetry()
    var end_bar: Rect2 = end["replay_bar"]
    # The authored TIME_MARKER is four design pixels wide.
    var end_fraction: float = float(end["replay_ticks"] - end["replay_begin_ticks"]) / \
        (end["replay_end_ticks"] - end["replay_begin_ticks"])
    var thumb_x := end_bar.position.x + floorf(end_bar.size.x * end_fraction)
    var grab := Vector2(thumb_x + 2, end_bar.get_center().y)
    var destination := Vector2(end_bar.position.x + 0.7 * end_bar.size.x + 2, grab.y)
    _replay_mouse_button(grab, true)
    _replay_motion(destination)
    if not await _replay_wait("replay_live_thumb_from_end", func(s: Dictionary) -> bool:
        return _replay_at_fraction(s, 0.7)):
        return
    _replay_mouse_button(destination, false)
    await create_timer(0.1).timeout
    _replay_click(Vector2(385, 438))
    if not await _replay_wait("replay_mouse_exit", func(s: Dictionary) -> bool: return s.get("screen") == "Pause Menu"):
        return
    var present: Dictionary = race.telemetry()
    if present["generation"] != first_generation or replay_present_position.distance_to(present["position"]) > 0.1:
        _fail("replay_exit_did_not_restore_present")
        return
    for row in range(8):
        if race.telemetry()["selected_item"] == REPLAY:
            await _tap(KEY_ENTER)
            break
        await _tap(KEY_DOWN)
        await create_timer(0.1).timeout
    if not await _replay_wait("replay_controller_entry", func(s: Dictionary) -> bool: return s["race_state"] == 4):
        return
    var controller := InputEventJoypadButton.new()
    controller.button_index = JOY_BUTTON_START
    controller.pressed = true
    Input.parse_input_event(controller)
    await create_timer(0.15).timeout
    var release := controller.duplicate() as InputEventJoypadButton
    release.pressed = false
    Input.parse_input_event(release)
    if not await _replay_wait("replay_controller_exit", func(s: Dictionary) -> bool: return s.get("screen") == "Pause Menu"):
        return
    print("HP2_MVP_REPLAY_MOUSE_PASS track=", expected_track, " live_drag=true")
    quit(0)

func _reflection_world_valid(scene: Node3D) -> bool:
    var rear := scene.get_node_or_null("RearViewTarget") as SubViewport
    var face := scene.get_node_or_null("CarReflectionAtlas/CarReflectionFace0") as SubViewport
    return rear != null and face != null and rear.world_3d == scene.get_world_3d() and face.world_3d != rear.world_3d

func _check_replay_atlas(epoch: int) -> void:
    await RenderingServer.frame_post_draw
    var image := original_atlas.get_texture().get_image()
    if image == null or image.is_empty() or image.get_pixel(128, 128).a <= 0.1:
        _fail("replay_reflection_atlas_content")
        return
    print("HP2_MVP_REPLAY_PASS track=", expected_track, " epoch=", epoch, " reflection_atlas=", image.get_size())
    quit(0)

func _capture(name: String) -> bool:
    if screenshot_dir.is_empty() or name == "nfs-intro":
        return true
    return await super._capture(name)

func _challenge_setup() -> bool:
    if not is_instance_valid(race):
        return false
    var sample: Dictionary = race.telemetry()
    var page: String = sample.get("screen", "")
    var item: int = sample.get("selected_item", 0)
    var now := Time.get_ticks_msec()
    if stage == 6:
        if now >= next_input:
            if item == RESUME + 1:
                _send(KEY_ENTER)
                stage = 60
            else:
                _send(KEY_DOWN)
        return true
    if stage == 60:
        if page == mode_screen and now >= next_input:
            if selected_mode in [3, 7]:
                _send(KEY_ENTER)
                stage = 17
            elif item == RESUME + 1:
                _send(KEY_RIGHT)
                mode_changes -= 1
                stage = 61
            else:
                _send(KEY_DOWN)
        return true
    if stage == 61:
        if now >= next_input:
            if mode_changes > 0:
                mode_changes -= 1
                _send(KEY_RIGHT)
            else:
                _send(KEY_ENTER)
                stage = 17
        return true
    if stage in [17, 18, 19] and page == "Select Track":
        if now >= next_input:
            if sample.get("picker_base_track", expected_track) == expected_track and \
                    sample.get("picker_direction", 0) == picker_direction:
                if sample.get("picker_availability", 0) != 0:
                    _fail("locked_challenge_track")
                    return true
                _send(KEY_ENTER)
            else:
                var direction: int = sample.get("picker_direction", 0)
                var base: int = sample.get("picker_base_track", expected_track)
                var current := direction * 1000 + base
                var target := picker_direction * 1000 + expected_track
                _send(KEY_RIGHT if current < target else KEY_LEFT)
            stage = 19
        return true
    if stage in [7, 19] and sample.get("race_state", 0) == 3:
        stage = 8
        return true
    return false

func _check_race_shortcuts(camera_view: int) -> void:
    await _tap(KEY_SHIFT)
    var state: Dictionary = race.telemetry()
    if state.controls_camera_view != camera_view or state.controls_nitro_active or state.controls_handbrake > 0.1:
        _fail("regular_race_shift_activated_another_action")
        return
    if state.controls_automatic:
        for key in [KEY_A, KEY_Z]:
            await _tap(key)
            if race.telemetry().controls_camera_state != 0:
                _fail("automatic_shift_requested_zone_camera")
                return
    await _tap(KEY_C)
    if race.telemetry().controls_camera_view == camera_view:
        _fail("c_did_not_change_the_regular_race_camera")
        return
    print("HP2_REGULAR_RACE_SHORTCUTS_PASS")
    check_shortcuts = false

func _send(key: Key) -> void:
    _tap(key)
    next_input = Time.get_ticks_msec() + 450

func _select_item(current: int, target: int) -> void:
    if current == target:
        _send(KEY_ENTER)
    else:
        _send(KEY_DOWN)
