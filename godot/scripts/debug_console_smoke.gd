extends "res://scripts/frontend_native.gd"
# Port-side synthetic harness: no original assets, runtime or movie player required.
var menu_events := 0
var updates := 0
var fps_label: Label

func _ready() -> void:
	render_host = DebugTools.host
	DebugTools.register_scene(self, "frontend")
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--expect-debug="):
			assert(render_host.is_debug_build() == (argument.trim_prefix("--expect-debug=") == "true"))
		if argument.begins_with("--expect-console="):
			assert(render_host.console_available() == (argument.trim_prefix("--expect-console=") == "true"))
	get_window().size = Vector2i(960, 600)
	fps_label = DebugTools.fps_label
	info_label = _make_label(Vector2(8, 22), Color.YELLOW)
	last_stats = {"quads": 12, "glyphs": 9, "textures": 3, "missing": 0, "clipped_out": 1}
	render_host.console_log("Synthetic debug console smoke: no original game assets.", false)
	_run.call_deferred()

func _process(delta: float) -> void:
	updates += 1
	_draw_console(delta)

func _send_joy_event(_event: int) -> void:
	menu_events += 1

func _key(code: Key, pressed: bool = true, echo: bool = false, unicode: int = 0) -> void:
	var event := InputEventKey.new()
	event.keycode = code
	event.pressed = pressed
	event.echo = echo
	event.unicode = unicode
	_input(event)

func _press(code: Key) -> void:
	_key(code)
	await get_tree().process_frame
	await get_tree().process_frame
	_key(code, false)
	await get_tree().process_frame

func _run() -> void:
	await get_tree().process_frame
	assert(fps_label.visible == render_host.is_debug_build())
	for code in [KEY_TAB, KEY_PAGEUP, KEY_PAGEDOWN, KEY_BRACKETLEFT, KEY_BRACKETRIGHT]:
		_key(code)
	assert(screen_index == 0 and not show_info)
	_key(KEY_DOWN)
	assert(menu_events == 1)
	if render_host.console_available():
		_key(KEY_F8)
		_key(KEY_F8, true, true)
		_key(KEY_F8, false)
		await get_tree().process_frame
		await get_tree().process_frame
		var state: Dictionary = DebugTools.last_frame
		assert(state.visible and state.batches > 0)
		_key(KEY_DOWN)
		_key(KEY_ESCAPE)
		assert(menu_events == 1)
		_key(KEY_DOWN, false)
		_key(KEY_ESCAPE, false)
		# TAB completion must turn 'stat' into the registered status command.
		for character in "stat":
			_key(KEY_NONE, true, false, character.unicode_at(0))
			await get_tree().process_frame
		await _press(KEY_TAB)
		await _press(KEY_ENTER)
		# Direct command access verifies host info synchronization separately from text IO.
		render_host.console_execute("info on")
		await get_tree().process_frame
		await get_tree().process_frame
		assert(show_info and info_label.visible)
		assert(fps_label.visible == render_host.is_debug_build())
		render_host.console_execute("info off")
		render_host.console_execute("status")
		render_host.console_execute("help")
		await _press(KEY_F8)
		state = DebugTools.last_frame
		assert(not state.visible and state.batches == 0)
		await _press(KEY_F8)
	get_window().size = Vector2i(1100, 700)
	for _step in 4:
		await get_tree().process_frame
	await get_tree().process_frame
	await get_tree().process_frame
	assert(fps_label.position.x + fps_label.get_minimum_size().x <= get_viewport_rect().size.x)
	assert(fps_label.position.x > get_viewport_rect().size.x / 2)
	assert(updates > 3)
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--capture=") and DisplayServer.get_name() != "headless":
			await RenderingServer.frame_post_draw
			assert(get_viewport().get_texture().get_image().save_png(argument.trim_prefix("--capture=")) == OK)
	print("DEBUG_CONSOLE_SMOKE_OK: available=%s debug=%s updates=%d" % [render_host.console_available(), render_host.is_debug_build(), updates])
	get_tree().quit(0)

func _exit_tree() -> void:
	DebugTools.unregister_scene(self)
