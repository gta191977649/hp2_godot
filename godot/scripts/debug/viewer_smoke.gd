extends Node
# Persistent asset-free integration driver. Activated only by --viewer-smoke.
func _ready() -> void:
	_run.call_deferred()

func _wait_frames(count: int = 4) -> void:
	for _step in count:
		await get_tree().process_frame
	var deadline := Time.get_ticks_msec() + 10000
	while is_instance_valid(DebugTools.scene_owner) and DebugTools.scene_owner.get("pending_load") == true:
		_check(Time.get_ticks_msec() < deadline, "Async viewer timeout")
		await get_tree().create_timer(0.001).timeout

func _check(value: bool, message: String) -> void:
	if not value:
		push_error("VIEWER_SMOKE_FAILED: " + message)
		get_tree().quit(1)
		assert(value, message)

func _key(code: Key, down: bool = true, repeat: bool = false) -> InputEventKey:
	var event := InputEventKey.new()
	event.keycode = code
	event.pressed = down
	event.echo = repeat
	return event

func _run() -> void:
	await _wait_frames()
	var host: Object = DebugTools.host
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--expect-debug="):
			_check(host.is_debug_build() == (argument.trim_prefix("--expect-debug=") == "true"), "Debug flag")
		if argument.begins_with("--expect-console="):
			_check(host.console_available() == (argument.trim_prefix("--expect-console=") == "true"), "Console flag")
	if not host.is_debug_build():
		_check(not DebugTools.request("car"), "Release must reject commands")
		print("VIEWER_SMOKE_OK: Release entry rejected")
		get_tree().quit(0)
		return
	# Default application path remains the real frontend. Only this harness substitutes it.
	get_window().size = Vector2i(960, 600)
	await _wait_frames()
	_check(DebugTools.frontend_scene == "res://scenes/frontend_native.tscn", "Frontend return target")
	DebugTools.frontend_scene = "res://scenes/debug/viewer_frontend_fixture.tscn"
	var original_host := host
	DebugTools.write_log("Viewer smoke session marker")
	for kind in ["car", "track"]:
		if host.console_available():
			host.console_execute("carviewer" if kind == "car" else "trackviewer")
		else:
			DebugTools.request(kind)
		await _wait_frames(6)
		var viewer: Node = DebugTools.scene_owner
		_check(is_instance_valid(viewer) and viewer.get("viewer_kind") == kind, "Scene transition to " + kind)
		_check(host == DebugTools.host, "Console host identity must persist")
		_check(viewer.fixture.visible and viewer.fixture_meshes > 0, "Synthetic fixture")
		# A first mouse event over the native panel must not reach the camera.
		var hover := InputEventMouseMotion.new()
		hover.position = Vector2(40, 55)
		_check(DebugTools.handle_input(hover), "ImGui first hover capture")
		var panel_camera: Vector3 = viewer.camera.position
		var panel_click := InputEventMouseButton.new()
		panel_click.position = hover.position
		panel_click.button_index = MOUSE_BUTTON_LEFT
		panel_click.pressed = true
		_check(DebugTools.handle_input(panel_click), "ImGui first click capture")
		viewer._unhandled_input(panel_click)
		panel_click.pressed = false
		DebugTools.handle_input(panel_click)
		viewer._unhandled_input(panel_click)
		var wheel := InputEventMouseButton.new()
		wheel.position = hover.position
		wheel.button_index = MOUSE_BUTTON_WHEEL_DOWN
		wheel.pressed = true
		_check(DebugTools.handle_input(wheel), "ImGui wheel capture")
		viewer._unhandled_input(wheel)
		_check(viewer.camera.position.is_equal_approx(panel_camera), "Panel wheel isolates camera")
		hover.position = get_viewport().get_visible_rect().size - Vector2(20, 20)
		DebugTools.handle_input(hover)
		panel_click.position = hover.position
		panel_click.pressed = true
		DebugTools.handle_input(panel_click)
		await _wait_frames(2)
		panel_click.pressed = false
		DebugTools.handle_input(panel_click)
		await _wait_frames(2)
		var initial: Vector3 = viewer.camera.position
		# Feed camera handlers through their normal input path.
		var button := InputEventMouseButton.new()
		button.button_index = MOUSE_BUTTON_LEFT if kind == "car" else MOUSE_BUTTON_RIGHT
		button.pressed = true
		viewer._unhandled_input(button)
		var motion := InputEventMouseMotion.new()
		motion.relative = Vector2(30, 12)
		viewer._unhandled_input(motion)
		if kind == "car":
			_check(not viewer.camera.position.is_equal_approx(initial), "Orbit camera")
		else:
			viewer._unhandled_input(_key(KEY_W))
			await _wait_frames()
			_check(not viewer.camera.position.is_equal_approx(initial), "Free camera movement")
		viewer.release_camera()
		DebugTools.request("reset")
		await _wait_frames()
		_check(viewer.camera.position.is_equal_approx(initial), "Camera reset")
		DebugTools.request("load", "synthetic path/request.bin")
		await _wait_frames()
		var state: Dictionary = host.viewer_state()
		_check(state.status == ("Failed" if kind == "car" else "Not implemented") and state.source == "synthetic path/request.bin", "Unavailable resource state")
		_check(viewer.fixture.visible == (kind == "track"), "Failed car load clears content; track keeps labeled fixture")
		DebugTools.request("clear")
		await _wait_frames()
		_check(not viewer.fixture.visible and str(host.viewer_state().source).is_empty(), "Clear")
		DebugTools.request("fixture")
		await _wait_frames()
		_check(viewer.fixture.visible, "Restore fixture")
		if host.console_available():
			viewer.looking = true
			DebugTools.handle_input(_key(KEY_F8))
			DebugTools.handle_input(_key(KEY_F8, true, true))
			DebugTools.handle_input(_key(KEY_F8, false))
			await _wait_frames()
			_check(DebugTools.is_console_open() and not viewer.looking, "Console toggle / mouse release")
			_check(Input.mouse_mode == Input.MOUSE_MODE_VISIBLE, "Mouse mode")
			var before: Vector3 = viewer.camera.position
			viewer._unhandled_input(button)
			viewer._unhandled_input(motion)
			viewer._unhandled_input(_key(KEY_W))
			var tick_before: int = viewer.updates
			await _wait_frames()
			_check(viewer.camera.position.is_equal_approx(before) and viewer.updates > tick_before, "Input isolation without pausing")
			DebugTools.handle_input(_key(KEY_F8))
			DebugTools.handle_input(_key(KEY_F8, false))
			await _wait_frames()
			_check(not DebugTools.is_console_open(), "Console close")
			_check(int(DebugTools.last_frame.logs) > 0, "Retained logs")
		if kind == "car":
			viewer.viewer_ui_action({"action": "preview", "enabled": true})
			var yaw_before: float = viewer.orbit_yaw
			await _wait_frames()
			_check(viewer.preview_mode and viewer.orbit_yaw != yaw_before, "Preview continuously orbits")
			_check(not viewer.grid_node.visible and not viewer.floor_node.visible, "Preview hides debug stage")
			var pitch_before: float = viewer.orbit_pitch
			viewer._unhandled_input(button)
			viewer._unhandled_input(motion)
			_check(viewer.orbit_pitch != pitch_before, "Preview mouse adjusts view")
			yaw_before = viewer.orbit_yaw
			await _wait_frames()
			_check(viewer.orbit_yaw != yaw_before, "Dragging does not stop preview orbit")
			viewer.release_camera()
			if host.console_available():
				DebugTools.handle_input(_key(KEY_F8))
				DebugTools.handle_input(_key(KEY_F8, false))
				yaw_before = viewer.orbit_yaw
				await _wait_frames()
				_check(viewer.orbit_yaw != yaw_before, "Console capture does not stop preview orbit")
				DebugTools.handle_input(_key(KEY_F8))
				DebugTools.handle_input(_key(KEY_F8, false))
			viewer.viewer_ui_action({"action": "preview", "enabled": false})
			_check(viewer.grid_node.visible, "Normal debug stage restored")
	get_window().size = Vector2i(1100, 700)
	await _wait_frames()
	_check(bool(DebugTools.last_frame.get("fps_visible", false)), "Shared ImGui Debug FPS")
	_check(float(DebugTools.last_frame.get("fps_right", 0)) == get_viewport().get_visible_rect().size.x - 8, "ImGui FPS right alignment")
	_check(not _has_controls(DebugTools.scene_owner), "Viewer uses native ImGui controls")
	DebugTools.request("close")
	await _wait_frames(6)
	_check(host.viewer_state().kind == "frontend" and DebugTools.scene_owner.name == "ViewerFrontendFixture", "Return to fresh frontend")
	DebugTools.request("reset")
	await _wait_frames()
	_check(host.viewer_state().kind == "frontend", "Viewer-only action rejected in frontend")
	# Repeat transition verifies resource cleanup and persistent session ownership.
	DebugTools.request("car")
	DebugTools.request("load", "queued-after-switch.bin")
	await _wait_frames(6)
	_check(DebugTools.host == original_host and DebugTools.scene_owner.viewer_kind == "car", "Repeated scene switch")
	_check(host.viewer_state().source == "queued-after-switch.bin", "Load queued immediately after scene switch")
	print("VIEWER_SMOKE_OK: scenes, queue, cameras, load stub, input isolation, persistent overlay")
	get_tree().quit(0)

func _has_controls(node: Node) -> bool:
	if node is Control:
		return true
	for child in node.get_children():
		if _has_controls(child):
			return true
	return false
