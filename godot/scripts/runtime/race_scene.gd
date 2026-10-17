extends Node3D
# TASK-0051 port-owned single-car runtime entry. Not a recovered race coordinator,
# AI, competition rules or original chase-camera implementation.
const OUTSIDE_FOV_BANGLE := 0x32DC # E-0050-U02, original horizontal FOV.
var track_id := 11
var car_code := "ELISE"
var track_renderer: Node3D
var vehicle_renderer: Node3D
var simulation: RefCounted
var input_adapter := preload("res://scripts/debug/driving_input.gd").new()
var skid_renderer: RefCounted
var camera: Camera3D
var status_label: Label
var state: Dictionary = {}
var track_state: Dictionary = {}
var car_state: Dictionary = {}
var phase := "Empty"
var diagnostic := ""
var selected: Dictionary = {}
var focused := true
var discard_elapsed := false
var local_target := Vector3.ZERO
var follow_distance := 6.0
var generation := 0

func _ready() -> void:
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--track="):
			track_id = int(argument.trim_prefix("--track="))
		if argument.begins_with("--car="):
			car_code = argument.trim_prefix("--car=")
	camera = Camera3D.new()
	camera.name = "RuntimeCamera"
	camera.near = 0.05
	camera.far = 20000
	camera.keep_aspect = Camera3D.KEEP_WIDTH
	camera.fov = float(OUTSIDE_FOV_BANGLE) * 360.0 / 65536.0
	add_child(camera)
	camera.current = true
	var canvas := CanvasLayer.new()
	add_child(canvas)
	var panel := VBoxContainer.new()
	panel.position = Vector2(16, 16)
	canvas.add_child(panel)
	status_label = Label.new()
	panel.add_child(status_label)
	var restart := Button.new()
	restart.text = "Restart runtime session"
	restart.pressed.connect(begin_session)
	panel.add_child(restart)
	var exit_button := Button.new()
	exit_button.text = "Return to frontend"
	exit_button.pressed.connect(exit_session)
	panel.add_child(exit_button)
	for required in ["Ps2TrackRenderer", "Ps2VehicleRenderer", "Ps2DrivingRuntime"]:
		if not ClassDB.class_exists(required):
			_fail("Required runtime class is unavailable: " + required)
			return
	track_renderer = ClassDB.instantiate("Ps2TrackRenderer")
	vehicle_renderer = ClassDB.instantiate("Ps2VehicleRenderer")
	track_renderer.name = "SharedTrackRenderer"
	vehicle_renderer.name = "SharedVehicleRenderer"
	add_child(track_renderer)
	add_child(vehicle_renderer)
	begin_session()
	if "--race-runtime-smoke" in OS.get_cmdline_user_args() and not get_tree().has_meta("race_runtime_smoke_started"):
		get_tree().set_meta("race_runtime_smoke_started", true)
		get_tree().root.add_child.call_deferred(preload("res://scripts/runtime/race_scene_smoke.gd").new())

func _reason(result: Dictionary) -> String:
	return str(result.get("error", result.get("diagnostic", "Original runtime dependency unavailable")))

func _valid(result: Dictionary) -> bool:
	return result.get("ok", false) and str(result.get("status", "")) not in ["Failed", "Blocked"] and not result.get("paused", true) and int(result.get("step", 0)) > 0 and result.get("real_track", false) and result.get("body_known", false) and result.get("wheels_known", false)

func _release() -> void:
	generation += 1
	input_adapter.clear()
	if simulation != null:
		simulation.action("pause")
		simulation = null
	if skid_renderer != null:
		skid_renderer.clear()
		skid_renderer = null
	if track_renderer != null:
		track_renderer.clear_track()
	if vehicle_renderer != null:
		vehicle_renderer.clear_car()
	state.clear()
	track_state.clear()
	car_state.clear()
	selected.clear()
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE

func begin_session() -> void:
	if track_renderer == null or vehicle_renderer == null:
		return
	_release()
	diagnostic = ""
	phase = "Loading"
	for entry in vehicle_renderer.get_catalog():
		if str(entry.code) == car_code:
			selected = entry
	if selected.is_empty() or not selected.get("available", false) or selected.get("skins", []).is_empty():
		_fail("Original vehicle is unavailable: " + car_code + " " + str(selected.get("error", "")))
		return
	track_state = track_renderer.request_track(track_id)
	car_state = vehicle_renderer.request_car(str(selected.path), int(selected.id), int(selected.skins[0].id))
	if (not track_state.get("loading", false) and not track_state.get("ok", false)) or (not car_state.get("loading", false) and not car_state.get("ok", false)):
		_fail("Original resource load failed: " + _reason(track_state) + " " + _reason(car_state))
		return
	_update_status()

func _start_simulation() -> void:
	simulation = ClassDB.instantiate("Ps2DrivingRuntime")
	state = simulation.load_config(car_code)
	if not state.get("configured", false):
		_fail(_reason(state))
		return
	state = simulation.set_track(track_id)
	if not state.get("ok", false) or not state.get("real_track", false):
		_fail(_reason(state))
		return
	state = simulation.action("run")
	state = simulation.tick()
	if not _valid(state):
		_fail(_reason(state))
		return
	var applied: Dictionary = vehicle_renderer.apply_driving_snapshot(state)
	if not applied.get("ok", false):
		_fail(_reason(applied))
		return
	var bounds: AABB = car_state.bounds
	local_target = bounds.get_center()
	follow_distance = maxf(4.0, bounds.size.z * 1.5) # Port follow distance only.
	skid_renderer = preload("res://scripts/debug/driving_skids.gd").new(self)
	phase = "Running"
	discard_elapsed = true
	_update_camera()
	_update_status()

func _update_camera() -> void:
	var forward: Vector3 = vehicle_renderer.transform.basis.z
	forward.y = 0
	if forward.length_squared() < 0.000001:
		return
	var target: Vector3 = vehicle_renderer.transform * local_target
	camera.position = target - forward.normalized() * follow_distance * cos(deg_to_rad(18.0)) + Vector3.UP * follow_distance * sin(deg_to_rad(18.0))
	camera.look_at(target, Vector3.UP)

func _process(delta: float) -> void:
	if phase == "Loading":
		track_state = track_renderer.pump_load()
		car_state = vehicle_renderer.pump_load()
		if not track_state.get("loading", false) and not car_state.get("loading", false):
			if not track_state.get("ok", false) or not car_state.get("ok", false):
				_fail("Original load failed: " + _reason(track_state) + " " + _reason(car_state))
			else:
				_start_simulation()
		_update_status()
		return
	if phase == "Running" and focused:
		if discard_elapsed:
			discard_elapsed = false
		else:
			var controls: Dictionary = input_adapter.frame()
			var accepted: Dictionary = simulation.set_driving_input(controls.throttle, controls.brake, controls.steering, controls.handbrake, controls.transmission, controls.shift, controls.neutral)
			if not accepted.get("ok", false):
				_fail(_reason(accepted))
				return
			var previous := int(state.get("step", 0))
			state = simulation.tick(delta)
			if not _valid(state):
				_fail(_reason(state))
				return
			if int(state.step) > previous:
				input_adapter.committed()
			var applied: Dictionary = vehicle_renderer.apply_driving_snapshot(state)
			if not applied.get("ok", false):
				_fail(_reason(applied))
				return
		_update_camera()
		var view: Dictionary = track_renderer.update_view(camera.position)
		if not view.get("ok", false):
			_fail(_reason(view))
			return
		track_state = view
		diagnostic = str(view.get("diagnostic", ""))
		var extent: float = get_viewport().get_visible_rect().size.x
		var projection_scale := extent * 0.5 / tan(deg_to_rad(camera.fov) * 0.5)
		var car_view: Dictionary = vehicle_renderer.update_view(camera.position, -camera.basis.z, projection_scale, -1)
		if not car_view.get("ok", false):
			_fail(_reason(car_view))
			return
		var clip: Projection = camera.get_camera_projection() * Projection(camera.global_transform.affine_inverse())
		var skids: Dictionary = simulation.render_skids(camera.global_transform, clip, get_viewport().get_visible_rect().size.y)
		if not skids.get("ok", false):
			_fail(_reason(skids))
			return
		var checked: Dictionary = simulation.get_state()
		if not _valid(checked):
			_fail(_reason(checked))
			return
		skid_renderer.publish(skids.get("commands", []))
		_update_status()

func _update_status() -> void:
	if status_label == null:
		return
	status_label.text = "Runtime single-car session | Track %d | %s\n%s | Native step %d\nW/S throttle/brake | A/D steer | R restart | Esc frontend\n%s" % [track_id, car_code, phase, int(state.get("step", 0)), diagnostic]

func _fail(reason: String) -> void:
	if phase == "Loading":
		_release()
	diagnostic = reason
	phase = "Failed"
	input_adapter.clear()
	if simulation != null:
		simulation.action("pause")
	_update_status()

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo:
		if event.physical_keycode == KEY_ESCAPE:
			exit_session()
			return
		if event.physical_keycode == KEY_R:
			begin_session()
			return
	if phase == "Running":
		input_adapter.handle_input(event, not focused)

func _notification(what: int) -> void:
	if what == NOTIFICATION_APPLICATION_FOCUS_OUT:
		focused = false
		input_adapter.clear()
		if simulation != null:
			simulation.action("pause")
	if what == NOTIFICATION_APPLICATION_FOCUS_IN:
		focused = true
		discard_elapsed = true
		if simulation != null and phase == "Running":
			simulation.action("run")

func exit_session() -> void:
	_release()
	phase = "Empty"
	get_tree().change_scene_to_file("res://scenes/frontend_native.tscn")

func _exit_tree() -> void:
	_release()
