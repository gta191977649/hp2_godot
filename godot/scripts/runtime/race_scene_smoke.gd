extends Node
# Original-asset runtime integration checks; not original-game equivalence.
class FaultRuntime extends RefCounted:
	var real: RefCounted
	func _init(runtime: RefCounted) -> void:
		real = runtime
	func set_driving_input(throttle: float, brake: float, steering: float, handbrake: float, transmission: int, shift: int, neutral: bool, horn: bool = false) -> Dictionary:
		return real.set_driving_input(throttle, brake, steering, handbrake, transmission, shift, neutral, horn)
	func tick(_delta: float) -> Dictionary:
		var result: Dictionary = real.get_state().duplicate(true)
		result.status = "Blocked"
		result.paused = true
		result.diagnostic = "Synthetic explicit runtime gap"
		return result
	func action(command: String) -> Dictionary:
		return real.action(command)
func _ready() -> void:
	_run.call_deferred()

func _wait(scene: Node) -> bool:
	var deadline := Time.get_ticks_msec() + 120000
	while scene.phase == "Loading":
		if Time.get_ticks_msec() > deadline:
			return false
		await get_tree().process_frame
	return scene.phase == "Running"

func _fail(reason: String) -> void:
	push_error(reason)
	get_tree().quit(1)

func _run() -> void:
	var scene: Node = get_tree().current_scene
	if not await _wait(scene):
		var asset_error: String = str(scene.vehicle_renderer.get_state().get("error", "")) if scene.vehicle_renderer != null else ""
		if asset_error.contains("not configured") or scene.diagnostic.begins_with("Configure HP2_DEBUG_ELF"):
			get_tree().quit(77)
			return
		_fail("Runtime original load failed: " + scene.diagnostic)
		return
	scene.set_process(false)
	if scene.get_script().resource_path != "res://scripts/runtime/race_scene.gd" or scene.track_renderer.get_class() != "Ps2TrackRenderer" or scene.vehicle_renderer.get_class() != "Ps2VehicleRenderer" or scene.simulation.get_class() != "Ps2DrivingRuntime":
		_fail("Runtime entry is a debug viewer wrapper")
		return
	if not scene.state.get("real_track", false) or not scene.state.get("body_known", false) or not scene.state.get("wheels_known", false):
		_fail("Runtime did not publish the real native track/car state")
		return
	if scene.vehicle_renderer.transform != scene.state.body or int(scene.track_state.get("triangles", 0)) <= 0:
		_fail("Runtime did not display the published car and original track")
		return
	# Rendering must remain ready even when this test window has no focus.
	var committed_step := int(scene.state.step)
	scene._notification(NOTIFICATION_APPLICATION_FOCUS_OUT)
	scene._process(0.0)
	if int(scene.state.step) != committed_step:
		_fail("Paused initial presentation advanced native physics")
		return
	scene._notification(NOTIFICATION_APPLICATION_FOCUS_IN)
	var initial_frame: Dictionary = scene.track_renderer.frame_summary()
	if int(initial_frame.get("commands", 0)) <= 0 or not scene.vehicle_renderer.visible or not scene.state.get("body_known", false):
		_fail("Independent runtime did not submit track and body rendering")
		return
	if DisplayServer.get_name() != "headless":
		await get_tree().process_frame
		await RenderingServer.frame_post_draw
		await get_tree().process_frame
		await RenderingServer.frame_post_draw
		var image := get_viewport().get_texture().get_image()
		if image.is_empty() or image.get_width() <= 0 or image.get_height() <= 0:
			_fail("Independent runtime GPU viewport is empty")
			return
		image.save_png(ProjectSettings.globalize_path("res://../local/task0051/race-runtime-gpu.png"))
		print("RACE_RUNTIME_GPU_FRAME commands=", initial_frame.commands, " body=", scene.state.body)
	# TASK0084 default75 applies to Test drive; this separate runtime retains its
	# existing original horizontal-FOV projection policy.
	var projection: Projection=scene.camera.get_camera_projection()
	var horizontal := rad_to_deg(2.0*atan(1.0/projection.x.x))
	var expected := float(scene.OUTSIDE_FOV_BANGLE)*360.0/65536.0
	if scene.camera.keep_aspect!=Camera3D.KEEP_WIDTH or absf(horizontal-expected)>0.0001:
		_fail("Independent runtime original horizontal projection changed")
		return
	var key := InputEventKey.new()
	key.physical_keycode = KEY_W
	key.pressed = true
	scene._unhandled_input(key)
	var previous := int(scene.state.step)
	for step in 20:
		scene._process(1.0 / 60.0)
		if scene.phase != "Running":
			_fail("Runtime native movement failed: " + scene.diagnostic)
			return
	if int(scene.state.step) <= previous or float(scene.state.get("throttle", 0)) != 1.0 or scene.vehicle_renderer.transform != scene.state.body:
		_fail("Runtime controls did not reach the existing native simulation/publication")
		return
	key.pressed = false
	scene._unhandled_input(key)
	if scene.track_renderer.has_method("get_lod_state"):
		var lod: Dictionary=scene.track_renderer.get_lod_state()
		if lod.backend!="original" or lod.host_lod_selection:
			_fail("Race still uses host LOD management: "+str(lod))
			return
		if DisplayServer.get_name()!="headless":
			if lod.native_culling_views.size()!=7 or not scene.track_state.get("environment_capture_ready",false):
				_fail("Native driving did not prepare main and six independent capture views: "+str(lod))
				return
			for view in lod.native_culling_views:
				if not lod.native_culling_views[view].native_selection:
					_fail("Capture bypassed original selection")
					return
			print("RACE_NATIVE_CAPTURE_VIEWS_OK ",lod.native_culling_views)
	scene._notification(NOTIFICATION_APPLICATION_FOCUS_OUT)
	if scene.focused or not scene.simulation.get_state().get("paused", false):
		_fail("Focus loss did not pause runtime")
		return
	scene._notification(NOTIFICATION_APPLICATION_FOCUS_IN)
	if not scene.focused or scene.simulation.get_state().get("paused", true):
		_fail("Focus regain did not resume runtime")
		return
	var committed_pose: Transform3D = scene.vehicle_renderer.transform
	var actual_runtime: RefCounted = scene.simulation
	scene.simulation = FaultRuntime.new(actual_runtime)
	scene.discard_elapsed = false
	scene._process(1.0 / 60.0)
	if scene.phase != "Failed" or scene.vehicle_renderer.transform != committed_pose or not scene.diagnostic.contains("Synthetic explicit runtime gap"):
		_fail("Runtime gap was hidden or changed the last committed pose")
		return
	scene.simulation = actual_runtime
	actual_runtime = null
	var original: int = scene.track_id
	scene.track_id = -1
	scene.begin_session()
	if scene.phase != "Failed" or scene.track_renderer.get_state().get("ok", false):
		_fail("Invalid runtime track silently fell back")
		return
	scene.track_id = original
	scene.set_process(true)
	scene.begin_session()
	if not await _wait(scene):
		_fail("Runtime restart after failure failed: " + scene.diagnostic)
		return
	scene.set_process(false)
	scene.exit_session()
	await get_tree().process_frame
	await get_tree().process_frame
	if get_tree().current_scene.scene_file_path != "res://scenes/frontend_native.tscn":
		_fail("Runtime exit did not reach frontend")
		return
	get_tree().change_scene_to_file("res://scenes/race_scene.tscn")
	await get_tree().process_frame
	await get_tree().process_frame
	scene = get_tree().current_scene
	if not await _wait(scene):
		_fail("Runtime re-entry failed: " + scene.diagnostic)
		return
	print("RACE_RUNTIME_SMOKE_OK: shared renderer, native controls, focus, failure, restart, frontend exit and re-entry")
	get_tree().quit(0)
