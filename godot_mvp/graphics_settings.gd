extends Node

const SECTION := "graphics"
const RESTORE_DEFAULTS := 9
const AUTO_FIT := 3
const ASPECTS := [Vector2i(16, 9), Vector2i(16, 10), Vector2i(4, 3), Vector2i.ZERO]
const ASPECT_LABELS := ["16:9", "16:10", "4:3", "Auto Fit"]
const MODE_LABELS := ["Full Screen", "Borderless", "Window"]
const MODES := [Window.MODE_EXCLUSIVE_FULLSCREEN, Window.MODE_FULLSCREEN, Window.MODE_WINDOWED]
const RESOLUTION_HEIGHTS := [480, 540, 600, 720, 768, 900, 1080, 1200, 1440, 1600, 1800, 2160, 2880, 4320]
const CAPS := [0, 120, 60, 30]
const AA := ["off", "fxaa", "smaa", "msaa2", "msaa4", "msaa8", "taa", "ssaa", "fsr2", "metalfx"]
const AA_LABELS := ["Off", "FXAA", "SMAA", "MSAA 2x", "MSAA 4x", "MSAA 8x", "TAA",
    "SSAA", "FSR2 Native AA", "MetalFX Temporal"]
const API_LABELS := {"vulkan": "Vulkan", "d3d12": "DirectX 12", "metal": "Metal"}

var _file: ConfigFile
var _save: Callable
var _default_renderer := ""
var _window: Window
var _mode := 2
var _windowed_size := Vector2i.ZERO
var _windowed_position := Vector2i.ZERO
var _aspect := AUTO_FIT
var _fullscreen_height := 0
var _cap := 0
var _vsync := true
var _aa := 1
var _show_fps := false
var _renderer := ""
var _renderers: Array[String] = []
var _aa_count := AA.size() - 1
var _resolution := Vector2i.ZERO
var _render_size := Vector2i.ZERO
var _fps: Label
var _fps_scale := 0.0

func configure(config: ConfigFile, save: Callable) -> void:
    _file = config
    _save = save

func _ready() -> void:
    if DisplayServer.get_name() == "headless":
        return
    _window = get_window()
    _renderers.assign(_rendering_drivers())
    var running := RenderingServer.get_current_rendering_driver_name()
    _default_renderer = _renderers[0] if not _renderers.is_empty() else running
    var preferred: String = str(_file.get_value("rendering", _renderer_key(), running))
    _renderer = preferred if preferred in _renderers else running
    _mode = clampi(int(_file.get_value(SECTION, "window_mode", 2)), 0, MODES.size() - 1)
    _aspect = clampi(int(_file.get_value(SECTION, "aspect", AUTO_FIT)), 0, ASPECTS.size() - 1)
    _fullscreen_height = maxi(0, int(_file.get_value(SECTION, "fullscreen_height", 0)))
    _cap = maxi(CAPS.find(int(_file.get_value(SECTION, "fps_limit", Engine.max_fps))), 0)
    _vsync = bool(_file.get_value(SECTION, "vsync", true))
    _show_fps = bool(_file.get_value(SECTION, "show_fps", false))

    var rd := RenderingServer.get_rendering_device()
    if running == "metal" and rd and rd.has_feature(RenderingDevice.SUPPORTS_METALFX_TEMPORAL):
        _aa_count = AA.size()
    _aa = AA.find(str(_file.get_value(SECTION, "anti_aliasing", "fxaa")))
    if _aa < 0 or _aa >= _aa_count or "--fxaa" in OS.get_cmdline_user_args():
        _aa = 1
    _apply_antialiasing()
    if _file.has_section_key(SECTION, "fps_limit") and "--max-fps" not in OS.get_cmdline_args():
        _apply_cap()
    _apply_vsync()

    _windowed_size = _window.size
    _windowed_position = _window.position
    var args := OS.get_cmdline_args()
    if "--fullscreen" in args:
        _mode = 0 if _window.mode == Window.MODE_EXCLUSIVE_FULLSCREEN else 1
    elif "--windowed" in args or "--maximized" in args:
        _mode = 2
    else:
        _apply_window_mode()
    _window.size_changed.connect(_update_resolution)
    _update_resolution()
    _create_fps_overlay()

func _rendering_drivers() -> Array[String]:
    match OS.get_name():
        "Windows": return ["vulkan", "d3d12"]
        "macOS":
            return ["metal", "vulkan"] if Engine.get_architecture_name() == "arm64" else ["vulkan"]
        "Linux", "FreeBSD", "NetBSD", "OpenBSD", "BSD": return ["vulkan"]
    var driver := RenderingServer.get_current_rendering_driver_name()
    return [driver] if driver in API_LABELS else []

func _renderer_key() -> String:
    match OS.get_name():
        "Windows": return "rendering_device/driver.windows"
        "macOS": return "rendering_device/driver.macos"
        _: return "rendering_device/driver"

func _apply_window_mode() -> void:
    if _mode == 2:
        if _window.mode in [Window.MODE_FULLSCREEN, Window.MODE_EXCLUSIVE_FULLSCREEN]:
            _window.mode = Window.MODE_WINDOWED
            _window.size = _windowed_size
            _window.position = _windowed_position
        _update_resolution()
        _fit_content()
        return
    if _window.mode == Window.MODE_WINDOWED:
        _windowed_size = _window.size
        _windowed_position = _window.position
    _window.mode = MODES[_mode]
    _update_resolution()
    _fit_content()

func _update_resolution() -> void:
    var pixels := DisplayServer.window_get_size(_window.get_window_id())
    if pixels.x < 2 or pixels.y < 2 or _resolution == pixels:
        return
    _resolution = pixels
    _fit_content.call_deferred()

func _native_game_size() -> Vector2i:
    if _aspect == AUTO_FIT:
        return _resolution
    var ratio: Vector2i = ASPECTS[_aspect]
    var factor := minf(float(_resolution.x) / ratio.x, float(_resolution.y) / ratio.y)
    return Vector2i(roundi(factor * ratio.x), roundi(factor * ratio.y))

func _fullscreen_resolutions(native: Vector2i) -> Array[int]:
    var heights: Array[int] = []
    for height in RESOLUTION_HEIGHTS:
        if height < native.y:
            heights.append(height)
    heights.append(0) # Native drawable size for the current monitor and aspect.
    return heights

func _fit_content() -> void:
    var native := _native_game_size()
    if native.x < 2 or native.y < 2:
        return
    _render_size = native
    if _mode != 2 and _fullscreen_height in _fullscreen_resolutions(native):
        if _fullscreen_height > 0:
            _render_size = Vector2i(roundi(float(native.x) * _fullscreen_height / native.y), _fullscreen_height)
    if _aspect == AUTO_FIT and _render_size == _resolution:
        _window.content_scale_mode = Window.CONTENT_SCALE_MODE_DISABLED
        _window.content_scale_size = Vector2i.ZERO
        return
    _window.content_scale_size = _render_size
    _window.content_scale_aspect = Window.CONTENT_SCALE_ASPECT_IGNORE if _aspect == AUTO_FIT else Window.CONTENT_SCALE_ASPECT_KEEP
    _window.content_scale_mode = Window.CONTENT_SCALE_MODE_VIEWPORT if _render_size != native else Window.CONTENT_SCALE_MODE_CANVAS_ITEMS

func _apply_cap() -> void:
    Engine.max_fps = CAPS[_cap]

func _apply_vsync() -> void:
    DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_ENABLED if _vsync else DisplayServer.VSYNC_DISABLED,
        _window.get_window_id())

func _apply_antialiasing() -> void:
    ProjectSettings.set_setting("hp2/graphics_aa", AA[_aa])
    var msaa := _aa - 2 if _aa in [3, 4, 5] else 0
    _window.msaa_2d = msaa

func _create_fps_overlay() -> void:
    var layer := CanvasLayer.new()
    layer.name = &"FpsOverlay"
    layer.layer = 120
    layer.process_mode = Node.PROCESS_MODE_ALWAYS
    add_child(layer)
    _fps = Label.new()
    _fps.name = &"FPS"
    _fps.mouse_filter = Control.MOUSE_FILTER_IGNORE
    _fps.add_theme_color_override("font_color", Color(0.3, 1.0, 0.25))
    _fps.add_theme_color_override("font_outline_color", Color.BLACK)
    layer.add_child(_fps)
    _fps.visible = _show_fps
    _update_fps()
    var timer := Timer.new()
    timer.wait_time = 0.25
    timer.ignore_time_scale = true
    timer.timeout.connect(_update_fps)
    layer.add_child(timer)
    timer.start()

func _update_fps() -> void:
    if not _fps.visible:
        return
    var dpi := DisplayServer.screen_get_dpi(_window.current_screen)
    var scale := float(dpi) / 96.0 if dpi > 0 else 1.0
    if _window.content_scale_mode == Window.CONTENT_SCALE_MODE_VIEWPORT:
        scale *= float(_render_size.y) / maxf(float(_native_game_size().y), 1.0)
    if not is_equal_approx(scale, _fps_scale):
        _fps_scale = scale
        _fps.position = Vector2(roundf(12.0 * scale), roundf(8.0 * scale))
        _fps.add_theme_font_size_override("font_size", roundi(28.0 * scale))
        _fps.add_theme_constant_override("outline_size", maxi(1, roundi(6.0 * scale)))
    _fps.text = "%d FPS" % Engine.get_frames_per_second()

func menu_settings(row: int, direction: int) -> PackedStringArray:
    var message := ""
    if direction != 0 and _window:
        message = _change(row, direction)
    var dimensions := _resolution if _mode == 2 else _render_size
    var resolution_adjustable := _mode != 2 and _fullscreen_resolutions(_native_game_size()).size() > 1
    return PackedStringArray([
        API_LABELS.get(_renderer, _renderer), MODE_LABELS[_mode], "%d x %d" % [dimensions.x, dimensions.y],
        ASPECT_LABELS[_aspect], "No Restriction" if _cap == 0 else "%d FPS" % CAPS[_cap],
        "On" if _vsync else "Off", AA_LABELS[_aa], "On" if _show_fps else "Off", message,
        "1" if resolution_adjustable else "0",
    ])

func _change(row: int, direction: int) -> String:
    var previous := _file.encode_to_text()
    var previous_values := [_mode, _aspect, _cap, _vsync, _aa, _show_fps, _renderer, _fullscreen_height]
    match row:
        0:
            if _renderers.size() <= 1:
                return ""
            var current := maxi(_renderers.find(_renderer), 0)
            _renderer = _renderers[posmod(current + direction, _renderers.size())]
            _file.set_value("rendering", _renderer_key(), _renderer)
        1:
            _mode = posmod(_mode + direction, MODES.size())
            _file.set_value(SECTION, "window_mode", _mode)
        2:
            if _mode == 2:
                return ""
            var sizes := _fullscreen_resolutions(_native_game_size())
            if sizes.size() <= 1:
                return ""
            var current := sizes.find(_fullscreen_height)
            if current < 0:
                current = sizes.size() - 1
            _fullscreen_height = sizes[posmod(current + direction, sizes.size())]
            _file.set_value(SECTION, "fullscreen_height", _fullscreen_height)
        3:
            _aspect = posmod(_aspect + direction, ASPECTS.size())
            _file.set_value(SECTION, "aspect", _aspect)
        4:
            _cap = posmod(_cap + direction, CAPS.size())
            _file.set_value(SECTION, "fps_limit", CAPS[_cap])
        5:
            _vsync = not _vsync
            _file.set_value(SECTION, "vsync", _vsync)
        6:
            _aa = posmod(_aa + direction, _aa_count)
            _file.set_value(SECTION, "anti_aliasing", AA[_aa])
        7:
            _show_fps = not _show_fps
            _file.set_value(SECTION, "show_fps", _show_fps)
        RESTORE_DEFAULTS:
            if _file.has_section(SECTION):
                _file.erase_section(SECTION)
            if _file.has_section_key("rendering", _renderer_key()):
                _file.erase_section_key("rendering", _renderer_key())
        _:
            return ""

    var error: Error = _save.call()
    if error != OK:
        _file.clear()
        _file.parse(previous)
        _mode = previous_values[0]
        _aspect = previous_values[1]
        _cap = previous_values[2]
        _vsync = previous_values[3]
        _aa = previous_values[4]
        _show_fps = previous_values[5]
        _renderer = previous_values[6]
        _fullscreen_height = previous_values[7]
        return "Could not save the graphic settings."
    if row == RESTORE_DEFAULTS:
        _mode = 2
        _aspect = AUTO_FIT
        _fullscreen_height = 0
        _cap = 0
        _vsync = true
        _aa = 1
        _show_fps = false
        _renderer = _default_renderer
        for item in [1, 4, 5, 6, 7]:
            _apply_change(item)
    else:
        _apply_change(row)
    if row == 0 or row == RESTORE_DEFAULTS:
        return "Restart the game to change Render API." if _renderer != RenderingServer.get_current_rendering_driver_name() else ""
    return ""

func _apply_change(row: int) -> void:
    match row:
        1: _apply_window_mode()
        2, 3: _fit_content()
        4: _apply_cap()
        5: _apply_vsync()
        6: _apply_antialiasing()
        7:
            _fps.visible = _show_fps
            _update_fps()

