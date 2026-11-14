extends "test_mvp_debug_menu.gd"

func select(id: int) -> void:
    for direction in [KEY_DOWN, KEY_UP]:
        for attempt in range(12):
            var selected: int = race.telemetry().get("selected_item", 0)
            if selected == id:
                return
            await key(direction)
            if race.telemetry().get("selected_item", 0) == selected:
                break
    fail("cannot select %x on %s" % [id, race.telemetry().get("screen")])

func wait_screen(name: String) -> void:
    var timeout := Time.get_ticks_msec() + 12000
    while race.telemetry().get("screen", "") != name:
        if Time.get_ticks_msec() > timeout:
            fail("expected screen %s" % name)
            return
        await process_frame

func visit(row: int, child: String) -> void:
    await select(FIRST + row)
    await key(KEY_ENTER)
    await wait_screen(child)
    await create_timer(0.45).timeout

func back(parent: String) -> void:
    await key(KEY_BACKSPACE)
    await wait_screen(parent)
    await create_timer(0.45).timeout

func round_trip(row: int, child: String, parent: String) -> void:
    await visit(row, child)
    await back(parent)
    print("MENU_BACK_PASS ", child, " -> ", parent)

func run() -> void:
    deadline = Time.get_ticks_msec() + 105000
    change_scene_to_file("res://native_game.tscn")
    while not is_instance_valid(race):
        await process_frame
        race = root.get_node_or_null("NativeGame/RacePresentation")
    if not OS.get_cmdline_user_args().is_empty():
        await pause_menus()
    else:
        await frontend_menus()
    print("MENU_NAVIGATION_PASS")
    quit(0)

func frontend_menus() -> void:
    await wait_screen(TITLE)
    await wait_for(func(state: Dictionary) -> bool: return state.get("video_request") == 2)
    await key(KEY_ENTER)
    await wait_for(func(state: Dictionary) -> bool: return state.get("video_request") == 0)
    await create_timer(4.5).timeout
    await key(KEY_ENTER)
    await wait_screen(MAIN)
    await create_timer(1.0).timeout
    await visit(0, "Cops On Mode Select")
    await round_trip(1, "ChallengeBeACop", "Cops On Mode Select")
    await back(MAIN)
    await visit(1, "Cops Off Mode Select")
    await visit(1, "Challenge")
    await visit(3, "Single Race Setup")
    await key(KEY_ENTER)
    await wait_screen("Select Car")
    await create_timer(1.0).timeout
    for attempt in range(4):
        if race.telemetry().get("screen") == "Select Track":
            break
        await key(KEY_ENTER)
        await create_timer(0.5).timeout
    await wait_screen("Select Track")
    await back("Select Car")
    print("MENU_BACK_PASS Select Track -> Select Car")
    await back("Challenge")
    await back("Cops Off Mode Select")
    await visit(2, "View Event Tree")
    await key(KEY_ENTER)
    await create_timer(1.0).timeout
    await key(KEY_BACKSPACE)
    await wait_screen("View Event Tree")
    await create_timer(0.8).timeout
    await back("Cops Off Mode Select")
    print("MENU_BACK_PASS View Event Tree details -> tree -> Cops Off Mode Select")
    await back(MAIN)
    await visit(2, "Options")
    await visit(0, "Audio Options")
    await round_trip(8, "JukeBox Music", "Audio Options")
    await back("Options")
    await visit(1, "Joystick PreConfig")
    await round_trip(0, "Joystick Config Port1", "Joystick PreConfig")
    await back("Options")
    await visit(2, "Display Options")
    await round_trip(6, "Customize P1 HUD", "Display Options")
    await round_trip(7, "Customize P2 HUD", "Display Options")
    await round_trip(8, "Calibrate display offset", "Display Options")
    await back("Options")
    await round_trip(3, "Phils Load Save", "Options")
    await round_trip(4, "NameEntry", "Options")
    await visit(5, "Credits")
    await round_trip(0, "EACredits", "Credits")
    await round_trip(1, "BBG Credits movie", "Credits")
    await round_trip(2, "Credits FE Movie", "Credits")
    await round_trip(3, "BBG Creation movie", "Credits")
    await round_trip(5, "Port Credits", "Credits")
    await back("Options")
    await back(MAIN)
    await visit(1, "Cops Off Mode Select")
    await visit(2, "View Event Tree")
    await key(KEY_ENTER)
    await create_timer(1.4).timeout
    await key(KEY_ENTER)
    await wait_screen("Select Car")
    await create_timer(1.2).timeout
    for attempt in range(4):
        if race.telemetry().get("screen") != "Select Car":
            break
        await key(KEY_ENTER)
        await create_timer(0.6).timeout
    await wait_screen("Pre Race Blurb")
    await key(KEY_ENTER)
    await wait_screen("Pre Race Stats")
    await back("Pre Race Blurb")
    print("MENU_BACK_PASS Pre Race Stats -> Pre Race Blurb")
    await key(KEY_ENTER)
    await wait_screen("Pre Race Stats")
    await key(KEY_ENTER)
    await wait_for(func(state: Dictionary) -> bool: return state.get("race_state", 0) == 3)

func pause_menus() -> void:
    await wait_for(func(state: Dictionary) -> bool:
        return state.get("race_state", 0) == 3 and state.get("step", 0) > 30)
    await key(KEY_BACKSPACE)
    await wait_screen("Pause Menu")
    await visit(2, "Pause Options")
    await round_trip(0, "Pause audio", "Pause Options")
    await round_trip(1, "Pause display", "Pause Options")
    await back("Pause Menu")
    if OS.get_cmdline_user_args()[0].begins_with("--race-event="):
        await round_trip(4, "Race Objective", "Pause Menu")
        await round_trip(5, "Pause BAC controls", "Pause Menu")
    if OS.get_cmdline_user_args().has("--debug"):
        await round_trip(7, "Debug Menu", "Pause Menu")
    for row in [1, 6]:
        await select(FIRST + row)
        await key(KEY_ENTER)
        await wait_for(func(state: Dictionary) -> bool: return state.get("modal", false))
        await key(KEY_BACKSPACE)
        await wait_for(func(state: Dictionary) -> bool: return not state.get("modal", false))
        if race.telemetry().get("screen") != "Pause Menu":
            fail("Cancel did not return to Pause Menu")
            return
    await select(FIRST + 3)
    await key(KEY_ENTER)
    await wait_for(func(state: Dictionary) -> bool: return state.get("race_state", 0) == 4)
    await key(KEY_BACKSPACE)
    await wait_screen("Pause Menu")
    print("MENU_BACK_PASS Instant Replay -> Pause Menu")
    await key(KEY_BACKSPACE)
    await wait_for(func(state: Dictionary) -> bool: return state.get("race_state", 0) == 3)
