extends "test_mvp_debug_menu.gd"

const NATIVE_DEBUG_ITEM := 0x52e039fa

func set_key(code: Key, held: bool) -> void:
    var event := InputEventKey.new()
    event.keycode = code
    event.physical_keycode = code
    event.pressed = held
    Input.parse_input_event(event)

func settle(seconds := 0.2) -> void:
    await create_timer(seconds).timeout

func run() -> void:
    deadline = Time.get_ticks_msec() + 80000
    change_scene_to_file("res://native_game.tscn")
    while not is_instance_valid(race):
        await process_frame
        race = root.get_node_or_null("NativeGame/RacePresentation")
    await wait_for(func(state: Dictionary) -> bool:
        return state.get("race_state", 0) == 3 and state.get("step", 0) > 30)
    var enabled := OS.get_cmdline_user_args().has("--debug")
    var caption := race.get_node_or_null("FrontendCanvas/NativeDebugCameraCaption") as Label
    if enabled != is_instance_valid(caption):
        fail("unexpected native debug launch state")
        return
    if enabled:
        await settle(0.5)
        var camera: Camera3D = race.race_scene().get_viewport().get_camera_3d()
        var driving := camera.global_transform
        set_key(KEY_ESCAPE, true)
        await settle()
        if race.telemetry().get("screen") == "Pause Menu":
            fail("Start paused before release")
            return
        set_key(KEY_TAB, true)
        await settle()
        if not caption.visible or camera.global_transform.is_equal_approx(driving):
            fail("native camera did not take focus: caption=%s start=%s select=%s pose=%s original=%s" % [
                caption.visible, Input.is_action_pressed("hp2_pause"), Input.is_action_pressed("hp2_select"),
                camera.global_transform, driving])
            return
        var focused := camera.global_transform
        set_key(KEY_RIGHT, true)
        set_key(KEY_TAB, false)
        set_key(KEY_ESCAPE, false)
        await settle()
        if not camera.global_transform.is_equal_approx(focused):
            fail("camera input was not gated until neutral")
            return
        set_key(KEY_RIGHT, false)
        await settle()
        set_key(KEY_RIGHT, true)
        await settle(0.6)
        if camera.global_transform.is_equal_approx(focused):
            fail("native orbit camera did not move")
            return
        set_key(KEY_RIGHT, false)
        set_key(KEY_ESCAPE, true)
        await settle()
        set_key(KEY_TAB, true)
        await settle()
        set_key(KEY_TAB, false)
        set_key(KEY_ESCAPE, false)
        await settle(0.5)
        if caption.visible or camera.global_position.distance_to(driving.origin) > 1.0:
            fail("driving camera was not restored")
            return
        set_key(KEY_W, true)
        await wait_for(func(state: Dictionary) -> bool: return state.get("speed", 0.0) > 12.0)
        set_key(KEY_W, false)
    await key(KEY_ESCAPE)
    await wait_screen("Pause Menu")
    var reachable := false
    for row in range(10):
        if race.telemetry().get("selected_item") == NATIVE_DEBUG_ITEM:
            reachable = true
            break
        await key(KEY_DOWN)
    if enabled != reachable:
        fail("unexpected native pause Debug row availability")
        return
    if enabled:
        for back in [KEY_BACKSPACE, JOY_BUTTON_B, KEY_ESCAPE, MOUSE_BUTTON_RIGHT]:
            await select(NATIVE_DEBUG_ITEM)
            await key(KEY_ENTER)
            await wait_screen("Debug Menu")
            if back == MOUSE_BUTTON_RIGHT:
                var event := InputEventMouseButton.new()
                event.button_index = MOUSE_BUTTON_RIGHT
                event.position = Vector2(root.size) * 0.5
                event.global_position = event.position
                event.pressed = true
                root.push_input(event, true)
                await settle()
                event = event.duplicate()
                event.pressed = false
                root.push_input(event, true)
            elif back == JOY_BUTTON_B:
                await pad(JOY_BUTTON_B)
            else:
                await key(back)
            await settle()
            if race.telemetry().get("screen") != "Pause Menu" or race.telemetry().get("race_state") != 6:
                fail("Back input %s did not return to the pause menu" % back)
                return
        await key(KEY_ESCAPE)
        await wait_for(func(state: Dictionary) -> bool: return state.get("race_state") == 3)
    print("HP2_MVP_NATIVE_DEBUG_PASS enabled=", enabled)
    quit()
