extends SceneTree

var deadline := Time.get_ticks_msec() + 60000
var started := false

func _initialize() -> void:
    if OS.get_environment("HP2_MVP_TEST_DRIVER_INTERPOLATION") == "1":
        DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
        Engine.max_fps = 120
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

    var driver := player.get_node_or_null("RaceDriver1") as MeshInstance3D
    if not driver or not driver.is_visible_in_tree():
        _fail("The player driver's authored mesh is missing")
        return
    var skeleton := driver.get_node_or_null("DriverPose") as Skeleton3D
    if not skeleton or skeleton.get_bone_count() < 30:
        _fail("The player driver has no authored skeleton")
        return
    var neutral_pose: Array[Transform3D] = []
    for bone in range(skeleton.get_bone_count()):
        neutral_pose.append(skeleton.get_bone_pose(bone))

    Input.action_press("hp2_accelerate")
    Input.action_press("hp2_steer_left")
    var peak := 0.0
    var separate_pose := false
    var changed_bones := 0
    var steering_subframes := 0
    var release_subframes := 0
    var last_step := -1
    var last_hand := Transform3D.IDENTITY
    var hand_deltas: Array[float] = []
    for frame in range(800):
        await process_frame
        var step: int = race.telemetry().step
        var hand := skeleton.get_bone_pose(25)
        if last_step >= 0 and frame <= 72:
            var distance := hand.origin.distance_to(last_hand.origin)
            var angle := hand.basis.get_rotation_quaternion().angle_to(last_hand.basis.get_rotation_quaternion())
            hand_deltas.append(distance + 0.25 * angle)
        if step == last_step and not hand.is_equal_approx(last_hand):
            if frame <= 36:
                steering_subframes += 1
            else:
                release_subframes += 1
        last_step = step
        last_hand = hand
        if frame == 36:
            Input.action_release("hp2_steer_left")
            for bone in range(neutral_pose.size()):
                if not skeleton.get_bone_pose(bone).is_equal_approx(neutral_pose[bone]):
                    changed_bones += 1
        for overlay in overlays:
            if not overlay.is_visible_in_tree():
                continue
            var alpha := float(overlay.get_instance_shader_parameter("hp2_wheel_blur_alpha"))
            peak = maxf(peak, alpha)
            var wheel := player.get_node("Wheel" + str(overlay.name).trim_prefix("WheelBlur")) as MeshInstance3D
            separate_pose = separate_pose or not wheel.basis.is_equal_approx(overlay.basis)
        if frame >= 72 and peak >= 89.9 and separate_pose:
            break
    Input.action_release("hp2_accelerate")
    Input.action_release("hp2_steer_left")
    if changed_bones < 3:
        _fail("Steering did not change the authored driver pose")
        return
    if OS.get_environment("HP2_MVP_TEST_DRIVER_INTERPOLATION") == "1" and (
        steering_subframes < 8 or release_subframes < 8 or hand_deltas.max() > 0.20
    ):
        _fail("Driver interpolation stalled or jumped: steering=%s release=%s max_motion=%s" %
            [steering_subframes, release_subframes, hand_deltas.max()])
        return
    print("HP2_DRIVER_SUBFRAMES steering=", steering_subframes, " release=", release_subframes)
    hand_deltas.sort()
    print("HP2_DRIVER_FRAME_MOTION p95=", hand_deltas[int(hand_deltas.size() * 0.95)],
        " max=", hand_deltas.back(), " samples=", hand_deltas.size())
    if peak < 89.9 or peak > 90.0 or not separate_pose:
        _fail("Wheel blur strength or independent pose failed, peak=%s state=%s" % [peak, race.telemetry()])
        return

    race.set_process(false)
    var viewport := scene.get_viewport()
    var race_camera := viewport.get_camera_3d()
    var shown := overlays.filter(func(overlay: MeshInstance3D) -> bool: return overlay.visible)
    var camera := Camera3D.new()
    scene.add_child(camera)
    var axle := player.global_basis.z.normalized()
    camera.cull_mask = 3
    camera.make_current()
    var changed := 0
    for sign in [-1.0, 1.0]:
        var side: Vector3 = axle * sign
        var side_wheels := shown.filter(func(overlay: MeshInstance3D) -> bool:
            return (overlay.global_position - player.global_position).dot(side) > 0.0)
        if side_wheels.is_empty():
            _fail("Wheel blur missing on one side of the moving car")
            return
        var target: MeshInstance3D = side_wheels[0]
        camera.look_at_from_position(target.global_position + side * 1.4 + Vector3.UP * 0.25, target.global_position)
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
        var side_pixels := 0
        var center := Vector2i(camera.unproject_position(target.global_position))
        for y in range(maxi(center.y - 110, 0), mini(center.y + 110, with_blur.get_height()), 2):
            for x in range(maxi(center.x - 110, 0), mini(center.x + 110, with_blur.get_width()), 2):
                var before := with_blur.get_pixel(x, y)
                var after := without_blur.get_pixel(x, y)
                if absf(before.r - after.r) + absf(before.g - after.g) + absf(before.b - after.b) > 0.03:
                    side_pixels += 1
        if side_pixels < 5:
            _fail("No visible wheel blur from side %s" % sign)
            return
        changed += side_pixels
    var seat := driver.global_position
    var front := player.global_basis.x.normalized()
    camera.look_at_from_position(seat + front * 2.8 + Vector3.UP * 1.2 + axle * 0.4, seat)
    await process_frame
    await RenderingServer.frame_post_draw
    var with_driver := viewport.get_texture().get_image()
    driver.hide()
    await process_frame
    await RenderingServer.frame_post_draw
    var without_driver := viewport.get_texture().get_image()
    driver.show()
    var driver_pixels := 0
    var seat_pixel := Vector2i(camera.unproject_position(seat))
    for y in range(maxi(seat_pixel.y - 180, 0), mini(seat_pixel.y + 180, with_driver.get_height()), 2):
        for x in range(maxi(seat_pixel.x - 180, 0), mini(seat_pixel.x + 180, with_driver.get_width()), 2):
            var difference := with_driver.get_pixel(x, y) - without_driver.get_pixel(x, y)
            if absf(difference.r) + absf(difference.g) + absf(difference.b) > 0.025:
                driver_pixels += 1
    race_camera.make_current()
    camera.queue_free()
    race.set_process(true)
    if driver_pixels < 10:
        _fail("The cabin driver was not visible through the windshield: %d pixels" % driver_pixels)
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
    print("HP2_MVP_WHEEL_BLUR_PASS peak=", peak, " authored_overlays=", overlays.size(),
        " changed_pixels=", changed, " driver_bones_changed=", changed_bones, " driver_pixels=", driver_pixels)
    quit(0)

func _fail(reason: String) -> void:
    push_error(reason)
    quit(1)
