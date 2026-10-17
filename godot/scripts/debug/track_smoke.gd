extends Node
# Port-side original-asset integration checks. No original equivalence claim.
func _ready() -> void:
	_run.call_deferred()

func _wait(viewer: Node) -> bool:
	var deadline := Time.get_ticks_msec() + 120000
	await get_tree().process_frame
	await get_tree().process_frame
	while viewer.track_pending or viewer.pending_load or DebugTools.operation_pending:
		if Time.get_ticks_msec() > deadline:
			return false
		await get_tree().create_timer(0.01).timeout
	return true

func _run() -> void:
	var viewer: Node = get_parent()
	if viewer.track_catalog.is_empty() and str(viewer.track_model.get_state().get("error", "")).contains("not configured"):
		print("Original track asset root is not configured")
		get_tree().quit(77)
		return
	if not await _wait(viewer):
		_fail("Initial load timed out")
		return
	print("TRACK_SMOKE_STATE: ", viewer.track_state)
	if not viewer.track_state.get("ok", false) or int(viewer.track_state.get("triangles", 0)) <= 0 or viewer.fixture.visible:
		_fail("Original track did not replace fixture: " + str(viewer.track_state))
		return
	if viewer.track_catalog.size() != 30:
		_fail("Expected 30 shipped catalogue tracks")
		return
	for track in viewer.track_catalog:
		if not track.get("available", false):
			_fail("Shipped empty TRACKA must remain available: " + str(track))
			return
	if not viewer.track_state.get("debug_ready", false) or viewer.render_collision or viewer.render_material:
		_fail("Track debug readiness/default flags are invalid")
		return
	if viewer.track_state.get("material_legend", []).is_empty():
		_fail("Original terrain material legend is empty")
		return
	var cached: Dictionary = viewer.track_model.get_state()
	for combination in [[false, false], [true, false], [false, true], [true, true]]:
		viewer.viewer_ui_action({"action": "render_collision", "enabled": combination[0]})
		viewer.viewer_ui_action({"action": "render_material", "enabled": combination[1]})
		var debug: Dictionary = viewer.track_model.get_state()
		for key in ["debug_triangles", "debug_edges", "render_nodes"]:
			if debug.get(key, -1) != cached.get(key, -2):
				_fail("Track toggles changed cached mesh counts: " + key)
				return
		if bool(debug.get("render_collision", false)) != combination[0] or bool(debug.get("render_material", false)) != combination[1]:
			_fail("Track debug controls are not independent: " + str(debug))
			return
		if bool(debug.get("debug_edges_visible", false)) != combination[0] or bool(debug.get("debug_fill_visible", false)) != (combination[0] or combination[1]):
			_fail("Track debug meshes do not match selected mode")
			return
	var original: int = int(viewer.track_state.track_id)
	var next: Dictionary = viewer.track_catalog[1]
	DebugTools.request("load", str(next.path))
	if not await _wait(viewer) or not viewer.track_state.get("ok", false) or int(viewer.track_state.track_id) != int(next.id):
		_fail("Track switch failed: " + str(viewer.track_state))
		return
	if not viewer.render_collision or not viewer.render_material or not viewer.track_state.get("render_collision", false) or not viewer.track_state.get("render_material", false):
		_fail("Track switch lost desired overlay flags")
		return
	DebugTools.request("load", "TRACKS/missing.bin")
	if not await _wait(viewer) or viewer.track_state.get("ok", false) or viewer.fixture.visible:
		_fail("Missing track did not clear original display")
		return
	if viewer.track_state.get("debug_ready", true) or int(viewer.track_state.get("debug_triangles", 0)) != 0 or int(viewer.track_state.get("debug_edges", 0)) != 0:
		_fail("Failed track retained debug geometry")
		return
	viewer.viewer_ui_action({"action": "render_collision", "enabled": false})
	if not viewer.render_collision or not viewer.render_material:
		_fail("Disabled overlay action changed desired flags")
		return
	DebugTools.request("load", "TRACKS/TRACKB%d.LZC" % original)
	if not await _wait(viewer) or not viewer.track_state.get("ok", false):
		_fail("Reload after failure failed")
		return
	var browse_aspect: int = viewer.camera.keep_aspect
	var browse_fov: float = viewer.camera.fov
	var browse_projection: Projection = viewer.camera.get_camera_projection()
	viewer.viewer_ui_action({"action": "track_drive"})
	await get_tree().process_frame
	if not viewer.driving.engaged():
		_fail("Real track driving entry failed: " + viewer.driving.error)
		return
	if not viewer.driving.state.get("real_track", false):
		_fail("Driving did not bind the original track")
		return
	await get_tree().process_frame
	await preload("res://scripts/debug/driving_projection_checks.gd").check(viewer.camera, get_tree())
	var driving_debug: Dictionary = viewer.track_model.get_state()
	if driving_debug.get("render_collision", true) or driving_debug.get("render_material", true) or not viewer.render_collision or not viewer.render_material:
		_fail("Browse overlays leaked into driving or desired flags were lost")
		return
	viewer.driving.stop()
	await get_tree().process_frame
	if viewer.camera.keep_aspect != browse_aspect or viewer.camera.fov != browse_fov or not is_equal_approx(viewer.camera.get_camera_projection().x.x, browse_projection.x.x) or not is_equal_approx(viewer.camera.get_camera_projection().y.y, browse_projection.y.y):
		_fail("Returning to browse did not restore projection")
		return
	if not viewer.track_state.get("render_collision", false) or not viewer.track_state.get("render_material", false):
		_fail("Returning to browse did not restore overlays")
		return
	if viewer.car_model.visible or not viewer.track_state.get("ok", false):
		_fail("Return did not restore TrackViewer")
		return
	DebugTools.request("clear")
	if not await _wait(viewer) or viewer.track_model.get_state().get("ok", false):
		_fail("Clear retained track content")
		return
	var cleared: Dictionary = viewer.track_model.get_state()
	if cleared.get("debug_ready", true) or int(cleared.get("debug_triangles", 0)) != 0 or int(cleared.get("debug_edges", 0)) != 0 or not viewer.render_collision or not viewer.render_material:
		_fail("Clear did not release geometry and retain session flags")
		return
	print("TRACK_VIEWER_SMOKE_OK")
	get_tree().quit(0)

func _fail(reason: String) -> void:
	push_error(reason)
	get_tree().quit(1)
