extends SceneTree

# Investigation probe: documents current P1 takeover, not split-screen support.
# Run through test_mvp_second_controller.py with two OS-visible virtual pads.
var race: Node
var command_file := OS.get_environment("HP2_SECOND_CONTROLLER_COMMAND")
var startup := OS.get_environment("HP2_SECOND_CONTROLLER_STARTUP") == "1"
var devices: Array[int] = []

func _initialize() -> void:
    run.call_deferred()

func command(value: String) -> void:
    var file := FileAccess.open(command_file, FileAccess.WRITE)
    file.store_string(value)

func expect(label: String, condition: Callable, seconds := 8.0) -> bool:
    var deadline := Time.get_ticks_msec() + int(seconds * 1000)
    while Time.get_ticks_msec() < deadline:
        await process_frame
        if condition.call():
            print("SECOND_CONTROLLER_PASS ", label, " devices=", Input.get_connected_joypads())
            return true
    printerr("SECOND_CONTROLLER_FAIL ", label, " devices=", Input.get_connected_joypads(),
        " state=", race.telemetry() if is_instance_valid(race) else {})
    quit(1)
    return false

func control(throttle: float, brake: float) -> bool:
    var state: Dictionary = race.telemetry()
    return absf(state.get("controls_throttle", -1.0) - throttle) < 0.1 and absf(
        state.get("controls_brake", -1.0) - brake) < 0.1

func run() -> void:
    if command_file.is_empty():
        printerr("Run through test_mvp_second_controller.py")
        quit(1)
        return
    change_scene_to_file("res://native_game.tscn")
    if not await expect("race node", func():
        race = root.get_node_or_null("NativeGame/RacePresentation")
        return is_instance_valid(race), 15.0): return
    if not await expect("single-player race active", func():
        return race.telemetry().get("race_state") == 3 and race.telemetry().has("position"), 75.0): return
    if not startup:
        if not await expect("no controllers at launch", func(): return Input.get_connected_joypads().is_empty()): return
        command("connect")
    if not await expect("two OS-visible controllers", func(): return Input.get_connected_joypads().size() == 2): return
    devices.assign(Input.get_connected_joypads())
    for device in devices:
        print("SECOND_CONTROLLER_DEVICE ", device, " name=", Input.get_joy_name(device), " guid=", Input.get_joy_guid(device))
    command("first_throttle")
    if not await expect("first pad independently detected and drives P1", func():
        return Input.is_joy_button_pressed(devices[0], JOY_BUTTON_A) and not Input.is_joy_button_pressed(devices[1], JOY_BUTTON_A) and control(1.0, 0.0)): return
    command("second_brake")
    if not await expect("second pad independently detected and takes over P1", func():
        return not Input.is_joy_button_pressed(devices[0], JOY_BUTTON_A) and Input.is_joy_button_pressed(devices[1], JOY_BUTTON_X) and control(0.0, 1.0)): return
    command("second_axis")
    if not await expect("second pad analog axis steers P1 independently", func():
        return absf(Input.get_joy_axis(devices[0], JOY_AXIS_LEFT_X)) < 0.1 and Input.get_joy_axis(devices[1], JOY_AXIS_LEFT_X) > 0.9 and absf(race.telemetry().get("controls_steering", 0.0)) > 0.5): return
    command("neutral")
    if not await expect("released controls", func(): return control(0.0, 0.0)): return
    var key := InputEventKey.new()
    key.physical_keycode = KEY_UP
    key.pressed = true
    Input.parse_input_event(key)
    if not await expect("keyboard drives P1 with both pads connected", func(): return control(1.0, 0.0)): return
    key = key.duplicate()
    key.pressed = false
    Input.parse_input_event(key)
    if not await expect("keyboard release", func(): return control(0.0, 0.0)): return
    command("disconnect_first")
    if not await expect("one pad remains", func(): return Input.get_connected_joypads().size() == 1): return
    command("second_throttle")
    if not await expect("remaining pad drives P1", func(): return control(1.0, 0.0)): return
    command("disconnect_all")
    if not await expect("all pads disconnected and input cleared", func(): return Input.get_connected_joypads().is_empty() and control(0.0, 0.0)): return
    command("reconnect")
    if not await expect("reconnected pad neutral", func(): return Input.get_connected_joypads().size() == 1 and control(0.0, 0.0)): return
    print("SECOND_CONTROLLER_RECONNECT old=", devices, " new=", Input.get_connected_joypads())
    command("second_throttle")
    if not await expect("reconnected pad drives P1", func(): return control(1.0, 0.0)): return
    print("SECOND_CONTROLLER_INVESTIGATION_PASS startup=", startup)
    quit(0)
