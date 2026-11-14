extends SceneTree

var deadline: int
var track := 61
var checking := false
var console_toggled := false

func _initialize() -> void:
    for arg in OS.get_cmdline_user_args():
        if arg.begins_with("--race-track="):
            track = arg.trim_prefix("--race-track=").to_int()
    deadline = Time.get_ticks_msec() + 45000
    call_deferred("change_scene_to_file", "res://native_game.tscn")

func _process(_delta: float) -> bool:
    if Time.get_ticks_msec() > deadline:
        push_error("Scenery animation smoke timed out")
        quit(1)
    if checking:
        return false
    var race := root.get_node_or_null("NativeGame/RacePresentation")
    if not race:
        return false
    if race.telemetry().get("debug_console_open", false):
        if not console_toggled:
            console_toggled = true
            _tap(KEY_QUOTELEFT)
        return false
    if race.telemetry().get("race_state") == 3 and race.telemetry().get("step", 0) >= 90:
        checking = true
        _check.call_deferred(race)
    return false

func _check(race: Node) -> void:
    var scene := race.get_node("RaceScene")
    var choices := {}
    var counts := {}
    for node in scene.get_children():
        if not node is MeshInstance3D and not node is MultiMeshInstance3D:
            continue
        var mesh: ArrayMesh = node.mesh if node is MeshInstance3D else node.multimesh.mesh
        if not mesh:
            continue
        for kind in ["morph", "rigid", "uv_scroll", "texture_sequence"]:
            if mesh.get_meta("hp2_" + kind, false):
                counts[kind] = counts.get(kind, 0) + 1
                if not choices.has(kind) or (kind == "rigid" and mesh.resource_name == "WATERWHEEL") or (
                    kind in ["uv_scroll", "texture_sequence"] and mesh.resource_name != "WATER" and
                    _mesh(choices[kind]).resource_name == "WATER"):
                    choices[kind] = node
    print("SCENERY_ANIMATION track=", track, " counts=", counts)
    var primary := "morph" if track == 61 else "rigid"
    if not choices.has(primary):
        push_error("Missing authored " + primary)
        quit(1)
        return

    if track == 61 and not await _check_pause(race):
        push_error("The source animation clock did not freeze and resume")
        quit(1)
        return
    race.set_process(false)
    var camera := scene.get_viewport().get_camera_3d()
    for kind in choices:
        var node: GeometryInstance3D = choices[kind]
        var mesh := _mesh(node)
        var pose: Transform3D = node.global_transform
        if node is MultiMeshInstance3D:
            pose *= node.multimesh.get_instance_transform(0)
        var bounds := mesh.get_aabb()
        var center := pose * bounds.get_center()
        var radius := maxf(0.75, bounds.size.length() * 0.65)
        camera.global_position = center + Vector3(0.75, 0.3, 1.0) * radius
        camera.look_at(center)
        # Show just the real placement. Other models and particles keep their visibility.
        var mask := node.layers
        var camera_mask := camera.cull_mask
        node.layers = 1 << 19
        camera.cull_mask = node.layers
        var phase := 0.0
        for i in mesh.get_surface_count():
            var material := mesh.surface_get_material(i) as ShaderMaterial
            if material and kind in ["rigid", "morph"]:
                var start = material.get_shader_parameter("animation_start")
                if start is int:
                    phase = start & 16383
                    break
        var first := await _capture(phase, 1.0, phase)
        var mesh_id := mesh.get_instance_id()
        var frozen := await _capture(phase, 1.0, phase)
        var fractional := await _capture(phase + 0.65, 1.0, phase)
        var later := await _capture(phase + 35.0, 2.35, phase + 35.0)
        if kind == "uv_scroll":
            later = await _capture(phase, 4.35, phase)
        elif kind == "texture_sequence":
            for frame in [15.0, 31.0, 43.0, 62.0, 90.0]:
                later = await _capture(phase, 1.0, frame)
                if _pixels(first, later) >= 8:
                    break
        node.layers = mask
        camera.cull_mask = camera_mask
        var steady := _pixels(first, frozen)
        var subframe := _pixels(first, fractional)
        var whole := _pixels(first, later)
        if _mesh(node).get_instance_id() != mesh_id or steady != 0:
            push_error("Fixed clock pixels or resident mesh changed")
            quit(1)
            return
        print("SCENERY_PIXELS ", kind, " ", mesh.resource_name, " frozen=", steady,
            " fractional=", subframe, " later=", whole)
        var path := OS.get_environment("HP2_MVP_SCREENSHOT_DIR")
        if not path.is_empty():
            first.save_png(path.path_join("%s-%s-first.png" % [track, kind]))
            later.save_png(path.path_join("%s-%s-later.png" % [track, kind]))
        if (kind == primary and (whole < 8 or subframe < 4)) or (
            track in [12, 32] and kind in ["uv_scroll", "texture_sequence"] and whole < 8):
            push_error("Authored motion or subframe interpolation is missing")
            quit(1)
            return
    quit(0)

func _check_pause(race: Node) -> bool:
    Input.action_press("hp2_pause")
    await create_timer(0.12).timeout
    Input.action_release("hp2_pause")
    for i in 60:
        await process_frame
        if race.telemetry().get("race_state") == 6:
            break
    if race.telemetry().get("race_state") != 6:
        print("SCENERY_PAUSE missing pause state=", race.telemetry().get("race_state"))
        return false
    for i in 3:
        await process_frame
    var frozen: Vector4 = race.telemetry()["animation_clock"]
    for i in 12:
        await process_frame
    var after: Vector4 = race.telemetry()["animation_clock"]
    Input.action_press("hp2_menu_confirm")
    await create_timer(0.12).timeout
    Input.action_release("hp2_menu_confirm")
    for i in 90:
        await process_frame
        var resumed: Vector4 = race.telemetry()["animation_clock"]
        if race.telemetry().get("race_state") == 3 and resumed.x > after.x:
            print("SCENERY_PAUSE frozen=", frozen.x, " resumed=", resumed.x)
            return frozen.x == after.x and frozen.y == after.y
    print("SCENERY_PAUSE missing resume state=", race.telemetry().get("race_state"), " frozen=", frozen, " after=", after)
    return false

func _tap(code: Key) -> void:
    var event := InputEventKey.new()
    event.keycode = code
    event.physical_keycode = code
    event.pressed = true
    Input.parse_input_event(event)
    await create_timer(0.07).timeout
    event = event.duplicate()
    event.pressed = false
    Input.parse_input_event(event)

func _mesh(node: GeometryInstance3D) -> ArrayMesh:
    return node.mesh if node is MeshInstance3D else node.multimesh.mesh

func _capture(tick: float, texture_time: float, real_tick: float) -> Image:
    RenderingServer.global_shader_parameter_set("hp2_animation_clock", Vector4(tick, texture_time, real_tick, real_tick))
    await process_frame
    await RenderingServer.frame_post_draw
    return root.get_texture().get_image()

func _pixels(a: Image, b: Image) -> int:
    var count := 0
    for y in range(0, a.get_height(), 2):
        for x in range(0, a.get_width(), 2):
            var d := a.get_pixel(x, y) - b.get_pixel(x, y)
            if absf(d.r) + absf(d.g) + absf(d.b) > 0.09:
                count += 1
    return count
