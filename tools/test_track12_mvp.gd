extends SceneTree

var race: Node
var start := {}
var started_at := 0
var deadline := 0
var capturing := false

func _initialize() -> void:
    if RenderingServer.get_current_rendering_method() != "forward_plus":
        _fail("renderer", {})
        return
    deadline = Time.get_ticks_msec() + 60000
    call_deferred("_open")

func _open() -> void:
    change_scene_to_file("res://native_game.tscn")

func _process(_delta: float) -> bool:
    if capturing:
        return false
    if Time.get_ticks_msec() > deadline:
        return _fail("timeout", {} if not is_instance_valid(race) else race.call("telemetry"))
    if not is_instance_valid(race):
        race = root.get_node_or_null("NativeGame/RacePresentation")
        return false
    var sample: Dictionary = race.call("telemetry")
    if sample.get("step", 0) < 2:
        return false
    if start.is_empty():
        if sample["placements"] <= 0 or sample["vehicles"] < 2:
            return _fail("scene_publish", sample)
        start = sample
        started_at = Time.get_ticks_msec()
        _key(KEY_W, true)
        return false
    if Time.get_ticks_msec() - started_at < 1500:
        return false
    var distance: float = sample["position"].distance_to(start["position"])
    if sample["generation"] != start["generation"]:
        return _fail("world_generation", sample)
    if distance < 40 or sample["speed"] < 12 or sample["step"] - start["step"] < 90:
        return false
    capturing = true
    _key(KEY_W, false)
    _capture(sample, distance)
    return false

func _key(key: Key, held: bool) -> void:
    var event := InputEventKey.new()
    event.physical_keycode = key
    event.keycode = key
    event.pressed = held
    Input.parse_input_event(event)

func _capture(sample: Dictionary, distance: float) -> void:
    await RenderingServer.frame_post_draw
    var path := OS.get_environment("HP2_MVP_SCREENSHOT")
    var image := root.get_texture().get_image()
    if path.is_empty() or image == null or image.save_png(path) != OK:
        _fail("screenshot", sample)
        return
    print("HP2_MVP_SMOKE start_step=%s end_step=%s generation=%s placements=%s vehicles=%s distance_m=%.2f speed_kmh=%.1f renderer=%s screenshot=%s" %
        [start["step"], sample["step"], sample["generation"], sample["placements"], sample["vehicles"], distance,
         sample["speed"] * 3.6, RenderingServer.get_current_rendering_method(), path])
    quit(0)

func _fail(stage: String, sample: Dictionary) -> bool:
    push_error("HP2_MVP_ASSERT stage=smoke_%s track=12 state=%s" % [stage, sample])
    quit(1)
    return true
