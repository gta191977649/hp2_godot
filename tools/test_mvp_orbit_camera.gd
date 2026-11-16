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
var input_only := OS.get_environment("HP2_MVP_TEST_ORBIT_INPUT") == "1"

func _initialize() -> void:
    OS.set_environment("HP2_MVP_TEST_CONTROLS", "1")
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
    if root.has_focus():
        started = true
        root.add_child.call_deferred(MouseFilter.new())
        run.call_deferred()
    return false

func move_mouse(relative: Vector2) -> void:
    var event := InputEventMouseMotion.new()
    event.device = TEST_MOUSE
    event.relative = relative
    event.screen_relative = relative
    root.push_input(event, true)

func set_key(code: Key, pressed: bool) -> void:
    var event := InputEventKey.new()
    event.physical_keycode = code
    event.pressed = pressed
    Input.parse_input_event(event)

func tap_key(code: Key) -> void:
    set_key(code, true)
    await create_timer(0.07).timeout
    set_key(code, false)
    await process_frame

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
    await tap_key(KEY_S)
    await tap_key(KEY_W)
    if not check(not race.telemetry().orbit_active, "An obsolete keyboard shortcut started mouse orbit"):
        return
    var hud: int = race.telemetry().controls_hud_profile
    await tap_key(KEY_TAB)
    if not check(race.telemetry().controls_hud_profile != hud, "Tab failed to cycle the native HUD"):
        return
    set_key(KEY_SPACE, true)
    await create_timer(0.1).timeout
    if not check(race.telemetry().controls_handbrake > 0.9, "Space failed to apply the native handbrake"):
        return
    set_key(KEY_SPACE, false)
    if input_only:
        await check_input_orbit(camera)
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
    if not check(race.telemetry().orbit_active, "Mouse movement did not re-engage orbit after it returned"):
        return
    var event := InputEventAction.new()
    event.action = &"hp2_camera"
    event.pressed = true
    Input.parse_input_event(event)
    await process_frame
    if not check(not race.telemetry().orbit_active, "Original camera action failed to cancel orbit"):
        return
    event.pressed = false
    Input.parse_input_event(event)
    if await check_zone_cameras_and_pause():
        print("ORBIT_CAMERA_PASS track=", race.telemetry().track, " quadrants=", angles.size())
        quit(0)

func check_input_orbit(camera: Camera3D) -> void:
    move_mouse(Vector2(90, -30))
    await create_timer(0.4).timeout
    var sample: Dictionary = race.telemetry()
    if not check(sample.orbit_active and camera.global_position.distance_to(sample.stock_camera_eye) > 0.1,
        "Mouse movement failed to engage orbit without a button or key"):
        return
    await create_timer(1.25).timeout
    move_mouse(Vector2(55, 0))
    await create_timer(0.25).timeout
    if not check(race.telemetry().orbit_active, "Mouse movement failed to interrupt the idle return"):
        return
    var until := Time.get_ticks_msec() + 3000
    while race.telemetry().orbit_active and Time.get_ticks_msec() < until:
        await process_frame
    sample = race.telemetry()
    if not check(not sample.orbit_active and sample.orbit_terrain_queries == 0 and
        camera.global_position.distance_to(sample.stock_camera_eye) < 0.001,
        "Idle mouse orbit did not return to the selected driving camera"):
        return
    print("MOUSE_ORBIT_INPUT_PASS track=", sample.track)
    if await check_zone_cameras_and_pause():
        print("ORBIT_CAMERA_PASS track=", sample.track, " input_only=true")
        quit(0)

func check_zone_cameras_and_pause() -> bool:
    await tap_key(KEY_E)
    if not check(race.telemetry().get("controls_camera_state") == 1 and not race.telemetry().orbit_active,
        "E failed to start native Zone Freeze: %s" % race.telemetry().get("controls_camera_kind")):
        return false
    var zone_end := Time.get_ticks_msec() + 9000
    while race.telemetry().get("controls_camera_state") == 1 and Time.get_ticks_msec() < zone_end:
        await process_frame
    if not check(race.telemetry().get("controls_camera_kind") == 1, "Zone Freeze failed to return to the driving view"):
        return false
    await create_timer(0.3).timeout
    await tap_key(KEY_Q)
    if not check(race.telemetry().get("controls_camera_state") == 2 and not race.telemetry().orbit_active,
        "Q failed to start native Zone Zoom: %s" % race.telemetry().get("controls_camera_kind")):
        return false
    zone_end = Time.get_ticks_msec() + 9000
    while race.telemetry().get("controls_camera_state") == 2 and Time.get_ticks_msec() < zone_end:
        await process_frame
    var event := InputEventAction.new()
    event.action = &"hp2_pause"
    event.pressed = true
    Input.parse_input_event(event)
    await process_frame
    return check(Input.mouse_mode == Input.MOUSE_MODE_VISIBLE, "Pause failed to release the cursor")
