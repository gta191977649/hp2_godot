extends Node
# TASK-0038: tool interaction acceptance, not original numerical equivalence.
class FaultLab extends RefCounted:
	var real: Object
	var calls := 0
	func _init(original: Object) -> void:
		real = original
	func load_config(code: String) -> Dictionary:
		return real.load_config(code)
	func action(name: String) -> Dictionary:
		return real.action(name)
	func set_input(throttle: float, brake: float, steering: float) -> Dictionary:
		return real.set_input(throttle, brake, steering)
	func set_driving_input(throttle: float, brake: float, steering: float, handbrake: float, transmission: int, shift: int, neutral: bool) -> Dictionary:
		return real.set_driving_input(throttle, brake, steering, handbrake, transmission, shift, neutral)
	func set_surface(mode: int, terrain: int) -> Dictionary:
		return real.set_surface(mode, terrain)
	func tick(_elapsed: float = 0) -> Dictionary:
		calls += 1
		var result: Dictionary = real.get_state().duplicate(true)
		result.ok = false
		result.status = "Blocked"
		result.diagnostic = "Synthetic explicit backend gap"
		return result

class KeyboardCapture extends RefCounted:
	var real: Object
	func _init(original: Object) -> void:
		real = original
	func console_visible() -> bool:
		return real.console_visible()
	func viewer_keyboard_capture() -> bool:
		return true

var viewer: Node
var drive: RefCounted
var os_output := ""
var os_clock := 0.0

func _ready() -> void:
	_run.call_deferred()

func _frames(count: int = 3) -> void:
	for frame in count:
		await get_tree().process_frame

func _wait_load() -> void:
	var deadline := Time.get_ticks_msec() + 20000
	while viewer.pending_load:
		assert(Time.get_ticks_msec() < deadline, "CarViewer model load timeout")
		await get_tree().process_frame
	await _frames()

func _key(code: Key, down: bool = true) -> InputEventKey:
	var event := InputEventKey.new()
	event.keycode = code
	event.physical_keycode = code
	event.pressed = down
	return event

func _send(event: InputEvent) -> void:
	Input.parse_input_event(event)
	await _frames(2)

func _click(position: Vector2) -> void:
	var move := InputEventMouseMotion.new()
	move.position = position
	await _send(move)
	var button := InputEventMouseButton.new()
	button.position = position
	button.button_index = MOUSE_BUTTON_LEFT
	button.pressed = true
	await _send(button)
	button = button.duplicate()
	button.pressed = false
	await _send(button)
	await _frames(3)

func _assert_rear() -> void:
	var target: Vector3 = viewer.car_model.transform * drive.local_target
	var offset: Vector3 = viewer.camera.position - target
	assert(offset.y > 0 and offset.normalized().dot(drive.heading) < -0.9, "Initial/recentered camera must be above and behind confirmed vehicle forward")
	assert(absf(offset.length() - drive.distance) < 0.001)

func _process(delta: float) -> void:
	if os_output.is_empty() or viewer == null:
		return
	os_clock += delta
	if os_clock < 0.05:
		return
	os_clock = 0
	var snapshot: Dictionary = drive.lab.get_state()
	var body: Transform3D = snapshot.get("body", Transform3D.IDENTITY)
	var velocity: Vector3 = snapshot.get("velocity", Vector3.ZERO)
	var target: Vector3 = viewer.car_model.transform * drive.local_target
	var ui_targets: Dictionary = {}
	for name in DebugTools.host.viewer_ui_targets():
		var point: Vector2 = DebugTools.host.viewer_ui_targets()[name]
		ui_targets[name] = [point.x, point.y]
	var row := {"ready": not viewer.pending_load and viewer.car_state.get("ok", false),
		"ui_kind": viewer.viewer_ui_snapshot().get("kind", ""), "viewport": str(viewer.get_viewport().get_visible_rect().size), "mouse_position": str(viewer.get_viewport().get_mouse_position()),
		"ui_targets": ui_targets, "input_pending": drive.input_adapter.commands,
		"input_requests": drive.input_adapter.command_requests, "input_commits": drive.input_adapter.command_commits,
		"input_last_command": drive.input_adapter.last_command, "input_last_frame": drive.last_input_frame,
		"keyboard_capture": DebugTools.host.viewer_keyboard_capture(), "powertrain": snapshot.get("powertrain", {}), "handbrake": snapshot.get("handbrake", 0), "skid_quads": int(drive.get("skid_quad_count")) if drive.get("skid_quad_count") != null else 0,
		"active": drive.active, "phase": drive.phase, "focused": viewer.focused, "window_focus": get_window().has_focus(),
		"console": DebugTools.is_console_open(), "mouse": Input.mouse_mode, "capture": drive.captures_pointer(),
		"step": snapshot.step, "throttle": snapshot.throttle, "brake": snapshot.brake,
		"steering": snapshot.steering, "position": [body.origin.x, body.origin.y, body.origin.z],
		"speed": velocity.length(), "yaw": drive.yaw_offset, "pitch": drive.pitch,
		"camera": [viewer.camera.position.x, viewer.camera.position.y, viewer.camera.position.z],
		"camera_behind": (viewer.camera.position-target).normalized().dot(drive.heading),
		"error": drive.error, "car": viewer.car_model.get_state().get("car_id", -1),
		"surface": snapshot.get("surface", {}), "grip": snapshot.get("wheels", []).map(func(wheel: Dictionary) -> Dictionary: return wheel.get("grip", {})),
		"surface_visual": drive.surface_key, "surface_labels": drive.surface_visuals.get_child_count() if is_instance_valid(drive.surface_visuals) else 0}
	var output := FileAccess.open(os_output + ".tmp", FileAccess.WRITE)
	output.store_string(JSON.stringify(row))
	output.close()
	DirAccess.rename_absolute(os_output + ".tmp", os_output)

func _click_target(name: String) -> void:
	await _frames(3)
	var targets: Dictionary = DebugTools.host.viewer_ui_targets()
	assert(targets.has(name), "Missing visible ImGui target: " + name)
	await _click(targets[name])

func _surface_smoke() -> void:
	# Actual ImGui selectors, with the unchanged Return button above them.
	# At1280x800 the mode/material fields begin at y103/y126.
	assert(drive.state.surface_known and drive.state.surface_catalog.size() == 5)
	assert(drive.state.surface_terrain_ids == [1, 2, 6, 7, 8])
	assert(drive.state.surface_boundaries == [25.0, 50.0, 75.0, 100.0])
	var previous_step: int = drive.state.step
	await _click_target("drive_material")
	await _click_target("drive_material_2") # DIRT, second material row.
	assert(int(drive.state.surface.mode) == 0 and int(drive.state.surface.terrain) == 2, "Real material selector must commit DIRT")
	assert(int(drive.state.step) > previous_step and drive.surface_key == "0:2", "Surface selection commits through continued simulation without restart")
	assert(viewer.floor_node.material_override == drive.surface_material)
	assert(not bool(drive.surface_material.get_shader_parameter("zones")))
	for wheel in drive.state.wheels:
		assert(wheel.has("grip") and int(wheel.grip.terrain) == 2)
		assert(is_finite(float(wheel.grip.static_coefficient)) and is_finite(float(wheel.grip.dynamic_coefficient)))
	await _click_target("drive_surface_mode")
	await _click_target("drive_surface_mode_1") # Zones, second mode row.
	assert(int(drive.state.surface.mode) == 1 and drive.surface_key.begins_with("1:"), "Real mode selector must commit zones")
	assert(bool(drive.surface_material.get_shader_parameter("zones")) and drive.surface_visuals.get_child_count() == 5)
	var zone_parameters: Vector4 = drive.surface_material.get_shader_parameter("boundaries")
	assert(zone_parameters == Vector4(25, 50, 75, 100), "Shader receives the shared native world boundaries")
	await _click_target("drive_grip") # Expand read-only grip table in zone layout.
	await _click(Vector2(900, 600))
	await _send(_key(KEY_W))
	var seen: Dictionary = {}
	var mixed := false
	for frame in 1200:
		drive.process(1.0 / 60.0)
		assert(drive.phase == drive.Phase.DRIVING, str(drive.error))
		var wheel_materials: Dictionary = {}
		for wheel in drive.state.wheels:
			var grip: Dictionary = wheel.grip
			seen[int(grip.terrain)] = true
			wheel_materials[int(grip.terrain)] = true
			assert(is_finite(float(grip.z_force)) and is_finite(float(grip.current_slip)) and is_finite(float(grip.road_noise)))
			var slip: Vector2 = grip.slip
			var force: Vector2 = grip.ground_force
			assert(slip.is_finite() and force.is_finite())
		mixed = mixed or wheel_materials.size() > 1
		if drive.state.body.origin.z > 110 and seen.size() == 5:
			break
	await _send(_key(KEY_W, false))
	assert(seen.has(1) and seen.has(2) and seen.has(6) and seen.has(7) and seen.has(8) and mixed, "World-anchored zones must publish all five materials with per-wheel crossings")
	assert(drive.state.body.origin.z > 100, "Test must traverse the final native X boundary")
	print("DRIVING_SURFACE_SMOKE_OK: real selectors, committed presentation and per-wheel five-zone diagnostics")

func _run() -> void:
	get_window().size = Vector2i(1280, 800)
	viewer = get_parent()
	drive = viewer.driving
	await _frames()
	if "--driving-smoke" in OS.get_cmdline_user_args():
		assert(not drive.start() and not drive.engaged(), "Missing original model must remain in preview")
		assert(not drive.lab.set_input(2, 0, 0).ok)
		print("DRIVING_LAB_SMOKE_OK: unavailable model remains explicit")
		get_tree().quit(0)
		return
	var selected: Dictionary = {}
	for entry in viewer.car_catalog:
		if entry.code == "911TURBO" and entry.available:
			selected = entry
	if selected.is_empty():
		print("DRIVING_ASSET_SMOKE_SKIP: original assets unavailable")
		get_tree().quit(77)
		return
	viewer.viewer_ui_action({"action": "select_car", "car_id": int(selected.id), "skin_id": int(selected.skins[0].id)})
	await _wait_load()
	assert(viewer.car_state.ok)
	viewer.set_preview_mode(true)
	await _frames()
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--drive-os-state="):
			os_output = argument.trim_prefix("--drive-os-state=")
			print("DRIVING_OS_READY")
			return # Only the external, owned-window OS driver supplies input.
	var original_floor_material: Material = viewer.floor_node.material_override
	var model_id: int = viewer.car_model.get_instance_id()
	var car_id: int = viewer.car_model.get_state().car_id
	var skin_id: int = viewer.car_model.get_state().skin_id
	# Startup failure is atomic with respect to the preview scene/camera.
	var original_lab: Object = drive.lab
	var fault := FaultLab.new(original_lab)
	drive.lab = fault
	var preview_pose: Transform3D = viewer.camera.transform
	var preview_aspect: int = viewer.camera.keep_aspect
	var preview_fov: float = viewer.camera.fov
	var preview_projection: Projection = viewer.camera.get_camera_projection()
	assert(not drive.start() and not drive.engaged() and not drive.error.is_empty())
	assert(viewer.preview_mode and viewer.camera.transform == preview_pose)
	assert(viewer.camera.keep_aspect == preview_aspect and viewer.camera.fov == preview_fov)
	assert(is_equal_approx(viewer.camera.get_camera_projection().x.x, preview_projection.x.x) and is_equal_approx(viewer.camera.get_camera_projection().y.y, preview_projection.y.y), "Failed start changed projection")
	drive.lab = original_lab
	# Actual ImGui pointer input, not a direct mode action call.
	await _click(Vector2(55, 238))
	assert(drive.active and drive.phase == drive.Phase.DRIVING and not drive.state.paused)
	assert(drive.captures_pointer() and DebugTools.host.viewer_state().kind == "car")
	await preload("res://scripts/debug/driving_projection_checks.gd").check(viewer.camera, get_tree())
	_assert_rear()
	var before: Vector3 = drive.state.body.origin
	await _send(_key(KEY_W))
	assert(drive.input_adapter.frame().throttle == 1)
	for frame in 45:
		drive.process(1.0 / 60.0)
	assert(drive.state.throttle == 1 and drive.state.velocity.length() > 0 and drive.state.body.origin.distance_to(before) > 0.1)
	for code in [KEY_A, KEY_D, KEY_S]:
		await _send(_key(code))
		drive.process(1.0 / 60.0)
		assert(float(drive.state.brake if code == KEY_S else drive.state.steering) == (1.0 if code != KEY_D else -1.0))
		await _send(_key(code, false))
	await _send(_key(KEY_W, false))
	drive.process(1.0 / 60.0)
	assert(drive.state.throttle == 0 and drive.state.brake == 0 and drive.state.steering == 0)
	# A sustained real-input turn must move the vehicle heading and its rear camera.
	var old_heading: Vector3 = drive.heading
	await _send(_key(KEY_W))
	await _send(_key(KEY_A))
	for frame in 75:
		drive.process(1.0 / 60.0)
	await _send(_key(KEY_W, false))
	await _send(_key(KEY_A, false))
	assert(absf(old_heading.signed_angle_to(drive.heading, Vector3.UP)) > 0.02)
	_assert_rear()
	# Keyboard capture zeroes controls while physics continues, rather than
	# permanently entering Session pause. Swap only the UI query seam synchronously.
	var actual_host: Object = DebugTools.host
	drive.input_adapter.handle_input(_key(KEY_W))
	DebugTools.host = KeyboardCapture.new(actual_host)
	drive.process(1.0 / 60.0)
	DebugTools.host = actual_host
	assert(drive.input_adapter.held.is_empty() and drive.state.throttle == 0 and drive.phase == drive.Phase.DRIVING)
	var look := InputEventMouseMotion.new()
	look.relative = Vector2(150, -30)
	await _send(look)
	assert(absf(drive.yaw_offset) > 0.2 and drive.pitch >= deg_to_rad(5))
	var turned: float = drive.yaw_offset
	drive._update_camera(0.3)
	assert(drive.yaw_offset == turned, "No early recenter before mouse idle threshold")
	for frame in 240:
		drive._update_camera(1.0 / 60.0)
	assert(absf(drive.yaw_offset) < 0.001 and absf(drive.pitch-deg_to_rad(15)) < 0.001)
	_assert_rear()
	for degrees in [-179, 179]:
		drive.yaw_offset = deg_to_rad(degrees)
		drive.mouse_idle = 2
		drive._update_camera(0.1)
		assert(absf(drive.yaw_offset) < deg_to_rad(179), "Recenter uses wrapped shortest angle")
	drive.yaw_offset = 0
	await _send(_key(KEY_ESCAPE))
	await _send(_key(KEY_ESCAPE, false))
	assert(not drive.captures_pointer() and drive.phase == drive.Phase.DRIVING)
	# Hovering the remaining panel does not claim WASD or suspend physics.
	var hover := InputEventMouseMotion.new()
	hover.position = Vector2(40, 115)
	await _send(hover)
	DebugTools.handle_input(hover)
	assert(DebugTools.input_captured() and not DebugTools.host.viewer_keyboard_capture())
	await _send(_key(KEY_W))
	drive.process(1.0 / 60.0)
	assert(drive.state.throttle == 1)
	await _send(_key(KEY_W, false))
	await _click(Vector2(900, 600))
	assert(drive.captures_pointer())
	var saved_step: int = drive.state.step
	viewer._notification(NOTIFICATION_APPLICATION_FOCUS_OUT)
	assert(drive.phase == drive.Phase.SUSPENDED and viewer.held.is_empty())
	drive.process(20)
	assert(drive.state.step == saved_step, "Suspension must not integrate elapsed time")
	viewer._notification(NOTIFICATION_APPLICATION_FOCUS_IN)
	assert(drive.phase == drive.Phase.DRIVING and drive.captures_pointer())
	drive.process(20)
	assert(drive.state.step == saved_step, "First resumed frame discards elapsed time even when resumed by notification")
	drive.process(1.0 / 60.0)
	assert(drive.state.step == saved_step + 1 and drive.state.dt < 0.1)
	if DebugTools.host.console_available():
		await _send(_key(KEY_F8))
		await _send(_key(KEY_F8, false))
		assert(drive.phase == drive.Phase.SUSPENDED and not drive.captures_pointer())
		await _send(_key(KEY_F8))
		await _send(_key(KEY_F8, false))
		assert(drive.phase == drive.Phase.DRIVING and not drive.state.paused)
	# Decoupled camera release cannot leave a hidden permanent pause.
	viewer.release_camera()
	drive.process(1.0 / 60.0)
	assert(drive.phase == drive.Phase.DRIVING and not drive.state.paused)
	# A true backend gap is terminal and cannot auto-resume after focus/console.
	var last_pose: Transform3D = viewer.car_model.transform
	fault = FaultLab.new(original_lab)
	drive.lab = fault
	drive.process(1.0 / 60.0)
	assert(drive.phase == drive.Phase.ERROR and viewer.car_model.transform == last_pose)
	var failed_calls := fault.calls
	viewer._notification(NOTIFICATION_APPLICATION_FOCUS_OUT)
	viewer._notification(NOTIFICATION_APPLICATION_FOCUS_IN)
	drive.process(1)
	assert(fault.calls == failed_calls and not drive.error.is_empty())
	drive.lab = original_lab
	await _click(Vector2(85, 90))
	assert(not drive.engaged())
	await _wait_load()
	assert(viewer.preview_mode and viewer.car_model.get_instance_id() == model_id)
	assert(viewer.floor_node.material_override == original_floor_material and not is_instance_valid(drive.surface_visuals), "Stopping restores preview material and removes surface labels")
	assert(viewer.camera.keep_aspect == preview_aspect and viewer.camera.fov == preview_fov)
	assert(is_equal_approx(viewer.camera.get_camera_projection().x.x, preview_projection.x.x) and is_equal_approx(viewer.camera.get_camera_projection().y.y, preview_projection.y.y), "Return changed preview projection")
	assert(viewer.camera.fov == drive.saved.fov and viewer.orbit_distance == drive.saved.orbit_distance and viewer.orbit_pitch == drive.saved.orbit_pitch)
	assert(viewer.car_model.get_state().car_id == car_id and viewer.car_model.get_state().skin_id == skin_id)
	viewer.car_model.advance_wheels(0.01)
	assert(not str(viewer.car_model.get_state().get("error", "")).contains("Preview integration is disabled"))
	# Re-entry from a restored preview must again work without Run.
	await _click(Vector2(55, 238))
	assert(drive.phase == drive.Phase.DRIVING)
	await _send(_key(KEY_W))
	drive.process(1.0 / 60.0)
	assert(drive.state.throttle == 1)
	await _send(_key(KEY_W, false))
	await _send(_key(KEY_ESCAPE))
	await _send(_key(KEY_ESCAPE, false))
	await _surface_smoke()
	await _send(_key(KEY_ESCAPE))
	await _send(_key(KEY_ESCAPE, false))
	assert(not drive.captures_pointer(), "Release captured pointer before final Return click")
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--driving-capture="):
			RenderingServer.force_draw(false)
			await _frames()
			assert(get_viewport().get_texture().get_image().save_png(argument.trim_prefix("--driving-capture=")) == OK)
	await _click(Vector2(85, 90))
	assert(not drive.engaged(), "Final Return exits Test drive")
	await _wait_load()
	assert(viewer.floor_node.material_override == original_floor_material and not is_instance_valid(drive.surface_visuals))
	print("DRIVING_ASSET_SMOKE_OK: real clicks/WASD, rear camera, pointer, auto-resume, explicit errors, surfaces and repeated preview restoration")
	get_tree().quit(0)
