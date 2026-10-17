extends RefCounted
# TASK-0038. Port-side third-person tool, not a recovered original game camera.
enum Phase { OFF, STARTING, DRIVING, SUSPENDED, ERROR }
const DEFAULT_PITCH := 15.0
const MOUSE_DEGREES := 0.15
var viewer: Node3D
var lab: Object
var phase := Phase.OFF
var active := false
var state: Dictionary = {}
var saved: Dictionary = {}
var error := ""
var capture_wanted := false
var discard_elapsed := false
var yaw_offset := 0.0
var pitch := deg_to_rad(DEFAULT_PITCH)
var mouse_idle := 0.0
var distance := 4.0
var local_target := Vector3.ZERO
var heading := Vector3.FORWARD

func _init(owner: Node3D) -> void:
	viewer = owner
	lab = ClassDB.instantiate("Ps2DrivingLab")

func engaged() -> bool:
	return phase != Phase.OFF

func _valid_step(next: Dictionary) -> bool:
	return bool(next.get("ok", false)) and str(next.get("status", "")) not in ["Blocked", "Failed"] and not bool(next.get("paused", true)) and bool(next.get("body_known", false)) and bool(next.get("wheels_known", false)) and int(next.get("step", 0)) > 0

func _reason(next: Dictionary) -> String:
	return str(next.get("error", next.get("diagnostic", "Native driving step failed")))

func _remember(native: Dictionary) -> void:
	saved = {"preview": viewer.preview_mode, "show_lods": viewer.show_lods,
		"source": str(DebugTools.host.viewer_state().source), "car_id": int(native.car_id), "skin_id": int(native.skin_id),
		"transform": viewer.car_model.transform, "floor_position": viewer.floor_node.position,
		"floor_size": viewer.floor_node.mesh.size, "grid_position": viewer.grid_node.position,
		"floor_visible": viewer.floor_node.visible, "grid_visible": viewer.grid_node.visible,
		"orbit_yaw": viewer.orbit_yaw, "orbit_pitch": viewer.orbit_pitch, "orbit_distance": viewer.orbit_distance,
		"orbit_center": viewer.orbit_center, "camera_transform": viewer.camera.transform, "fov": viewer.camera.fov}

func start() -> bool:
	if engaged() or viewer.pending_load:
		return false
	error = ""
	discard_elapsed = false
	var native: Dictionary = viewer.car_model.get_state()
	var code := ""
	for entry in viewer.car_catalog:
		if int(entry.id) == int(native.get("car_id", -1)):
			code = str(entry.code)
	if code.is_empty() or not viewer.car_state.get("ok", false):
		error = "Load an original car before Test drive"
		return false
	state = lab.load_config(code)
	if not state.get("configured", false):
		error = _reason(state)
		return false
	state = lab.action("run")
	state = lab.tick()
	if not _valid_step(state):
		error = _reason(state)
		return false
	_remember(native)
	var bounds: AABB = viewer.car_state.bounds
	local_target = bounds.get_center()
	distance = maxf(4.0, bounds.size.z * 1.5)
	yaw_offset = 0
	pitch = deg_to_rad(DEFAULT_PITCH)
	mouse_idle = 0
	viewer.release_camera()
	viewer.preview_mode = false
	viewer.grid_node.visible = true
	viewer.floor_node.visible = true
	viewer.floor_node.mesh.size = Vector2(4000, 4000)
	viewer.floor_node.position.y = 0
	phase = Phase.STARTING
	if viewer.show_lods:
		viewer.show_lods = false
		viewer._queue_car(viewer.car_model.set_show_lods(false), false)
	if not viewer.pending_load:
		_finish_start()
	return engaged()

func _finish_start() -> void:
	if not viewer.car_state.get("ok", false):
		_start_failed("Original model could not prepare Test drive")
		return
	var result: Dictionary = viewer.car_model.apply_driving_snapshot(state)
	if not result.get("ok", false):
		_start_failed(str(result.get("error", "Initial driving pose failed")))
		return
	active = true
	phase = Phase.DRIVING
	capture_wanted = true
	viewer.camera.fov = 60
	_update_camera(0)
	sync_lifecycle()
	_sync_pointer()

func _start_failed(reason: String) -> void:
	stop()
	error = reason
	DebugTools.write_log(error, true)

func stop() -> void:
	if not engaged():
		return
	lab.action("pause")
	active = false
	phase = Phase.OFF
	capture_wanted = false
	viewer.release_camera()
	lab.action("restart")
	viewer.car_model.transform = saved.transform
	viewer.floor_node.position = saved.floor_position
	viewer.floor_node.mesh.size = saved.floor_size
	viewer.floor_node.visible = saved.floor_visible
	viewer.grid_node.visible = saved.grid_visible
	viewer.grid_node.position = saved.grid_position
	viewer.show_lods = bool(saved.show_lods)
	# The same cached renderer restores preview attachment bindings on commit.
	DebugTools.host.viewer_apply("load", saved.source)
	viewer.car_model.set_show_lods(viewer.show_lods)
	viewer._queue_car(viewer.car_model.request_car(saved.source, saved.car_id, saved.skin_id), false)
	viewer.preview_mode = bool(saved.preview)
	viewer.orbit_yaw = saved.orbit_yaw
	viewer.orbit_pitch = saved.orbit_pitch
	viewer.orbit_distance = saved.orbit_distance
	viewer.orbit_center = saved.orbit_center
	viewer.camera.fov = saved.fov
	viewer.camera.transform = saved.camera_transform
	error = ""

func _fail(reason: String) -> void:
	if phase == Phase.ERROR:
		return
	error = reason if not reason.is_empty() else "Native driving stopped unexpectedly"
	lab.action("pause")
	phase = Phase.ERROR
	capture_wanted = false
	viewer.release_camera()
	DebugTools.write_log(error, true)

func captures_pointer() -> bool:
	return phase == Phase.DRIVING and capture_wanted and viewer.focused and not DebugTools.is_console_open()

func _sync_pointer() -> void:
	var wanted: bool = captures_pointer()
	var mode := Input.MOUSE_MODE_CAPTURED if wanted else Input.MOUSE_MODE_VISIBLE
	if Input.mouse_mode != mode:
		Input.mouse_mode = mode

func sync_lifecycle() -> bool:
	# Returns true on a transition: the caller discards that frame's elapsed time.
	if phase not in [Phase.DRIVING, Phase.SUSPENDED]:
		return false
	var suspend: bool = not viewer.focused or DebugTools.is_console_open()
	if suspend and phase == Phase.DRIVING:
		state = lab.action("pause")
		viewer.release_camera()
		phase = Phase.SUSPENDED
		discard_elapsed = true
		return true
	if not suspend and phase == Phase.SUSPENDED:
		viewer.held.clear()
		discard_elapsed = true
		state = lab.action("run")
		if state.get("paused", true) or str(state.get("status", "")) in ["Failed", "Blocked"]:
			_fail(_reason(state))
		else:
			phase = Phase.DRIVING
			_sync_pointer()
		return true
	return false

func controls() -> Vector3:
	# E-0038-U01: positive native steering is source+Y / driver's left.
	var value := Vector3(float(viewer.held.has(KEY_W)), float(viewer.held.has(KEY_S)), float(viewer.held.has(KEY_A)) - float(viewer.held.has(KEY_D)))
	var devices := Input.get_connected_joypads()
	if not devices.is_empty():
		var device: int = devices[0]
		value.x = maxf(value.x, clampf(Input.get_joy_axis(device, JOY_AXIS_TRIGGER_RIGHT), 0, 1))
		value.y = maxf(value.y, clampf(Input.get_joy_axis(device, JOY_AXIS_TRIGGER_LEFT), 0, 1))
		var steering := Input.get_joy_axis(device, JOY_AXIS_LEFT_X)
		if absf(steering) > 0.1 and value.z == 0:
			value.z = -steering
	return value

func handle_input(event: InputEvent, consumed: bool = false) -> void:
	if event is InputEventKey:
		var code: int = event.physical_keycode if event.physical_keycode != 0 else event.keycode
		if not event.pressed:
			viewer.held.erase(code)
		if consumed or phase != Phase.DRIVING or not viewer.focused:
			viewer.held.clear()
			return
		if code == KEY_ESCAPE and event.pressed and not event.echo:
			capture_wanted = false
			viewer.release_camera()
		elif code in [KEY_W, KEY_A, KEY_S, KEY_D] and event.pressed:
			viewer.held[code] = true
	elif not consumed and phase == Phase.DRIVING:
		if event is InputEventMouseMotion and captures_pointer():
			yaw_offset = wrapf(yaw_offset - deg_to_rad(event.relative.x * MOUSE_DEGREES), -PI, PI)
			pitch = clampf(pitch + deg_to_rad(event.relative.y * MOUSE_DEGREES), deg_to_rad(5), deg_to_rad(65))
			mouse_idle = 0
		elif event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
			capture_wanted = true
			_sync_pointer()

func _update_camera(delta: float) -> void:
	# Original source+X forward becomes host basis+Z in Ps2DrivingLab. This is
	# deliberately not Godot's conventional camera/node -Z forward assumption.
	var forward: Vector3 = viewer.car_model.transform.basis.z
	forward.y = 0
	if forward.length_squared() > 0.000001:
		heading = forward.normalized()
	mouse_idle += delta
	if mouse_idle >= 1.0:
		var weight := 1.0 - exp(-4.0 * delta)
		yaw_offset = wrapf(lerp_angle(yaw_offset, 0, weight), -PI, PI)
		pitch = lerpf(pitch, deg_to_rad(DEFAULT_PITCH), weight)
	var target: Vector3 = viewer.car_model.transform * local_target
	var back: Vector3 = (-heading).rotated(Vector3.UP, yaw_offset)
	viewer.camera.position = target + distance * (back * cos(pitch) + Vector3.UP * sin(pitch))
	viewer.camera.look_at(target, Vector3.UP)
	var tile := Vector3(floorf(target.x / 10) * 10, 0, floorf(target.z / 10) * 10)
	viewer.floor_node.position = tile
	viewer.grid_node.position = tile + Vector3(0, 0.002, 0)

func process(delta: float) -> void:
	if phase == Phase.STARTING:
		if not viewer.pending_load:
			_finish_start()
		return
	if not active:
		return
	if sync_lifecycle():
		return
	if phase != Phase.DRIVING:
		return
	_sync_pointer()
	if discard_elapsed:
		discard_elapsed = false
		return
	var value := Vector3.ZERO
	if DebugTools.host.viewer_keyboard_capture():
		viewer.held.clear()
	else:
		value = controls()
	var next: Dictionary = lab.set_input(value.x, value.y, value.z)
	if not next.get("ok", false):
		_fail(_reason(next))
		return
	state = lab.tick(delta)
	if not _valid_step(state):
		_fail(_reason(state))
		return
	var result: Dictionary = viewer.car_model.apply_driving_snapshot(state)
	if not result.get("ok", false):
		_fail(str(result.get("error", "Driving pose publication failed")))
		return
	_update_camera(delta)

func action(request: Dictionary) -> void:
	if str(request.get("action", "")) == "drive_exit":
		stop()

func snapshot() -> Dictionary:
	return {"kind": "driving", "status": "Preparing Test drive" if phase == Phase.STARTING else "",
		"diagnostic": error, "driving_telemetry": "", "driving_recording": false}
