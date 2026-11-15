extends "test_frontend_mvp.gd"

const SIZES := [Vector2i(800, 600), Vector2i(1120, 630), Vector2i(1260, 540), Vector2i(540, 720)]
var direct := false
var track_picker := false
var resize_pending := false
var tested_screens := {}
var initial_mode: Window.Mode
var resize_failed := false
var loading_captured := false
var mouse_pause := false
var check_pause_bindings := false

func _initialize() -> void:
    direct = not OS.get_cmdline_user_args().is_empty()
    mouse_pause = OS.get_environment("HP2_MVP_MOUSE_PAUSE") == "1"
    check_pause_bindings = OS.get_environment("HP2_MVP_PAUSE_BINDINGS") == "1"
    track_picker = OS.get_environment("HP2_MVP_RESIZE_TRACK_PICKER") == "1"
    initial_mode = root.mode
    super._initialize()

func _process(delta: float) -> bool:
    if resize_failed:
        return true
    if resize_pending:
        return false
    if not direct:
        if is_instance_valid(race):
            var state: Dictionary = race.call("telemetry")
            var page: String = state.get("screen", "")
            if state.get("loading", false) and not loading_captured:
                loading_captured = true
                _capture("loading")
            if track_picker:
                if stage == 6:
                    stage = 30
                    _tap(KEY_DOWN)
                    next_input = Time.get_ticks_msec() + 400
                    return false
                if stage == 30 and Time.get_ticks_msec() >= next_input:
                    stage = 31
                    _tap(KEY_ENTER)
                    return false
                if stage == 31 and page == "Challenge":
                    stage = 32
                    next_input = Time.get_ticks_msec() + 950
                    return false
                if stage == 32 and page == "Challenge" and Time.get_ticks_msec() >= next_input:
                    stage = 17
                    _tap(KEY_ENTER)
                    return false
                if tested_screens.has("select-track"):
                    print("HP2_MVP_RESIZE_PASS select-track")
                    quit(0)
                    return false
            var label: String = {
                "New Main Menu": "main-menu", "View Event Tree": "event-tree",
                "Select Car": "select-car", "Select Track": "select-track",
                "Pre Race Blurb": "pre-race-blurb", "Pre Race Stats": "pre-race-stats",
                "End Game Menu": "end-game-menu", "Race Results": "race-results",
            }.get(page, "")
            if page == "Cops On Mode Select" and stage == 29:
                if Time.get_ticks_msec() < next_input:
                    return false
                var item: int = state.get("selected_item", 0)
                if item == 0x52e039f6:
                    stage = 14
                    _tap(KEY_ENTER)
                    return false
                _tap(KEY_DOWN if item < 0x52e039f6 else KEY_UP)
                next_input = Time.get_ticks_msec() + 500
                return false
            var request: int = state.get("video_request", 0)
            var player := _find_video(root) if page == "Title Screen" and request in [1, 2] else null
            if player and player.visible and player.stream_position >= (0.75 if request == 1 else 2.0):
                label = "ea-logo-video" if request == 1 else "intro-video"
            if page == "Title Screen" and stage == 11 and state.get("video_request") == 0:
                label = "title-screen"
            if not label.is_empty() and not tested_screens.has(label):
                _resize_sweep(label, label == "main-menu")
                return false
            if pursuit and route == null and state.get("race_state") == 3 and state.get("track") == 13:
                for required in ["ea-logo-video", "intro-video", "title-screen", "main-menu", "event-tree",
                        "select-car", "pre-race-blurb", "pre-race-stats"]:
                    if not tested_screens.has(required):
                        return _fail("missing_resize_" + required)
                print("HP2_MVP_RESIZE_PASS pursuit ", tested_screens, " loading_capture=", loading_captured)
                quit(0)
                return false
        return super._process(delta)
    if Time.get_ticks_msec() > deadline:
        return _fail("direct_resize_timeout")
    if not is_instance_valid(race):
        race = root.get_node_or_null("NativeGame/RacePresentation")
        _dismiss_debug_console()
        return false
    var sample: Dictionary = race.call("telemetry")
    var screen: String = sample.get("screen", "")
    if stage == 0 and sample.get("race_state") == 3 and sample.get("step", 0) > 20:
        stage = 1
        _key(KEY_W, true)
        _resize_sweep("direct-hud")
    elif stage == 1 and sample.get("speed", 0) > 12.0:
        stage = 2
        _key(KEY_W, false)
        if check_pause_bindings:
            stage = 41
            _verify_pause_bindings()
        else:
            _tap(KEY_ESCAPE)
    elif stage == 2 and screen == "Pause Menu":
        var pointer := race.get_node_or_null("FrontendCanvas/FrontendPointer") as Control
        if not pointer or pointer.get_child_count() != 0:
            return _fail("frontend_pointer_controls")
        if pointer.mouse_default_cursor_shape != Control.CURSOR_ARROW:
            return _fail("frontend_pointer_cursor")
        stage = 3
        _resize_sweep("direct-pause", true)
    elif stage == 3:
        if mouse_pause:
            root.size = Vector2i(800, 600)
            stage = 33
            next_input = Time.get_ticks_msec() + 150
        else:
            stage = 4
            next_input = 0
    elif stage == 33 and Time.get_ticks_msec() >= next_input:
        if sample.has("debug_console_open"):
            _tap(KEY_QUOTELEFT)
            stage = 38
            next_input = Time.get_ticks_msec() + 250
            return false
        return _open_restart_popup()
    elif stage == 38 and Time.get_ticks_msec() >= next_input:
        if not sample.get("debug_console_open", false):
            return _fail("debug_console_did_not_open")
        _click_design(Vector2(270, 165))
        stage = 39
        next_input = Time.get_ticks_msec() + 350
    elif stage == 39 and Time.get_ticks_msec() >= next_input:
        if screen != "Pause Menu" or sample.get("race_state") != 6:
            return _fail("debug_console_mouse_passthrough")
        _dismiss_debug_console()
        stage = 40
        next_input = Time.get_ticks_msec() + 250
    elif stage == 40 and Time.get_ticks_msec() >= next_input:
        if sample.get("debug_console_open", false):
            return _fail("debug_console_did_not_close")
        return _open_restart_popup()
    elif stage == 4 and sample.get("selected_item") == 0x52e039f4:
        stage = 5
        _tap(KEY_ENTER)
        next_input = Time.get_ticks_msec() + 650
    elif stage == 4 and Time.get_ticks_msec() >= next_input:
        _tap(KEY_DOWN if sample.get("selected_item", 0) < 0x52e039f4 else KEY_UP)
        next_input = Time.get_ticks_msec() + 650
    elif stage == 5 and Time.get_ticks_msec() >= next_input:
        if sample.get("selected_item") != 0x52e039f4:
            return _fail("restart_selection")
        stage = 6
        _resize_sweep("direct-confirm")
    elif stage == 6:
        if mouse_pause:
            root.size = Vector2i(800, 600)
            stage = 34
            next_input = Time.get_ticks_msec() + 150
            return false
        return _finish_direct()
    elif stage == 34 and Time.get_ticks_msec() >= next_input:
        _click_design(Vector2(25, 25))
        stage = 35
        next_input = Time.get_ticks_msec() + 350
    elif stage == 35 and Time.get_ticks_msec() >= next_input:
        if screen != "Pause Menu" or sample.get("race_state") != 6:
            return _fail("modal_outside_click")
        _click_design(Vector2(475, 288))
        stage = 36
        next_input = Time.get_ticks_msec() + 650
    elif stage == 36 and Time.get_ticks_msec() >= next_input:
        _click_design(Vector2(270, 165))
        stage = 37
    elif stage == 37 and sample.get("race_state") == 3:
        return _finish_direct()
    return false

func _open_restart_popup() -> bool:
    _click_design(Vector2(270, 197))
    stage = 5
    next_input = Time.get_ticks_msec() + 650
    return false

func _verify_pause_bindings() -> void:
    const PAUSE := &"hp2_pause"
    root.size = Vector2i(800, 600)
    await create_timer(0.25).timeout
    if race.telemetry().has("debug_console_open"):
        await _tap(KEY_QUOTELEFT)
        await _tap(KEY_BACKSPACE)
        if not race.telemetry().debug_console_open or race.telemetry().race_state != 3:
            _fail("console_backspace_passthrough")
            return
        await _tap(KEY_QUOTELEFT)
    for key in [KEY_ESCAPE, KEY_BACKSPACE]:
        _key(key, true)
        await create_timer(0.7).timeout
        if race.telemetry().screen != "Pause Menu":
            _key(key, false)
            _fail("pause_key_or_repeat_%s" % key)
            return
        if key == KEY_ESCAPE:
            _key(KEY_BACKSPACE, true)
            _key(key, false)
            await create_timer(0.35).timeout
            if race.telemetry().screen != "Pause Menu":
                _key(KEY_BACKSPACE, false)
                _fail("combined_pause_keys")
                return
            _key(KEY_BACKSPACE, false)
        else:
            _key(key, false)
        await create_timer(0.25).timeout
        _click_design(Vector2(270, 165))
        await create_timer(0.7).timeout
        if race.telemetry().race_state != 3:
            _fail("resume_after_pause_key_%s" % key)
            return
    var original := InputMap.action_get_events(PAUSE)
    InputMap.action_erase_events(PAUSE)
    var mapped := InputEventKey.new()
    mapped.physical_keycode = KEY_P
    InputMap.action_add_event(PAUSE, mapped)
    await _tap(KEY_ESCAPE)
    await _tap(KEY_BACKSPACE)
    if race.telemetry().race_state != 3:
        _fail("unbound_pause_key")
        return
    await _tap(KEY_P)
    await create_timer(0.5).timeout
    InputMap.action_erase_events(PAUSE)
    for event in original:
        InputMap.action_add_event(PAUSE, event)
    if race.telemetry().screen != "Pause Menu":
        _fail("remapped_pause_key")
        return
    print("HP2_MVP_PAUSE_BINDINGS_PASS")
    stage = 2

func _finish_direct() -> bool:
    stage = 7
    print("HP2_MVP_RESIZE_PASS direct ", tested_screens)
    quit(0)
    return false

func _click_design(position: Vector2) -> void:
    var canvas := race.get_node("FrontendCanvas") as Node2D
    for down in [true, false]:
        var event := InputEventMouseButton.new()
        event.button_index = MOUSE_BUTTON_LEFT
        event.position = canvas.to_global(position)
        event.global_position = event.position
        event.pressed = down
        root.push_input(event)

func _capture(name: String) -> bool:
    if tested_screens.has(name):
        return true
    return await super._capture(name)

func _resize_sweep(name: String, maximize := false) -> bool:
    resize_pending = true
    var before: Dictionary = race.call("telemetry")
    var video: VideoStreamPlayer
    if name in ["ea-logo-video", "intro-video", "main-menu"]:
        video = _find_video(root)
    var original_video_rect := Rect2(video.position, video.size) if video else Rect2()
    var original_video_position := video.stream_position if video else 0.0
    var canvas := race.get_node_or_null("FrontendCanvas") as Node2D
    if canvas == null:
        return _resize_error("canvas_" + name)
    var original_size := root.size
    root.mode = Window.MODE_WINDOWED
    var sizes := SIZES.duplicate()
    if name == "select-car" or name == "select-track":
        # Capture the loaded preview after the initial screen fade.
        await create_timer(0.6).timeout
        sizes.append(Vector2i(1280, 900))
    for wanted in sizes:
        root.size = wanted
        var image: Image
        for attempt in range(5):
            await process_frame
            await RenderingServer.frame_post_draw
            image = root.get_texture().get_image()
            if root.size == wanted and image != null and image.get_size() == wanted:
                break
        if root.size != wanted or image == null or image.get_size() != wanted:
            return _resize_error("extent_" + name)
        var fitted_scale := minf(wanted.x / 640.0, wanted.y / 480.0)
        if not canvas.scale.is_equal_approx(Vector2.ONE * fitted_scale) or not canvas.position.is_equal_approx(
                (Vector2(wanted) - Vector2(640, 480) * fitted_scale) * 0.5):
            return _resize_error("centered_aspect_" + name)
        if name == "direct-hud":
            var mirror := canvas.get_node_or_null("RearViewMirror") as TextureRect
            var scene: Node3D = race.race_scene()
            var target := scene.get_node_or_null("RearViewTarget") as SubViewport if scene else null
            if not mirror or not mirror.visible or not target or mirror.texture != target.get_texture():
                return _resize_error("rear_view_target")
            var mirror_rect := mirror.get_global_rect()
            if not is_equal_approx(mirror_rect.get_center().x, wanted.x * 0.5):
                return _resize_error("rear_view_center")
            var expected_height := maxi(64, ceili(64 * fitted_scale))
            if target.size != Vector2i(roundi(expected_height * 2.88), expected_height):
                return _resize_error("rear_view_projection_size")
        if name == "select-car" or name == "select-track":
            var display := canvas.get_node_or_null("PreviewDisplay1") as TextureRect
            var preview := display.get_node_or_null("PreviewViewport") as SubViewport if display else null
            var pixels := Vector2i((Vector2(640, 480) * fitted_scale).ceil())
            if not preview or not display.visible or preview.size != pixels or display.size != Vector2(640, 480):
                return _resize_error("preview_resolution_" + name)
            if name == "select-car" and wanted == sizes.back():
                if not _has_race_preview_materials(preview):
                    return _resize_error("preview_race_materials")
                if not _has_car_preview_effects(preview):
                    return _resize_error("preview_effects")
                if not await _has_nebula_pixels(preview):
                    return _resize_error("preview_nebula_pixels")
                if pursuit and not await _verify_car_environment(preview):
                    return _resize_error("preview_car_environment")
            print("HP2_MVP_PREVIEW screen=", name, " window=", wanted, " render_target=", preview.size)
        if name in ["ea-logo-video", "intro-video", "main-menu"]:
            if not video or not video.visible or not Rect2(video.position, video.size).is_equal_approx(original_video_rect):
                return _resize_error("video_alignment_" + name)
            if name == "intro-video":
                if absf(video.size.x - 640.0) > 2.0 or not is_equal_approx(video.size.y, 304.0) or not is_equal_approx(video.position.y, 65.0):
                    return _resize_error("intro_authored_rect_%s_%s" % [video.position, video.size])
            elif not is_equal_approx(video.size.aspect(), 4.0 / 3.0):
                return _resize_error("video_aspect")
        if image.save_png(screenshot_dir.path_join("%s-%dx%d.png" % [name, wanted.x, wanted.y])) != OK:
            return _resize_error("screenshot_" + name)
    if maximize:
        root.mode = Window.MODE_MAXIMIZED
        await create_timer(0.35).timeout
        await RenderingServer.frame_post_draw
        if root.mode != Window.MODE_MAXIMIZED:
            return _resize_error("maximize_" + name)
    root.mode = Window.MODE_WINDOWED
    root.size = original_size
    if initial_mode == Window.MODE_MAXIMIZED:
        root.mode = initial_mode
    await process_frame
    var after: Dictionary = race.call("telemetry")
    for key in ["screen", "generation", "video_request"]:
        if before.get(key) != after.get(key):
            return _resize_error("state_" + name + "_" + key)
    if video and video.stream_position < original_video_position:
        return _resize_error("video_restarted_" + name)
    tested_screens[name] = sizes
    print("HP2_MVP_RESIZE screen=", name, " sizes=", sizes)
    resize_pending = false
    return true

func _find_video(node: Node) -> VideoStreamPlayer:
    if node is VideoStreamPlayer:
        return node
    for child in node.get_children():
        var player := _find_video(child)
        if player:
            return player
    return null

func _has_race_preview_materials(preview: SubViewport) -> bool:
    var surfaces := 0
    var texture_keys := {}
    for node in preview.get_children():
        if node is Light3D:
            return false
        if not node is MeshInstance3D or not node.visible or not node.mesh or node.get_meta("source_reflected_car", false):
            continue
        for i in range(node.mesh.get_surface_count()):
            var surface: Material = node.mesh.surface_get_material(i)
            if surface is ShaderMaterial and surface.shader.resource_path in [
                    "res://shader/car_paint.gdshader", "res://shader/car_paint_glass.gdshader",
                    "res://shader/frontend_platform.gdshader"]:
                var paint: Texture2D = surface.get_shader_parameter("base_image")
                if not paint:
                    return false
                if surface.shader.resource_path != "res://shader/frontend_platform.gdshader" and (
                        node.mesh.surface_get_arrays(i)[Mesh.ARRAY_CUSTOM0].is_empty()):
                    return false
                texture_keys[paint.resource_name] = true
                surfaces += 1
                continue
            var material := surface as StandardMaterial3D
            if surface is ShaderMaterial and surface.shader.resource_path.begins_with("res://shader/mesh_surface_"):
                var albedo: Texture2D = surface.get_shader_parameter("albedo_texture")
                if albedo:
                    texture_keys[albedo.resource_name] = true
                    surfaces += 1
                    continue
            if material and material.albedo_texture:
                texture_keys[material.albedo_texture.resource_name] = true
            if not material or material.shading_mode != BaseMaterial3D.SHADING_MODE_UNSHADED:
                return false
            var arrays: Array = node.mesh.surface_get_arrays(i)
            if not material.vertex_color_use_as_albedo or not material.vertex_color_is_srgb or arrays[Mesh.ARRAY_COLOR].is_empty():
                return false
            surfaces += 1
    print("HP2_MVP_PREVIEW unshaded_surfaces=", surfaces, " texture_keys=", texture_keys.keys())
    # The first Ultimate Racer event selects the Lotus Elise. Check its
    # resolved off-state headlamp and brake lamp texture resources.
    if pursuit and (not texture_keys.has("544547642") or not texture_keys.has("199674477")):
        return false
    return surfaces > 0

func _has_car_preview_effects(preview: SubViewport) -> bool:
    var reflections := 0
    var platform := false
    for node in preview.get_children():
        if node is MeshInstance3D and node.visible and node.mesh and node.mesh.has_meta("hp2_frontend_platform"):
            platform = true
        if node is MeshInstance3D and node.visible and node.get_meta("source_reflected_car", false):
            if node.mesh.get_surface_count() > 0:
                reflections += 1
            for i in range(node.mesh.get_surface_count()):
                var surface: Material = node.mesh.surface_get_material(i)
                if surface is ShaderMaterial and surface.shader.resource_path == "res://shader/car_paint.gdshader":
                    if int(surface.get_shader_parameter("paint_flags")) & 0x100:
                        return false
    var shadow_indices := 0
    var nebula := false
    for node in preview.find_children("*", "MeshInstance3D", true, false):
        if node.visible and node.material_override is ShaderMaterial and (
                node.material_override.shader.resource_path == "res://shader/car_shadow.gdshader"):
            shadow_indices += node.mesh.surface_get_arrays(0)[Mesh.ARRAY_INDEX].size()
    for node in preview.find_children("*", "GPUParticles3D", true, false):
        if not node.visible or node.get_meta("source_effect_key", 0) != 0xcdb36848: # XE_SelectCarNebula
            continue
        var curve: PackedVector4Array = node.material_override.get_shader_parameter("color_curve")
        var middle := ((curve[0] * 0.5 + curve[1]) * 0.5 + curve[2]) * 0.5 + curve[3]
        nebula = middle.z > middle.x and middle.w > 0.0
        print("HP2_MVP_NEBULA curve_mid=", middle)
    print("HP2_MVP_PREVIEW_EFFECTS reflections=", reflections, " shadow_indices=", shadow_indices,
        " platform=", platform, " nebula=", nebula)
    return reflections > 0 and shadow_indices == 108 and platform and nebula

func _has_nebula_pixels(preview: SubViewport) -> bool:
    var nebula: GPUParticles3D
    for node in preview.find_children("*", "GPUParticles3D", true, false):
        if node.visible and node.get_meta("source_effect_key", 0) == 0xcdb36848:
            nebula = node
            break
    if not nebula:
        return false
    # Hold the source frame so that changing the visibility isolates the nebula.
    race.set_process(false)
    await process_frame
    await RenderingServer.frame_post_draw
    var visible := preview.get_texture().get_image()
    nebula.hide()
    await process_frame
    await RenderingServer.frame_post_draw
    var hidden := preview.get_texture().get_image()
    nebula.show()
    race.set_process(true)
    var blue_samples := 0
    for y in range(0, visible.get_height(), 4):
        for x in range(0, visible.get_width(), 4):
            var pixel := visible.get_pixel(x, y)
            var background := hidden.get_pixel(x, y)
            if pixel.b > background.b + 1.0 / 255.0 and pixel.b > maxf(pixel.r, pixel.g):
                blue_samples += 1
    print("HP2_MVP_NEBULA blue_samples=", blue_samples)
    return blue_samples > 20

func _verify_car_environment(preview: SubViewport) -> bool:
    var materials := {}
    var glass := false
    for node in preview.get_children():
        if not node is MeshInstance3D or not node.visible or not node.mesh:
            continue
        for i in range(node.mesh.get_surface_count()):
            var material: Material = node.mesh.surface_get_material(i)
            if not material is ShaderMaterial or material.shader.resource_path not in [
                    "res://shader/car_paint.gdshader", "res://shader/car_paint_glass.gdshader"]:
                continue
            var flags: int = material.get_shader_parameter("paint_flags")
            if not flags & 0x100:
                continue
            var environment: Texture2D = material.get_shader_parameter("reflection_image")
            if not environment or environment.resource_name != "1322022556": # CARSELECTENVMAP
                return false
            var base: Texture2D = material.get_shader_parameter("base_image")
            if material.shader.resource_path.ends_with("_glass.gdshader") and base.resource_name == "2202763287":
                glass = true # WINDOW
            materials[material] = flags
    if not glass or materials.is_empty():
        return false
    # Hold the source frame so that only the sphere-map pass changes.
    race.set_process(false)
    await process_frame
    await RenderingServer.frame_post_draw
    var with_map := preview.get_texture().get_image()
    for material in materials:
        material.set_shader_parameter("paint_flags", int(materials[material]) & ~0x100)
    await process_frame
    await RenderingServer.frame_post_draw
    var without_map := preview.get_texture().get_image()
    for material in materials:
        material.set_shader_parameter("paint_flags", materials[material])
    race.set_process(true)
    if with_map.save_png(screenshot_dir.path_join("elise-environment-on.png")) != OK or (
            without_map.save_png(screenshot_dir.path_join("elise-environment-off.png")) != OK):
        return false
    var changed := 0
    for y in range(0, with_map.get_height(), 4):
        for x in range(0, with_map.get_width(), 4):
            var on := with_map.get_pixel(x, y)
            var off := without_map.get_pixel(x, y)
            if maxf(on.r - off.r, maxf(on.g - off.g, on.b - off.b)) > 6.0 / 255.0:
                changed += 1
    print("HP2_MVP_CAR_ENVIRONMENT materials=", materials.size(), " window=", glass, " changed_samples=", changed)
    return changed > 100

func _resize_error(reason: String) -> bool:
    resize_failed = true
    _fail(reason)
    return false
