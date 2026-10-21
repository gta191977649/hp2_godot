extends SceneTree
# TASK0088 host inputs, original POV/projection and actual renderer lifecycle.
class ViewerFixture extends Node3D:
	var focused := true
	var held: Dictionary = {}
	var camera := Camera3D.new()
	var car_model: Node3D
	var floor_node := Node3D.new()
	var grid_node := Node3D.new()
	func _init() -> void:
		add_child(camera)
		add_child(floor_node)
		add_child(grid_node)
		car_model = ClassDB.instantiate("Ps2VehicleRenderer")
		add_child(car_model)
	func release_camera() -> void: Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
func key(code: Key, pressed: bool = true, echo: bool = false) -> InputEventKey:
	var event := InputEventKey.new()
	event.physical_keycode = code
	event.pressed = pressed
	event.echo = echo
	return event
func _initialize() -> void: _run.call_deferred()
func _check(value: bool, message: String) -> bool:
	if not value:
		printerr("DRIVING_CAMERA_HOST_FAIL: ", message)
		quit(1)
	return value
func _settle(drive: Object) -> void:
	for frame in 120: drive._update_camera(1.0 / 60.0)
func _run() -> void:
	var viewer := ViewerFixture.new()
	root.add_child(viewer)
	var model: Dictionary = viewer.car_model.load_car("CARS/911TURBO/GEOMETRY.BIN")
	if not _check(model.get("ok", false), str(model)): return
	var visual_before: Dictionary = viewer.car_model.get_state()
	var drive = load("res://scripts/debug/driving_lab.gd").new(viewer)
	var loaded: Dictionary = drive.lab.load_config("911TURBO")
	if not _check(loaded.get("ok", false), str(loaded)): return
	drive.lab.action("run")
	var vehicle: Dictionary = drive.lab.tick(1.0 / 60.0)
	if not _check(vehicle.get("ok", false), str(vehicle)): return
	var applied: Dictionary = viewer.car_model.apply_driving_snapshot(vehicle)
	if not _check(applied.get("ok", false), str(applied)): return
	drive.phase = drive.Phase.DRIVING
	drive.active = true
	drive.capture_wanted = true
	drive._update_camera(0)
	if not _check(drive.camera_state.get("ok", false), str(drive.camera_state)): return
	if not _check(drive.camera_state.pov_index == 1 and drive.camera_state.pov_type == 2 and drive.camera_state.lookback_param == 0, "original default exterior POV"): return
	if not _check(viewer.camera.near == 0.5 and viewer.car_model.visible, "exterior near and own-car visibility"): return
	await load("res://scripts/debug/driving_projection_checks.gd").check(viewer.camera, self, 0x32DC)
	var rear: Dictionary = drive.camera_state.duplicate()
	if not _check(viewer.camera.position.is_equal_approx(rear.position), "native camera is default"): return
	var motion := InputEventMouseMotion.new()
	motion.relative = Vector2(200, -80)
	drive.handle_input(motion)
	drive.driving_camera.apply(viewer.camera, rear, 0.3)
	var yaw: float = drive.driving_camera.yaw
	if not _check(drive.driving_camera.manual and not viewer.camera.position.is_equal_approx(rear.position), "mouse overrides exterior pose"): return
	drive.driving_camera.mouse_motion(Vector2.ZERO)
	drive.driving_camera.apply(viewer.camera, rear, 1.0)
	if not _check(drive.driving_camera.yaw == yaw, "idle delay preserves orbit"): return
	for frame in 360: drive.driving_camera.apply(viewer.camera, rear, 1.0 / 60.0)
	if not _check(not drive.driving_camera.manual and viewer.camera.position.is_equal_approx(rear.position), "idle returns exactly to native pose"): return
	# Actual authored no-dash cycle: far exterior1 -> near exterior2 -> bumper0.
	drive.handle_input(key(KEY_V))
	drive.handle_input(key(KEY_V, true, true))
	if not _check(drive.camera_state.pov_index == 2 and drive.camera_state.pov_type == 2, "V cycles once without repeat"): return
	if not _check(viewer.camera.position.distance_to(rear.position) > 0.5 and viewer.car_model.visible, "second authored exterior changes eye distance"): return
	drive.handle_input(key(KEY_V), true)
	if not _check(drive.camera_state.pov_index == 2, "UI consumed V is ignored"): return
	drive.handle_input(key(KEY_V))
	if not _check(drive.camera_state.pov_index == 0 and drive.camera_state.pov_type == 0 and drive.camera_state.hide_car, "V reaches original bumper POV"): return
	if not _check(not viewer.car_model.visible and viewer.camera.near == 0.5, "bumper suppresses own car subtree"): return
	if not _check((-viewer.camera.basis.z).dot(vehicle.body.basis.z.normalized()) > 0.99, "bumper faces along the car forward direction"): return
	await load("res://scripts/debug/driving_projection_checks.gd").check(viewer.camera, self, 0x4000)
	var bumper: Dictionary = drive.camera_state.duplicate()
	drive.handle_input(motion)
	drive.driving_camera.apply(viewer.camera, bumper, 0.1)
	if not _check(viewer.camera.position.is_equal_approx(bumper.position) and (-viewer.camera.basis.z).dot((bumper.target - bumper.position).normalized()) < 0.99, "bumper mouse swivels eye without exterior orbit"): return
	for frame in 360: drive.driving_camera.apply(viewer.camera, bumper, 1.0 / 60.0)
	if not _check(not drive.driving_camera.manual and viewer.camera.position.is_equal_approx(bumper.position), "bumper mouse idle restores native pose"): return
	# B is held state. Release returns to the selected bumper, not default exterior.
	drive.handle_input(key(KEY_B))
	drive.handle_input(key(KEY_B, true, true))
	_settle(drive)
	if not _check(drive.driving_camera.look_back and drive.camera_state.lookback_param > 0.999 and drive.camera_state.pov_index == 0, "held B eases original bumper lookback"): return
	if not _check((-viewer.camera.basis.z).dot(vehicle.body.basis.z.normalized()) < -0.99 and not viewer.car_model.visible, "held B looks rearward without exposing own car"): return
	drive.handle_input(key(KEY_B, false), true)
	_settle(drive)
	if not _check(not drive.driving_camera.look_back and drive.camera_state.lookback_param < 0.001 and drive.camera_state.pov_index == 0, "consumed B release restores selected bumper"): return
	if not _check((-viewer.camera.basis.z).dot(vehicle.body.basis.z.normalized()) > 0.99, "B release restores forward bumper view"): return
	drive.handle_input(key(KEY_B))
	_settle(drive)
	drive.handle_input(motion)
	viewer.focused = false
	if not _check(drive.sync_lifecycle() and not drive.driving_camera.manual and not drive.driving_camera.look_back, "focus suspension clears held lookback and mouse"): return
	viewer.focused = true
	if not _check(drive.sync_lifecycle(), "focus resumes"): return
	_settle(drive)
	if not _check(drive.camera_state.pov_index == 0 and drive.camera_state.lookback_param < 0.001, "focus resume retains selected POV without stuck lookback"): return
	drive.handle_input(key(KEY_B))
	drive.handle_input(key(KEY_W), true)
	if not _check(not drive.driving_camera.look_back, "UI keyboard capture clears held lookback"): return
	drive.handle_input(key(KEY_V))
	if not _check(drive.camera_state.pov_index == 1 and drive.camera_state.pov_type == 2 and viewer.car_model.visible, "V cycle returns exterior visibility"): return
	var visual_after: Dictionary = viewer.car_model.get_state()
	if not _check(visual_before.reflection_surfaces == visual_after.reflection_surfaces, "view suppression preserves loaded reflection resources"): return
	var after: Dictionary = drive.lab.get_state()
	if not _check(after.body == vehicle.body and after.velocity == vehicle.velocity and after.step == vehicle.step, "camera updates do not advance physics"): return
	drive.handle_input(key(KEY_V))
	drive.handle_input(key(KEY_V))
	drive.lab.action("restart")
	drive.driving_camera.reset()
	drive.lab.action("run")
	drive.lab.tick(1.0 / 60.0)
	drive._update_camera(0)
	if not _check(drive.camera_state.pov_index == 1 and viewer.car_model.visible and drive.camera_state.position.is_equal_approx(rear.position), "restart resets native POV and restores exterior visibility"): return
	if "--camera-track" in OS.get_cmdline_user_args():
		var track: Dictionary = drive.lab.set_track(11)
		if not _check(track.get("ok", false), str(track)): return
		drive.lab.action("run")
		for frame in 30:
			var on_track: Dictionary = drive.lab.tick(1.0 / 60.0)
			if not _check(on_track.get("ok", false), str(on_track)): return
			drive._update_camera(1.0 / 60.0)
			if not _check(drive.camera_state.get("ok", false) and viewer.camera.position.is_finite(), "original track camera pose and clearance"): return
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	drive = null
	viewer.queue_free()
	await process_frame
	print("DRIVING_CAMERA_HOST_OK")
	quit(0)
