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

func _initialize() -> void:
    deadline = Time.get_ticks_msec() + 170000
    screenshot_dir = OS.get_environment("HP2_MVP_SCREENSHOT_DIR")
    championship = OS.get_environment("HP2_MVP_CHAMPIONSHIP") == "1"
    pursuit = OS.get_environment("HP2_MVP_PURSUIT") == "1"
    returning = OS.get_environment("HP2_MVP_RETURN") == "1"
    var course_path := OS.get_environment("HP2_ROUTE_CAPTURE")
    if not course_path.is_empty():
        route = RouteDriver.new()
        if not pursuit or not route.load_course(course_path, _key):
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
        if is_instance_valid(race) and OS.get_environment("HP2_MVP_DEBUG_CONSOLE") == "1":
            _tap(KEY_F8)
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
    if stage == 15 and Time.get_ticks_msec() >= next_input:
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
        if route == null:
            _key(KEY_W, true)
        stage = 8
    if route != null and stage >= 8:
        return _run_full_event(sample, screen)
    if stage == 8 and sample.get("speed", 0) >= 12 and sample["step"] >= 80:
        _key(KEY_W, false)
        _capture("race-hud")
        _tap(KEY_P)
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
