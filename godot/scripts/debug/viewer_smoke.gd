extends Node
# Persistent asset-free integration driver. Activated only by --viewer-smoke.
func _ready() -> void:
	_run.call_deferred()

func _wait_frames(count: int = 4) -> void:
	for _step in count:
		await get_tree().process_frame

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
		_check(state.status == "Not implemented" and state.source == "synthetic path/request.bin", "Unavailable loader state")
		_check(viewer.fixture.visible, "Requested asset is not substituted by a loaded fixture")
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
	_check(DebugTools.fps_label.visible, "Debug FPS")
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
