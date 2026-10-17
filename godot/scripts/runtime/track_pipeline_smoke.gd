extends Node3D
# TASK-0051 compares two consumers of the same native track frame pipeline.
var runtime_renderer: Node3D
var viewer_renderer: Node3D
var camera: Camera3D
var gpu := false

func _ready() -> void:
	_run.call_deferred()

func _fail(reason: String) -> void:
	push_error(reason)
	get_tree().quit(1)

func _load_pair(number: int) -> bool:
	var runtime_state: Dictionary = runtime_renderer.request_track(number)
	var viewer_state: Dictionary = viewer_renderer.request_track(number)
	var deadline := Time.get_ticks_msec() + 120000
	while runtime_state.get("loading", false) or viewer_state.get("loading", false):
		if Time.get_ticks_msec() > deadline:
			return false
		runtime_state = runtime_renderer.pump_load()
		viewer_state = viewer_renderer.pump_load()
		await get_tree().process_frame
	return runtime_state.get("ok", false) and viewer_state.get("ok", false)

func _frame(renderer: Node3D) -> Dictionary:
	var result: Dictionary = renderer.update_view(camera.position)
	if not result.get("ok", false):
		return {"error": str(result.get("error", "Shared renderer frame failed"))}
	return renderer.frame_summary()

func _capture(renderer: Node3D) -> Image:
	runtime_renderer.visible = renderer == runtime_renderer
	viewer_renderer.visible = renderer == viewer_renderer
	_frame(renderer)
	await get_tree().process_frame
	await RenderingServer.frame_post_draw
	await get_tree().process_frame
	await RenderingServer.frame_post_draw
	return get_viewport().get_texture().get_image()

func _run() -> void:
	gpu = "--track-parity-gpu" in OS.get_cmdline_user_args()
	# Compare only 3D pipeline output; a changing FPS overlay is unrelated.
	var overlay := get_node_or_null("/root/DebugTools")
	if overlay != null:
		overlay.set("visible", false)
		overlay.set_process(false)
	if gpu and DisplayServer.get_name() == "headless":
		get_tree().quit(77)
		return
	get_window().size = Vector2i(1280, 720)
	camera = Camera3D.new()
	camera.near = 0.05
	camera.far = 20000
	camera.keep_aspect = Camera3D.KEEP_WIDTH
	camera.fov = float(0x32DC) * 360.0 / 65536.0
	add_child(camera)
	camera.current = true
	runtime_renderer = ClassDB.instantiate("Ps2TrackRenderer")
	viewer_renderer = ClassDB.instantiate("Ps2TrackViewer")
	add_child(runtime_renderer)
	add_child(viewer_renderer)
	viewer_renderer.visible = false
	if not runtime_renderer.has_method("frame_summary") or not viewer_renderer.has_method("frame_summary"):
		_fail("Shared native frame summary is unavailable")
		return
	if runtime_renderer.get_catalog().is_empty():
		var error: String = str(runtime_renderer.get_state().get("error", ""))
		if error.contains("not configured"):
			get_tree().quit(77)
			return
		_fail("Original track catalogue is unavailable: " + error)
		return
	if not await _load_pair(11):
		_fail("Consumers failed to load the same original track")
		return
	var original: Dictionary = runtime_renderer.get_state()
	var position: Vector3 = original.start_position
	var forward: Vector3 = original.start_forward
	for offset in [0.0, 50.0, -25.0]:
		var target: Vector3 = position + forward * offset
		camera.position = target - forward * 8.0 + Vector3.UP * 3.0
		camera.look_at(target, Vector3.UP)
		await get_tree().process_frame
		var a := _frame(runtime_renderer)
		var b := _frame(viewer_renderer)
		if not str(a.get("error", "")).is_empty() or not str(b.get("error", "")).is_empty():
			_fail("Native frame preparation failed: " + str(a) + " " + str(b))
			return
		for field in ["packet_digest", "commands", "sections", "zone", "visible_instances"]:
			if not a.has(field) or not b.has(field) or a[field] != b[field]:
				_fail("Viewer/runtime native frame mismatch at " + str(offset) + ": " + field)
				return
		if offset == 0.0 and (int(a.commands) <= 0 or int(a.visible_instances) <= 0 or str(a.packet_digest).is_empty()):
			_fail("Matching empty frames do not demonstrate track rendering")
			return
	if gpu:
		var first: Image = await _capture(runtime_renderer)
		var second: Image = await _capture(viewer_renderer)
		first.save_png(ProjectSettings.globalize_path("res://../local/task0051/runtime-track-gpu.png"))
		second.save_png(ProjectSettings.globalize_path("res://../local/task0051/viewer-track-gpu.png"))
		if first.is_empty() or second.is_empty() or first.get_size() != second.get_size() or first.get_data() != second.get_data():
			_fail("Actual viewer/runtime GPU images differ for identical native frame/camera")
			return
		var pixels: PackedByteArray = first.get_data()
		var nonblack := 0
		for index in range(0, pixels.size(), 4):
			if pixels[index] > 4 or pixels[index + 1] > 4 or pixels[index + 2] > 4:
				nonblack += 1
		if nonblack < 1000:
			_fail("GPU parity capture contains no meaningful track coverage")
			return
	for renderer in [runtime_renderer, viewer_renderer]:
		renderer.clear_track()
		var cleared: Dictionary = renderer.get_state()
		if cleared.get("ok", false) or cleared.get("loading", true) or int(cleared.get("render_nodes", -1)) != 0 or int(renderer.frame_summary().get("commands", -1)) != 0:
			_fail("Shared renderer clear retained frame/display resources")
			return
	if not await _load_pair(11):
		_fail("Shared renderers failed to re-enter after clear")
		return
	print("TRACK_PIPELINE_GPU_OK" if gpu else "TRACK_PIPELINE_PARITY_OK")
	get_tree().quit(0)
