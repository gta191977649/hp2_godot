extends SceneTree
# TASK-0098. Port acceptance harness: real scene, native runtime and GPU output.
# No replacement page/services, direct menu_open, profile unlocks or simulated race.
var frontend: Node
var output := ""
var checkpoints: Array[Dictionary] = []

func _initialize() -> void:
	call_deferred("_run")

func _snapshot(label: String) -> Dictionary:
	var state: Dictionary = frontend.layout.menu_state()
	checkpoints.append({"label": label, "state": state.duplicate(true), "render": frontend.last_stats.duplicate(true)})
	print("FRONTEND_ROUTE_STATE ", label, " ", JSON.stringify(state))
	return state

func _press(code: Key) -> void:
	var event := InputEventKey.new()
	event.keycode = code
	event.pressed = true
	Input.parse_input_event(event)
	await process_frame
	event = InputEventKey.new()
	event.keycode = code
	event.pressed = false
	Input.parse_input_event(event)
	await process_frame

func _await_class(expected: String) -> bool:
	var deadline := Time.get_ticks_msec() + 12000
	while Time.get_ticks_msec() < deadline:
		if frontend.runtime_error_count > 0:
			return false
		var state: Dictionary = frontend.layout.menu_state()
		if state.get("active_class", "") == expected and not state.get("exiting", false):
			return true
		await process_frame
	return false

func _await_loaded() -> bool:
	var deadline := Time.get_ticks_msec() + 20000
	while Time.get_ticks_msec() < deadline:
		if frontend.runtime_error_count > 0:
			return false
		var selection: Dictionary = frontend.layout.menu_state().get("selection", {})
		if selection.get("active", false) and selection.get("loaded", false) and selection.get("overlay", 130) == 0 and frontend.last_stats.get("shared_frontend_target", false) and frontend.last_stats.get("shared_vertices", 0) > 0:
			return true
		await process_frame
	return false

func _finish(ok: bool, reason: String) -> void:
	_snapshot("final")
	await RenderingServer.frame_post_draw
	var image := root.get_texture().get_image()
	var image_error := image.save_png(output.path_join("final.png"))
	var result := {"ok": ok and image_error == OK and frontend.runtime_error_count == 0, "reason": reason,
		"runtime_error_count": frontend.runtime_error_count,
		"renderer": RenderingServer.get_current_rendering_method(),
		"checkpoints": checkpoints, "screenshot_error": image_error}
	var file := FileAccess.open(output.path_join("result.json"), FileAccess.WRITE)
	if file == null:
		print("FRONTEND_SELECTION_ROUNDTRIP_FAIL: cannot write result")
		quit(2)
		return
	file.store_string(JSON.stringify(result, "\t"))
	file.close()
	print("FRONTEND_SELECTION_ROUNDTRIP_%s: %s" % ["OK" if result.ok else "FAIL", reason])
	frontend.queue_free()
	await process_frame
	await create_timer(0.2).timeout
	quit(0 if result.ok else 2)

func _run() -> void:
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--roundtrip-output="):
			output = argument.trim_prefix("--roundtrip-output=")
	if output.is_empty() or DisplayServer.get_name() == "headless":
		print("FRONTEND_SELECTION_ROUNDTRIP_FAIL: requires output directory and real GPU window")
		quit(2)
		return
	frontend = load("res://scenes/frontend_native.tscn").instantiate()
	root.add_child(frontend)
	await process_frame
	_snapshot("main-menu")
	if not await _await_class("New Main Menu"):
		await _finish(false, "main menu not ready")
		return
	await _press(KEY_SPACE)
	if not await _await_class("Cops On Mode Select"):
		await _finish(false, "main menu did not enter cops-on mode selection")
		return
	_snapshot("mode-select")
	# Recovered mode-select MENUITEM_4 opens the Hot Pursuit / Ultimate Racer tree.
	for _step in 3:
		await _press(KEY_DOWN)
	await _press(KEY_SPACE)
	if not await _await_class("View Event Tree"):
		await _finish(false, "Ultimate Racer event tree not reached")
		return
	_snapshot("event-tree")
	for cycle in 3:
		# First confirm zooms tree -> details; second confirm starts the event.
		if cycle == 0:
			await _press(KEY_SPACE)
			await create_timer(1.2).timeout
			_snapshot("event-details")
		await _press(KEY_SPACE)
		if not await _await_class("Select Car"):
			await _finish(false, "event confirmation did not enter Select Car (cycle %d)" % cycle)
			return
		if not await _await_loaded():
			await _finish(false, "actual vehicle/shared target did not become ready")
			return
		await RenderingServer.frame_post_draw
		var vehicle_image := root.get_texture().get_image()
		vehicle_image.save_png(output.path_join("select-car-%d.png" % cycle))
		# Port smoke threshold rejects the former black vehicle region. This is
		# presence detection, not an original-image equivalence tolerance.
		var visible_pixels := 0
		for y in range(150, 380):
			for x in range(100, 550):
				var colour := vehicle_image.get_pixel(x, y)
				if maxf(colour.r, maxf(colour.g, colour.b)) > 0.06:
					visible_pixels += 1
		if visible_pixels < 1000:
			await _finish(false, "vehicle region is blank (%d visible pixels)" % visible_pixels)
			return
		_snapshot("select-car-%d" % cycle)
		# Exercise the actual host input translation, including skin and car directions.
		var original_skin: int = frontend.layout.menu_state()["selection"]["skin"]
		await _press(KEY_DOWN)
		if frontend.layout.menu_state()["selection"]["skin"] == original_skin:
			await _finish(false, "skin input did not change the original selected skin")
			return
		for code in [KEY_UP, KEY_RIGHT, KEY_LEFT]:
			await _press(code)
		if not await _await_loaded():
			await _finish(false, "vehicle did not recover after directional inputs")
			return
		await _press(KEY_BACKSPACE)
		if not await _await_class("View Event Tree"):
			await _finish(false, "selection cancel did not return to event tree")
			return
		await create_timer(1.2).timeout
		_snapshot("returned-%d" % cycle)
	await _finish(true, "three production event/selection/cancel/reentry cycles completed")
