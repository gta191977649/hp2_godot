extends Node

const CONFIG_NAME := "launcher.cfg"
const RACE_ARGUMENT := "--race-track="
const EVENT_ARGUMENT := "--race-event="
const ISO_PROMPT := "Please Drop / Browse Your Game ISO"

var title_renderer := ""
var title_suffix := ""
var last_window_title := ""
var launch_root := ""
var profile_directory := ""
var launch_config := ConfigFile.new()
var launch_action: Callable
var iso_panel: CenterContainer
var iso_error: Label
var iso_dialog: FileDialog
var selecting_iso := false
var graphics := preload("res://graphics_settings.gd").new()

func _ready() -> void:
    var arguments := OS.get_cmdline_user_args()
    var select_iso := arguments.has("--select-iso")
    arguments.erase("--select-iso")
    var native_debug := arguments.has("--debug")
    if native_debug:
        arguments.remove_at(arguments.find("--debug"))
    if arguments.has("--fxaa"):
        arguments.remove_at(arguments.find("--fxaa"))
        ProjectSettings.set_setting("hp2/race_fxaa", true)
    launch_root = OS.get_executable_path().get_base_dir() if OS.has_feature("template") \
        else ProjectSettings.globalize_path("res://").simplify_path()
    var load_error := launch_config.load(launch_root.path_join(CONFIG_NAME))
    if load_error != OK:
        launch_config.clear()
    graphics.configure(launch_config, _save_launcher_config)
    graphics.name = &"GraphicsSettings"
    add_child(graphics)
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

    var profiles := launch_root.path_join("profiles")
    var test_profile := OS.get_environment("HP2_MVP_TEST_PROFILE") == "1"
    var isolated := test_profile or native_debug or track_id >= 0 or event_tree >= 0
    profile_directory = profiles.path_join("diagnostics/%s-%s" % [OS.get_process_id(), Time.get_ticks_usec()]) \
        if isolated else profiles
    launch_action = _launch.bind(track_id, event_tree, event_number, native_debug)
    _resolve_assets(select_iso, track, load_error)

func _resolve_assets(select_iso: bool, track: String, load_error: Error) -> void:
    var reason := ""
    if load_error == ERR_FILE_NOT_FOUND:
        launch_config.set_value("game", "iso_path", "")
        if DisplayServer.get_name() != "headless":
            reason = _write_launcher_config()
    elif load_error != OK:
        reason = "Cannot read launcher.cfg. Choose your ISO to update it."
    if select_iso:
        _show_iso_picker(reason, track)
        return

    var assets := ""
    if not OS.has_feature("template"):
        assets = OS.get_environment("HP2_PS2_ASSET_ROOT")
        if not assets.is_empty():
            _open_configured_assets(assets, track)
            return
    var saved: Variant = launch_config.get_value("game", "iso_path", "")
    if saved is String and not saved.is_empty():
        assets = saved if saved.is_absolute_path() else launch_root.path_join(saved)
        _open_configured_assets(assets, track, true)
        return

    if not OS.has_feature("template"):
        var config := OS.get_environment("HP2_PS2_ASSET_CONFIG")
        if config.is_empty():
            config = str(ProjectSettings.get_setting("hp2/asset_config", ""))
        if not config.is_empty() and FileAccess.file_exists(config):
            var contents: Variant = JSON.parse_string(FileAccess.get_file_as_string(config))
            if contents is Dictionary:
                var configured: Variant = contents.get("mvp_root", contents.get("root"))
                if configured is String:
                    assets = configured
    if not assets.is_empty():
        _open_configured_assets(assets, track)
        return
    _show_iso_picker(reason, track)

func _open_configured_assets(assets: String, track: String, require_iso := false) -> void:
    if require_iso or assets.get_extension().to_lower() == "iso":
        var reason := _try_iso(assets)
        if not reason.is_empty():
            _show_iso_picker(reason, track)
        return
    var reason: String = launch_action.call(assets)
    if not reason.is_empty():
        _fail("asset_source", track, reason)

func _launch(assets: String, track_id: int,
        event_tree: int, event_number: int, native_debug: bool) -> String:
    var race: Node = ClassDB.instantiate(&"RacePresentation")
    race.name = &"RacePresentation"
    race.call(&"set_graphics_menu_settings", graphics.menu_settings)
    add_child(race)
    var reason: String = race.call(&"start_game", assets, profile_directory,
        track_id, event_tree, event_number, native_debug)
    if not reason.is_empty():
        remove_child(race)
        race.free()
    return reason

func _try_iso(path: String) -> String:
    var reason: String = ClassDB.class_call_static(&"RacePresentation", &"validate_game_iso", path)
    if reason.is_empty():
        reason = launch_action.call(path)
    if reason.is_empty():
        return ""
    return "Invalid game ISO: " + reason + "\nChoose the US PS2 Hot Pursuit 2 ISO and try again."

func _write_launcher_config() -> String:
    var path := launch_root.path_join(CONFIG_NAME)
    var error := _save_launcher_config()
    return "Cannot save %s (%s)." % [path, error_string(error)] if error != OK else ""

func _save_launcher_config() -> Error:
    var path := launch_root.path_join(CONFIG_NAME)
    var temporary := path + ".%s-%s.tmp" % [OS.get_process_id(), Time.get_ticks_usec()]
    var error := launch_config.save(temporary)
    if error == OK:
        error = DirAccess.rename_absolute(temporary, path)
    if error != OK:
        if FileAccess.file_exists(temporary):
            DirAccess.remove_absolute(temporary)
    return error

func _show_iso_picker(reason: String, track: String) -> void:
    if DisplayServer.get_name() == "headless":
        _fail("asset_source", track, reason if not reason.is_empty() else "Set the game ISO in launcher.cfg.")
        return
    iso_panel = CenterContainer.new()
    iso_panel.name = &"IsoPicker"
    add_child(iso_panel)
    iso_panel.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
    var column := VBoxContainer.new()
    iso_panel.add_child(column)
    var browse := Button.new()
    browse.text = ISO_PROMPT
    browse.flat = true
    browse.mouse_default_cursor_shape = Control.CURSOR_POINTING_HAND
    browse.add_theme_font_size_override("font_size", 24)
    column.add_child(browse)
    iso_error = Label.new()
    iso_error.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
    iso_error.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
    iso_error.custom_minimum_size.x = 320
    column.add_child(iso_error)
    iso_error.text = reason
    iso_error.visible = not reason.is_empty()
    iso_dialog = FileDialog.new()
    iso_dialog.title = "Choose your Hot Pursuit 2 ISO"
    iso_dialog.access = FileDialog.ACCESS_FILESYSTEM
    iso_dialog.file_mode = FileDialog.FILE_MODE_OPEN_FILE
    iso_dialog.use_native_dialog = true
    iso_dialog.add_filter("*.iso", "PS2 disc images")
    add_child(iso_dialog)
    browse.pressed.connect(func() -> void: iso_dialog.popup_centered_ratio(0.65))
    iso_dialog.file_selected.connect(_select_one_iso, CONNECT_DEFERRED)
    get_window().files_dropped.connect(_on_iso_dropped, CONNECT_DEFERRED)

func _on_iso_dropped(files: PackedStringArray) -> void:
    if selecting_iso:
        return
    if files.size() == 1:
        _select_one_iso(files[0])
    else:
        _show_iso_error("Drop one game ISO at a time.")

func _show_iso_error(reason: String) -> void:
    iso_error.text = reason
    iso_error.show()

func _select_one_iso(path: String) -> void:
    if selecting_iso:
        return
    selecting_iso = true
    var reason := _try_iso(path)
    if not reason.is_empty():
        _show_iso_error(reason)
        selecting_iso = false
        return
    get_window().files_dropped.disconnect(_on_iso_dropped)
    iso_panel.queue_free()
    iso_dialog.queue_free()
    launch_config.set_value("game", "iso_path", path)
    var config_error := _write_launcher_config()
    if not config_error.is_empty():
        var dialog := AcceptDialog.new()
        dialog.title = "Cannot save game ISO path"
        dialog.dialog_text = config_error
        add_child(dialog)
        dialog.confirmed.connect(dialog.queue_free)
        dialog.canceled.connect(dialog.queue_free)
        dialog.popup_centered()

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
