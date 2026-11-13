extends Node

const RACE_ARGUMENT := "--race-track="

var title_renderer := ""
var title_suffix := ""
var last_window_title := ""

func _ready() -> void:
    _start_window_title()
    var arguments := OS.get_cmdline_user_args()
    var frontend := arguments.is_empty()
    var track := "12"
    if not frontend:
        if arguments.size() != 1 or not arguments[0].begins_with(RACE_ARGUMENT):
            _fail("launch", "unknown", "expected --race-track=<id>")
            return
        track = arguments[0].trim_prefix(RACE_ARGUMENT)
        if not track.is_valid_int() or track.to_int() < 0:
            _fail("launch", track, "expected a nonnegative track ID")
            return
        if track.to_int() != 12:
            _fail("race_entry", track, "Godot scene is unimplemented for this track")
            return
    if not ClassDB.class_exists(&"RacePresentation"):
        _fail("extension", track, "RacePresentation GDExtension class is unavailable")
        return

    var assets := OS.get_environment("HP2_PS2_ASSET_ROOT")
    if assets.is_empty():
        var config := OS.get_environment("HP2_PS2_ASSET_CONFIG")
        if not config.is_empty() and FileAccess.file_exists(config):
            var contents: Variant = JSON.parse_string(FileAccess.get_file_as_string(config))
            if contents is Dictionary:
                var configured: Variant = contents.get("mvp_root", contents.get("root"))
                if configured is String:
                    assets = configured
    if assets.is_empty():
        _fail("asset_source", track, "set HP2_PS2_ASSET_ROOT to the complete original game")
        return

    var profile := OS.get_environment("HP2_PROFILE_DIR")
    if profile.is_empty():
        profile = ProjectSettings.globalize_path("user://profile")
    var race: Node = ClassDB.instantiate(&"RacePresentation")
    race.name = &"RacePresentation"
    add_child(race)
    if frontend:
        race.call(&"start_frontend", assets, profile)
    else:
        race.call(&"start_race", assets, profile)

func _start_window_title() -> void:
    if DisplayServer.get_name() == "headless":
        return
    var driver := RenderingServer.get_current_rendering_driver_name()
    match driver:
        "vulkan":
            title_renderer = "Vulkan"
        "d3d12":
            title_renderer = "Direct3D 12"
        "opengl3", "opengl3_angle":
            title_renderer = "OpenGL"
        "metal":
            title_renderer = "Metal"
        _:
            title_renderer = driver.to_upper() if not driver.is_empty() else "Unknown"
    if OS.get_environment("HP2_MVP_DEBUG_MODE") == "1":
        title_suffix = " | Debug Mode"
    _update_window_title()
    var timer := Timer.new()
    timer.wait_time = 1.0
    timer.ignore_time_scale = true
    timer.process_mode = Node.PROCESS_MODE_ALWAYS
    timer.timeout.connect(_update_window_title)
    add_child(timer)
    timer.start()

func _update_window_title() -> void:
    var caption := "hp2 - (%s) %d FPS%s" % [title_renderer, Engine.get_frames_per_second(), title_suffix]
    if last_window_title != caption:
        DisplayServer.window_set_title(caption, get_window().get_window_id())
        last_window_title = caption

func _unhandled_key_input(event: InputEvent) -> void:
    if event is InputEventKey and event.pressed and event.physical_keycode == KEY_ESCAPE:
        get_tree().quit()

func _fail(stage: String, track: String, reason: String) -> void:
    if ClassDB.class_exists(&"RacePresentation"):
        var host: Node = ClassDB.instantiate(&"RacePresentation")
        add_child(host)
        host.call(&"report_launch_error", stage, track, reason)
        return
    push_error("HP2_MVP_ASSERT stage=%s track=%s reason=%s" % [stage, track, reason])
    get_tree().quit(1)
