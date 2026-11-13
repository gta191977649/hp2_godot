extends SceneTree

var race: Node
var deadline := 0
var stage := 0
var next_input := 0
var screenshot_dir := ""
var championship := false
var title_captured := false

func _initialize() -> void:
    deadline = Time.get_ticks_msec() + 170000
    screenshot_dir = OS.get_environment("HP2_MVP_SCREENSHOT_DIR")
    championship = OS.get_environment("HP2_MVP_CHAMPIONSHIP") == "1"
    call_deferred("_open")

func _open() -> void:
    change_scene_to_file("res://native_game.tscn")

func _process(_delta: float) -> bool:
    if Time.get_ticks_msec() > deadline:
        return _fail("timeout")
    if not is_instance_valid(race):
        race = root.get_node_or_null("NativeGame/RacePresentation")
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
        _tap(KEY_DOWN)
        stage = 4
        next_input = Time.get_ticks_msec() + 250
    if stage == 4 and Time.get_ticks_msec() >= next_input:
        _tap(KEY_ENTER)
        stage = 5
    if stage == 5 and screen == "Cops Off Mode Select":
        stage = 6
        next_input = Time.get_ticks_msec() + 1000
    if stage == 6 and Time.get_ticks_msec() >= next_input:
        _capture("mode-select")
        if championship:
            _tap(KEY_DOWN)
            stage = 12
            next_input = Time.get_ticks_msec() + 300
        else:
            _tap(KEY_ENTER)
            stage = 7
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
    if stage == 19 and sample.get("step", 0) >= 1:
        stage = 7
    if stage == 7 and sample.get("step", 0) >= 1:
        _key(KEY_W, true)
        stage = 8
    if stage == 8 and sample.get("speed", 0) >= 12 and sample["step"] >= 80:
        _key(KEY_W, false)
        stage = 9
        _finish(sample)
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

func _finish(sample: Dictionary) -> void:
    if not await _capture("race"):
        return
    print("HP2_MVP_FRONTEND_SMOKE renderer=%s race=%s" %
        [RenderingServer.get_current_rendering_method(), sample])
    quit(0)

func _fail(reason: String) -> bool:
    push_error("HP2_MVP_ASSERT stage=frontend_smoke reason=%s progress=%s state=%s" %
        [reason, stage, {} if not is_instance_valid(race) else race.call("telemetry")])
    quit(1)
    return true
