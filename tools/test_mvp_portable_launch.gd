extends SceneTree

func _initialize() -> void:
    run.call_deferred()

func run() -> void:
    var iso := OS.get_environment("HP2_MVP_TEST_LAUNCH_ISO")
    var bad_iso := OS.get_environment("HP2_MVP_TEST_BAD_ISO")
    var changed := OS.get_cmdline_user_args().has("--select-iso")
    var project := ProjectSettings.globalize_path("res://")
    var config_path := project.path_join("launcher.cfg")
    var had_config := FileAccess.file_exists(config_path)
    var old_config := FileAccess.get_file_as_string(config_path) if had_config else ""
    change_scene_to_file("res://native_game.tscn")
    await process_frame
    await process_frame
    var launch := root.get_node("NativeGame")
    if changed or not had_config:
        if not launch.has_node("IsoPicker"):
            fail("ISO picker did not open")
            return
        var initial := FileAccess.get_file_as_string(config_path)
        if changed and initial != old_config:
            fail("Opening the picker modified the saved path")
            return
        var corrupt_iso := project.path_join("portable-launch-corrupt-%d.iso" % OS.get_process_id())
        var file := FileAccess.open(corrupt_iso, FileAccess.WRITE)
        if file == null:
            fail("Could not write a malformed ISO fixture")
            return
        file.store_string("Not a disc image")
        file.close()
        var rejected_paths := [bad_iso, corrupt_iso]
        var incomplete_iso := OS.get_environment("HP2_MVP_TEST_INCOMPLETE_ISO")
        if not incomplete_iso.is_empty():
            if ClassDB.class_call_static(&"RacePresentation", &"validate_game_iso", incomplete_iso) != "":
                fail("Incomplete fixture must pass the disc identity check")
                return
            rejected_paths.append(incomplete_iso)
        for rejected: String in rejected_paths:
            if rejected == corrupt_iso:
                launch.iso_dialog.file_selected.emit(rejected)
            else:
                root.files_dropped.emit(PackedStringArray([rejected]))
            await process_frame
            if not launch.iso_error.visible or not launch.iso_error.text.begins_with("Invalid game ISO:") or \
                    launch.has_node("RacePresentation") or \
                    FileAccess.get_file_as_string(config_path) != initial:
                DirAccess.remove_absolute(corrupt_iso)
                fail("Invalid ISO was accepted or modified the config")
                return
        DirAccess.remove_absolute(corrupt_iso)
        root.files_dropped.emit(PackedStringArray([iso]))
        await process_frame
    var expected := ConfigFile.new()
    if expected.load(config_path) != OK or expected.get_value("game", "iso_path", "") != iso:
        fail("Valid ISO path was not stored beside the project")
        return
    var deadline := Time.get_ticks_msec() + 25000
    var profile := ""
    var next_confirm := 0
    while Time.get_ticks_msec() < deadline:
        var race := launch.get_node_or_null("RacePresentation")
        if race and race.telemetry().get("screen") in ["Title Screen", "New Main Menu"]:
            for file in DirAccess.get_files_at(project.path_join("profiles")):
                if file == "profile.json" or (file.begins_with("profile-") and file.ends_with(".json") and not file.ends_with(".backup.json")):
                    profile = file
                    break
            if not profile.is_empty():
                print("PORTABLE_LAUNCH_PASS picker=", changed or not had_config, " profile=", profile)
                quit(0)
                return
        if race and race.telemetry().get("creating_profile", false):
            for character in "Launch":
                await tap(KEY_NONE, character.unicode_at(0))
            await tap(KEY_ENTER)
        elif race and race.telemetry().get("screen") == "BootAutoload" and Time.get_ticks_msec() >= next_confirm:
            next_confirm = Time.get_ticks_msec() + 400
            await tap(KEY_ENTER)
        await process_frame
    fail("The game or portable profile did not start")

func tap(code: Key, character := 0) -> void:
    var event := InputEventKey.new()
    event.keycode = code
    event.physical_keycode = code
    event.unicode = character
    event.pressed = true
    Input.parse_input_event(event)
    await create_timer(0.08).timeout
    event = event.duplicate()
    event.pressed = false
    Input.parse_input_event(event)
    await create_timer(0.08).timeout

func fail(reason: String) -> void:
    push_error(reason)
    quit(1)
