extends SceneTree

const RouteDriver = preload("mvp_test_route_driver.gd")
var route: RouteDriver
var end_selection := 0

var race: Node
var deadline := 0
var stage := 0
var next_input := 0
var screenshot_dir := ""
var championship := false
var pursuit := false
var pursuit_moves := 0
var title_captured := false
var countdown_captured := false
var returning := false
var pause_selection := 0
var arrest_nis := false
var arrest_seen := false
var arrest_midpoint := 0
var arrest_motion_captured := false
var name_test := OS.get_environment("HP2_MVP_NAME_TEST")
var check_frontend_rate := OS.get_environment("HP2_MVP_FRONTEND_RATE") == "1"
var frontend_rate_start := 0
var frontend_rate_loop := 0

func _initialize() -> void:
    deadline = Time.get_ticks_msec() + 170000
    screenshot_dir = OS.get_environment("HP2_MVP_SCREENSHOT_DIR")
    championship = OS.get_environment("HP2_MVP_CHAMPIONSHIP") == "1"
    pursuit = OS.get_environment("HP2_MVP_PURSUIT") == "1"
    returning = OS.get_environment("HP2_MVP_RETURN") == "1"
    arrest_nis = OS.get_environment("HP2_MVP_TEST_ARREST_NIS") == "1"
    var course_path := OS.get_environment("HP2_ROUTE_CAPTURE")
    if not course_path.is_empty():
        route = RouteDriver.new()
        var driving_input := _key
        if OS.get_environment("HP2_MVP_PURSUIT_AI") == "1":
            driving_input = func(_keycode: Key, _held: bool): pass
        if not pursuit or not route.load_course(course_path, driving_input):
            _fail("invalid_pursuit_route_capture")
            return
        deadline = Time.get_ticks_msec() + 900000
    call_deferred("_open")

func _open() -> void:
    change_scene_to_file("res://native_game.tscn")

func _process(_delta: float) -> bool:
    if Time.get_ticks_msec() > deadline:
        return _fail("timeout")
    if not is_instance_valid(race):
        race = root.get_node_or_null("NativeGame/RacePresentation")
        _dismiss_debug_console()
        return false
    var sample: Dictionary = race.call("telemetry")
    var screen: String = sample.get("screen", "")
    var playing: bool = sample.get("video_playing", false)
    var request: int = sample.get("video_request", 0)
    if stage == 0 and screen == "Title Screen" and playing:
        stage = 1
        next_input = Time.get_ticks_msec() + 1500
    if stage == 1 and Time.get_ticks_msec() >= next_input:
        _capture("title-video")
        stage = 2
    if stage == 2 and screen == "Title Screen" and request == 2 and playing:
        stage = 10
        next_input = Time.get_ticks_msec() + 9500
    if stage == 10 and Time.get_ticks_msec() >= next_input:
        _capture("nfs-intro")
        stage = 11
        next_input = Time.get_ticks_msec() + 300
    if stage == 11 and screen == "Title Screen" and Time.get_ticks_msec() >= next_input:
        if request == 0 and not title_captured:
            title_captured = true
            _capture("title")
        _tap(KEY_ENTER)
        next_input = Time.get_ticks_msec() + 550
    if stage == 11 and screen == "New Main Menu":
        if not name_test.is_empty():
            stage = 90
            _test_player_name.call_deferred()
            return false
        stage = 3
        next_input = Time.get_ticks_msec() + 1400
    if stage == 3 and Time.get_ticks_msec() >= next_input:
        _capture("main-menu")
        if not pursuit:
            _tap(KEY_DOWN)
        stage = 4
        next_input = Time.get_ticks_msec() + 250
    if stage == 4 and Time.get_ticks_msec() >= next_input:
        _tap(KEY_ENTER)
        stage = 5
    if stage == 5 and screen == ("Cops On Mode Select" if pursuit else "Cops Off Mode Select"):
        if OS.get_environment("HP2_MVP_PAUSE_BINDINGS") == "1":
            stage = 47
            next_input = Time.get_ticks_msec() + 600
            return false
        stage = 6
        next_input = Time.get_ticks_msec() + 1000
    if stage == 47 and Time.get_ticks_msec() >= next_input:
        _tap(KEY_BACKSPACE)
        stage = 48
    if stage == 48 and screen == "New Main Menu":
        print("HP2_MVP_FRONTEND_BACKSPACE_PASS")
        stage = 49
        next_input = Time.get_ticks_msec() + 1000
    if stage == 49 and Time.get_ticks_msec() >= next_input:
        var wanted := 0x52e039f3 if pursuit else 0x52e039f4
        var selected: int = sample.get("selected_item", 0)
        if selected == wanted:
            _tap(KEY_ENTER)
            stage = 50
        else:
            _tap(KEY_UP if selected > wanted else KEY_DOWN)
            next_input = Time.get_ticks_msec() + 400
    if stage == 50 and screen == ("Cops On Mode Select" if pursuit else "Cops Off Mode Select"):
        stage = 6
        next_input = Time.get_ticks_msec() + 1000
    if stage == 6 and Time.get_ticks_msec() >= next_input:
        _capture("mode-select")
        if pursuit:
            stage = 29
        elif championship:
            _tap(KEY_DOWN)
            stage = 12
            next_input = Time.get_ticks_msec() + 300
        else:
            _tap(KEY_ENTER)
            stage = 7
    if stage == 29 and Time.get_ticks_msec() >= next_input:
        if pursuit_moves < 3:
            pursuit_moves += 1
            _tap(KEY_DOWN)
            next_input = Time.get_ticks_msec() + 300
        else:
            _tap(KEY_ENTER)
            stage = 14
    if stage == 12 and Time.get_ticks_msec() >= next_input:
        _tap(KEY_DOWN)
        stage = 13
        next_input = Time.get_ticks_msec() + 300
    if stage == 13 and Time.get_ticks_msec() >= next_input:
        _tap(KEY_ENTER)
        stage = 14
    if stage == 14 and screen == "View Event Tree":
        stage = 15
        next_input = Time.get_ticks_msec() + 1100
        frontend_rate_start = Time.get_ticks_msec()
        frontend_rate_loop = sample.get("frontend_loop", 0)
    if stage == 15 and Time.get_ticks_msec() >= next_input:
        if check_frontend_rate:
            var rate := 1000.0 * (int(sample.frontend_loop) - frontend_rate_loop) / (Time.get_ticks_msec() - frontend_rate_start)
            if rate < 85.0:
                return _fail("event_tree_frontend_rate_%s" % rate)
            print("HP2_MVP_EVENT_TREE_RATE_PASS ", rate)
        _capture("event-tree")
        _tap(KEY_ENTER)
        stage = 16
        next_input = Time.get_ticks_msec() + 1400
    if stage == 16 and Time.get_ticks_msec() >= next_input:
        _capture("event-details")
        _tap(KEY_ENTER)
        stage = 17
    if stage == 17 and screen == "Select Car":
        stage = 18
        next_input = Time.get_ticks_msec() + 1800
    if stage == 18 and screen == "Select Car" and Time.get_ticks_msec() >= next_input:
        _capture("select-car")
        _tap(KEY_ENTER)
        stage = 19
        next_input = Time.get_ticks_msec() + 1000
    if stage == 19 and screen == "Select Car" and Time.get_ticks_msec() >= next_input:
        _tap(KEY_ENTER)
        next_input = Time.get_ticks_msec() + 1000
    if (stage == 7 or stage == 19) and screen.begins_with("Pre Race") and Time.get_ticks_msec() >= next_input:
        _capture("pre-race")
        _tap(KEY_ENTER)
        next_input = Time.get_ticks_msec() + 1500
    if stage == 19 and sample.get("race_state", 0) == 3:
        stage = 7
    if (stage == 7 or stage == 19) and sample.get("race_state", 0) == 2 and not countdown_captured:
        countdown_captured = true
        _capture("countdown")
    if stage == 7 and sample.get("race_state", 0) == 3 and sample.get("step", 0) >= 1:
        if pursuit and sample.get("track", -1) != 13:
            return _fail("expected_track_13")
        if route == null and not arrest_nis:
            _key(KEY_W, true)
        stage = 8
    if route != null and stage >= 8:
        return _run_full_event(sample, screen)
    if arrest_nis and stage == 8:
        var parts: int = sample.get("nis_parts", 0)
        if parts >= 2 and not arrest_seen:
            arrest_seen = true
            arrest_midpoint = sample.get("step", 0) + 30
            print("HP2_NIS_ACTORS ", parts)
            _capture("busted")
        elif parts >= 2 and arrest_seen and not arrest_motion_captured and sample.get("step", 0) >= arrest_midpoint:
            arrest_motion_captured = true
            _capture("busted-motion")
        elif arrest_seen and parts == 0:
            if not arrest_motion_captured:
                return _fail("arrest_animation_ended_early")
            stage = 46
            _finish(sample, "busted-cleanup")
        return false
    if stage == 8 and sample.get("speed", 0) >= 12 and sample["step"] >= 80:
        _key(KEY_W, false)
        _capture("race-hud")
        _tap(KEY_ESCAPE)
        stage = 9
    if stage == 9 and screen == "Pause Menu":
        _capture("pause")
        pause_selection = sample.get("selected_item", 0)
        stage = 20
        next_input = Time.get_ticks_msec() + 450
    if stage == 20 and Time.get_ticks_msec() >= next_input:
        if returning:
            _tap(KEY_UP)
            stage = 23
            next_input = Time.get_ticks_msec() + 450
        else:
            _tap(KEY_ENTER)
            stage = 21
    if stage == 21 and sample.get("race_state", 0) == 3 and screen != "Pause Menu":
        stage = 22
        _finish(sample)
    if stage == 23 and Time.get_ticks_msec() >= next_input:
        if sample.get("selected_item", 0) == pause_selection:
            return _fail("pause_selection")
        _tap(KEY_ENTER)
        stage = 24
        next_input = Time.get_ticks_msec() + 500
    if stage == 24 and Time.get_ticks_msec() >= next_input:
        _capture("quit-confirm")
        _tap(KEY_LEFT)
        stage = 25
        next_input = Time.get_ticks_msec() + 450
    if stage == 25 and Time.get_ticks_msec() >= next_input:
        _tap(KEY_ENTER)
        stage = 26
    if stage == 26 and (screen == "New Main Menu" or (pursuit and screen == "View Event Tree")):
        stage = 27
        next_input = Time.get_ticks_msec() + 500
    if stage == 27 and Time.get_ticks_msec() >= next_input:
        stage = 28
        _finish(sample, "returned")
    return false

func _run_full_event(sample: Dictionary, screen: String) -> bool:
    var now := Time.get_ticks_msec()
    if stage == 8:
        var state: int = sample.get("race_state", 0)
        if state == 3:
            route.drive(sample)
        elif state == 7:
            route.release()
            if not route.reached_finish():
                return _fail("race_ended_before_route_finish")
            _capture("first-result")
            stage = 40
            next_input = now + 2000
    if stage == 40 and now >= next_input and not screen.is_empty():
        print("RESULT_SCREEN ", screen, " selected=", sample.get("selected_item", 0))
        if screen == "End Game Menu":
            _capture("end-game-menu")
            end_selection = sample.get("selected_item", 0)
            _tap(KEY_UP)
            stage = 41
        else:
            _tap(KEY_ENTER)
        next_input = now + 700
    if stage == 41 and now >= next_input:
        if sample.get("selected_item", 0) == end_selection:
            return _fail("end_game_selection")
        _tap(KEY_ENTER)
        stage = 42
        next_input = now + 700
    if stage == 42 and now >= next_input:
        _capture("results-confirm")
        _tap(KEY_LEFT)
        stage = 43
        next_input = now + 500
    if stage == 43 and now >= next_input:
        _tap(KEY_ENTER)
        stage = 44
    if stage == 44 and screen == "View Event Tree":
        stage = 45
        _finish(sample, "results-return")
    return false

func _dismiss_debug_console() -> void:
    if is_instance_valid(race) and race.telemetry().get("debug_console_open", false):
        _tap(KEY_QUOTELEFT)

func _name_wait(screen: String, editing := false, item := 0) -> void:
    while true:
        var state: Dictionary = race.telemetry()
        if state.get("screen") == screen and state.get("name_editing", false) == editing and (
                item == 0 or state.get("selected_item") == item):
            return
        await process_frame

func _name_key(code: Key, character := 0) -> void:
    var event := InputEventKey.new()
    event.keycode = code
    event.physical_keycode = code
    event.unicode = character
    event.pressed = true
    Input.parse_input_event(event)
    await create_timer(0.055).timeout
    event = event.duplicate()
    event.pressed = false
    Input.parse_input_event(event)
    await create_timer(0.055).timeout

func _name_click(position: Vector2) -> void:
    var event := InputEventMouseButton.new()
    event.button_index = MOUSE_BUTTON_LEFT
    event.position = position
    event.global_position = position
    event.pressed = true
    Input.parse_input_event(event)
    await create_timer(0.1).timeout
    event = event.duplicate()
    event.pressed = false
    Input.parse_input_event(event)
    await create_timer(0.15).timeout

func _test_player_name() -> void:
    await _name_wait("New Main Menu")
    await create_timer(1.2).timeout
    for item in [0x52e039f5, 0x52e039f7]:
        var screen: String = race.telemetry().screen
        while race.telemetry().selected_item != item:
            await _name_key(KEY_DOWN)
            await create_timer(0.18).timeout
        await _name_key(KEY_ENTER)
        await _name_wait("Options" if screen == "New Main Menu" else "NameEntry")
        await create_timer(0.8).timeout
    await _name_wait("NameEntry", false, 0x52e039f3)
    await _name_key(KEY_ENTER)
    await _name_wait("NameEntry", true)
    if name_test == "reload":
        for i in range(7):
            await _name_key(KEY_RIGHT)
        await _name_key(KEY_BACKSPACE)
        await _name_key(KEY_W, 87)
        await _name_key(KEY_ESCAPE)
    else:
        for i in range(7):
            await _name_key(KEY_DELETE)
        if race.telemetry().has("debug_console_open"):
            await _name_key(KEY_QUOTELEFT)
            await _name_key(KEY_W, 87)
            await _name_key(KEY_QUOTELEFT)
        for letter in "WaSd9.!Z":
            await _name_key(letter.to_upper().unicode_at(0), letter.unicode_at(0))
        await _name_key(KEY_LEFT)
        await _name_key(KEY_BACKSPACE)
        await _name_key(KEY_3, 35)
        await _name_key(KEY_DELETE)
        await create_timer(0.35).timeout
        await _capture("player-name")
        var canvas := Vector2(root.size)
        var scale_factor := minf(canvas.x / 640, canvas.y / 480)
        await _name_click((canvas - Vector2(640, 480) * scale_factor) * 0.5 +
            Vector2(58, 326) * scale_factor)
        await _name_key(KEY_ENTER)
    await _name_wait("NameEntry")
    await _name_key(KEY_BACKSPACE)
    await _name_wait("Options")
    var file := OS.get_environment("HP2_PROFILE_DIR").path_join("profile.json")
    var saved: Dictionary = JSON.parse_string(FileAccess.get_file_as_string(file))
    var actual: String = saved.players[0].settings.player_name
    var expected := "WaSd9#W" if name_test == "reload" else "WaSd9#a"
    if actual != expected:
        _fail("name_saved_%s_expected_%s" % [actual, expected])
        return
    print("HP2_MVP_NAME_PASS ", name_test, " ", actual)
    quit()

func _key(key: Key, held: bool) -> void:
    var event := InputEventKey.new()
    event.physical_keycode = key
    event.keycode = key
    event.pressed = held
    Input.parse_input_event(event)

func _tap(key: Key) -> void:
    _key(key, true)
    await create_timer(0.14).timeout
    _key(key, false)

func _capture(name: String) -> bool:
    await RenderingServer.frame_post_draw
    var image := root.get_texture().get_image()
    if image == null:
        _fail("screenshot_" + name)
        return false
    if name == "nfs-intro" and not _has_video_image(image):
        _fail("intro_black_frame")
        return false
    if name == "main-menu" and not _has_menu_highlight(image):
        _fail("main_menu_highlight")
        return false
    if image.save_png(screenshot_dir.path_join(name + ".png")) != OK:
        _fail("screenshot_" + name)
        return false
    return true

func _has_video_image(image: Image) -> bool:
    var non_black := 0
    for y in range(1, 10):
        for x in range(2, 8):
            var pixel := image.get_pixel(int(image.get_width() * x * 0.1), int(image.get_height() * y * 0.1))
            if maxf(pixel.r, maxf(pixel.g, pixel.b)) > 0.08:
                non_black += 1
    return non_black >= 12

func _has_menu_highlight(image: Image) -> bool:
    var scale := minf(image.get_width() / 640.0, image.get_height() / 480.0)
    var origin := (Vector2(image.get_size()) - Vector2(640, 480) * scale) * 0.5
    var warm_pixels := 0
    for y in range(80, 106, 2):
        for x in range(155, 315, 2):
            var point := Vector2i(origin + Vector2(x, y) * scale)
            var pixel := image.get_pixelv(point)
            if pixel.r > 0.5 and pixel.g > 0.15 and pixel.r > pixel.g and pixel.b < pixel.g * 0.6:
                warm_pixels += 1
    return warm_pixels >= 30

func _finish(sample: Dictionary, screenshot := "race") -> void:
    if not await _capture(screenshot):
        return
    print("HP2_MVP_FRONTEND_SMOKE renderer=%s race=%s" %
        [RenderingServer.get_current_rendering_method(), sample])
    quit(0)

func _fail(reason: String) -> bool:
    push_error("HP2_MVP_ASSERT stage=frontend_smoke reason=%s progress=%s state=%s" %
        [reason, stage, {} if not is_instance_valid(race) else race.call("telemetry")])
    quit(1)
    return true
