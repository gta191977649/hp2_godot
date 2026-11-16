extends "test_frontend_mvp.gd"

var checking := false
var device := -1
var layout := OS.get_environment("HP2_MVP_TEST_PAD_LAYOUT").to_int() if OS.has_environment("HP2_MVP_TEST_PAD_LAYOUT") else 2
var test_rumble := OS.get_environment("HP2_MVP_TEST_PAD_RUMBLE") == "1"
var failed_controls: Array[String] = []

func _initialize() -> void:
    OS.set_environment("HP2_MVP_TEST_CONTROLS", "1")
    super._initialize()

func _process(delta: float) -> bool:
    if checking:
        return false
    if (stage == 0 and is_instance_valid(race) and race.telemetry().get("screen") in ["BootAutoload", "Select Profile"]
            and Time.get_ticks_msec() >= next_input):
        _tap(KEY_ENTER)
        next_input = Time.get_ticks_msec() + 1200
    if stage == 3 or stage == 4:
        if Time.get_ticks_msec() >= next_input:
            var selected: int = race.telemetry().get("selected_item", 0)
            if selected == 0x52e039f4:
                _tap(KEY_ENTER)
                stage = 5
            elif selected:
                _tap(KEY_UP if selected > 0x52e039f4 else KEY_DOWN)
                next_input = Time.get_ticks_msec() + 400
        return false
    if stage == 7 and is_instance_valid(race) and race.telemetry().get("race_state") == 3:
        checking = true
        verify_race_controls.call_deferred()
        return false
    return super._process(delta)

func set_button(button: JoyButton, down: bool) -> void:
    var event := InputEventJoypadButton.new()
    event.device = device
    event.button_index = button
    event.pressed = down
    Input.parse_input_event(event)

func set_stick(value: float, axis := JOY_AXIS_LEFT_X) -> void:
    var event := InputEventJoypadMotion.new()
    event.device = device
    event.axis = axis
    event.axis_value = value
    Input.parse_input_event(event)

func check_input(label: String, throttle: float, brake: float, steer: float) -> void:
    var until := Time.get_ticks_msec() + 4000
    while Time.get_ticks_msec() < until:
        var state: Dictionary = race.telemetry()
        if (absf(state.get("controls_throttle", -1.0) - throttle) < 0.12
                and absf(state.get("controls_brake", -1.0) - brake) < 0.12
                and absf(state.get("controls_steering", 10.0) - steer) < 0.12):
            print("CONTROL_LAYOUT_PASS ", label)
            return
        await process_frame
    failed_controls.append(label)
    push_error("CONTROL_LAYOUT_FAIL %s %s" % [label, race.telemetry()])

func expect_state(label: String, check: Callable, wait_ms := 6000) -> bool:
    var until := Time.get_ticks_msec() + wait_ms
    while Time.get_ticks_msec() < until:
        if check.call(race.telemetry()):
            print("CONTROL_LAYOUT_PASS ", label)
            return true
        await process_frame
    return not _fail(label)

func tap_button(button: JoyButton) -> void:
    set_button(button, true)
    await create_timer(0.13).timeout
    set_button(button, false)
    await create_timer(0.13).timeout

func select_row(id: int) -> bool:
    if race.telemetry().get("selected_item") == id:
        return true
    # Native menus may stop at their final row instead of wrapping.
    for direction in [JOY_BUTTON_DPAD_DOWN, JOY_BUTTON_DPAD_UP]:
        for attempt in range(10):
            var previous: int = race.telemetry().get("selected_item", 0)
            await tap_button(direction)
            var current: int = race.telemetry().get("selected_item", 0)
            if current == id:
                return true
            if current == previous:
                break
    return not _fail("could not select menu row %x" % id)

func verify_race_controls() -> void:
    if not await verify_switching():
        return
    if failed_controls.is_empty():
        print("CONTROLLER_DRIVING_LAYOUT_COMPLETE ", char(65 + layout))
    quit(0 if failed_controls.is_empty() else 1)

func verify_vibration_preview() -> bool:
    if not await select_row(0x52e039f5):
        return false
    await tap_button(JOY_BUTTON_DPAD_RIGHT)
    if Input.get_joy_vibration_strength(device) != Vector2.ZERO:
        return not _fail("turning vibration off did not silence both motors")
    set_button(JOY_BUTTON_DPAD_RIGHT, true)
    if not await expect_state("original rumble preview drives the strong motor", func(_s):
        var motors := Input.get_joy_vibration_strength(device)
        return is_zero_approx(motors.x) and motors.y > 0.0 and motors.y < 1.0):
        return false
    set_button(JOY_BUTTON_DPAD_RIGHT, false)
    return await expect_state("original preview decays to zero",
        func(_s): return Input.get_joy_vibration_strength(device) == Vector2.ZERO)

func verify_switching() -> bool:
    var pads := Input.get_connected_joypads()
    if pads.is_empty():
        _fail("Connect a gamepad and seed an isolated profile with Layout C and swapped Up/Down")
        return false
    device = pads[0]
    _key(KEY_DOWN, true)
    _key(KEY_RIGHT, true)
    await check_input("rebound keyboard throttle and steering", 1.0, 0.0, -1.0)
    set_stick(0.05)
    set_stick(0.35, JOY_AXIS_RIGHT_X)
    await create_timer(0.1).timeout
    if race.telemetry().get("controls_device") != "Keyboard":
        _fail("drift or right stick X took control")
        return false
    # Leave the keyboard held while taking control with less than half stick travel.
    set_stick(0.24)
    if not await expect_state("small left stick movement takes over", func(s):
        return (s.controls_device != "Keyboard" and s.controls_throttle < 0.1 and
            s.controls_steering < -0.06 and s.controls_steering > -0.35)):
        return false
    _key(KEY_DOWN, false)
    _key(KEY_RIGHT, false)
    set_stick(0.0)
    set_stick(0.0, JOY_AXIS_RIGHT_X)
    set_stick(-0.65)
    if not await expect_state("left steering", func(s): return s.controls_steering > 0.4):
        return false
    set_stick(0.0)
    var brake := JOY_BUTTON_A if layout >= 2 else JOY_BUTTON_X
    var throttle := JOY_BUTTON_X if layout >= 2 else JOY_BUTTON_A
    set_button(brake, true)
    await check_input("Layout %s face button brakes" % layout, 0.0, 1.0, 0.0)
    set_button(brake, false)
    set_button(throttle, true)
    await check_input("Layout %s face button accelerates" % layout, 1.0, 0.0, 0.0)
    set_stick(0.05)
    await check_input("stick dead zone", 1.0, 0.0, 0.0)
    set_stick(0.8)
    await check_input("analog steering", 1.0, 0.0, -0.88)
    _key(KEY_UP, true)
    await check_input("keyboard brake releases gamepad throttle and steering", 0.0, 1.0, 0.0)
    _key(KEY_UP, false)
    set_stick(0.84)
    await create_timer(0.1).timeout
    if race.telemetry().get("controls_device") != "Keyboard":
        _fail("held stick retook control without crossing threshold")
        return false
    set_button(throttle, false)
    set_stick(0.0)
    set_stick(-0.25, JOY_AXIS_RIGHT_Y)
    if not await expect_state("small right stick movement controls throttle", func(s):
        return (s.controls_device != "Keyboard" and s.controls_throttle > 0.08 and
            s.controls_throttle < 0.4 and s.controls_brake < 0.1)):
        return false
    set_stick(0.7, JOY_AXIS_RIGHT_Y)
    if not await expect_state("right stick controls brake", func(s):
        return s.controls_brake > 0.4 and s.controls_throttle < 0.1):
        return false
    set_stick(0.0, JOY_AXIS_RIGHT_Y)
    await check_input("sticks return to neutral", 0.0, 0.0, 0.0)
    var handbrake := JOY_BUTTON_RIGHT_SHOULDER if layout % 2 == 0 else JOY_BUTTON_B
    var horn := JOY_BUTTON_B if layout % 2 == 0 else JOY_BUTTON_RIGHT_SHOULDER
    set_button(handbrake, true)
    if not await expect_state("handbrake applied", func(s): return s.controls_handbrake > 0.9):
        return false
    set_button(handbrake, false)
    if not await expect_state("handbrake released", func(s): return s.controls_handbrake < 0.1):
        return false
    set_button(horn, true)
    if not await expect_state("horn pressed", func(s): return s.controls_horn):
        return false
    set_button(horn, false)
    if not await expect_state("horn released", func(s): return not s.controls_horn):
        return false
    set_button(JOY_BUTTON_LEFT_SHOULDER, true)
    if not await expect_state("look behind pressed", func(s): return s.controls_look_back):
        return false
    set_button(JOY_BUTTON_LEFT_SHOULDER, false)
    if not await expect_state("look behind released", func(s): return not s.controls_look_back):
        return false
    var view: int = race.telemetry().controls_camera_view
    await tap_button(JOY_BUTTON_Y)
    if not await expect_state("cycle camera", func(s): return s.controls_camera_view != view):
        return false
    var hud: int = race.telemetry().controls_hud_profile
    await tap_button(JOY_BUTTON_DPAD_UP)
    if not await expect_state("cycle HUD", func(s): return s.controls_hud_profile != hud):
        return false
    await tap_button(JOY_BUTTON_START)
    if not await expect_state("Start opens pause menu", func(s): return s.screen == "Pause Menu"):
        return false
    if not await select_row(0x52e039f5):
        return false
    await tap_button(JOY_BUTTON_A)
    if not await expect_state("open pause options", func(s): return s.screen == "Pause Options"):
        return false
    if test_rumble and not await verify_vibration_preview():
        return false
    if not await select_row(0x52e039f8):
        return false
    await tap_button(JOY_BUTTON_DPAD_RIGHT)
    await tap_button(JOY_BUTTON_B)
    if not await expect_state("return to pause menu", func(s): return s.screen == "Pause Menu"):
        return false
    if not await select_row(0x52e039f3):
        return false
    await tap_button(JOY_BUTTON_A)
    if not await expect_state("resume in manual transmission", func(s): return s.race_state == 3 and not s.controls_automatic):
        return false
    var gear: int = race.telemetry().controls_gear_request
    set_stick(0.8, JOY_AXIS_TRIGGER_RIGHT)
    if not await expect_state("R2 shifts up", func(s): return s.controls_gear_request == gear + 1):
        return false
    set_stick(0.0, JOY_AXIS_TRIGGER_RIGHT)
    set_stick(0.8, JOY_AXIS_TRIGGER_LEFT)
    if not await expect_state("L2 shifts down", func(s): return s.controls_gear_request == gear):
        return false
    set_stick(0.0, JOY_AXIS_TRIGGER_LEFT)
    var recovery_started: int = race.telemetry().controls_recovery_started
    await tap_button(JOY_BUTTON_BACK)
    if not await expect_state("Select resets the car", func(s): return s.controls_recovery_started > recovery_started):
        return false
    if test_rumble:
        set_button(throttle, true)
        set_stick(0.7)
        if not await expect_state("driving triggers original rumble", func(_s):
            return Input.get_joy_vibration_strength(device) != Vector2.ZERO, 20000):
            return false
        _key(KEY_DOWN, true)
        if not await expect_state("keyboard takeover stops the motors",
            func(_s): return Input.get_joy_vibration_strength(device) == Vector2.ZERO):
            return false
        _key(KEY_DOWN, false)
        set_button(throttle, false)
        set_stick(0.0)
    return true
