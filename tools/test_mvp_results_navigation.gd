extends SceneTree

var race: Node
var last_state: Dictionary

func _initialize() -> void:
    change_scene_to_file.call_deferred("res://native_game.tscn")
    run.call_deferred()

func screen() -> String:
    last_state = race.telemetry()
    return last_state.get("screen", "")

func wait_for_screen(expected: String, timeout := 4.0) -> bool:
    var until := Time.get_ticks_msec() + int(timeout * 1000)
    while Time.get_ticks_msec() < until:
        await process_frame
        if screen() == expected:
            await create_timer(0.3).timeout
            return true
    push_error("Expected %s, got %s (state %s)" % [expected, screen(), last_state])
    quit(1)
    return false

func input_tap(event: InputEvent) -> void:
    event.pressed = true
    Input.parse_input_event(event)
    await create_timer(0.12).timeout
    var release := event.duplicate()
    release.pressed = false
    Input.parse_input_event(release)
    await create_timer(0.12).timeout

func tap(key: Key) -> void:
    var event := InputEventKey.new()
    event.physical_keycode = key
    await input_tap(event)

func click_design(point: Vector2) -> void:
    var canvas := race.get_node("FrontendCanvas") as Node2D
    for down in [true, false]:
        var event := InputEventMouseButton.new()
        event.button_index = MOUSE_BUTTON_LEFT
        event.position = canvas.to_global(point)
        event.global_position = event.position
        event.pressed = down
        root.push_input(event, true)

func select_row(target: int) -> bool:
    for i in range(8):
        screen()
        var current: int = last_state.get("selected_item", 0)
        if current == target:
            return true
        await tap(KEY_DOWN if current < target else KEY_UP)
    push_error("Could not select results row %x" % target)
    quit(1)
    return false

func run() -> void:
    var until := Time.get_ticks_msec() + 20000
    while Time.get_ticks_msec() < until:
        await process_frame
        race = root.get_node_or_null("NativeGame/RacePresentation")
        if race and race.telemetry().get("race_state", 0) == 7:
            break
    if not race or race.telemetry().get("race_state", 0) != 7:
        push_error("Results navigation fixture did not finish the race")
        quit(1)
        return

    var pages: Array[String] = []
    for i in range(12):
        var page := screen()
        if page == "End Game Menu":
            break
        if not page.is_empty() and not pages.has(page):
            pages.append(page)
        await tap(KEY_ENTER)
        await create_timer(0.3).timeout
    if not await wait_for_screen("End Game Menu"):
        return
    if not pages.is_empty():
        await tap(KEY_BACKSPACE)
        if not await wait_for_screen(pages.back()):
            return
        for i in range(pages.size() - 1, 0, -1):
            await tap(KEY_BACKSPACE)
            if not await wait_for_screen(pages[i - 1]):
                return
        for i in range(1, pages.size()):
            await tap(KEY_ENTER)
            if not await wait_for_screen(pages[i]):
                return
        await tap(KEY_ENTER)
        if not await wait_for_screen("End Game Menu"):
            return
    print("RESULTS_PAGE_NAVIGATION_PASS ", pages)

    # Optional replay/restart rows shift the two trailing items.
    var last_row := 0
    for i in range(7):
        screen()
        last_row = maxi(last_row, last_state.get("selected_item", 0))
        await tap(KEY_DOWN)
    if OS.get_environment("HP2_MVP_MOUSE_REPLAY") == "1" and last_row == 0x52e039f5:
        print("RESULTS_REPLAY_MOUSE_UNAVAILABLE: Quick Race results omitted the Replay row")
    elif OS.get_environment("HP2_MVP_MOUSE_REPLAY") == "1":
        if not await select_row(last_row - 2):
            return
        await tap(KEY_ENTER)
        var replay_until := Time.get_ticks_msec() + 4000
        while race.telemetry().get("race_state", 0) != 4:
            if Time.get_ticks_msec() >= replay_until:
                push_error("Results menu did not enter Replay (last row %x, state %s)" % [last_row, race.telemetry()])
                quit(1)
                return
            await process_frame
        click_design(Vector2(385, 438))
        if not await wait_for_screen("End Game Menu"):
            return
        print("RESULTS_REPLAY_MOUSE_PASS")
    for key in [KEY_BACKSPACE, KEY_ESCAPE, JOY_BUTTON_B]:
        if not await select_row(last_row - 1):
            return
        await tap(KEY_ENTER)
        if not await wait_for_screen("Lap Times 1P"):
            return
        if key == JOY_BUTTON_B:
            var event := InputEventJoypadButton.new()
            event.button_index = JOY_BUTTON_B
            await input_tap(event)
        else:
            await tap(key)
        if not await wait_for_screen("End Game Menu"):
            return
    if not await select_row(last_row):
        return
    await tap(KEY_ENTER)
    if not race.telemetry().get("modal", false):
        push_error("Expected exit confirmation after leaving Lap Times")
        quit(1)
        return
    await tap(KEY_BACKSPACE)
    if not await wait_for_screen("End Game Menu"):
        return
    if race.telemetry().get("modal", false):
        push_error("Backspace did not close the results confirmation")
        quit(1)
        return
    print("RESULTS_NAVIGATION_PASS")
    quit(0)
