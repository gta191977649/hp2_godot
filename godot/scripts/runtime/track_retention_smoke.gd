extends SceneTree
# TASK0076: real loaded scene and an actual missing-camera render exception.
func _initialize() -> void:
	_run.call_deferred()
func _require(value: bool, reason: String) -> bool:
	if not value:
		push_error(reason)
		quit(1)
	return value
func _run() -> void:
	var renderer: Node3D = ClassDB.instantiate("Ps2TrackRenderer")
	root.add_child(renderer)
	var camera := Camera3D.new()
	root.add_child(camera)
	camera.current = true
	var state: Dictionary = renderer.request_track(11)
	while state.get("loading", false):
		await process_frame
		state = renderer.pump_load()
	if not _require(state.get("ok", false), "Track retention fixture could not load original track"):
		return
	camera.position = state.start_position - state.start_forward * 12.0 + Vector3.UP * 5.0
	camera.look_at(state.start_position)
	renderer.restore_render_defaults()
	var native_start: Vector3 = Vector3(state.start_position.z, state.start_position.x, state.start_position.y)
	renderer.set_visual_state({"available":true, "environment_position_native":native_start})
	state = renderer.update_view(camera.position)
	if not _require(state.ok and state.render_ok and int(state.triangles) > 0, "Initial track render failed"):
		return
	var original_triangles := int(state.triangles)
	var original_nodes := int(state.render_nodes)
	# Remove the main camera only. Internal capture cameras own other viewports.
	camera.current = false
	state = renderer.update_view(camera.position)
	if not _require(state.ok and not state.render_ok and str(state.render_error).contains("active Camera3D") and state.debug_ready, "Frame failure unloaded the track/topology or hid its diagnostic"):
		return
	if not _require(int(state.triangles) == original_triangles and int(state.render_nodes) == original_nodes, "Frame failure released submitted track geometry"):
		return
	camera.current = true
	state = renderer.update_view(camera.position)
	if not _require(state.ok and state.render_ok and str(state.render_error).is_empty() and int(state.triangles) > 0, "Valid frame did not recover the retained track"):
		return
	if DisplayServer.get_name() != "headless":
		await process_frame
		await RenderingServer.frame_post_draw
		var capture := root.get_texture().get_image()
		if not _require(not capture.is_empty(), "Recovered track GPU viewport is empty"):
			return
		for argument in OS.get_cmdline_user_args():
			if argument.begins_with("--retention-capture="):
				if not _require(capture.save_png(argument.trim_prefix("--retention-capture=")) == OK, "Could not save retention capture"):
					return
	for row in state.view_submissions:
		if not _require(int(row.view_id) == 1, "Default-off reflections still submitted capture views"):
			return
	if DisplayServer.get_name() != "headless":
		renderer.set_effect_enabled("environment", true)
		for frame in 8:
			state = renderer.update_view(camera.position)
			await process_frame
			await RenderingServer.frame_post_draw
		if not _require(state.ok and state.render_ok and state.view_submissions.size() > 1 and renderer.get_environment_capture().available, "Explicit reflection opt-in did not retain capture support"):
			return
		renderer.restore_render_defaults()
		state = renderer.update_view(camera.position)
		if not _require(state.ok and state.render_ok and state.view_submissions.size() == 1 and not renderer.get_environment_capture().enabled and not renderer.get_environment_capture().available, "Disabling reflection retained scheduled capture work"):
			return
	renderer.clear_track()
	if not _require(not renderer.get_state().ok, "Explicit clear no longer unloads track"):
		return
	print("TRACK_RENDER_RETENTION_OK: frame gap retained geometry/topology; recovery and default-off captures passed")
	quit(0)
