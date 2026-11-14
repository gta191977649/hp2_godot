extends Node

const RACE_ARGUMENT := "--race-track="
const EVENT_ARGUMENT := "--race-event="

var title_renderer := ""
var title_suffix := ""
var last_window_title := ""

func _ready() -> void:
    var arguments := OS.get_cmdline_user_args()
    var native_debug := arguments.has("--debug")
    if native_debug:
        arguments.remove_at(arguments.find("--debug"))
    _start_window_title(native_debug)
    var frontend := arguments.is_empty()
    var track := "unknown"
    var track_id := -1
    var event_tree := -1
    var event_number := 0
    if not frontend:
        if arguments.size() != 1:
            _fail("launch", "unknown", "expected --race-track=<id> or --race-event=<tree>:<id>")
            return
        if arguments[0].begins_with(RACE_ARGUMENT):
            track = arguments[0].trim_prefix(RACE_ARGUMENT)
            track_id = _parse_id(track)
            if track_id < 0:
                _fail("launch", track, "expected a nonnegative 32-bit track ID")
                return
        elif arguments[0].begins_with(EVENT_ARGUMENT):
            var event_spec := arguments[0].trim_prefix(EVENT_ARGUMENT)
            var fields := event_spec.split(":")
            if fields.size() == 2:
                event_tree = ["championship", "ultimate-racer"].find(fields[0])
                event_number = _parse_id(fields[1])
            if event_tree < 0 or event_number <= 0:
                _fail("launch", "unknown", "expected --race-event=championship:<id> or --race-event=ultimate-racer:<id>")
                return
        else:
            _fail("launch", "unknown", "expected --race-track=<id> or --race-event=<tree>:<id>")
            return
    if not ClassDB.class_exists(&"RacePresentation"):
        _fail("extension", track, "RacePresentation GDExtension class is unavailable")
        return

    var assets := OS.get_environment("HP2_PS2_ASSET_ROOT")
    if assets.is_empty():
        var config := OS.get_environment("HP2_PS2_ASSET_CONFIG")
        if config.is_empty():
            config = str(ProjectSettings.get_setting("hp2/asset_config", ""))
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
        if native_debug:
            profile = ProjectSettings.globalize_path("user://native-debug/%s-%s" % [
                OS.get_process_id(), Time.get_ticks_usec()])
        elif event_tree >= 0:
            profile = ProjectSettings.globalize_path("user://diagnostic-events/%s-%s-%s-%s" % [
                event_tree, event_number, OS.get_process_id(), Time.get_ticks_usec()])
        else:
            profile = ProjectSettings.globalize_path("user://profile")
    var race: Node = ClassDB.instantiate(&"RacePresentation")
    race.name = &"RacePresentation"
    add_child(race)
    if frontend:
        race.call(&"start_frontend", assets, profile, native_debug)
    elif event_tree >= 0:
        race.call(&"start_event_race", assets, profile, event_tree, event_number, native_debug)
    else:
        race.call(&"start_race", assets, profile, track_id, native_debug)

func _parse_id(value: String) -> int:
    if value.length() <= 10 and value.is_valid_int():
        var parsed := value.to_int()
        if parsed >= 0 and parsed <= 2147483647:
            return parsed
    return -1

func _start_window_title(native_debug: bool) -> void:
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
    var debug_mode := OS.get_environment("HP2_MVP_DEBUG_MODE")
    if native_debug or debug_mode == "1" or (debug_mode.is_empty() and ProjectSettings.get_setting("hp2/debug_mode", false)):
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

func _fail(stage: String, track: String, reason: String) -> void:
    if ClassDB.class_exists(&"RacePresentation"):
        var host: Node = ClassDB.instantiate(&"RacePresentation")
        add_child(host)
        host.call(&"report_launch_error", stage, track, reason)
        return
    push_error("HP2_MVP_ASSERT stage=%s track=%s reason=%s" % [stage, track, reason])
    get_tree().quit(1)
