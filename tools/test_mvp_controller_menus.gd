extends "test_mvp_menu_navigation.gd"

# Use a connected controller (physical or virtual). Its device ID makes these
# events exercise the same Godot device state that the race adapter polls.
var device := -1
var keyboard := OS.get_environment("HP2_MVP_MENU_KEYBOARD") == "1"

func key(code: Key) -> void:
    if keyboard:
        await super.key(code)
        return
    var button: JoyButton = {
        KEY_UP: JOY_BUTTON_DPAD_UP,
        KEY_DOWN: JOY_BUTTON_DPAD_DOWN,
        KEY_LEFT: JOY_BUTTON_DPAD_LEFT,
        KEY_RIGHT: JOY_BUTTON_DPAD_RIGHT,
        KEY_ENTER: JOY_BUTTON_A,
        KEY_BACKSPACE: JOY_BUTTON_B,
        KEY_ESCAPE: JOY_BUTTON_START,
        KEY_SPACE: JOY_BUTTON_X,
        KEY_TAB: JOY_BUTTON_BACK,
    }[code]
    await pad(button)

func pad(button: JoyButton) -> void:
    var event := InputEventJoypadButton.new()
    event.device = device
    event.button_index = button
    await input_tap(event)

func run() -> void:
    var keyboard_actions := {}
    for action in InputMap.get_actions():
        if not String(action).begins_with("hp2_"):
            continue
        for binding in InputMap.action_get_events(action):
            if binding is InputEventKey:
                var code: Key = binding.physical_keycode if binding.physical_keycode else binding.keycode
                if keyboard_actions.has(code):
                    fail("Duplicate key binding in %s and %s" % [keyboard_actions[code], action])
                    return
                keyboard_actions[code] = action
    OS.set_environment("HP2_MVP_MOUSE_REPLAY", "1")
    if not keyboard:
        var until := Time.get_ticks_msec() + 5000
        while Input.get_connected_joypads().is_empty() and Time.get_ticks_msec() < until:
            await process_frame
        if Input.get_connected_joypads().is_empty():
            fail("Connect a gamepad for the controller menu smoke")
            return
        device = Input.get_connected_joypads()[0]
    deadline = Time.get_ticks_msec() + 180000
    change_scene_to_file("res://native_game.tscn")
    while not is_instance_valid(race):
        await process_frame
        race = root.get_node_or_null("NativeGame/RacePresentation")
    if OS.get_cmdline_user_args().is_empty():
        await frontend_menus()
    else:
        await pause_menus()
    print("INPUT_MENU_NAVIGATION_PASS keyboard=", keyboard)
    quit(0)

func frontend_menus() -> void:
    if not keyboard and OS.get_environment("HP2_MVP_CONTROLLER_BOOT") == "1":
        await create_profile()
    await super.frontend_menus()
    await pause_menus()

func create_profile() -> void:
    await wait_screen("BootAutoload")
    for attempt in range(6):
        await create_timer(0.7).timeout
        if race.telemetry().get("creating_profile", false):
            break
        await pad(JOY_BUTTON_A)
    if not race.telemetry().get("creating_profile", false):
        fail("Controller could not open the profile name editor")
        return
    await create_timer(0.6).timeout
    await pad(JOY_BUTTON_A) # a
    await pad(JOY_BUTTON_X) # uppercase
    await pad(JOY_BUTTON_DPAD_RIGHT)
    await pad(JOY_BUTTON_A) # B
    await pad(JOY_BUTTON_DPAD_UP)
    await pad(JOY_BUTTON_DPAD_LEFT)
    await pad(JOY_BUTTON_DPAD_LEFT)
    await pad(JOY_BUTTON_A) # Done
    await wait_screen(TITLE)
    var directory: String = race.get_parent().profile_directory
    for file in DirAccess.get_files_at(directory):
        if file.begins_with("profile-") and file.ends_with(".json"):
            var saved: Variant = JSON.parse_string(FileAccess.get_file_as_string(directory.path_join(file)))
            if saved is Dictionary and saved.players[0].settings.player_name == "aB":
                print("CONTROLLER_PROFILE_PASS")
                return
    fail("Controller name entry did not save aB")

func pause_menus() -> void:
    await super.pause_menus()
    await create_timer(2.0).timeout
    # Exercise the replay-specific inputs in addition to entering and exiting it.
    await key(KEY_ESCAPE)
    await wait_screen("Pause Menu")
    await select(FIRST + 3)
    await key(KEY_ENTER)
    await wait_for(func(state: Dictionary) -> bool:
        return state.get("race_state") == 4 and not state.get("replay_paused", true))
    await key(KEY_ENTER)
    await wait_for(func(state: Dictionary) -> bool: return state.get("replay_paused", false))
    await key(KEY_ENTER)
    await wait_for(func(state: Dictionary) -> bool: return not state.get("replay_paused", true))
    await key(KEY_SPACE)
    await wait_for(func(state: Dictionary) -> bool: return state.get("replay_slow", false))
    await key(KEY_TAB)
    await wait_for(func(state: Dictionary) -> bool: return not state.get("replay_hud", true))
    await key(KEY_TAB)
    await wait_for(func(state: Dictionary) -> bool: return state.get("replay_hud", false))
    await key(KEY_ESCAPE)
    await wait_screen("Pause Menu")
    print("INPUT_REPLAY_PASS keyboard=", keyboard)
