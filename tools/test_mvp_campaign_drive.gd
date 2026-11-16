extends SceneTree

var race: Node
var deadline := 0
var start := {}
var recovering := false

func _initialize() -> void:
    deadline = Time.get_ticks_msec() + 45000
    call_deferred("change_scene_to_file", "res://native_game.tscn")

func _process(_delta: float) -> bool:
    if Time.get_ticks_msec() > deadline:
        _accelerate(false)
        push_error("Campaign diagnostic timed out: %s" % (race.telemetry() if is_instance_valid(race) else {}))
        quit(1)
        return true
    if not is_instance_valid(race):
        race = root.get_node_or_null("NativeGame/RacePresentation")
        if is_instance_valid(race) and race.telemetry().get("debug_console_open", false):
            var event := InputEventKey.new()
            event.physical_keycode = KEY_QUOTELEFT
            event.pressed = true
            Input.parse_input_event(event)
            event = InputEventKey.new()
            event.physical_keycode = KEY_QUOTELEFT
            Input.parse_input_event(event)
        return false
    var sample: Dictionary = race.telemetry()
    if sample.get("race_state") != 3 or sample.get("placements", 0) <= 0:
        return false
    if start.is_empty():
        start = sample
        _accelerate(true)
    elif sample["generation"] == start["generation"] and sample["step"] > start["step"]:
        var distance: float = start["position"].distance_to(sample["position"])
        if distance >= 5.0 and sample["speed"] >= 5.0:
            _accelerate(false)
            print("HP2_MVP_CAMPAIGN_DRIVE track=%d event=%d distance=%.2f speed=%.2f" %
                [sample["track"], sample["selected_event"], distance, sample["speed"]])
            quit(0)
            return true
        elif not recovering and sample["step"] - start["step"] >= 90 and sample["speed"] < 1.0:
            recovering = true
            start = sample
            _recover()
    return false

func _accelerate(pressed: bool) -> void:
    _key(KEY_UP, pressed)

func _recover() -> void:
    _key(KEY_TAB, true)
    await create_timer(0.2).timeout
    _key(KEY_TAB, false)

func _key(key: Key, pressed: bool) -> void:
    var event := InputEventKey.new()
    event.physical_keycode = key
    event.keycode = key
    event.pressed = pressed
    Input.parse_input_event(event)
