extends SceneTree
# TASK0085: real native wheel emissions and actual GPU draws. Not PS2 equivalence.
var holder: Node3D
var camera: Camera3D
var simulation: RefCounted
var track: Node3D
var failed := false
var peak := 0

func _initialize() -> void:
	_run.call_deferred()

func _require(condition: bool, message: String) -> bool:
	if condition:
		return true
	printerr("GPU_PARTICLE_LIFECYCLE_FAIL: ", message)
	failed = true
	quit(1)
	return false

func _ok(value: Dictionary) -> bool:
	return _require(bool(value.get("ok", false)), str(value))

func _draw() -> void:
	await process_frame
	await RenderingServer.frame_post_draw

func _stats(value: Dictionary) -> Dictionary:
	if track != null:
		return track.get_optimization_state().get("particles", {})
	return value.get("stats", {})

func _present() -> Dictionary:
	if track != null:
		track.set_visual_timestep(0.0)
		var value: Dictionary = track.update_view(camera.position)
		if not str(value.get("render_error", "")).is_empty():
			value["ok"] = false
		return value
	return simulation.present_particles(holder, camera)

func _pose(value: Dictionary) -> void:
	if value.has("body"):
		var body: Transform3D = value.body
		camera.position = body.origin + Vector3(5, 4, -8)
		camera.look_at(body.origin)

func _exercise(track_mode: bool) -> bool:
	if not _ok(simulation.load_config("911TURBO")):
		return false
	if track_mode:
		track = ClassDB.instantiate("Ps2TrackRenderer")
		holder.add_child(track)
		var loaded: Dictionary = track.request_track(11)
		while loaded.get("loading", false):
			await process_frame
			loaded = track.pump_load()
		if not _ok(loaded) or not _ok(simulation.set_track(11)):
			return false
		track.set_camera_mode(1)
		track.set_driving_effect_source(simulation)
	if not _ok(simulation.action("run")):
		return false
	var latest: Dictionary
	for frame in range(300):
		if not _ok(simulation.set_driving_input(1.0, 0.0, 0.0 if frame < 100 else 1.0, 1.0 if frame >= 160 else 0.0)):
			return false
		var state: Dictionary = simulation.tick(1.0 / 60.0)
		if not _ok(state):
			return false
		_pose(state)
		latest = _present()
		if not _ok(latest):
			return false
		await _draw()
		var stats := _stats(latest)
		if not _require(stats.get("backend", "") == "godot_gpu", "GPU backend must be selected"):
			return false
		peak = maxi(peak, int(stats.get("gpu_emitted_particles", 0)))
	if not _require(peak > 0, "Real wheel run emitted no particles"):
		return false
	if not _ok(simulation.action("pause")):
		return false
	var paused := _stats(latest).duplicate(true)
	if not _require(int(paused.get("native_dynamic_groups", 0)) > 0, "Pause/reset fixture has no live dynamic group"):
		return false
	for repeat in range(8):
		latest = _present()
		if not _ok(latest):
			return false
		await _draw()
		var stats := _stats(latest)
		for key in ["gpu_emitted_particles", "gpu_manual_steps", "gpu_nodes"]:
			if not _require(stats.get(key, -1) == paused.get(key, -2), "Paused/repeated present changed " + key):
				return false
	# Invalid owner/camera fails before a candidate is committed; retry the same step.
	# There is no public mid-frame fault injection hook, so this is a boundary check.
	if not track_mode:
		if not _ok(simulation.action("run")) or not _ok(simulation.tick(1.0 / 60.0)):
			return false
		var rejected: Dictionary = simulation.present_particles(holder, null)
		if not _require(not rejected.get("ok", true), "Invalid camera was accepted"):
			return false
		latest = _present()
		if not _ok(latest):
			return false
		await _draw()
	else:
		if not _ok(simulation.action("run")) or not _ok(simulation.tick(1.0 / 60.0)):
			return false
		latest = _present()
		if not _ok(latest):
			return false
		await _draw()
	if not _require(int(_stats(latest).get("gpu_manual_steps", 0)) > int(paused.get("gpu_manual_steps", 0)), "Resume did not advance GPU"):
		return false
	# Same runtime pointer, reset step counter. Paused presentation must detach old car groups.
	if not _ok(simulation.action("restart")):
		return false
	latest = _present()
	if not _ok(latest):
		return false
	await _draw()
	var reset_stats := _stats(latest)
	if not _require(int(reset_stats.get("native_dynamic_groups", -1)) == 0, "Same-pointer restart retained dynamic groups"):
		return false
	if not _ok(simulation.load_config("CORVETTE")):
		return false
	latest = _present()
	if not _ok(latest):
		return false
	await _draw()
	if not _require(int(_stats(latest).get("native_dynamic_groups", -1)) == 0, "Vehicle reload retained dynamic groups"):
		return false
	if track != null:
		track.set_driving_effect_source(null)
		latest = _present()
		if not _ok(latest):
			return false
		await _draw()
		if not _require(int(_stats(latest).get("native_dynamic_groups", -1)) == 0, "Detach retained dynamic groups"):
			return false
		track.clear_track()
		track.queue_free()
		track = null
	simulation.clear_particles()
	await _draw()
	if not _require(holder.get_child_count() == 1, "Unload retained particle nodes"):
		return false
	print("GPU_PARTICLE_LIFECYCLE_CASE_OK: ", "Track11" if track_mode else "flat")
	return true

func _run() -> void:
	if not _require(DisplayServer.get_name() != "headless", "Requires a real GPU window and draws"):
		return
	holder = Node3D.new()
	root.add_child(holder)
	camera = Camera3D.new()
	holder.add_child(camera)
	camera.current = true
	camera.near = 0.05
	simulation = ClassDB.instantiate("Ps2DrivingRuntime")
	if not await _exercise(false):
		return
	peak = 0
	if not await _exercise(true):
		return
	print("GPU_PARTICLE_LIFECYCLE_OK: flat + Track11, real draw per step; mid-frame injection unavailable")
	quit(0)
