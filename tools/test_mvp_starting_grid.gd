extends SceneTree

var race: Node
var deadline := Time.get_ticks_msec() + 45000
const FLYBY_DURATION_TICKS := 720.0
var start := 0
var previous := 0
var frames: Array[float] = []
var draws: Array[int] = []
var triangles: Array[int] = []
var camera_travel := 0.0
var last_camera := Vector3.ZERO
var confirmed := false
var repeat_confirm_at := 0
var flyby_start_tick := 0.0
var capture_directory := OS.get_environment("HP2_GRID_CAPTURE_DIR")
var next_capture_tick := 120.0

func _initialize() -> void:
    change_scene_to_file.call_deferred("res://native_game.tscn")

func _process(_delta: float) -> bool:
    var now := Time.get_ticks_usec()
    if now / 1000 > deadline:
        push_error("Starting grid timed out: %s" % [race.telemetry() if race else {}])
        quit(1)
        return false
    if not race:
        race = root.get_node_or_null("NativeGame/RacePresentation")
        return false
    var state: Dictionary = race.telemetry()
    if state.get("track") != 13:
        return false
    if confirmed and state.get("screen") == "Pre Race Stats" and now >= repeat_confirm_at:
        repeat_confirm_at = now + 500000
        confirm()
    if state.get("race_state") == 1 and state.get("screen") == "Pre Race Stats" and not confirmed:
        var camera: Camera3D = race.race_scene().get_viewport().get_camera_3d()
        var clock: Vector4 = state.animation_clock
        if start == 0:
            start = now
            flyby_start_tick = clock.z
            last_camera = camera.global_position
            print("REFLECTION_BEGIN ", Engine.get_process_frames())
        else:
            frames.push_back((now - previous) / 1000.0)
            draws.push_back(int(Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME)))
            triangles.push_back(int(Performance.get_monitor(Performance.RENDER_TOTAL_PRIMITIVES_IN_FRAME)))
            camera_travel += last_camera.distance_to(camera.global_position)
            last_camera = camera.global_position
        previous = now
        if not capture_directory.is_empty() and clock.z - flyby_start_tick >= next_capture_tick:
            capture_frame.call_deferred(int(next_capture_tick))
            next_capture_tick += 120.0
        if clock.z - flyby_start_tick < FLYBY_DURATION_TICKS:
            return false
        if frames.size() < 120 or camera_travel < 1.0:
            push_error("Starting grid did not play its track flyby")
            quit(1)
            return false
        var slow := frames.filter(func(ms: float) -> bool: return ms > 1000.0 / 120.0).size()
        frames.sort()
        draws.sort()
        triangles.sort()
        print("REFLECTION_END ", Engine.get_process_frames(), " GRID duration_ms=", (previous - start) / 1000.0,
            " frame_p50/p95/p99=", quantile(0.50), "/", quantile(0.95), "/", quantile(0.99),
            " over_budget=", slow, "/", frames.size(), " draw_p50=", draws[draws.size() / 2],
            " triangles_p50=", triangles[triangles.size() / 2], " camera_travel=", camera_travel)
        confirmed = true
        repeat_confirm_at = now + 500000
        confirm()
    elif state.get("race_state") == 3 and confirmed:
        print("GRID_GO_PASS")
        quit(0)
    return false

func quantile(percent: float) -> float:
    return frames[int((frames.size() - 1) * percent)]

func capture_frame(tick: int) -> void:
    await RenderingServer.frame_post_draw
    root.get_texture().get_image().save_png(capture_directory.path_join("flyby-%03d.png" % tick))

func confirm() -> void:
    var input := InputEventKey.new()
    input.physical_keycode = KEY_ENTER
    input.keycode = KEY_ENTER
    input.pressed = true
    Input.parse_input_event(input)
    await create_timer(0.08).timeout
    input = input.duplicate()
    input.pressed = false
    Input.parse_input_event(input)
