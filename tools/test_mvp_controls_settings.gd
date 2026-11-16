extends "test_mvp_menu_navigation.gd"

var verify_reload := OS.get_environment("HP2_MVP_CONTROLS_RELOAD") == "1"
var device := -1
var audio_mask_height := 0.0
var audio_row_pitch := 0.0

func expect(condition: bool, description: String) -> void:
    if not condition:
        fail(description)
        assert(condition, description)

func binding(action: StringName) -> Key:
    for event in InputMap.action_get_events(action):
        if event is InputEventKey:
            return event.physical_keycode
    return KEY_NONE

func config() -> Dictionary:
    var saved := ConfigFile.new()
    var path: String = race.get_parent().launch_root.path_join("launcher.cfg")
    expect(saved.load(path) == OK, "Could not load saved input settings")
    return saved.get_value("controls", "overrides", {})

func profile() -> Dictionary:
    var directory: String = race.get_parent().profile_directory
    for file in DirAccess.get_files_at(directory):
        if file == "profile.json" or (file.begins_with("profile-") and file.ends_with(".json") and not file.ends_with(".backup.json")):
            return JSON.parse_string(FileAccess.get_file_as_string(directory.path_join(file))) as Dictionary
    return {}

func pad(button: JoyButton) -> void:
    var event := InputEventJoypadButton.new()
    event.button_index = button
    event.device = device
    await input_tap(event)

func object_hash(label: String) -> int:
    var result := 0xffffffff
    for byte in label.to_ascii_buffer():
        result = (result * 33 + byte) & 0xffffffff
    return result

func row_id(index: int) -> int:
    return object_hash("HP2_CONTROLS_ROW_%s" % index)

func click_row(index: int) -> void:
    var bounds: Dictionary = race.telemetry().get("menu_alignment_bounds", {})
    var row: Rect2 = bounds[row_id(index)]
    var canvas := race.get_node("FrontendCanvas") as Node2D
    await click_at(canvas.to_global(row.get_center()))

func check_layout() -> void:
    await wait_for(func(_state: Dictionary) -> bool: return race.telemetry().get("video_playing", false))
    await create_timer(0.8).timeout
    await process_frame
    var state: Dictionary = race.telemetry()
    check_movie_backdrop(state.get("screen", ""))
    var panel := race.get_node("FrontendCanvas/FrontendVideoPanel") as Control
    var bounds: Dictionary = state.get("menu_alignment_bounds", {})
    if state.get("screen") == "Controls":
        expect(not bounds.has(object_hash("HP2_CONTROLS_VALUE_0")) and
            not bounds.has(object_hash("HP2_CONTROLS_VALUE_1")), "Controls rows still display submenu markers")
    var title: Rect2 = bounds[object_hash("HP2_CONTROLS_TITLE")]
    var device_label := object_hash("HP2_CONTROLS_DEVICE_LABEL")
    var is_hub: bool = state.get("screen") == "Controls"
    var is_keyboard: bool = state.get("screen") == "Keyboard Settings"
    expect(bounds.has(device_label) == is_hub and
        bounds.has(object_hash("HP2_CONTROLS_DEVICE_VALUE")) == is_hub, "Unexpected Current Input Device row")
    var first_item := row_id(25 if is_keyboard else 0)
    var first_row: Rect2 = bounds[device_label if is_hub else first_item]
    for label in ["HP2_CONTROLS_HINT", "HP2_CONTROLS_SCROLL", "HP2_CONTROLS_GROUP_NOTE"]:
        expect(not bounds.has(object_hash(label)), "Persistent Controls help text is visible")
    expect(not bounds.has(0xaa5d7d3f), "Fixed Display Options stripes leaked into Controls")
    var footer: Rect2 = bounds[0x5e11b599]
    var description: Rect2 = bounds[object_hash("HP2_CONTROLS_DESCRIPTION")]
    expect(footer.grow(1.0).encloses(description), "The native description is outside its footer")
    if is_hub:
        var device_shade: Rect2 = bounds[object_hash("HP2_CONTROLS_MASK_DEVICE")]
        expect(device_shade.grow(2.0).encloses(first_row), "Device row shade is misaligned")
        for index in range(2):
            var key := object_hash("HP2_CONTROLS_MASK_%s" % index)
            if state["selected_item"] == row_id(index):
                expect(not bounds.has(key), "The selected row did not clear its mask")
            else:
                var shade: Rect2 = bounds[key]
                var label: Rect2 = bounds[row_id(index)]
                expect(shade.grow(2.0).encloses(label), "The authored shade does not follow the row")
                expect(absf(shade.size.y - audio_mask_height) <= 1.0 and
                    absf(shade.position.y - device_shade.position.y - (index + 1) * audio_row_pitch) <= 1.0,
                    "Controls row padding differs from HP2 Audio Options")
    var selected: Rect2 = bounds[state["selected_item"]]
    var upper: Rect2 = bounds[0xc671680c]
    var lower: Rect2 = bounds[0x32c91dae]
    var tab: bool = is_keyboard and state["selected_item"] in [row_id(25), row_id(26), row_id(27)]
    expect(absf(title.position.x - first_row.position.x) <= 2.0, "Controls heading is misaligned")
    if not tab:
        expect(absf(selected.position.x - first_row.position.x) <= 2.0, "Controls rows have uneven left alignment")
    var movie := panel.get_rect()
    for rail in [upper, lower]:
        if tab:
            expect(rail.size.x < movie.size.x / 2 and rail.position.x < selected.position.x and rail.end.x >= selected.end.x,
                "Tab selection rail extends beyond its tab")
        else:
            expect(absf(rail.position.x - movie.position.x) <= 1.0 and absf(rail.end.x - movie.end.x) <= 1.0,
                "Row selection rail does not span the panel")
    expect(panel.get_rect().encloses(first_row) and panel.get_rect().encloses(selected),
        "Controls text spills out of the video panel")
    expect(upper.get_center().y < selected.position.y and lower.get_center().y >= selected.end.y,
        "The selection lines do not bracket the selected row")

func check_scroll_arrows(up: bool, down: bool) -> void:
    var bounds: Dictionary = race.telemetry().get("menu_alignment_bounds", {})
    expect(bounds.has(0xedf2e4ee) == up and bounds.has(0x20d73d01) == down,
        "HP2 scroll arrows do not match the binding list")

func check_description_slide(description_id: int) -> void:
    var bounds: Dictionary = race.telemetry().get("menu_alignment_bounds", {})
    var rest: Rect2 = bounds[description_id]
    var moved := false
    var animation_end := Time.get_ticks_msec() + 1600
    key(KEY_DOWN)
    while Time.get_ticks_msec() < animation_end:
        await process_frame
        bounds = race.telemetry().get("menu_alignment_bounds", {})
        if not bounds.has(description_id):
            continue
        var desc: Rect2 = bounds[description_id]
        moved = moved or desc.position.x > rest.position.x + 8.0
        if moved and absf(desc.position.x - rest.position.x) <= 1.0:
            return
    expect(false, "The native footer description did not slide in and settle")

func run() -> void:
    OS.set_environment("HP2_MVP_TEST_CONTROLS", "1")
    OS.set_environment("HP2_MVP_TEST_MENU_ALIGNMENT", "1")
    deadline = Time.get_ticks_msec() + 100000
    change_scene_to_file("res://native_game.tscn")
    while not is_instance_valid(race):
        await process_frame
        race = root.get_node_or_null("NativeGame/RacePresentation")
    if not Input.get_connected_joypads().is_empty():
        device = Input.get_connected_joypads()[0]
    await wait_screen(TITLE)
    await wait_for(func(state: Dictionary) -> bool: return state.get("video_request") == 2)
    await key(KEY_ENTER)
    await wait_for(func(state: Dictionary) -> bool: return state.get("video_request") == 0)
    await create_timer(4.5).timeout
    if race.telemetry().get("screen") == TITLE:
        await key(KEY_ENTER)
    await wait_screen(MAIN)
    await create_timer(0.8).timeout
    await visit(2, "Options")
    if OS.get_environment("HP2_MVP_TEST_LANGUAGES_ONLY") == "1":
        await check_language_options()
        quit(0)
        return
    await wait_for(func(state: Dictionary) -> bool: return state.get("video_playing", false))
    check_movie_backdrop("Options")
    await capture("options-video-panel")
    await key(KEY_DELETE)
    expect(race.telemetry().get("controls_help_open"), "Delete did not open frontend help")
    if device >= 0:
        await pad(JOY_BUTTON_Y)
    else:
        await key(KEY_DELETE)
    expect(not race.telemetry().get("controls_help_open"), "Help did not close")
    await visit(0, "Audio Options")
    await create_timer(0.85).timeout
    var audio: Dictionary = race.telemetry().get("menu_alignment_bounds", {})
    var audio_second: Rect2 = audio[0x708a0bf1]
    var audio_third: Rect2 = audio[0x72df3492]
    audio_mask_height = audio_second.size.y
    audio_row_pitch = audio_third.position.y - audio_second.position.y
    expect(audio_row_pitch > audio_mask_height, "The original Audio options have no row clearance")
    await capture("audio-reference")
    await check_description_slide(0x75980cf0)
    await back("Options")
    await visit(1, "Controls")
    await check_layout()
    expect(race.telemetry().get("controls_device") == "Keyboard", "Expected the keyboard status")
    if device >= 0:
        var selection: int = race.telemetry().get("selected_item")
        await pad(JOY_BUTTON_LEFT_SHOULDER)
        expect(race.telemetry().get("controls_device") == Input.get_joy_name(device),
            "The device status did not use Godot's controller name")
        expect(race.telemetry().get("selected_item") == selection, "Device change moved menu focus")
        await capture("controller-device-name")
        var command_path := OS.get_environment("HP2_MVP_CONTROLLER_COMMAND")
        if not command_path.is_empty():
            FileAccess.open(command_path, FileAccess.WRITE).store_string("disconnect")
            await wait_for(func(state: Dictionary) -> bool: return state.get("controls_device") == "Keyboard")
            expect(Input.get_connected_joypads().is_empty(), "The test controller is still connected")
            expect(race.telemetry().get("selected_item") == selection, "Disconnect moved menu focus")
            FileAccess.open(command_path, FileAccess.WRITE).store_string("reconnect")
            await wait_for(func(_state: Dictionary) -> bool: return not Input.get_connected_joypads().is_empty())
            device = Input.get_connected_joypads()[0]
        await pad(JOY_BUTTON_LEFT_SHOULDER)
        await key(KEY_F9)
        expect(race.telemetry().get("controls_device") == "Keyboard", "Keyboard input did not restore its status")
        expect(race.telemetry().get("selected_item") == selection, "Keyboard input moved menu focus")
    await capture("controls-hub")
    await check_description_slide(object_hash("HP2_CONTROLS_DESCRIPTION"))
    await check_layout()
    await capture("controls-hub-controller-selected")
    await key(KEY_UP)
    await key(KEY_ENTER)
    await wait_screen("Keyboard Settings")
    await check_layout()
    expect(race.telemetry().get("selected_item") == row_id(25), "Keyboard Settings did not start on Frontend")
    expect(not InputMap.has_action(&"hp2_orbit_camera") and not InputMap.has_action(&"hp2_look_forward"),
        "The obsolete mouse orbit shortcuts still exist")
    await capture("keyboard-frontend")
    if verify_reload:
        expect(binding(&"hp2_accelerate") == KEY_DOWN and binding(&"hp2_brake") == KEY_UP,
            "Swapped keys were not restored from launcher.cfg")
        var restore := object_hash("HP2_CONTROLS_ROW_24")
        for row in range(30):
            if race.telemetry().get("selected_item") == restore:
                break
            await key(KEY_DOWN)
        expect(race.telemetry().get("selected_item") == restore, "Restore Defaults is missing from Keyboard Settings")
        await key(KEY_ENTER)
        await wait_for(func(state: Dictionary) -> bool: return state.get("modal", false))
        expect(binding(&"hp2_menu_left") == KEY_F8, "Frontend binding did not reload")
        await key(KEY_F8)
        await key(KEY_ENTER)
        expect(binding(&"hp2_accelerate") == KEY_UP and binding(&"hp2_brake") == KEY_DOWN,
            "Restore Defaults did not restore Up/Down")
        expect(binding(&"hp2_zone_zoom") == KEY_Q and binding(&"hp2_zone_freeze") == KEY_E and
            binding(&"hp2_handbrake") == KEY_SPACE and binding(&"hp2_roadblock") == KEY_J and
            binding(&"hp2_menu_left") == KEY_LEFT, "Restore Defaults did not restore all three tabs")
        expect(config().get("steering_dead_zone") == 10 and config().get("steering_sensitivity") == 110,
            "Keyboard reset changed controller tuning")
    else:
        await key(KEY_DOWN)
        expect(race.telemetry().get("selected_item") == row_id(18), "Down did not focus the first Frontend binding")
        await key(KEY_UP)
        expect(race.telemetry().get("selected_item") == row_id(25), "Up did not return to the tab")
        if device >= 0:
            await pad(JOY_BUTTON_DPAD_RIGHT)
            expect(race.telemetry().get("selected_item") == row_id(26), "Controller did not switch to Race Mode")
        else:
            await key(KEY_RIGHT)
        await check_layout()
        check_scroll_arrows(false, true)
        await capture("keyboard-race")
        for row in range(6):
            await key(KEY_DOWN)
        await check_smooth_row(KEY_DOWN, object_hash("HP2_CONTROLS_ROW_3"))
        check_scroll_arrows(true, true)
        var scrolled_bounds: Dictionary = race.telemetry().get("menu_alignment_bounds", {})
        var scrolled_mask: Rect2 = scrolled_bounds[object_hash("HP2_CONTROLS_MASK_3")]
        var scrolled_label: Rect2 = scrolled_bounds[row_id(3)]
        expect(scrolled_mask.grow(1.0).encloses(scrolled_label), "A row moved away from its mask during scrolling")
        await check_layout()
        await capture("keyboard-race-scrolled")
        for row in range(5):
            await key(KEY_UP)
        await check_smooth_row(KEY_UP, object_hash("HP2_CONTROLS_ROW_3"))
        var defaults := {
            &"hp2_accelerate": KEY_UP, &"hp2_brake": KEY_DOWN,
            &"hp2_steer_left": KEY_LEFT, &"hp2_steer_right": KEY_RIGHT,
            &"hp2_handbrake": KEY_SPACE, &"hp2_r2": KEY_A, &"hp2_l2": KEY_Z,
            &"hp2_horn": KEY_H, &"hp2_camera": KEY_C, &"hp2_l1": KEY_B,
            &"hp2_select": KEY_R, &"hp2_pause": KEY_ESCAPE,
            &"hp2_roadblock": KEY_J, &"hp2_helicopter": KEY_N, &"hp2_nitro": KEY_SHIFT,
            &"hp2_zone_zoom": KEY_Q, &"hp2_zone_freeze": KEY_E, &"hp2_cycle_hud": KEY_TAB,
            &"hp2_menu_confirm": KEY_ENTER, &"hp2_menu_back": KEY_BACKSPACE,
            &"hp2_menu_up": KEY_UP, &"hp2_menu_down": KEY_DOWN,
            &"hp2_menu_left": KEY_LEFT, &"hp2_menu_right": KEY_RIGHT,
        }
        for action in defaults:
            expect(binding(action) == defaults[action], "Incorrect default key for " + String(action))
        await key(KEY_ENTER)
        await key(KEY_DELETE)
        await key(KEY_ESCAPE)
        expect(binding(&"hp2_accelerate") == KEY_UP, "Reserved key or cancel changed the binding")
        expect(race.telemetry().get("screen") == "Keyboard Settings", "Esc left the page during capture")
        await key(KEY_ENTER)
        await key(KEY_DOWN)
        await wait_for(func(state: Dictionary) -> bool: return state.get("modal", false))
        await capture("keyboard-conflict")
        await key(KEY_ENTER)
        expect(binding(&"hp2_accelerate") == KEY_UP, "Cancel changed the binding")
        await key(KEY_ENTER)
        await key(KEY_DOWN)
        await wait_for(func(state: Dictionary) -> bool: return state.get("modal", false))
        await key(KEY_LEFT)
        await key(KEY_ENTER)
        expect(binding(&"hp2_accelerate") == KEY_DOWN and binding(&"hp2_brake") == KEY_UP,
            "Swap did not update both InputMap actions")
        var keyboard: Dictionary = config().get("keyboard", {})
        expect(keyboard.get("hp2_accelerate") == KEY_DOWN and keyboard.get("hp2_brake") == KEY_UP,
            "Swap did not persist both keys")
        if OS.get_environment("HP2_MVP_CONTROLS_SAVE_FAILURE") == "1":
            await check_save_failure()
        var look_forward := row_id(14)
        for row in range(18):
            if race.telemetry().get("selected_item") == look_forward:
                break
            await key(KEY_DOWN)
        expect(race.telemetry().get("selected_item") == look_forward, "Look Ahead is missing")
        await key(KEY_ENTER)
        await key(KEY_J)
        await wait_for(func(state: Dictionary) -> bool: return state.get("modal", false))
        await key(KEY_ENTER)
        expect(binding(&"hp2_zone_zoom") == KEY_Q and binding(&"hp2_roadblock") == KEY_J,
            "Cancel changed the cross-tab conflict")
        await key(KEY_ENTER)
        await key(KEY_F)
        await select(object_hash("HP2_CONTROLS_ROW_15"))
        await key(KEY_ENTER)
        await key(KEY_X)
        keyboard = config().get("keyboard", {})
        expect(keyboard.get("hp2_zone_zoom") == KEY_F and keyboard.get("hp2_zone_freeze") == KEY_X,
            "Camera shortcuts could not be rebound and saved")
        await click_row(27)
        await check_layout()
        check_scroll_arrows(false, false)
        var cop_bounds: Dictionary = race.telemetry().get("menu_alignment_bounds", {})
        expect(cop_bounds.has(object_hash("HP2_CONTROLS_SIREN_LABEL")) and
            cop_bounds.has(object_hash("HP2_CONTROLS_SIREN_VALUE")), "Cop Mode is missing its shared Siren key")
        await capture("keyboard-cop")
        expect(race.telemetry().get("selected_item") == row_id(27), "Mouse did not select the Cop Mode tab")
        await key(KEY_DOWN)
        expect(race.telemetry().get("selected_item") == row_id(11), "Cop Mode did not show Nitro")
        await key(KEY_DOWN)
        expect(race.telemetry().get("selected_item") == row_id(12), "Cop Mode did not show Roadblock")
        await key(KEY_ENTER)
        await key(KEY_F6)
        expect(binding(&"hp2_roadblock") == KEY_F6, "Cop Mode binding was not saved")
        await click_row(26)
        await key(KEY_DOWN)
        expect(race.telemetry().get("selected_item") != row_id(0), "Race tab lost its scroll position")
        await click_row(25)
        await select(row_id(22))
        await key(KEY_ENTER)
        await key(KEY_F8)
        expect(binding(&"hp2_menu_left") == KEY_F8, "Frontend binding was not saved")
    await back("Controls")
    if device >= 0:
        await pad(JOY_BUTTON_DPAD_DOWN)
        await pad(JOY_BUTTON_A)
    else:
        await key(KEY_DOWN)
        await key(KEY_ENTER)
    await wait_screen("Controller Settings")
    await check_layout()
    await capture("controller-settings")
    if verify_reload:
        expect(config().get("steering_dead_zone") == 10 and config().get("steering_sensitivity") == 110,
            "Stick settings did not survive restart")
        await select(object_hash("HP2_CONTROLS_ROW_3"))
        await key(KEY_ENTER)
        await wait_for(func(state: Dictionary) -> bool: return state.get("modal", false))
        await key(KEY_ENTER)
        expect(config().get("steering_dead_zone") == 10 and config().get("steering_sensitivity") == 110,
            "Cancel changed the controller settings")
        await key(KEY_ENTER)
        await wait_for(func(state: Dictionary) -> bool: return state.get("modal", false))
        await key(KEY_LEFT)
        await key(KEY_ENTER)
        await wait_for(func(state: Dictionary) -> bool:
            return state.get("screen") == "Controller Settings" and profile().get("joystick_configurations", [0])[0] == 30)
        expect(config().get("steering_dead_zone") == 8 and config().get("steering_sensitivity") == 100,
            "Restore Defaults did not reset controller stick tuning")
        var saved := profile()
        expect(saved.get("joystick_configurations", [0])[0] == 30 and saved["players"][0]["settings"]["authored_option"] == 1,
            "Restore Defaults did not save Layout A and enable vibration")
    else:
        await key(KEY_DOWN)
        await check_layout()
        await capture("controller-steering-adjustment")
        var bounds: Dictionary = race.telemetry().get("menu_alignment_bounds", {})
        expect(bounds.has(0x9345e4a7), "The steering increment arrow has no pointer target")
        var arrow: Rect2 = bounds[0x9345e4a7]
        var percentage: Rect2 = bounds[object_hash("HP2_CONTROLS_VALUE_1")]
        expect(absf(arrow.get_center().y - percentage.get_center().y) <= 1.0,
            "The adjustment arrow is not centered on the percentage")
        var canvas := race.get_node("FrontendCanvas") as Node2D
        await click_at(canvas.to_global(arrow.get_center()))
        await wait_for(func(_state: Dictionary) -> bool: return config().get("steering_dead_zone") == 10)
        await key(KEY_DOWN)
        await key(KEY_RIGHT)
        expect(config().get("steering_dead_zone") == 10 and config().get("steering_sensitivity") == 110,
            "Steering settings were not saved")
        await key(KEY_UP)
        await key(KEY_UP)
        await key(KEY_ENTER)
        await wait_screen("Joystick PreConfig")
        await key(KEY_ENTER)
        await wait_screen("Joystick Config Port1")
        await key(KEY_RIGHT)
        await key(KEY_RIGHT)
        await key(KEY_ENTER)
        await wait_screen("Joystick PreConfig")
        await back("Controller Settings")
    await back("Controls")
    await back("Options")
    await visit(2, "Display Options")
    var display_bounds: Dictionary = race.telemetry().get("menu_alignment_bounds", {})
    expect(display_bounds.has(0x5e11b599) and display_bounds.has(0xaa5d7d3f) and
        not display_bounds.has(object_hash("HP2_CONTROLS_TITLE")),
        "Controls leaked its borrowed artwork into Display Options")
    await back("Options")
    if OS.get_environment("HP2_MVP_TEST_LANGUAGES") == "1":
        await check_language_options()
    print("HP2_CONTROLS_SETTINGS_PASS reload=", verify_reload)
    quit(0)

func check_save_failure() -> void:
    var path: String = race.get_parent().launch_root.path_join("launcher.cfg")
    var backup := path + ".smoke-backup"
    expect(DirAccess.rename_absolute(path, backup) == OK, "Could not back up the test launcher configuration")
    var blocked := DirAccess.make_dir_absolute(path)
    if blocked == OK:
        await key(KEY_ENTER)
        await key(KEY_F7)
    var dialog_open: bool = race.telemetry().get("modal", false)
    if blocked == OK:
        expect(DirAccess.remove_absolute(path) == OK, "Could not remove the temporary configuration directory")
    expect(DirAccess.rename_absolute(backup, path) == OK, "Could not restore the test launcher configuration")
    expect(blocked == OK and dialog_open, "Could not open the HP2 save error dialog")
    await key(KEY_ENTER)
    expect(not race.telemetry().get("modal", false), "The save error dialog did not close")
    expect(binding(&"hp2_accelerate") == KEY_DOWN, "A failed save changed the live binding")

func check_language_options() -> void:
    await select(FIRST)
    await create_timer(0.85).timeout
    var options: Dictionary = race.telemetry().get("menu_alignment_bounds", {})
    expect(not options.has(0xaa5d7d3f), "Options still draws the fixed gap strips")
    var masks := [0x708a0bf1, 0x72df3492, 0x7e88ffb7, 0x75345d33, 0x778985d4, 0x79deae75, 0x7c33d716]
    var control_mask: Rect2 = options[masks[0]]
    var display_mask: Rect2 = options[masks[1]]
    var pitch := display_mask.position.y - control_mask.position.y
    expect(pitch > control_mask.size.y, "Options rows have no gap")
    for index in range(masks.size()):
        var mask: Rect2 = options[masks[index]]
        expect(absf(mask.size.y - control_mask.size.y) <= 0.3 and
            absf(mask.position.y - control_mask.position.y - pitch * index) <= 0.3,
            "Options row gaps are uneven")
    var english_display: Rect2 = options[FIRST + 2]
    await capture("options-even-gaps")
    await visit(6, "Language Options")
    await capture("language-options")
    await select(FIRST + 1)
    await key(KEY_ENTER)
    await wait_for(func(state: Dictionary) -> bool:
        return state.get("screen") == "Options" and profile().get("language", -1) == 1)
    var french: Dictionary = race.telemetry().get("menu_alignment_bounds", {})
    var french_display: Rect2 = french[FIRST + 2]
    expect(absf(french_display.size.x - english_display.size.x) > 2.0, "Native Options text did not switch to French")
    await capture("options-french")
    await visit(6, "Language Options")
    expect(race.telemetry().get("selected_item") == FIRST + 1, "Languages did not focus the current language")
    await select(FIRST)
    await key(KEY_ENTER)
    await wait_for(func(state: Dictionary) -> bool:
        return state.get("screen") == "Options" and profile().get("language", -1) == 0)
    print("HP2_LANGUAGE_OPTIONS_PASS")
