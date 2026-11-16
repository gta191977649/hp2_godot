extends "test_mvp_debug_menu.gd"

const SUBTITLE := 0xaee77883
const ZONE_CAMS := FIRST + 5
const FIRST_HUD := FIRST + 6
const UPPER := 0xc671680c
const LOWER := 0x32c91dae
const PAUSE_SUBTITLE := 0xaee77886
const PAUSE_HUD_HOTKEY := 0xaee77885

func run() -> void:
    deadline = Time.get_ticks_msec() + 85000
    var root_path := ProjectSettings.globalize_path("res://").simplify_path()
    if root_path != OS.get_environment("HP2_SUBTITLE_TEST_ROOT").simplify_path():
        fail("Use an isolated subtitle project")
        return
    var cfg := root_path.path_join("launcher.cfg")
    change_scene_to_file("res://native_game.tscn")
    while not is_instance_valid(race):
        await process_frame
        race = root.get_node_or_null("NativeGame/RacePresentation")

    if OS.get_cmdline_user_args().is_empty():
        await check_menu(cfg)
    else:
        await check_radio(cfg)

func check_menu(cfg: String) -> void:
    await wait_screen("Title Screen")
    var launch := race.get_parent()
    if launch.call("_subtitle_setting", -1):
        fail("Subtitle should default to Off")
        return
    await wait_for(func(state: Dictionary) -> bool: return state.get("video_request") == 2)
    await key(KEY_ENTER)
    await wait_for(func(state: Dictionary) -> bool: return state.get("video_request") == 0)
    await create_timer(4.5).timeout
    await key(KEY_ENTER)
    await wait_screen("New Main Menu")
    await select(FIRST + 2)
    await key(KEY_ENTER)
    await wait_screen("Options")
    await select(FIRST + 2)
    await key(KEY_ENTER)
    await wait_screen("Display Options")
    await select(ZONE_CAMS)
    await key(KEY_DOWN)
    if race.telemetry().get("selected_item") != SUBTITLE:
        fail("Subtitle is not immediately below Zone Cams")
        return
    if not await check_alignment():
        return
    await key(KEY_RIGHT)
    var config := ConfigFile.new()
    if config.load(cfg) != OK or not config.get_value("accessibility", "police_subtitles", false):
        fail("Subtitle On was not saved globally")
        return
    await capture("display-subtitle")
    await key(KEY_DOWN)
    if race.telemetry().get("selected_item") != FIRST_HUD or not await check_alignment():
        fail("Select down did not reach Customize P1 HUD")
        return
    await key(KEY_UP)
    await key(KEY_ENTER)
    if config.load(cfg) != OK or config.get_value("accessibility", "police_subtitles", true):
        fail("Subtitle Off was not saved globally")
        return
    print("SUBTITLE_MENU_PASS")
    quit(0)

func check_alignment() -> bool:
    await create_timer(0.55).timeout
    var state: Dictionary = race.telemetry()
    var bounds: Dictionary = state.get("menu_alignment_bounds", {})
    var row: int = state.get("selected_item", 0)
    if not (bounds.has(row) and bounds.has(UPPER) and bounds.has(LOWER)):
        fail("Could not measure subtitle menu alignment")
        return false
    var y: float = bounds[row].get_center().y
    if not (bounds[UPPER].get_center().y < y and y < bounds[LOWER].get_center().y):
        fail("Display highlight does not bracket the selected row")
        return false
    return true

func open_pause_display() -> void:
    await select(FIRST + 2)
    await key(KEY_ENTER)
    await wait_screen("Pause Options")
    await select(FIRST + 1)
    await key(KEY_ENTER)
    await wait_screen("Pause display")

func select_pause_subtitle() -> bool:
    for attempt in range(13):
        if race.telemetry().get("selected_item") == PAUSE_SUBTITLE:
            return true
        await key(KEY_UP)
    fail("Pause Display did not select Subtitle")
    return false

func check_pause_display(cfg: String) -> bool:
    await open_pause_display()
    await process_frame
    var before: Dictionary = race.telemetry().get("menu_alignment_bounds", {})
    if not await select_pause_subtitle():
        return false
    await create_timer(0.55).timeout
    if OS.get_environment("HP2_MVP_TEST_MENU_ALIGNMENT") == "1":
        var after: Dictionary = race.telemetry().get("menu_alignment_bounds", {})
        if not before.has(PAUSE_HUD_HOTKEY) or before.has(PAUSE_SUBTITLE) or after.has(FIRST) or not after.has(PAUSE_SUBTITLE):
            fail("Pause Display did not scroll Subtitle into view")
            return false
        var last_slot: Rect2 = before[PAUSE_HUD_HOTKEY]
        var subtitle: Rect2 = after[PAUSE_SUBTITLE]
        if absf(last_slot.position.y - subtitle.position.y) > 2.0:
            fail("Subtitle did not scroll into the last authored row")
            return false
    await key(KEY_LEFT)
    var config := ConfigFile.new()
    if config.load(cfg) != OK or config.get_value("accessibility", "police_subtitles", true):
        fail("Pause Display did not save Subtitle Off")
        return false
    await capture("pause-display-subtitle-off")
    await key(KEY_BACKSPACE)
    await wait_screen("Pause Options")
    await key(KEY_BACKSPACE)
    await wait_screen("Pause Menu")
    await open_pause_display()
    if not await select_pause_subtitle():
        return false
    await create_timer(0.55).timeout
    await key(KEY_ENTER)
    if config.load(cfg) != OK or not config.get_value("accessibility", "police_subtitles", false):
        fail("Pause Display did not restore Subtitle On")
        return false
    await capture("pause-display-subtitle-on")
    await key(KEY_BACKSPACE)
    await wait_screen("Pause Options")
    await key(KEY_BACKSPACE)
    await wait_screen("Pause Menu")
    print("SUBTITLE_PAUSE_MENU_PASS")
    return true

func check_radio(cfg: String) -> void:
    var launch := race.get_parent()
    var expected := OS.get_environment("HP2_SUBTITLE_TEST_EXPECT_ON") == "1"
    if bool(launch.call("_subtitle_setting", -1)) != expected:
        fail("Launcher did not restore the subtitle preference")
        return
    await wait_for(func(state: Dictionary) -> bool: return state.get("race_state") == 3)
    var started := false
    var cleared := false
    var last_sequence := 0
    var end_step := 790
    while race.telemetry().get("step", 0) < end_step:
        var state: Dictionary = race.telemetry()
        var caption: String = state.get("police_subtitle", "")
        var sequence: int = state.get("speech_caption_sequence", 0)
        if sequence and sequence != last_sequence:
            print("SUBTITLE_AUDIO sequence=", sequence, " event=", state.get("speech_caption_event"),
                " parts=", state.get("speech_caption_parts"), " verified=", state.get("subtitle_source_verified"))
            last_sequence = sequence
        if not caption.is_empty() and not started:
            if not expected:
                fail("Subtitle showed while the setting was Off")
                return
            started = true
            await capture("race-police-subtitle")
            await key(KEY_ESCAPE)
            await wait_screen("Pause Menu")
            if not race.telemetry().get("police_subtitle", "").is_empty():
                fail("Caption stayed visible during pause")
                return
            if not await check_pause_display(cfg):
                return
            await key(KEY_ESCAPE)
            await wait_for(func(after: Dictionary) -> bool: return after.get("race_state") == 3)
        elif started and caption.is_empty():
            cleared = true
        await process_frame
    if expected and (not started or not cleared):
        fail("Radio caption did not appear and retire")
        return
    if not last_sequence:
        fail("Original radio audio did not start")
        return
    print("SUBTITLE_RADIO_PASS enabled=", expected)
    quit(0)
