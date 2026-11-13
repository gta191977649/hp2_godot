extends SceneTree

var race: Node
var deadline := 0
var initial_seconds := -1.0
var support_checked := false
var next_input := 0
var starting_support := Vector2i.ZERO
var helicopter_requested := false
var roadblock_requested := false
var hud_only := false

func _initialize() -> void:
    hud_only = OS.get_environment("HP2_MVP_POLICE_HUD_ONLY") == "1"
    deadline = Time.get_ticks_msec() + 220000
    call_deferred("change_scene_to_file", "res://native_game.tscn")

func _process(_delta: float) -> bool:
    if Time.get_ticks_msec() > deadline:
        push_error("Be the Cop smoke timed out: %s" % (race.telemetry() if is_instance_valid(race) else {}))
        quit(1)
        return true
    if not is_instance_valid(race):
        race = root.get_node_or_null("NativeGame/RacePresentation")
        if is_instance_valid(race) and race.telemetry().get("debug_console_open", false):
            _tap(KEY_QUOTELEFT)
        return false
    var sample: Dictionary = race.telemetry()
    if sample.get("race_state") == 3 and sample.get("police_hud", false):
        var seconds: float = sample["police_seconds"]
        if initial_seconds < 0.0:
            if not sample.get("player_is_pursuing_officer", false):
                push_error("The player is not the donor's pursuing officer")
                quit(1)
                return true
            initial_seconds = seconds
            starting_support = Vector2i(sample["police_roadblocks"], sample["police_helicopters"])
            print("HP2_POLICE_HUD ", sample)
            if hud_only:
                quit(0)
                return true
            _key(KEY_W, true)
        if not support_checked:
            var flags: int = sample["police_support_flags"]
            roadblock_requested = roadblock_requested or (flags & 0x18) != 0 or sample["police_roadblocks"] < starting_support.x
            helicopter_requested = helicopter_requested or (flags & 0x1e0) != 0 or sample["police_helicopters"] < starting_support.y
            if roadblock_requested and helicopter_requested:
                support_checked = true
                _key(KEY_W, false)
                print("HP2_POLICE_SUPPORT ", sample)
            elif Time.get_ticks_msec() >= next_input:
                if sample["police_target_active"]:
                    _tap(KEY_T if roadblock_requested else KEY_R)
                else:
                    _tap(KEY_H)
                next_input = Time.get_ticks_msec() + 1000
    elif sample.get("race_state") == 7 and Time.get_ticks_msec() >= next_input:
        if sample.get("screen") == "Be The Cop Results 1P":
            if initial_seconds <= 0.0 or not support_checked:
                push_error("Missing authored police HUD or support input evidence")
                quit(1)
            else:
                print("HP2_POLICE_RESULTS ", sample)
                quit(0)
            return true
        if not sample.get("screen", "").is_empty():
            _tap(KEY_ENTER)
            next_input = Time.get_ticks_msec() + 800
    return false

func _tap(key: Key) -> void:
    _key(key, true)
    await create_timer(0.2).timeout
    _key(key, false)

func _key(key: Key, pressed: bool) -> void:
    var event := InputEventKey.new()
    event.physical_keycode = key
    event.keycode = key
    event.pressed = pressed
    Input.parse_input_event(event)
