extends SceneTree

const MAIN := "New Main Menu"
const DEBUG := "Debug"
const TITLE := "Title Screen"
const FIRST := 0x52e039f3
const DEMO_STATE := 8

var race: Node
var deadline := Time.get_ticks_msec() + 330000
var mode := OS.get_environment("HP2_MVP_DEMO_TEST")

func _initialize() -> void:
    call_deferred("run")

func _process(_delta: float) -> bool:
    if Time.get_ticks_msec() > deadline:
        fail("timed out")
    return false

func fail(reason: String) -> void:
    push_error("Debug menu smoke: %s, state=%s" % [reason, race.telemetry() if race else {}])
    quit(1)

func wait_for(predicate: Callable) -> Dictionary:
    while true:
        var state: Dictionary = race.telemetry()
        if predicate.call(state):
            return state
        await process_frame
    return {}

func wait_screen(name: String) -> void:
    await wait_for(func(state: Dictionary) -> bool: return state.get("screen", "") == name)

func input_tap(event: InputEvent) -> void:
    event.pressed = true
    Input.parse_input_event(event)
    await create_timer(0.12).timeout
    event = event.duplicate()
    event.pressed = false
    Input.parse_input_event(event)
    await create_timer(0.12).timeout

func key(code: Key) -> void:
    var event := InputEventKey.new()
    event.keycode = code
    event.physical_keycode = code
    await input_tap(event)

func pad(button: JoyButton) -> void:
    var event := InputEventJoypadButton.new()
    event.button_index = button
    await input_tap(event)

func select(id: int) -> void:
    for attempt in range(7):
        var selected: int = race.telemetry().get("selected_item", 0)
        if selected == id:
            return
        await key(KEY_DOWN if selected < id else KEY_UP)
    fail("cannot select %x" % id)

func open_debug() -> void:
    await wait_screen(MAIN)
    await create_timer(1.2).timeout
    var canvas := race.get_node("FrontendCanvas") as Node2D
    await click_at(canvas.to_global(Vector2(200, 258)))
    await wait_screen(DEBUG)
    await create_timer(0.25).timeout

func popup_highlight_present() -> bool:
    return not race.find_children("*", "Node2D", true, false).filter(
        func(node: Node) -> bool: return node.get_meta("flailer_highlight_screen", 0) == 0x74d2dfa2
    ).is_empty()

func capture(label: String) -> void:
    var dir := OS.get_environment("HP2_MVP_SCREENSHOT_DIR")
    if dir.is_empty():
        return
    DirAccess.make_dir_recursive_absolute(dir)
    await RenderingServer.frame_post_draw
    root.get_texture().get_image().save_png(dir.path_join(label + ".png"))

func demo() -> Dictionary:
    return await wait_for(func(state: Dictionary) -> bool:
        return (state.get("race_state") == DEMO_STATE and state.get("step", 0) >= 120 and
            state.get("vehicles", 0) > 0 and state.get("radial_blur_active", false)))

func profile_hashes(directory: String) -> Dictionary:
    var hashes := {}
    if DirAccess.dir_exists_absolute(directory):
        for file in DirAccess.get_files_at(directory):
            if file.ends_with(".json"):
                hashes[file] = FileAccess.get_sha256(directory.path_join(file))
    return hashes

func run() -> void:
    change_scene_to_file("res://native_game.tscn")
    while not is_instance_valid(race):
        await process_frame
        race = root.get_node_or_null("NativeGame/RacePresentation")
    await wait_screen(TITLE)
    if race.telemetry().get("debug_console_open", false):
        fail("debug console opened without input")
        return
    await wait_for(func(state: Dictionary) -> bool: return state.get("video_request", 0) == 2)
    if mode != "idle":
        await key(KEY_ENTER)
    await wait_for(func(state: Dictionary) -> bool: return state.get("video_request", 0) == 0)
    var title_started := Time.get_ticks_msec()
    var profile := str(race.get_parent().profile_directory)
    var hashes_before := profile_hashes(profile)
    if mode.begins_with("idle"):
        var idle_state := await demo()
        var idle_ms := Time.get_ticks_msec() - title_started
        if idle_ms < 30000:
            fail("demo started before the title timeout")
            return
        await capture("idle-demo")
        await wait_screen(TITLE)
        print("HP2_MVP_IDLE_DEMO_PASS ms=", idle_ms, " track=", idle_state.track)
    else:
        await create_timer(4.5).timeout
        await key(KEY_ENTER)
        await wait_screen(MAIN)
        if mode == "release":
            await select(FIRST + 3)
            await key(KEY_DOWN)
            if race.telemetry().get("selected_item") == FIRST + 4:
                fail("unexpected fifth row in Release")
                return
            print("HP2_MVP_RELEASE_MENU_PASS")
            quit()
            return
        await select(FIRST + 3)
        await key(KEY_DOWN)
        if race.telemetry().get("selected_item") != FIRST + 4:
            fail("Debug must appear immediately before Exit")
            return
        await open_debug()
        await capture("debug-options")
        var canvas := race.get_node("FrontendCanvas") as Node2D
        await click_at(canvas.to_global(Vector2(200, 122)))
        await wait_for(func(state: Dictionary) -> bool: return state.get("modal", false))
        await process_frame
        if not popup_highlight_present():
            fail("Continue highlight missing while popup is open")
            return
        await capture("car-viewer-popup")
        await click_at(canvas.to_global(Vector2(365, 288)))
        await wait_for(func(state: Dictionary) -> bool: return not state.get("modal", false))
        for frame in range(5):
            await process_frame
            if popup_highlight_present():
                fail("Continue highlight survived popup dismissal")
                return
        if race.telemetry().get("selected_item") != FIRST + 1:
            fail("popup did not preserve Car Viewer selection")
            return
        print("HP2_MVP_DEBUG_POINTER_POPUP_PASS")
        if mode == "pointer":
            quit()
            return
        if mode != "demo-track":
            await pad(JOY_BUTTON_B)
            await wait_screen(MAIN)
            await open_debug()
            await key(KEY_ENTER)
            await wait_for(func(state: Dictionary) -> bool: return state.get("track_viewer", false) and state.get("step", 0) > 0)
            await click_viewer_main_menu()
            await open_debug()
        if OS.get_cmdline_user_args().has("--debug") and not race.has_node("FrontendCanvas/NativeDebugCameraCaption"):
            fail("returning from Track Viewer lost the native debug launch flag")
            return
        await select(FIRST + 2)
        await key(KEY_ENTER)
        await wait_screen("Demo Scene")
        if mode == "demo-track":
            await key(KEY_BACKSPACE)
            await wait_screen(DEBUG)
            await select(FIRST + 2)
            await key(KEY_ENTER)
            await wait_screen("Demo Scene")
            await select(FIRST)
            await key(KEY_RIGHT)
            await key(KEY_LEFT)
            await key(KEY_RIGHT)
            await create_timer(0.5).timeout
            var bounds: Dictionary = race.telemetry().get("menu_alignment_bounds", {})
            if not bounds.has(0xd1cda594) or not bounds.has(0x9345e4a7):
                await capture("missing-track-arrows")
                fail("Demo Scene did not publish both track arrows")
                return
            var next_arrow: Rect2 = bounds[0x9345e4a7]
            await click_at(canvas.to_global(next_arrow.get_center()))
            if race.race_scene() or race.telemetry().get("screen") != "Demo Scene":
                fail("Selecting a demo track started the race before confirmation")
                return
            await create_timer(0.7).timeout
            await capture("demo-track-selected")
        await select(FIRST + 1)
        await key(KEY_ENTER)
        var manual_state := await demo()
        if mode == "demo-track" and manual_state.track != 12:
            fail("The selected Track 12 did not launch: %s" % manual_state.track)
            return
        await capture("manual-demo")
        await key(KEY_ENTER)
        await wait_screen(MAIN)
        if race.race_scene():
            fail("demo scene survived return to Main Menu")
            return
        print("HP2_MVP_MANUAL_DEMO_PASS track=", manual_state.track)
    if hashes_before != profile_hashes(profile):
        fail("demo or viewer modified the player save")
        return
    if not mode.begins_with("idle"):
        await select(FIRST + 1)
        await key(KEY_ENTER)
        await wait_screen("Cops Off Mode Select")
        await select(FIRST)
        await key(KEY_ENTER)
        await wait_screen("Pre Race Stats")
        await create_timer(0.5).timeout
        await key(KEY_ENTER)
        var regular := await wait_for(func(state: Dictionary) -> bool:
            return state.get("race_state", 0) == 3 and state.get("step", 0) > 0)
        print("HP2_MVP_DEMO_RACE_REENTRY_PASS track=", regular.track)
    quit()

func click_viewer_main_menu() -> void:
    # The viewer toolbar uses display DPI. Its Main Menu follows the Track combo.
    var dpi := DisplayServer.screen_get_dpi()
    var scale := clampf(float(dpi) / 96.0, 1.0, 3.0)
    await click_at(Vector2(375, 17) * scale)

func click_at(position: Vector2) -> void:
    var motion := InputEventMouseMotion.new()
    motion.position = position
    motion.global_position = position
    root.push_input(motion, true)
    await process_frame
    var event := InputEventMouseButton.new()
    event.button_index = MOUSE_BUTTON_LEFT
    event.position = position
    event.global_position = position
    event.pressed = true
    root.push_input(event, true)
    await create_timer(0.12).timeout
    event = event.duplicate()
    event.pressed = false
    root.push_input(event, true)
    await create_timer(0.12).timeout
