extends "res://scripts/runtime/frontend_selection_roundtrip.gd"
# TASK-0105: real production route; samples exclude screenshot/export work.
var samples: Dictionary = {}
var operations: Array[Dictionary] = []
var soak_seconds := 0.0
var debug_compare := false
var debug_vsync := false

func _press(code: Key) -> void:
	var begin := Time.get_ticks_usec()
	await super._press(code)
	operations.append({"key":code,"wall_ms":(Time.get_ticks_usec()-begin)/1000.0,"scope":"input plus two process frames"})

func _sample(phase: String, count: int, key: Key = KEY_NONE, cadence: int = 30) -> void:
	var rows: Array = []
	var previous := Time.get_ticks_usec()
	for index in count:
		var input_ms := 0.0
		if key != KEY_NONE and index % cadence == 0:
			var event := InputEventKey.new()
			event.keycode = key
			event.pressed = true
			var input_begin := Time.get_ticks_usec()
			Input.parse_input_event(event)
			input_ms = (Time.get_ticks_usec()-input_begin)/1000.0
		if key != KEY_NONE and index % cadence == 1:
			var event := InputEventKey.new()
			event.keycode = key
			Input.parse_input_event(event)
		await process_frame
		var now := Time.get_ticks_usec()
		rows.append({"frame_ms": (now-previous)/1000.0,
			"input_ms":input_ms,
			"frontend_ms": frontend.last_stats.get("frontend_cpu_ms",0),
			"render": frontend.last_stats.duplicate(true),
			"selection": frontend.layout.menu_state().get("selection",{}),
			"loaded": frontend.layout.menu_state().get("selection",{}).get("loaded",false),
			"viewport_gpu_ms": RenderingServer.viewport_get_measured_render_time_gpu(root.get_viewport_rid())})
		previous = now
	samples[phase] = rows

func _run() -> void:
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--roundtrip-output="): output=argument.trim_prefix("--roundtrip-output=")
		if argument.begins_with("--selection-soak="): soak_seconds=float(argument.trim_prefix("--selection-soak="))
		debug_compare = debug_compare or argument == "--selection-debug-compare"
		debug_vsync = debug_vsync or argument == "--selection-debug-vsync"
	if output.is_empty() or DisplayServer.get_name()=="headless":
		quit(2)
		return
	root.size=Vector2i(1920,1080)
	if debug_compare and not debug_vsync:
		DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
		Engine.max_fps = 0
	if debug_compare:
		root.get_node("ProfileDebug").capture_active = false
	RenderingServer.viewport_set_measure_render_time(root.get_viewport_rid(),true)
	frontend=load("res://scenes/frontend_native.tscn").instantiate()
	root.add_child(frontend)
	await process_frame
	root.size=Vector2i(1920,1080)
	await process_frame
	if not await _await_class("New Main Menu"): await _finish(false,"main");return
	if debug_compare:
		await _press(KEY_DOWN)
	await _press(KEY_SPACE)
	if not await _await_class("Cops Off Mode Select" if debug_compare else "Cops On Mode Select"): await _finish(false,"mode");return
	for step in (2 if debug_compare else 3): await _press(KEY_DOWN)
	await _press(KEY_SPACE)
	if not await _await_class("View Event Tree"): await _finish(false,"tree");return
	await _press(KEY_SPACE)
	await create_timer(1.2).timeout
	await _press(KEY_SPACE)
	if not await _await_class("Select Car") or not await _await_loaded(): await _finish(false,"selection");return
	await create_timer(1.0).timeout
	if debug_compare:
		await _compare_debug_ui()
		return
	await _sample("idle",240)
	await _sample("cars",180,KEY_RIGHT)
	if not await _await_loaded(): await _finish(false,"cars");return
	await _sample("skins",180,KEY_DOWN)
	if not await _await_loaded(): await _finish(false,"skins");return
	await _sample("warm_skins",180,KEY_UP)
	await _sample("rapid_skins",120,KEY_DOWN,2)
	if not await _await_loaded(): await _finish(false,"rapid skins");return
	# Cancel with a new skin request in flight; the route stays entirely native.
	await _press(KEY_DOWN)
	await _press(KEY_BACKSPACE)
	if not await _await_class("View Event Tree"): await _finish(false,"cancel");return
	await create_timer(1.2).timeout
	await _press(KEY_SPACE)
	await _sample("reentry",120)
	if not await _await_loaded(): await _finish(false,"reentry");return
	var file := FileAccess.open(output.path_join("performance.json"),FileAccess.WRITE)
	file.store_string(JSON.stringify({"samples":samples,"operations":operations,"resolution":root.size,"gpu_scope":"root viewport time plus asynchronous selection_gpu_ms timestamps where available"}))
	file.close()
	if soak_seconds>0: await create_timer(soak_seconds).timeout
	await _finish(true,"performance scenarios completed")

func _compare_debug_ui() -> void:
	var debug: Node = root.get_node("DebugTools")
	if debug.host == null or not debug.host.console_available() or not debug.host.is_debug_build():
		await _finish(false, "Debug ImGui DLL required")
		return
	var selection: Dictionary = frontend.layout.menu_state().get("selection", {})
	for index in int(selection.get("visible_cars", 0)):
		if int(selection.get("car", -1)) == 41:
			break
		await _press(KEY_RIGHT)
		selection = frontend.layout.menu_state().get("selection", {})
	if int(selection.get("car", -1)) != 41:
		await _finish(false, "expected the original empty-profile Lotus Elise selection")
		return
	if not await _await_loaded():
		await _finish(false, "Lotus Elise preview did not settle")
		return
	selection = frontend.layout.menu_state().get("selection", {})
	var profiler: Node = root.get_node("ProfileDebug")
	var phases := ["info", "fps_only", "no_imgui", "info_repeat"] if not debug_vsync else ["info"]
	for phase: String in phases:
		var info := phase.begins_with("info")
		var suspended := phase == "no_imgui"
		debug.set_info(info)
		debug.debug_ui_sample_mode = 2 if suspended else 1
		for index in 30:
			await process_frame
		var ui: Dictionary = debug.last_frame
		if bool(ui.get("info", false)) != info or bool(ui.get("suspended", false)) != suspended or bool(ui.get("fps_visible", false)) == suspended or bool(ui.get("visible", false)):
			await _finish(false, "unexpected Debug UI state for " + phase)
			return
		var rows: Array[Dictionary] = []
		rows.resize(120)
		var previous := Time.get_ticks_usec()
		for index in rows.size():
			await process_frame
			var now := Time.get_ticks_usec()
			ui = debug.last_frame
			var stats: Dictionary = frontend.last_stats
			rows[index] = {
				"frame_ms": (now - previous) / 1000.0,
				"frontend_ms": stats.get("frontend_cpu_ms"),
				"packet_ms": stats.get("selection_packet_cpu_ms"),
				"gpu_submit_cpu_ms": stats.get("gpu_submit_cpu_ms"),
				"selection_gpu_ms": stats.get("selection_gpu_ms"),
				"gpu_vertex_upload_bytes": stats.get("gpu_vertex_upload_bytes"),
				"gpu_frame": stats.get("gpu_frame"),
				"content_size": str(stats.get("content_size")),
				"viewport_gpu_ms": RenderingServer.viewport_get_measured_render_time_gpu(root.get_viewport_rid()),
				"imgui_host_ms": ui.get("host_ms"),
				"imgui_draw_data_ms": ui.get("draw_data_ms", 0.0),
				"imgui_pack_ms": ui.get("pack_ms", 0.0),
				"imgui_canvas_ms": ui.get("canvas_ms", 0.0),
				"imgui_batches": ui.get("batches"),
				"profiler_collector_ms": profiler.latest.get("collector_ms"),
			}
			previous = now
		samples[phase] = rows
	var file := FileAccess.open(output.path_join("debug-performance.json"), FileAccess.WRITE)
	if file == null:
		await _finish(false, "cannot write Debug comparison")
		return
	file.store_string(JSON.stringify({
		"samples": samples, "selection": selection, "resolution": str(root.size),
		"renderer": RenderingServer.get_current_rendering_method(),
		"driver": RenderingServer.get_current_rendering_driver_name(),
		"render_thread": "safe" if RenderingServer.is_on_render_thread() else "separate",
		"gpu": RenderingServer.get_video_adapter_name(),
		"refresh_hz": DisplayServer.screen_get_refresh_rate(root.current_screen),
		"vsync": DisplayServer.window_get_vsync_mode(), "max_fps": Engine.max_fps,
		"profiler_collector_active": profiler.enabled,
		"engine": Engine.get_version_info(),
	}))
	file.close()
	debug.debug_ui_sample_mode = 0
	if not await _check_debug_selection_controls(debug, int(selection.skin)):
		await _finish(false, "Debug selection interaction check failed")
		return
	await _finish(true, "Debug ImGui comparison completed")

func _check_debug_selection_controls(debug: Node, original_skin: int) -> bool:
	await _press(KEY_F8)
	if not debug.is_console_open():
		return false
	await _press(KEY_F8)
	if debug.is_console_open():
		return false
	await _press(KEY_DOWN)
	if not await _await_loaded() or int(frontend.layout.menu_state().selection.skin) == original_skin:
		return false
	var original_size := root.size
	root.size = Vector2i(1600, 900)
	for index in 10:
		await process_frame
	if not frontend.last_stats.get("shared_frontend_target", false) or root.size != Vector2i(1600, 900):
		return false
	root.size = original_size
	await _press(KEY_BACKSPACE)
	if not await _await_class("View Event Tree"):
		return false
	await create_timer(1.2).timeout
	await _press(KEY_SPACE)
	return await _await_class("Select Car") and await _await_loaded()
