extends "test_mvp_debug_menu.gd"

const BOOT := "BootAutoload"
const BUTTON := 0xe0105663
const PICKER := "Select Profile"
const FIRST_PROFILE_ROW := 0x48525000
const SAVE_LOAD := "Phils Load Save"
var profile: String
var scenario := OS.get_environment("HP2_MVP_BOOT_PROFILE")

func run() -> void:
    deadline = Time.get_ticks_msec() + 85000
    var project := ProjectSettings.globalize_path("res://").simplify_path()
    if project != OS.get_environment("HP2_MVP_BOOT_PROFILE_PROJECT").simplify_path() or scenario.is_empty():
        fail("Use a disposable boot profile project")
        return
    profile = project.path_join("profiles/profile.json")
    if scenario == "load" and not FileAccess.file_exists(profile):
        var names := DirAccess.get_files_at(profile.get_base_dir())
        var named := Array(names).filter(func(file: String) -> bool:
            return file.begins_with("profile-") and file.ends_with(".json") and not file.ends_with(".backup.json"))
        if named.size() != 1:
            fail("Expected a single saved profile to reload")
            return
        profile = profile.get_base_dir().path_join(named[0])
    var original := FileAccess.get_sha256(profile) if FileAccess.file_exists(profile) else ""
    change_scene_to_file("res://native_game.tscn")
    while not is_instance_valid(race):
        await process_frame
        race = root.get_node_or_null("NativeGame/RacePresentation")
    if race.get_parent().profile_directory != profile.get_base_dir():
        fail("Ordinary launch used a different profile directory")
        return
    await wait_screen(BOOT)
    await key(KEY_ENTER)
    if race.telemetry().get("screen") != BOOT:
        fail("Confirm skipped the original check")
        return
    await create_timer(0.35).timeout
    await capture("Checking")

    if scenario == "options":
        if not await check_options(project):
            return
    elif scenario == "scroll":
        if not await check_scroll():
            return
    elif scenario == "load":
        await wait_screen(TITLE)
        if not FileAccess.file_exists(profile) or (not original.is_empty() and FileAccess.get_sha256(profile) != original):
            fail("Loading changed or failed to restore the profile")
            return
        var loaded := JSON.parse_string(FileAccess.get_file_as_string(profile)) as Dictionary
        if loaded.get("players", []).is_empty():
            fail("Loaded file is not a native profile")
            return
    elif scenario == "corrupt":
        await wait_screen(PICKER)
        await key(KEY_ENTER)
        await wait_for(func(state: Dictionary) -> bool: return state.get("modal", false))
        await key(KEY_ENTER)
        if FileAccess.get_sha256(profile) != original or race.telemetry().get("screen") != PICKER:
            fail("Corrupt profile was changed or bypassed")
            return
    else:
        await wait_for(has_continue)
        if not await create_new():
            return
    print("BOOT_PROFILE_PASS ", scenario)
    quit(0)

func has_continue(state: Dictionary) -> bool:
    var bounds: Dictionary = state.get("menu_alignment_bounds", {})
    return state.get("screen") == BOOT and state.get("modal", false) and bounds.has(BUTTON)

func create_new() -> bool:
    await capture("Create-Prompt")
    if not await click_object(BUTTON):
        return false
    await wait_for(func(state: Dictionary) -> bool: return state.get("creating_profile", false))
    await key(KEY_ENTER)
    if FileAccess.file_exists(profile):
        fail("Empty name created a save")
        return false
    await pad(JOY_BUTTON_B)
    await wait_for(has_continue)
    await key(KEY_ENTER)
    await wait_for(func(state: Dictionary) -> bool: return state.get("creating_profile", false))
    await key(KEY_ESCAPE)
    await wait_for(has_continue)
    await key(KEY_ENTER)
    await wait_for(func(state: Dictionary) -> bool: return state.get("creating_profile", false))
    await create_timer(0.6).timeout
    if not await click_object(flailer_hash("CHAR_00")):
        return false
    await key(KEY_BACKSPACE)
    for letter in " Arx ":
        var event := InputEventKey.new()
        event.unicode = letter.unicode_at(0)
        await input_tap(event)
    await key(KEY_LEFT)
    await key(KEY_BACKSPACE)
    await type_c()
    await key(KEY_LEFT)
    await key(KEY_DELETE)
    await type_c()
    await capture("Enter-Name")

    if scenario == "collision":
        var fixture := OS.get_environment("HP2_MVP_BOOT_PROFILE_SEED")
        if DirAccess.copy_absolute(fixture, profile) != OK:
            fail("Could not install the concurrent save fixture")
            return false
        var hash_before := FileAccess.get_sha256(profile)
        await key(KEY_ENTER)
        await wait_screen(TITLE)
        if FileAccess.get_sha256(profile) != hash_before:
            fail("New name overwrote a concurrent save")
            return false
    if scenario == "retry":
        var folder := profile.get_base_dir()
        if DirAccess.make_dir_recursive_absolute(folder) != OK or DirAccess.rename_absolute(folder, folder + "-hold") != OK:
            fail("Could not install the blocked-save directory fixture")
            return false
        var blocker := FileAccess.open(folder, FileAccess.WRITE)
        blocker.store_string("Blocked save directory fixture")
        blocker.close()
        await key(KEY_ENTER)
        await wait_for(func(state: Dictionary) -> bool: return state.get("modal", false))
        await capture("Save-Retry")
        if DirAccess.remove_absolute(folder) != OK or DirAccess.rename_absolute(folder + "-hold", folder) != OK:
            fail("Could not restore the save directory fixture")
            return false
        await key(KEY_ENTER)
    elif scenario == "mouse":
        if not await click_object(flailer_hash("CHAR_2D")):
            return false
    elif scenario != "collision":
        await key(KEY_ENTER)
    await wait_screen(TITLE)
    var files := DirAccess.get_files_at(profile.get_base_dir())
    var created: Array[String] = []
    for file in files:
        if file.begins_with("profile-") and file.ends_with(".json") and not file.ends_with(".backup.json"):
            created.append(file)
    if created.size() != 1:
        fail("Expected a single newly created profile")
        return false
    var saved := JSON.parse_string(FileAccess.get_file_as_string(profile.get_base_dir().path_join(created[0]))) as Dictionary
    if saved.get("autosave") != 1 or saved.get("players", []).is_empty() or saved.players[0].settings.player_name != "Arc":
        fail("The edited name was not saved")
        return false
    return true

func type_c() -> void:
    var event := InputEventKey.new()
    event.unicode = "c".unicode_at(0)
    await input_tap(event)

func choose_row(row: int) -> void:
    var first := FIRST_PROFILE_ROW if race.telemetry().get("screen") == PICKER else FIRST
    for direction in [KEY_DOWN, KEY_UP]:
        for attempt in range(10):
            var selected: int = race.telemetry().get("selected_item", 0)
            if selected == first + row:
                await key(KEY_ENTER)
                return
            await key(direction)
            if race.telemetry().get("selected_item", 0) == selected:
                break
    fail("Could not select row %d" % row)

func check_options(project: String) -> bool:
    var second := project.path_join("profiles/profile-second.json")
    var old_first := FileAccess.get_sha256(profile)
    var old_second := FileAccess.get_sha256(second)
    await wait_screen(PICKER)
    await capture("Startup-Profile-Picker")
    await choose_row(0)
    await wait_screen(TITLE)
    await wait_for(func(state: Dictionary) -> bool: return state.get("video_request") == 2)
    await key(KEY_ENTER)
    await wait_for(func(state: Dictionary) -> bool: return state.get("video_request") == 0)
    await create_timer(4.5).timeout
    await key(KEY_ENTER)
    await wait_screen(MAIN)
    await choose_row(2)
    await wait_screen("Options")
    await choose_row(3)
    await wait_screen(SAVE_LOAD)
    await capture("Save-Load")
    await select(FIRST + 2)
    await key(KEY_RIGHT) # On to off makes an unsaved change.
    await choose_row(1)
    await wait_screen(PICKER)
    await choose_row(1)
    await wait_for(func(state: Dictionary) -> bool: return state.get("modal", false))
    await capture("Unsaved-Changes")
    await key(KEY_ENTER) # Cancel is the initial choice.
    if race.telemetry().get("screen") != PICKER:
        fail("Cancel left the picker")
        return false
    await choose_row(1)
    await wait_for(func(state: Dictionary) -> bool: return state.get("modal", false))
    await key(KEY_LEFT)
    await key(KEY_ENTER) # Discard, then load Zed.
    await wait_screen(SAVE_LOAD)
    if FileAccess.get_sha256(profile) != old_first or FileAccess.get_sha256(second) != old_second:
        fail("Discard changed a save file")
        return false
    await select(FIRST + 2)
    await key(KEY_RIGHT)
    await choose_row(1)
    await wait_screen(PICKER)
    await choose_row(0)
    await wait_for(func(state: Dictionary) -> bool: return state.get("modal", false))
    await key(KEY_LEFT)
    await key(KEY_LEFT)
    await key(KEY_ENTER) # Save Zed and switch back to Arc.
    await wait_screen(SAVE_LOAD)
    var second_save := JSON.parse_string(FileAccess.get_file_as_string(second)) as Dictionary
    var backup := project.path_join("profiles/profile-second.backup.json")
    if FileAccess.get_sha256(profile) != old_first or second_save.get("autosave") != 0 or FileAccess.get_sha256(backup) != old_second:
        fail("Save and Continue did not write only the active profile and its backup")
        return false
    await choose_row(3)
    await wait_for(func(state: Dictionary) -> bool: return state.get("creating_profile", false))
    await key(KEY_ESCAPE)
    await wait_screen(SAVE_LOAD)
    await choose_row(3)
    await wait_for(func(state: Dictionary) -> bool: return state.get("creating_profile", false))
    for letter in "Neo":
        var event := InputEventKey.new()
        event.unicode = letter.unicode_at(0)
        await input_tap(event)
    await key(KEY_ENTER)
    await wait_screen(SAVE_LOAD)
    for file in DirAccess.get_files_at(project.path_join("profiles")):
        if file in ["profile.json", "profile-second.json", "profile-second.backup.json"]:
            continue
        var created := JSON.parse_string(FileAccess.get_file_as_string(project.path_join("profiles").path_join(file))) as Dictionary
        if created.get("autosave") == 1 and created.players[0].settings.player_name == "Neo":
            return true
    fail("Options did not create a separate profile")
    return false

func check_scroll() -> bool:
    await wait_screen(PICKER)
    await select(FIRST_PROFILE_ROW + 6)
    await check_smooth_row(KEY_DOWN, FIRST_PROFILE_ROW + 3)
    await key(KEY_DOWN)
    await capture("Scrolled-Profile-Picker")
    await key(KEY_ENTER)
    await wait_for(func(state: Dictionary) -> bool: return state.get("creating_profile", false))
    await key(KEY_ESCAPE)
    await wait_screen(PICKER)
    await key(KEY_UP)
    await key(KEY_ENTER)
    await wait_screen(TITLE)
    return true

func flailer_hash(value: String) -> int:
    var hash_value := 0xffffffff
    for code in value.to_ascii_buffer():
        hash_value = (hash_value * 33 + code) & 0xffffffff
    return hash_value

func click_object(key_value: int) -> bool:
    var bounds: Dictionary = race.telemetry().get("menu_alignment_bounds", {})
    if not bounds.has(key_value):
        fail("Missing Flailer object %x" % key_value)
        return false
    var canvas := race.get_node("FrontendCanvas") as Node2D
    var rect: Rect2 = bounds[key_value]
    await click_at(canvas.to_global(rect.get_center()))
    return true
