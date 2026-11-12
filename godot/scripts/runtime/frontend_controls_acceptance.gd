extends SceneTree
# TASK-0104: real scene, Control GUI input, native actions and GPU/window checks.
const InputAdapter = preload("res://scripts/runtime/frontend_input.gd")
var frontend: Control
var output := ""
var phase := "main"
var failures: Array[String] = []
var checks: Array[String] = []
var screenshots: Array[String] = []

func _initialize() -> void: call_deferred("_run")

func _check(value: bool, label: String) -> void:
	checks.append(label)
	if not value:
		failures.append(label)
		print("CONTROL_CHECK_FAIL ",label)

func _ready_class(name: String) -> bool:
	var deadline := Time.get_ticks_msec()+15000
	while Time.get_ticks_msec()<deadline:
		if frontend.runtime_error_count>0: return false
		var state: Dictionary = frontend.layout.menu_state()
		if state.get("active_class","")==name and not state.get("exiting",false):
			await process_frame
			return true
		await process_frame
	return false

func _press(key: Key) -> void:
	var event := InputEventKey.new()
	event.keycode=key
	event.physical_keycode=key
	event.pressed=true
	Input.parse_input_event(event)
	await process_frame
	event=event.duplicate()
	event.pressed=false
	Input.parse_input_event(event)
	await process_frame

func _mouse(point: Vector2, pressed: bool, index: MouseButton=MOUSE_BUTTON_LEFT) -> void:
	Input.warp_mouse(point)
	var event := InputEventMouseButton.new()
	event.position=point
	event.global_position=point
	event.button_index=index
	event.button_mask=MOUSE_BUTTON_MASK_LEFT if pressed and index==MOUSE_BUTTON_LEFT else 0
	event.pressed=pressed
	Input.parse_input_event(event)
	await process_frame

func _click(control: Control) -> void:
	var point := control.get_global_rect().get_center()
	Input.warp_mouse(point)
	var motion := InputEventMouseMotion.new()
	motion.relative=Vector2(1,1)
	motion.position=point
	motion.global_position=point
	Input.parse_input_event(motion)
	await process_frame
	await _mouse(point,true)
	await _mouse(point,false)

func _menu(name: String) -> Control:
	for control in frontend.controls.nodes.values():
		var widget: Dictionary=control.get_meta("frontend_authored")
		if widget.name==name and widget.interactive: return control
	return null

func _capture(name: String) -> void:
	await RenderingServer.frame_post_draw
	var file := name+".png"
	_check(root.get_texture().get_image().save_png(output.path_join(file))==OK,"capture "+name)
	screenshots.append(file)

func _finish() -> void:
	await _capture("final-"+phase)
	var result := {"ok":failures.is_empty() and frontend.runtime_error_count==0,"phase":phase,
		"failures":failures,"checks":checks,"runtime_errors":frontend.runtime_error_count,
		"screenshots":screenshots,"state":frontend.layout.menu_state(),"render":frontend.last_stats}
	var file := FileAccess.open(output.path_join("result-"+phase+".json"),FileAccess.WRITE)
	file.store_string(JSON.stringify(result,"\t"))
	file.close()
	print("FRONTEND_CONTROLS_ACCEPTANCE_", "OK" if result.ok else "FAIL", " ",phase," ",failures)
	frontend.queue_free()
	await process_frame
	await process_frame
	quit(0 if result.ok else 2)

func _run() -> void:
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--controls-output="): output=argument.trim_prefix("--controls-output=")
		if argument.begins_with("--controls-phase="): phase=argument.trim_prefix("--controls-phase=")
	if output.is_empty() or DisplayServer.get_name()=="headless": quit(2); return
	frontend=load("res://scenes/frontend_native.tscn").instantiate()
	root.add_child(frontend)
	root.get_node("DebugTools").set_info(false)
	frontend.controls.action_requested.connect(func(action: Dictionary): print("CONTROL_ACTION ",JSON.stringify(action)," state=",JSON.stringify(frontend.layout.menu_state())))
	await process_frame
	_check(await _ready_class("New Main Menu"),"production main menu ready")
	_check(frontend.controls.nodes.size()>0 and _menu("MENUITEM_1") is Button,"native menu represented by real Buttons")
	_check(not frontend.controls.fonts.is_empty(),"original bitmap fonts loaded into FontFile")
	if phase=="reload":
		_check(frontend.frontend_input.hint("confirm").contains("F6"),"custom binding survived process restart")
		await _press(KEY_ENTER)
		_check(frontend.layout.menu_state().active_class=="New Main Menu","old confirm removed after restart")
		await _press(KEY_F6)
		_check(await _ready_class("Cops On Mode Select"),"persisted binding activates exactly one native route")
		_check(frontend.layout.menu_state().history_depth==1,"rebound input was not double submitted")
		await _finish()
		return
	var generation: int=frontend.controls.generation
	var selected: int=frontend.layout.menu_state().current
	for dimensions in [Vector2i(640,480),Vector2i(1280,720),Vector2i(1920,1080),Vector2i(2560,1080),Vector2i(3440,1440),Vector2i(1024,768),Vector2i(800,1000)]:
		root.size=dimensions
		await create_timer(0.25).timeout
		print("CONTROL_RESIZE ",dimensions," state=",JSON.stringify(frontend.layout.menu_state()))
		_check(root.size==dimensions,"window size "+str(dimensions))
		_check(frontend.controls.generation==generation and frontend.layout.menu_state().current==selected,"resize preserved native state "+str(dimensions))
		for control in frontend.controls.nodes.values():
			if control is Button:
				_check(Rect2(Vector2.ZERO,Vector2(dimensions)).encloses(control.get_global_rect()),"button inside responsive window "+str(dimensions)+" "+str(control.name))
		await _capture("main-%dx%d" % [dimensions.x,dimensions.y])
	root.mode=Window.MODE_MAXIMIZED
	await create_timer(0.3).timeout
	_check(frontend.controls.generation==generation,"maximize preserved page")
	root.mode=Window.MODE_WINDOWED
	root.size=Vector2i(1280,720)
	await create_timer(0.25).timeout
	_check(frontend.controls.generation==generation,"restore preserved page")
	var invalid: Dictionary=frontend.layout.ui_action({"generation":generation-1,"id":"invalid","kind":"activate"})
	_check(not invalid.ok,"native generation rejects stale click")
	# Actual GUI mouse click, not direct native activation.
	await _click(_menu("MENUITEM_3"))
	if not await _ready_class("Options"):
		_check(false,"mouse activated Options")
		await _finish()
		return
	_check(true,"mouse activated Options")
	await _click(_menu("MENUITEM_1"))
	if not await _ready_class("Audio Options"):
		_check(false,"mouse activated Audio Options")
		await _finish()
		return
	var sliders: Array[Control]=[]
	for control in frontend.controls.nodes.values():
		if control is HSlider: sliders.append(control)
	_check(sliders.size()==7,"seven real audio sliders")
	if not sliders.is_empty():
		var slider: HSlider=sliders[0]
		var before := slider.value
		var point := slider.get_global_rect().position+Vector2(slider.size.x*slider.scale.x*0.2,slider.size.y*slider.scale.y/2)
		await _mouse(point,true)
		await _mouse(point,false)
		_check(absf(slider.value-before)>0.05,"mouse adjusted native audio scalar")
	await _capture("audio-controls")
	# PC modal blocks underlying native navigation while capturing a binding.
	frontend.controls.show_bindings()
	await process_frame
	var old_page: String=frontend.layout.menu_state().active_class
	frontend.controls.bindings_panel._begin_capture("confirm","keyboard")
	await _press(KEY_F6)
	_check(frontend.layout.menu_state().active_class==old_page,"binding capture did not activate underlying page")
	_check(frontend.frontend_input.hint("confirm").contains("F6"),"binding capture changed InputMap")
	_check(FileAccess.file_exists(frontend.frontend_input.path),"binding file saved")
	var conflict := InputEventKey.new()
	conflict.physical_keycode=KEY_ESCAPE
	var attempt: Dictionary=frontend.frontend_input.rebind("confirm",conflict)
	_check(not attempt.ok and attempt.has("conflicts"),"conflict requires explicit replacement")
	frontend.controls.bindings_panel.hide()
	await _press(KEY_ESCAPE)
	_check(await _ready_class("Options"),"Escape returned from audio")
	await _press(KEY_ESCAPE)
	_check(await _ready_class("New Main Menu"),"native Back returned to main")
	var baseline: int=frontend.controls.nodes.size()
	for cycle in 10:
		await _click(_menu("MENUITEM_3"))
		if not await _ready_class("Options"): _check(false,"repeat Options entry"); break
		await _press(KEY_ESCAPE)
		if not await _ready_class("New Main Menu"): _check(false,"repeat native Back return"); break
		await process_frame
		_check(frontend.controls.nodes.size()==baseline,"stable Control count roundtrip %d" % cycle)
	_check(frontend.frontend_input.held.is_empty(),"no retained held input")
	await _finish()
