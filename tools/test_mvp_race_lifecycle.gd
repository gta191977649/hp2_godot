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
var start_position := Vector3.ZERO
var selected_mode := 0
var mode_changes := 0
var check_replay := false
var mode_screen := ""
var picker_direction := 0
var campaign_moves := 0

func _initialize() -> void:
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
                original_scene = race.get_node_or_null("RaceScene")
                if sample.get("track") != expected_track or not original_scene or \
                        sample.get("placements", 0) <= 0 or sample.get("vehicles", 0) <= 0 or \
                        root.get_camera_3d() == null:
                    return _fail("expected_committed_scene")
                if selected_mode >= 3 and sample.get("game_type") != selected_mode:
                    return _fail("expected_challenge_mode")
                if selected_mode == 7 and (not sample.get("player_is_pursuing_officer", false) or not sample.get("police_hud", false)):
                    return _fail("expected_police_role_and_hud")
                if selected_mode >= 3 and sample.has("reverse") and sample["reverse"] != (picker_direction == 1):
                    return _fail("expected_challenge_direction")
                first_generation = sample["generation"]
                start_position = sample["position"]
                _key(KEY_W, true)
                race_phase = 1
        1:
            if active and sample.get("speed", 0) >= 12.0 and \
                    start_position.distance_to(sample["position"]) >= 10.0 and \
                    (not check_replay or sample.get("step", 0) >= 360):
                _key(KEY_W, false)
                _send(KEY_ESCAPE)
                race_phase = 2
        2:
            if paused:
                var target := REPLAY if check_replay else RESUME
                _select_item(selected, target)
                if selected == target:
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
                if race.has_node("RaceScene"):
                    return _fail("return_scene_retirement")
                print("HP2_MVP_LIFECYCLE_PASS track=", expected_track, " direct=", direct)
                quit(0)
        12:
            if sample.get("race_state") == 4 and sample.get("replay_epoch", 0) > 0:
                print("HP2_MVP_REPLAY_PASS track=", expected_track, " epoch=", sample["replay_epoch"])
                quit(0)
    return false

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

func _send(key: Key) -> void:
    _tap(key)
    next_input = Time.get_ticks_msec() + 450

func _select_item(current: int, target: int) -> void:
    if current == target:
        _send(KEY_ENTER)
    else:
        _send(KEY_DOWN)
