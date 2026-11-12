extends "res://scripts/runtime/frontend_controls_acceptance.gd"
# TASK-0104: actual ImGui toolbar, F8 capture, info/profile windows and resize.
func _run() -> void:
	phase="toolbar"
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--controls-output="): output=argument.trim_prefix("--controls-output=")
	if output.is_empty() or DisplayServer.get_name()=="headless": quit(2); return
	frontend=load("res://scenes/frontend_native.tscn").instantiate()
	root.add_child(frontend)
	var debug: Node=root.get_node("DebugTools")
	_check(debug.host.is_debug_build() and debug.host.console_available(),"loaded DLL is Debug with console enabled")
	_check(await _ready_class("New Main Menu"),"toolbar production main menu ready")
	for frame in 8: await process_frame
	_check(frontend.controls.tools.get_child_count()==1 and frontend.controls.tools.get_child(0).text=="KeyBinding","only KeyBinding remains in toolbar")
	_check(debug.layer>0 and debug.get_parent()==root,"debug overlay retains independent global CanvasLayer")
	var generation: int=frontend.controls.generation
	for dimensions in [Vector2i(1280,720),Vector2i(3440,1440),Vector2i(800,1000)]:
		root.size=dimensions
		for frame in 6: await process_frame
		var fps: Rect2=debug.last_frame.get("fps_region",Rect2())
		_check(debug.last_frame.get("fps_visible",false) and fps.has_area(),"existing ImGui FPS active "+str(dimensions))
		_check(fps.position.y>=frontend.controls.content_size().y and fps.position.x<24,"FPS region is bottom left "+str(dimensions))
		_check(debug.statistics.fps>0,"FPS samples update from real engine")
		await _capture("toolbar-%dx%d"%[dimensions.x,dimensions.y])
	root.size=Vector2i(1280,720)
	for frame in 6: await process_frame
	var dpi: Node=root.get_node("UiDisplay")
	var original_scale: float=dpi.scale_factor
	# Hold the injected service value against physical-monitor polling.
	dpi.set_process(false)
	for factor in [1.0,1.5,2.0]:
		dpi._apply_scale(factor)
		for frame in 6: await process_frame
		_check(is_equal_approx(float(debug.last_frame.get("ui_scale",0)),factor),"ImGui uses DPI service once "+str(factor))
		_check(debug.last_frame.get("fps_region",Rect2())==frontend.controls.fps_region(),"DPI FPS tracks footer rectangle "+str(factor))
		await _capture("toolbar-dpi-"+str(factor))
	dpi._apply_scale(original_scale)
	dpi.set_process(true)
	for frame in 6: await process_frame
	var current: int=frontend.layout.menu_state().current
	await _press(KEY_F8)
	_check(debug.is_console_open(),"F8 opens original ImGui console")
	await _press(KEY_DOWN)
	_check(frontend.controls.generation==generation and frontend.layout.menu_state().current==current,"console input does not navigate native menu")
	await _capture("toolbar-console")
	var logs_before: int=debug.last_frame.get("logs",0)
	for character in "status":
		var key := InputEventKey.new()
		key.keycode=character.to_upper().unicode_at(0)
		key.physical_keycode=key.keycode
		key.unicode=character.unicode_at(0)
		key.pressed=true
		Input.parse_input_event(key)
		await process_frame
		key=key.duplicate()
		key.pressed=false
		key.unicode=0
		Input.parse_input_event(key)
		await process_frame
	await _press(KEY_ENTER)
	for frame in 3: await process_frame
	_check(debug.last_frame.get("logs",0)>logs_before,"typed console command executes through actual input bar")
	await _capture("toolbar-console-command")
	debug.host.console_execute("status")
	_check(debug.host.console_visible(),"original console executes status command")
	await _press(KEY_F8)
	_check(not debug.is_console_open(),"F8 closes original console")
	debug.set_info(true)
	for frame in 4: await process_frame
	_check(debug.last_frame.get("info",false),"info command enables restored ImGui information window")
	await _capture("toolbar-info")
	_check(root.get_node("ProfileDebug").ui_state().get("enabled",false),"existing performance window enabled by profile flag")
	await _capture("toolbar-profile")
	debug.set_info(false)
	await _click(frontend.controls.tools.get_node("KeyBinding"))
	_check(frontend.controls.bindings_panel.visible,"KeyBinding opens existing remap modal")
	frontend.controls.bindings_panel.hide()
	await _finish()
