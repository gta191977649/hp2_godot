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
	var row := {"ready": not viewer.pending_load and viewer.car_state.get("ok", false),
		"active": drive.active, "phase": drive.phase, "focused": viewer.focused, "window_focus": get_window().has_focus(),
		"console": DebugTools.is_console_open(), "mouse": Input.mouse_mode, "capture": drive.captures_pointer(),
		"step": snapshot.step, "throttle": snapshot.throttle, "brake": snapshot.brake,
		"steering": snapshot.steering, "position": [body.origin.x, body.origin.y, body.origin.z],
		"speed": velocity.length(), "yaw": drive.yaw_offset, "pitch": drive.pitch,
		"camera": [viewer.camera.position.x, viewer.camera.position.y, viewer.camera.position.z],
		"camera_behind": (viewer.camera.position-target).normalized().dot(drive.heading),
		"error": drive.error, "car": viewer.car_model.get_state().get("car_id", -1)}
	var output := FileAccess.open(os_output + ".tmp", FileAccess.WRITE)
	output.store_string(JSON.stringify(row))
	output.close()
	DirAccess.rename_absolute(os_output + ".tmp", os_output)

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
	var model_id: int = viewer.car_model.get_instance_id()
	var car_id: int = viewer.car_model.get_state().car_id
	var skin_id: int = viewer.car_model.get_state().skin_id
	# Startup failure is atomic with respect to the preview scene/camera.
	var original_lab: Object = drive.lab
	var fault := FaultLab.new(original_lab)
	drive.lab = fault
	var preview_pose: Transform3D = viewer.camera.transform
	assert(not drive.start() and not drive.engaged() and not drive.error.is_empty())
	assert(viewer.preview_mode and viewer.camera.transform == preview_pose)
	drive.lab = original_lab
	# Actual ImGui pointer input, not a direct mode action call.
	await _click(Vector2(55, 238))
	assert(drive.active and drive.phase == drive.Phase.DRIVING and not drive.state.paused)
	assert(drive.captures_pointer() and DebugTools.host.viewer_state().kind == "car")
	_assert_rear()
	var before: Vector3 = drive.state.body.origin
	await _send(_key(KEY_W))
	assert(viewer.held.has(KEY_W))
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
	viewer.held[KEY_W] = true
	DebugTools.host = KeyboardCapture.new(actual_host)
	drive.process(1.0 / 60.0)
	DebugTools.host = actual_host
	assert(viewer.held.is_empty() and drive.state.throttle == 0 and drive.phase == drive.Phase.DRIVING)
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
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--driving-capture="):
			RenderingServer.force_draw(false)
			await _frames()
			assert(get_viewport().get_texture().get_image().save_png(argument.trim_prefix("--driving-capture=")) == OK)
	await _click(Vector2(85, 90))
	await _wait_load()
	print("DRIVING_ASSET_SMOKE_OK: real clicks/WASD, rear camera, pointer, auto-resume, explicit errors and repeated preview restoration")
	get_tree().quit(0)
