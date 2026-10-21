extends SceneTree
# TASK0075 bounded host integration; actual resources/physics, no PS2 equivalence.
var owner: Node3D
var camera: Camera3D
var simulation: RefCounted
var track_renderer: Node3D
func _initialize() -> void:
	_run.call_deferred()

func _check(result: Dictionary) -> bool:
	if not result.get("ok", false):
		printerr("DRIVING_PARTICLE_HOST_FAIL: ", result)
		quit(1)
		return false
	return true

func _run() -> void:
	owner = Node3D.new()
	root.add_child(owner)
	camera = Camera3D.new()
	owner.add_child(camera)
	camera.current = true
	camera.near = 0.05
	simulation = ClassDB.instantiate("Ps2DrivingRuntime")
	if not _check(simulation.load_config("911TURBO")):
		return
	if "--particle-track" in OS.get_cmdline_user_args():
		track_renderer = ClassDB.instantiate("Ps2TrackRenderer")
		owner.add_child(track_renderer)
		var track: Dictionary = track_renderer.request_track(11)
		while track.get("loading", false):
			await process_frame
			track = track_renderer.pump_load()
		if not _check(track) or not _check(simulation.set_track(11)):
			return
		track_renderer.set_camera_mode(1)
		track_renderer.set_driving_effect_source(simulation)
	if not _check(simulation.action("run")):
		return
	var max_active := 0
	var max_visible := 0
	var gaps: Array = []
	for frame in range(360):
		if not _check(simulation.set_driving_input(1.0, 0.0, 0.0 if frame < 120 else 1.0, 1.0 if frame >= 180 else 0.0)):
			return
		var state: Dictionary = simulation.tick(1.0 / 60.0)
		if not _check(state):
			return
		var body: Transform3D = state.body
		camera.position = body.origin + Vector3(5, 4, -8)
		camera.look_at(body.origin)
		var particles: Dictionary
		if track_renderer != null:
			track_renderer.set_visual_timestep(1.0 / 60.0)
			var track: Dictionary = track_renderer.update_view(camera.position)
			if not _check(track):
				return
			particles = {"ok":true, "active":int(track.get("particles_active", 0)), "visible":int(track.get("particles_visible", 0)), "gaps":track.get("particle_recovery_gaps", [])}
		else:
			particles = simulation.present_particles(owner, camera)
		if not _check(particles):
			return
		max_active = maxi(max_active, int(particles.active))
		max_visible = maxi(max_visible, int(particles.visible))
		for gap in particles.gaps:
			if gap not in gaps:
				gaps.append(gap)
		if frame % 8 == 0 or "--particle-backend=godot_gpu" in OS.get_cmdline_user_args():
			await process_frame
		if frame == 240 and DisplayServer.get_name() != "headless":
			await process_frame
			for argument in OS.get_cmdline_user_args():
				if argument.begins_with("--particle-capture="):
					root.get_texture().get_image().save_png(argument.trim_prefix("--particle-capture="))
	if max_active == 0 or max_visible == 0:
		printerr("DRIVING_PARTICLE_HOST_FAIL: no emitted/visible particles ", max_active, " / ", max_visible, " gaps=", gaps)
		quit(1)
		return
	var before := owner.get_child_count()
	if track_renderer != null:
		track_renderer.set_driving_effect_source(null)
		track_renderer.clear_track()
		track_renderer.queue_free()
		track_renderer = null
		await process_frame
	simulation.clear_particles()
	if owner.get_child_count() != 1 or (before <= 1):
		printerr("DRIVING_PARTICLE_HOST_FAIL: display cleanup")
		quit(1)
		return
	simulation.action("restart")
	simulation.action("run")
	simulation.tick(1.0 / 60.0)
	var restarted: Dictionary = simulation.present_particles(owner, camera)
	if not _check(restarted) or int(restarted.active) != 0:
		printerr("DRIVING_PARTICLE_HOST_FAIL: restart retained particles")
		quit(1)
		return
	simulation.clear_particles()
	print("DRIVING_PARTICLE_HOST_OK: active=", max_active, " visible=", max_visible, " gaps=", gaps)
	quit(0)
