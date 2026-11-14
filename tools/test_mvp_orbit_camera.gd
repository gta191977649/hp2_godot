extends SceneTree

const TEST_MOUSE := 100

class MouseFilter extends Node:
    func _input(event: InputEvent) -> void:
        if event is InputEventMouseMotion and event.device != TEST_MOUSE:
            get_viewport().set_input_as_handled()

var race: Node
var started := false
var deadline := Time.get_ticks_msec() + 45000
var requested_focus := false

func _initialize() -> void:
    change_scene_to_file.call_deferred("res://native_game.tscn")

func _process(_delta: float) -> bool:
    if Time.get_ticks_msec() > deadline:
        push_error("Orbit smoke timed out, focus=%s captured=%s" % [root.has_focus(), Input.mouse_mode])
        quit(1)
    if started:
        return false
    race = root.get_node_or_null("NativeGame/RacePresentation")
    if not race or race.telemetry().get("race_state", 0) != 3:
        return false
    if not root.has_focus() and not requested_focus:
        requested_focus = true
        root.grab_focus()
    if Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
        started = true
        root.add_child.call_deferred(MouseFilter.new())
        run.call_deferred()
    return false

func move_mouse(relative: Vector2) -> void:
    var event := InputEventMouseMotion.new()
    event.device = TEST_MOUSE
    event.relative = relative
    event.screen_relative = relative
    Input.parse_input_event(event)

func check(condition: bool, detail: String) -> bool:
    if not condition:
        push_error(detail)
        quit(1)
    return condition

func body_center(body: MeshInstance3D) -> Vector3:
    return body.to_global(body.get_aabb().get_center())

func run() -> void:
    var scene: Node3D = race.race_scene()
    var camera: Camera3D = scene.get_viewport().get_camera_3d()
    var body: MeshInstance3D
    for child in scene.get_children():
        if child.get_meta(&"hp2_player_vehicle", false):
            body = child.get_child(0) as MeshInstance3D
            break
    if not check(is_instance_valid(body), "Player body mesh is missing"):
        return
    if not check(race.telemetry().orbit_terrain_queries == 0, "Idle camera queried terrain"):
        return
    move_mouse(Vector2(0, -1600))
    await process_frame
    await process_frame
    var stock: Dictionary = race.telemetry()
    if not check(camera.global_position.distance_to(stock.stock_camera_eye) < 0.35 and
        (-camera.global_basis.z).dot(stock.stock_camera_forward) > 0.985,
        "Orbit entry jumped away from the displayed HP2 camera"):
        return
    var entry_end := Time.get_ticks_msec() + 300
    while Time.get_ticks_msec() < entry_end:
        await process_frame
    var open_radius := camera.global_position.distance_to(body_center(body))
    move_mouse(Vector2(0, 1600))
    await process_frame
    await process_frame
    var close := camera.global_position - body_center(body)
    var local_eye := body.to_local(camera.global_position)
    var direction_to_eye := close.normalized()
    if not check(close.y < -0.05 and close.length() < open_radius * 0.85 and
        local_eye.x < body.get_aabb().get_center().x and not body.get_aabb().has_point(local_eye) and
        camera.global_basis.z.dot(direction_to_eye) > 0.99999,
        "Low rear orbit must retract toward the bumper and aim at the body center (radius=%s, arm=%s, local=%s)" %
        [open_radius, close, local_eye]):
        return
    move_mouse(Vector2(0, -1600))
    var extend_deadline := Time.get_ticks_msec() + 650
    while Time.get_ticks_msec() < extend_deadline:
        await process_frame
    if not check(camera.global_position.distance_to(body_center(body)) > open_radius * 0.9,
        "Clear camera arm did not extend after the low rear orbit"):
        return
    move_mouse(Vector2(0, 300))
    await process_frame
    await process_frame
    var angles := {}
    for i in range(48):
        move_mouse(Vector2(65.0, 0.0))
        await process_frame
        var direction := camera.global_basis.z
        angles[int(fposmod(atan2(direction.x, direction.z), TAU) / (PI / 2.0))] = true
    if not check(angles.size() == 4, "Camera did not orbit through all four quadrants: %s" % [angles]):
        return
    move_mouse(Vector2(0, -1600))
    await process_frame
    await process_frame
    var high := (camera.global_position - body_center(body)).normalized()
    if not check(high.y > 0.995 and (-camera.global_basis.z).dot(-high) > 0.99999,
        "Mouse up must orbit overhead and aim at the car body center (arm=%s, aim=%s)" %
        [high, (-camera.global_basis.z).dot(-high)]):
        return
    move_mouse(Vector2(0, 230))
    await process_frame
    await process_frame
    var lower := (camera.global_position - body_center(body)).normalized()
    if not check(lower.y < high.y - 0.05 and (-camera.global_basis.z).dot(-lower) > 0.99999,
        "Mouse down must lower the camera and keep aiming at the car body center"):
        return
    if not check(race.telemetry().orbit_active and race.telemetry().orbit_terrain_queries > 0,
        "Camera did not apply mouse input and terrain constraints"):
        return
    var blend_start := Time.get_ticks_msec() + 1650
    while Time.get_ticks_msec() < blend_start:
        await process_frame
    if not check(race.telemetry().orbit_active, "Camera returned before the blend finished"):
        return
    move_mouse(Vector2(32, 0))
    await create_timer(0.7).timeout
    if not check(race.telemetry().orbit_active, "Mouse failed to interrupt the blend"):
        return
    var return_deadline := Time.get_ticks_msec() + 2500
    while race.telemetry().orbit_active and Time.get_ticks_msec() < return_deadline:
        await process_frame
    if not check(not race.telemetry().orbit_active and race.telemetry().orbit_terrain_queries == 0,
        "Camera failed to return and stop terrain queries"):
        return
    stock = race.telemetry()
    if not check(camera.global_position.distance_to(stock.stock_camera_eye) < 0.001 and
        (-camera.global_basis.z).dot(stock.stock_camera_forward) > 0.999,
        "Camera did not return to the current HP2 pose"):
        return
    move_mouse(Vector2(90, 0))
    await process_frame
    var event := InputEventAction.new()
    event.action = &"hp2_camera"
    event.pressed = true
    Input.parse_input_event(event)
    await process_frame
    if not check(not race.telemetry().orbit_active, "Original camera action failed to cancel orbit"):
        return
    event.pressed = false
    Input.parse_input_event(event)
    event = InputEventAction.new()
    event.action = &"hp2_pause"
    event.pressed = true
    Input.parse_input_event(event)
    await process_frame
    if not check(Input.mouse_mode == Input.MOUSE_MODE_VISIBLE, "Pause failed to release the cursor"):
        return
    print("ORBIT_CAMERA_PASS track=", race.telemetry().track, " quadrants=", angles.size())
    quit(0)
