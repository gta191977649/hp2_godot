extends SceneTree

var deadline := Time.get_ticks_msec() + 60000
var started := false

func _initialize() -> void:
    call_deferred("change_scene_to_file", "res://native_game.tscn")

func _process(_delta: float) -> bool:
    if Time.get_ticks_msec() > deadline:
        push_error("Wheel blur smoke timed out")
        quit(1)
        return true
    var race := root.get_node_or_null("NativeGame/RacePresentation")
    if not started and race and race.race_scene() and race.telemetry().get("step", 0) > 60:
        started = true
        _check.call_deferred(race)
    return false

func _check(race: Node) -> void:
    var scene: Node3D = race.race_scene()
    var player: Node3D
    for child in scene.get_children():
        if child.get_meta("hp2_player_vehicle", false):
            player = child
            break
    if not player:
        _fail("Missing player vehicle")
        return
    var overlays: Array[MeshInstance3D] = []
    for wheel in range(4):
        var overlay := player.get_node_or_null("WheelBlur%d" % wheel) as MeshInstance3D
        if overlay:
            var material := overlay.mesh.surface_get_material(0) as ShaderMaterial
            if not material or material.shader.resource_path != "res://shader/wheel_blur.gdshader":
                _fail("Wheel blur did not bind the authored overlay material")
                return
            overlays.append(overlay)
    if overlays.is_empty():
        _fail("Fixture car has no authored wheel blur model")
        return
    for overlay in overlays:
        if overlay.visible and float(overlay.get_instance_shader_parameter("hp2_wheel_blur_alpha")) > 1.0:
            _fail("Wheel blur was active before accelerating")
            return

    Input.action_press("hp2_accelerate")
    var peak := 0.0
    var separate_pose := false
    for frame in range(800):
        await process_frame
        # Turn briefly so the chase camera sees the face of a rim.
        if frame == 180: Input.action_press("hp2_steer_left")
        if frame == 300: Input.action_release("hp2_steer_left")
        for overlay in overlays:
            if not overlay.is_visible_in_tree():
                continue
            var alpha := float(overlay.get_instance_shader_parameter("hp2_wheel_blur_alpha"))
            peak = maxf(peak, alpha)
            var wheel := player.get_node("Wheel" + str(overlay.name).trim_prefix("WheelBlur")) as MeshInstance3D
            separate_pose = separate_pose or not wheel.basis.is_equal_approx(overlay.basis)
        if peak >= 89.9 and separate_pose:
            break
    Input.action_release("hp2_accelerate")
    Input.action_release("hp2_steer_left")
    if peak < 89.9 or peak > 90.0 or not separate_pose:
        _fail("Wheel blur strength or independent pose failed, peak=%s state=%s" % [peak, race.telemetry()])
        return

    race.set_process(false)
    var viewport := scene.get_viewport()
    var race_camera := viewport.get_camera_3d()
    var shown := overlays.filter(func(overlay: MeshInstance3D) -> bool: return overlay.visible)
    var target: MeshInstance3D = shown[0]
    var camera := Camera3D.new()
    scene.add_child(camera)
    var side := player.global_basis.z.normalized()
    if (race_camera.global_position - target.global_position).dot(side) < 0.0:
        side = -side
    camera.look_at_from_position(target.global_position + side * 1.4 + Vector3.UP * 0.25, target.global_position)
    camera.cull_mask = 3
    camera.make_current()
    await process_frame
    await RenderingServer.frame_post_draw
    var with_blur := viewport.get_texture().get_image()
    for overlay in shown:
        overlay.hide()
    await process_frame
    await RenderingServer.frame_post_draw
    var without_blur := viewport.get_texture().get_image()
    for overlay in shown:
        overlay.show()
    var changed := 0
    for overlay in shown:
        if camera.is_position_behind(overlay.global_position):
            continue
        var center := Vector2i(camera.unproject_position(overlay.global_position))
        for y in range(maxi(center.y - 110, 0), mini(center.y + 110, with_blur.get_height()), 2):
            for x in range(maxi(center.x - 110, 0), mini(center.x + 110, with_blur.get_width()), 2):
                var before := with_blur.get_pixel(x, y)
                var after := without_blur.get_pixel(x, y)
                if absf(before.r - after.r) + absf(before.g - after.g) + absf(before.b - after.b) > 0.03:
                    changed += 1
    race_camera.make_current()
    camera.queue_free()
    race.set_process(true)
    if changed < 5:
        var material := shown[0].mesh.surface_get_material(0) as ShaderMaterial
        var texture: Texture2D = material.get_shader_parameter("wheel_blur_texture")
        var texture_image := texture.get_image()
        var maximum_alpha := 0.0
        for y in range(texture_image.get_height()):
            for x in range(texture_image.get_width()):
                maximum_alpha = maxf(maximum_alpha, texture_image.get_pixel(x, y).a)
        _fail("Active overlay made no rim pixels change, count=%s alpha_test=%s texture_alpha=%s" %
            [shown.size(), material.get_shader_parameter("alpha_test"), maximum_alpha])
        return

    Input.action_press("hp2_brake")
    var reduced := false
    for frame in range(650):
        await process_frame
        if absf(float(race.telemetry().get("speed", 100.0))) < 1.0:
            reduced = true
            for overlay in overlays:
                if overlay.visible and float(overlay.get_instance_shader_parameter("hp2_wheel_blur_alpha")) > 10.0:
                    reduced = false
            if reduced:
                break
    Input.action_release("hp2_brake")
    if not reduced:
        _fail("Wheel blur did not fade while braking")
        return
    print("HP2_MVP_WHEEL_BLUR_PASS peak=", peak, " authored_overlays=", overlays.size(), " changed_pixels=", changed)
    quit(0)

func _fail(reason: String) -> void:
    push_error(reason)
    quit(1)
