extends SceneTree

var deadline := Time.get_ticks_msec() + 45000
var checking := false
var nitro_run := OS.get_environment("HP2_MVP_TEST_NITRO") == "1"
var profile_run := OS.get_environment("HP2_MVP_TEST_POST_PROFILE") == "1"

func _initialize() -> void:
    var fps := OS.get_environment("HP2_MVP_TEST_POST_FPS").to_int()
    if fps > 0:
        Engine.max_fps = fps
    call_deferred("change_scene_to_file", "res://native_game.tscn")

func _process(_delta: float) -> bool:
    if Time.get_ticks_msec() > deadline:
        push_error("Postprocessing smoke timed out")
        quit(1)
    if checking:
        return false
    var race := root.get_node_or_null("NativeGame/RacePresentation")
    if race and race.telemetry().get("debug_console_open", false):
        var event := InputEventKey.new()
        event.physical_keycode = KEY_QUOTELEFT
        event.keycode = KEY_QUOTELEFT
        event.pressed = true
        Input.parse_input_event(event)
        return false
    if race and race.telemetry().get("race_state") == 3 and race.telemetry().get("step", 0) > 70:
        checking = true
        if nitro_run:
            _check_nitro.call_deferred(race)
        elif profile_run:
            _profile.call_deferred(race)
        else:
            _check.call_deferred(race)
    return false

func _profile(race: Node) -> void:
    race.set_process(false)
    var effect: CompositorEffect = race.race_scene().get_viewport().get_camera_3d().compositor.compositor_effects[0]
    for mode in [0, 5, 3]:
        effect.call("set_test_mode", mode)
        effect.enabled = mode != 0
        await create_timer(0.5).timeout
        print("RACE_POST_PROFILE_BEGIN ", mode)
        await create_timer(2.5).timeout
        print("RACE_POST_PROFILE_END ", mode)
    quit()

func _check_nitro(race: Node) -> void:
    var path := OS.get_environment("HP2_PHOTO_DIR")
    Input.action_press("hp2_accelerate")
    await create_timer(0.8).timeout
    var before := await _frame()
    Input.action_press("hp2_nitro")
    while not race.telemetry().get("radial_blur_active", false):
        await process_frame
    var onset: int = race.telemetry().step
    await create_timer(0.25).timeout
    var during := await _frame()
    Input.action_release("hp2_nitro")
    while race.telemetry().get("radial_blur_active", false):
        await process_frame
    var after := await _frame()
    Input.action_release("hp2_accelerate")
    if not path.is_empty():
        before.save_png(path.path_join("nitro-before.png"))
        during.save_png(path.path_join("nitro-during.png"))
        after.save_png(path.path_join("nitro-after.png"))
    print("RACE_POST_NITRO_PASS track=", race.telemetry().track, " fps_limit=", Engine.max_fps,
        " onset=", onset, " stopped=", race.telemetry().step)
    quit(0)

func _check(race: Node) -> void:
    race.set_process(false)
    var scene: Node3D = race.race_scene()
    var camera: Camera3D = scene.get_viewport().get_camera_3d()
    var effect: CompositorEffect = camera.compositor.compositor_effects[0]
    var native := await _frame()
    var atlas := scene.get_node_or_null("CarReflectionAtlas") as SubViewport
    if not atlas or atlas.size != Vector2i(256, 256):
        push_error("Missing race paint reflection atlas")
        quit(1)
        return
    var atlas_image := atlas.get_texture().get_image()
    var painted := 0
    for y in range(8, 248, 8):
        for x in range(8, 248, 8):
            if atlas_image.get_pixel(x, y).a > 0.1:
                painted += 1
    if painted < 100:
        push_error("Race paint atlas has no remapped face coverage")
        quit(1)
        return
    print("RACE_PAINT_ATLAS_PASS samples=", painted)
    var paint_surfaces := 0
    for vehicle in scene.get_children():
        if not vehicle is Node3D:
            continue
        for part in vehicle.get_children():
            if not part is MeshInstance3D or not part.mesh or not part.mesh.has_meta("hp2_car_paint"):
                continue
            for surface in range(part.mesh.get_surface_count()):
                var material: Material = part.mesh.surface_get_material(surface)
                if material is ShaderMaterial and material.shader.resource_path == "res://shader/car_paint.gdshader":
                    paint_surfaces += 1
    if paint_surfaces == 0:
        push_error("Race vehicles did not bind the authored paint material")
        quit(1)
        return
    print("RACE_PAINT_MATERIAL_PASS surfaces=", paint_surfaces)
    effect.enabled = false
    var unprocessed := await _frame()
    print("RACE_POST_PIXELS authored_weather=", _difference(native, unprocessed))
    var path := OS.get_environment("HP2_PHOTO_DIR")
    for mode in [1, 2, 4, 3]:
        camera.position += Vector3.RIGHT * 0.35
        effect.enabled = false
        var before := await _frame()
        effect.call("set_test_mode", mode)
        await _frame()
        if mode == 3:
            camera.position += Vector3.RIGHT * 1.0
        var after := await _frame()
        if mode == 3:
            effect.enabled = false
            before = await _frame()
        var changed := _difference(before, after)
        print("RACE_POST_PIXELS mode=", mode, " changed=", changed)
        if not path.is_empty():
            before.save_png(path.path_join("post-%s-before.png" % mode))
            after.save_png(path.path_join("post-%s-after.png" % mode))
        if changed < 20:
            push_error("Missing post effect %s" % mode)
            quit(1)
            return
    effect.call("set_test_mode", 9)
    var depth := await _frame()
    var dark := false
    var bright := false
    for y in range(12, depth.get_height() - 12, 6):
        for x in range(12, depth.get_width() - 12, 6):
            var pixel := depth.get_pixel(x, y)
            if absf(pixel.r - pixel.g) < 0.01 and absf(pixel.r - pixel.b) < 0.01:
                dark = dark or pixel.r < 0.01
                bright = bright or pixel.r > 0.035
    if not dark or not bright:
        push_error("Resolved reverse-Z buffer did not distinguish scene geometry and far depth")
        quit(1)
        return
    print("RACE_POST_NATIVE_DEPTH_PASS")
    effect.call("set_test_mode", 11)
    var legacy := await _frame()
    effect.call("set_test_mode", 10)
    var direct := await _frame()
    if _difference(legacy, direct) != 0:
        push_error("Direct depth fog differs from the legacy canvas pass")
        quit(1)
        return
    print("RACE_POST_DIRECT_FOG_PASS")
    if not await _check_history(effect):
        quit(1)
        return
    quit(0)

func _check_history(effect: CompositorEffect) -> bool:
    # Odd dimensions place one actual pixel at the quadrant center.
    root.size = Vector2i(961, 721)
    await _frame()
    for mode in [7, 8]:
        effect.call("set_test_mode", 6)
        await _frame()
        effect.call("set_test_mode", mode)
        var image := await _frame()
        var size := image.get_size()
        var right := size.x - 1
        var bottom := size.y - 1
        var points := [Vector2i(0, 0), Vector2i(right, 0), Vector2i(0, bottom),
            Vector2i(right, bottom), Vector2i(size.x / 2, size.y / 2),
            Vector2i(size.x / 5, size.y / 5), Vector2i(size.x / 3, size.y / 3)]
        for point: Vector2i in points:
            var uv := (Vector2(point) + Vector2(0.5, 0.5)) / Vector2(size)
            var q := Vector2.ONE - (2.0 * uv - Vector2.ONE).abs()
            var vertex_alpha := floorf(204.0 * (1.0 - maxf(0.0, q.x + q.y - 1.0)))
            var weight := floorf(63.0 * vertex_alpha / 128.0) / 128.0
            if mode == 8:
                weight = sqrt(weight)
            var history_x := lerpf(0.5 / 512.0, 1.0 - 0.5 / 512.0, uv.x) * size.x - 0.5
            var old_red := floorf(clampf(minf(history_x, right - history_x), 0, 1) * 255.0)
            var expected := Vector2(floorf(old_red * weight), 255.0 + floorf(-255.0 * weight))
            var pixel := image.get_pixelv(point)
            var measured := Vector2(pixel.r, pixel.g) * 255.0
            if (measured - expected).abs().length() > 5.0:
                push_error("GS history mode %s at %s: got %s, expected %s" %
                    [mode, point, measured, expected])
                return false
        print("RACE_POST_GS_HISTORY_PASS mode=", mode, " pixels=", points.size())
    root.size = Vector2i(801, 601)
    # The first resized frame publishes its new GPU textures. Check the
    # first processed image produced with those bindings.
    await _frame()
    var resized := await _frame()
    if resized.get_pixel(0, 0).g < 0.98:
        push_error("Resize frame did not show a clean history sample: %s" % resized.get_pixel(0, 0))
        return false
    effect.call("set_test_mode", 7)
    var reset := await _frame()
    if reset.get_pixel(0, 0).g < 0.98:
        push_error("Old history survived an explicit reset")
        return false
    print("RACE_POST_GS_HISTORY_RESET_PASS")
    return true

func _frame() -> Image:
    await process_frame
    await RenderingServer.frame_post_draw
    return root.get_texture().get_image()

func _difference(a: Image, b: Image) -> int:
    var count := 0
    for y in range(0, a.get_height(), 3):
        for x in range(0, a.get_width(), 3):
            var d := a.get_pixel(x,y) - b.get_pixel(x,y)
            if absf(d.r) + absf(d.g) + absf(d.b) > 0.045:
                count += 1
    return count
