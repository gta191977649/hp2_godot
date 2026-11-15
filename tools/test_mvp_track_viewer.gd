extends SceneTree

var race: Node
var deadline := Time.get_ticks_msec() + 60000
var running := false
var screenshots := OS.get_environment("HP2_MVP_SCREENSHOT_DIR")
var matrix := OS.get_environment("HP2_MVP_VIEWER_MATRIX") == "1"

func _initialize() -> void:
    change_scene_to_file.call_deferred("res://native_game.tscn")

func _process(_delta: float) -> bool:
    if Time.get_ticks_msec() > deadline:
        push_error("Track Viewer smoke timed out: %s" % [race.telemetry() if is_instance_valid(race) else {}])
        quit(1)
    if not running:
        race = root.get_node_or_null("NativeGame/RacePresentation")
        if is_instance_valid(race) and race.telemetry().has("debug_console_open"):
            running = true
            run.call_deferred()
    return false

func wait_for(predicate: Callable) -> Dictionary:
    while true:
        var state: Dictionary = race.telemetry()
        if predicate.call(state):
            return state
        await process_frame
    return {}

func key(code: Key, unicode := 0, pressed := true) -> void:
    var event := InputEventKey.new()
    event.keycode = code
    event.physical_keycode = code
    event.unicode = unicode
    event.pressed = pressed
    Input.parse_input_event(event)

func tap(code: Key, unicode := 0) -> void:
    key(code, unicode)
    await process_frame
    await process_frame
    key(code, unicode, false)
    await process_frame

func move_pointer(at: Vector2) -> void:
    var motion := InputEventMouseMotion.new()
    motion.position = at
    motion.global_position = at
    Input.parse_input_event(motion)
    await process_frame

func click(at: Vector2, button := MOUSE_BUTTON_LEFT) -> void:
    await move_pointer(at)
    mouse_button(at, button, true)
    await process_frame
    await process_frame
    await process_frame
    await process_frame
    mouse_button(at, button, false)
    await process_frame

func mouse_button(at: Vector2, button: MouseButton, pressed: bool) -> void:
    var event := InputEventMouseButton.new()
    event.button_index = button
    event.position = at
    event.global_position = at
    event.pressed = pressed
    Input.parse_input_event(event)

func capture(label: String) -> void:
    if screenshots.is_empty():
        return
    DirAccess.make_dir_recursive_absolute(screenshots)
    await RenderingServer.frame_post_draw
    root.get_texture().get_image().save_png(screenshots.path_join(label + ".png"))

func run() -> void:
    root.grab_focus()
    var direct := not OS.get_cmdline_user_args().is_empty()
    if direct:
        await wait_for(func(state: Dictionary) -> bool: return state.get("track", 0) == 12)
    else:
        await wait_for(func(state: Dictionary) -> bool: return state.get("screen", "") == "Title Screen")
    await create_timer(0.4).timeout
    var profile_path := str(race.get_parent().profile_directory).path_join("profile.json")
    var profile_before := FileAccess.get_sha256(profile_path) if FileAccess.file_exists(profile_path) else ""
    if not race.telemetry().get("debug_console_open", false):
        await tap(KEY_QUOTELEFT)
    for letter in "trackviewer":
        await tap(KEY_NONE, letter.unicode_at(0))
    await tap(KEY_ENTER)
    if direct:
        await wait_for(func(state: Dictionary) -> bool: return state.get("viewer_confirm", false))
        await capture("confirm")
        await click(Vector2(root.size) * 0.5 + Vector2(-130, 20))
    var loaded := await wait_for(func(state: Dictionary) -> bool:
        return state.get("track_viewer", false) and state.get("viewer_entities", 0) > 0)
    print("TRACK_VIEWER_LOADED ", loaded)
    if loaded.get("viewer_catalogue_size", 0) != 30 or loaded.get("vehicles", -1) != 0:
        push_error("Expected 30 authored tracks and no rendered cars")
        quit(1)
        return
    if loaded.has("race_state") or race.has_node("FrontendCanvas"):
        push_error("The viewer entered the gameplay runtime")
        quit(1)
        return
    if OS.get_environment("HP2_MVP_VIEWER_EXIT_ACTIVE") == "1":
        await RenderingServer.frame_post_draw
        print("TRACK_VIEWER_ACTIVE_EXIT")
        quit(0)
        return
    if matrix:
        await run_matrix(loaded)
        return
    var container := race.get_node("TrackViewerViewport") as SubViewportContainer
    if OS.get_environment("HP2_MVP_VIEWER_VISIBILITY") == "1":
        var renderer := Vector2(455, 35 + container.position.y)
        var checkbox := Vector2(33, 265 + container.position.y)
        await click(renderer)
        var enabled: Dictionary = race.telemetry()
        await capture("visibility-on")
        await click(checkbox)
        await create_timer(0.15).timeout
        var disabled: Dictionary = race.telemetry()
        await capture("visibility-off")
        await click(checkbox)
        await create_timer(0.15).timeout
        var restored: Dictionary = race.telemetry()
        print("TRACK_VIEWER_VISIBILITY ", enabled, " -> ", disabled, " -> ", restored)
        if (not enabled.scenery_zones_enabled or disabled.scenery_zones_enabled or
            not restored.scenery_zones_enabled or disabled.scenery_sections <= enabled.scenery_sections or
            restored.scenery_sections != enabled.scenery_sections):
            push_error("Track Viewer visibility toggle did not restore the full track and original zone")
            quit(1)
        else:
            await click(Vector2(738, 34))
            await wait_for(func(state: Dictionary) -> bool:
                return not state.get("track_viewer", false) and state.get("screen", "") == "New Main Menu")
            print("TRACK_VIEWER_VISIBILITY_PASS")
            quit(0)
        return
    if OS.get_environment("HP2_MVP_VIEWER_HIERARCHY") == "1":
        if not await check_hierarchy(container, loaded.track):
            quit(1)
            return
    await capture("viewer")
    print("TRACK_VIEWER_WINDOW ", root.size, " focus=", root.has_focus())
    await click(Vector2(160, 34))
    await create_timer(0.2).timeout
    await capture("tracks")
    if direct:
        await click(Vector2(170, 287))
    else:
        await click(Vector2(170, 116))
    var next_track := 21 if direct else 12
    var switched := await wait_for(func(state: Dictionary) -> bool:
        return state.get("track", 0) == next_track and state.get("generation", 0) > loaded.generation)
    print("TRACK_VIEWER_SWITCH ", switched)
    if OS.get_environment("HP2_MVP_VIEWER_HIERARCHY") == "1":
        var selection := _scene_viewport(container).get_node("TrackViewerEntities/TrackViewerSelection") as MeshInstance3D
        if selection.visible or selection.mesh != null or not _bundles_match(selection, next_track):
            push_error("Hierarchy or highlight survived a track change")
            quit(1)
            return
    await create_timer(0.2).timeout
    var sidebar_offset := Vector2(0, container.position.y)
    await click(Vector2(220, 35) + sidebar_offset)
    await capture("entities")
    await click(Vector2(455, 35) + sidebar_offset)
    await capture("render")
    if OS.get_environment("HP2_MVP_VIEWER_ROADSHINE") == "1":
        var roadshine := Vector2(35, 633) + sidebar_offset
        if not race.telemetry().get("roadshine_enabled", false):
            push_error("Roadshine is disabled by default")
            quit(1)
            return
        await click(roadshine)
        await process_frame
        if race.telemetry().get("roadshine_enabled", true):
            push_error("Render tab did not disable Roadshine")
            quit(1)
            return
        await click(roadshine)
        await process_frame
        if not race.telemetry().get("roadshine_enabled", false):
            push_error("Render tab did not restore Roadshine")
            quit(1)
            return
        print("TRACK_VIEWER_ROADSHINE_PASS")
    if OS.get_environment("HP2_MVP_VIEWER_PROFILE") == "1":
        await create_timer(1.0).timeout
        print("VIEWER_PROFILE_OFF_BEGIN ", Engine.get_process_frames())
        await create_timer(4.0).timeout
        print("VIEWER_PROFILE_OFF_END ", Engine.get_process_frames())
        await click(Vector2(33, 219) + sidebar_offset)
        await create_timer(1.0).timeout
        print("VIEWER_PROFILE_ON_BEGIN ", Engine.get_process_frames())
        await create_timer(4.0).timeout
        print("VIEWER_PROFILE_ON_END ", Engine.get_process_frames())
        quit()
        return
    await click(Vector2(33, 81) + sidebar_offset)
    await create_timer(0.18).timeout
    if OS.get_environment("HP2_MVP_VIEWER_CAPTURE_OVERVIEW") == "1":
        await click(Vector2(345, 35) + sidebar_offset)
        await click(Vector2(90, 217) + sidebar_offset)
    await capture("entity-view")
    var viewport := _scene_viewport(container)
    if OS.get_environment("HP2_MVP_VIEWER_WIREFRAME") == "1":
        await click(Vector2(33, 127) + sidebar_offset)
        await create_timer(0.15).timeout
        if viewport.debug_draw != Viewport.DEBUG_DRAW_WIREFRAME or not viewport.get_node("RaceScene").visible:
            push_error("Wireframe view did not replace Entity view")
            quit(1)
            return
        await capture("wireframe")
        await create_timer(0.2).timeout
        await click(Vector2(33, 127) + sidebar_offset)
        await create_timer(0.15).timeout
        if viewport.debug_draw != Viewport.DEBUG_DRAW_DISABLED or not viewport.get_node("RaceScene").visible:
            push_error("Disabling Wireframe view did not restore the track")
            quit(1)
            return
        await click(Vector2(33, 81) + sidebar_offset)
        print("TRACK_VIEWER_WIREFRAME_PASS")
    if OS.get_environment("HP2_MVP_VIEWER_EFFECTS") == "1":
        if not await check_environment(viewport, next_track, sidebar_offset):
            quit(1)
            return
    var camera := viewport.get_camera_3d()
    await create_timer(0.2).timeout
    var flight_point := container.position + container.size * 0.5
    await move_pointer(flight_point)
    mouse_button(flight_point, MOUSE_BUTTON_RIGHT, true)
    await process_frame
    var movement_start := camera.position
    key(KEY_W)
    await create_timer(0.24).timeout
    mouse_button(flight_point, MOUSE_BUTTON_RIGHT, false)
    var released := camera.position
    await create_timer(0.15).timeout
    key(KEY_W, 0, false)
    print("TRACK_VIEWER_CAMERA moved=", movement_start.distance_to(camera.position), " mouse=", Input.mouse_mode)
    if movement_start.distance_to(released) < 1 or not released.is_equal_approx(camera.position) or Input.mouse_mode != Input.MOUSE_MODE_VISIBLE:
        push_error("Free camera movement or release failed")
        quit(1)
        return
    if OS.get_environment("HP2_MVP_VIEWER_COORDINATES") == "1":
        await create_timer(0.2).timeout
        await click(Vector2(345, 35) + sidebar_offset)
        await create_timer(0.2).timeout
        await capture("camera-coordinates")
        var copy_position := Vector2(120, 273) + sidebar_offset
        var previous_clipboard := DisplayServer.clipboard_get()
        await click(copy_position)
        await create_timer(0.15).timeout
        var pose := camera.global_transform
        var eye := pose.origin
        var target := eye - pose.basis.z.normalized()
        var expected := "Position (%.3f, %.3f, %.3f)\nLook At (%.3f, %.3f, %.3f)" % [
            eye.x, -eye.z, eye.y, target.x, -target.z, target.y]
        var copied := DisplayServer.clipboard_get().replace("\r\n", "\n")
        DisplayServer.clipboard_set(previous_clipboard)
        if copied != expected:
            push_error("Camera coordinates clipboard mismatch. Expected %s, got %s" % [
                JSON.stringify(expected), JSON.stringify(copied)])
            quit(1)
            return
        print("TRACK_VIEWER_COORDINATES_PASS")
    await create_timer(0.2).timeout
    await click(Vector2(738, 34))
    await create_timer(0.2).timeout
    print("TRACK_VIEWER_RETURN ", race.telemetry())
    await wait_for(func(state: Dictionary) -> bool:
        return not state.get("track_viewer", false) and state.get("screen", "") == "New Main Menu")
    if not profile_before.is_empty() and FileAccess.get_sha256(profile_path) != profile_before:
        push_error("Player profile changed during the viewer session")
        quit(1)
        return
    if OS.get_environment("HP2_MVP_VIEWER_REENTER") == "1":
        await verify_race_reentry()
    print("TRACK_VIEWER_PASS")
    quit()

func verify_race_reentry() -> void:
    await create_timer(1.3).timeout
    await tap(KEY_DOWN)
    await create_timer(0.3).timeout
    await tap(KEY_ENTER)
    await wait_for(func(state: Dictionary) -> bool: return state.get("screen", "") == "Cops Off Mode Select")
    await create_timer(0.9).timeout
    await tap(KEY_ENTER)
    await wait_for(func(state: Dictionary) -> bool: return state.get("screen", "") == "Pre Race Stats")
    await create_timer(0.5).timeout
    await tap(KEY_ENTER)
    var resumed := await wait_for(func(state: Dictionary) -> bool:
        return state.get("race_state", 0) == 3 and state.get("vehicles", 0) > 0)
    print("TRACK_VIEWER_RACE_REENTRY ", resumed.track)

func run_matrix(loaded: Dictionary) -> void:
    deadline = Time.get_ticks_msec() + 180000
    var index := 0
    var container := race.get_node("TrackViewerViewport") as SubViewportContainer
    for decade in [10, 20, 30, 40, 60]:
        for variant in range(1, 7):
            var track: int = decade + variant
            if track != loaded.track:
                var started := Time.get_ticks_msec()
                var generation: int = loaded.generation
                await click(Vector2(160, 34))
                await create_timer(0.1).timeout
                var row := index
                if index >= 29:
                    await capture("last-popup")
                await click(Vector2(170, 83 + 34 * row))
                loaded = await wait_for(func(state: Dictionary) -> bool:
                    return state.get("track", 0) == track and state.get("generation", 0) > generation)
                print("TRACK_VIEWER_LOAD_MS ", track, " ", Time.get_ticks_msec() - started)
            if _scene_viewport(container).get_child_count() != 3 or loaded.vehicles != 0 or loaded.reverse or loaded.viewer_entities <= 0:
                push_error("Incorrect viewer scene or retired children for track %d" % track)
                quit(1)
                return
            print("TRACK_VIEWER_MATRIX ", track, " entities=", loaded.viewer_entities)
            index += 1
    await capture("last-track")
    print("TRACK_VIEWER_MATRIX_PASS")
    quit()

func _scene_viewport(container: SubViewportContainer) -> SubViewport:
    var output := container.get_child(0) as SubViewport
    var scene := output.get_node_or_null("RaceRenderTarget") as SubViewport
    return scene if scene else output

func _bundles_match(selection: MeshInstance3D, track: int) -> bool:
    var names: PackedStringArray = selection.get_meta("track_bundles", PackedStringArray())
    return names.size() == 2 and names[0].ends_with("TRACKA%d.BUN" % track) and names[1].ends_with("TRACKB%d.BUN" % track)

func check_hierarchy(container: SubViewportContainer, track: int) -> bool:
    var viewport := _scene_viewport(container)
    var selection := viewport.get_node("TrackViewerEntities/TrackViewerSelection") as MeshInstance3D
    var camera := viewport.get_camera_3d()
    if not _bundles_match(selection, track) or container.position.x < 1:
        push_error("Hierarchy must start left of one viewport and list only the current track bundles")
        return false
    var point := Vector2.ZERO
    for y in [0.58, 0.72, 0.85]:
        for x in [0.50, 0.35, 0.65, 0.23, 0.77]:
            point = container.position + container.size * Vector2(x, y)
            await click(point)
            if selection.visible:
                break
        if selection.visible:
            break
    if not selection.visible or selection.mesh == null or Input.mouse_mode != Input.MOUSE_MODE_VISIBLE:
        push_error("Left click did not select a model")
        return false
    var section: int = selection.get_meta("section")
    var instance: int = selection.get_meta("instance")
    await click(point)
    if selection.visible or selection.mesh != null:
        push_error("Clicking the same model did not clear the selection")
        return false
    await click(point)
    if selection.get_meta("section", -1) != section or selection.get_meta("instance", -1) != instance:
        push_error("Selection did not return to the same source instance")
        return false
    await create_timer(0.6).timeout
    if not selection.has_meta("hierarchy_row"):
        push_error("Selection was not revealed in Hierarchy")
        return false
    await capture("hierarchy-picked")
    var row: Vector2 = selection.get_meta("hierarchy_row")
    var start := camera.global_position
    await click(row)
    await click(row)
    var samples: Array[Vector3] = []
    for i in range(5):
        samples.append(camera.global_position)
        await create_timer(0.075).timeout
    await create_timer(0.35).timeout
    var finish := camera.global_position
    var travel := finish - start
    if travel.length() < 0.1 or selection.get_meta("instance", -1) != instance:
        push_error("Double-click did not focus the selected instance")
        return false
    var intermediate := false
    for sample in samples:
        var along := (sample - start).dot(travel) / travel.length_squared()
        if (sample - start - travel * along).length() > 0.05:
            push_error("Focus moved the camera off the straight path")
            return false
        intermediate = intermediate or (along > 0.05 and along < 0.95)
    if not intermediate:
        push_error("Focus did not animate through an intermediate position")
        return false
    await capture("hierarchy-focused")
    var splitter := Vector2(container.position.x - 3, container.position.y + 155)
    var initial_width := container.position.x
    await move_pointer(splitter)
    mouse_button(splitter, MOUSE_BUTTON_LEFT, true)
    await process_frame
    await move_pointer(splitter + Vector2(75, 0))
    await process_frame
    mouse_button(splitter + Vector2(75, 0), MOUSE_BUTTON_LEFT, false)
    await process_frame
    if container.position.x < initial_width + 30 or Input.mouse_mode != Input.MOUSE_MODE_VISIBLE:
        push_error("Dragging the inspector splitter did not resize the viewport")
        return false
    var inspector_button: Vector2 = selection.get_meta("inspector_button")
    await click(inspector_button)
    await create_timer(0.15).timeout
    if container.position.x > 1 or not selection.visible:
        push_error("Hiding the inspector did not expand the viewport or preserve selection: %s, %s" %
            [container.position, selection.visible])
        return false
    await click(inspector_button)
    await create_timer(0.15).timeout
    if container.position.x < initial_width + 30:
        push_error("Showing the inspector did not restore its resized width")
        return false
    await move_pointer(Vector2(container.position.x - 3, splitter.y))
    mouse_button(Vector2(container.position.x - 3, splitter.y), MOUSE_BUTTON_LEFT, true)
    await process_frame
    await move_pointer(splitter)
    mouse_button(splitter, MOUSE_BUTTON_LEFT, false)
    await process_frame
    print("TRACK_VIEWER_HIERARCHY_PASS")
    return true

func source_hash(name: String) -> int:
    var result := 0xFFFFFFFF
    for byte in name.to_ascii_buffer():
        result = (result * 33 + byte) & 0xFFFFFFFF
    return result

func check_environment(viewport: SubViewport, track: int, sidebar_offset: Vector2) -> bool:
    # Move the actual viewer camera to the original emitter coordinates,
    # leaving the runtime to schedule particle births and visibility.
    var target := Vector3(-654.92, -53.85, -190.07) if track == 12 else Vector3(1517.87, 15.37, -206.27)
    var names := ["XE_FFire_FlmL_01", "XE_FFire_FlmXL_01", "XE_FFire_SmkBLK_01",
        "XE_FFire_SmkWHT_01", "XE_FFire_Sparks_01"] if track == 12 else ["XE_Sandstorm_XL_01", "XE_SandDebris_XL_01"]
    var keys := []
    for name in names:
        keys.append(source_hash(name))
    var camera := viewport.get_camera_3d()
    var effects := viewport.get_node("RaceScene/RaceEffects")
    await create_timer(0.2).timeout
    await click(Vector2(33, 81) + sidebar_offset) # Restore normal scene rendering.
    await process_frame
    if not viewport.get_node("RaceScene").visible:
        push_error("Could not restore normal scene rendering")
        return false
    var until := Time.get_ticks_msec() + 8000
    while Time.get_ticks_msec() < until:
        camera.look_at_from_position(target + Vector3(-35, 195, 18), target, Vector3.UP)
        await process_frame
        var active := 0
        for node in effects.get_children():
            if node is GPUParticles3D and node.visible and node.get_meta("source_effect_key", 0) in keys:
                active += 1
        if active >= 2:
            print("TRACK_VIEWER_ENVIRONMENT ", track, " active=", active)
            await capture("environment")
            return true
    push_error("Expected environment particles near the free camera on track %d" % track)
    await capture("environment-missing")
    return false
