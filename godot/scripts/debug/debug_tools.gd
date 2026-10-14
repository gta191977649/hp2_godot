extends CanvasLayer
# Port-side session overlay. Survives scene changes; never owns recovered game state.
signal console_toggled(open: bool)
var host: Object
var info_enabled := false
var frame_ms := 0.0
var statistics: Dictionary = {"screen": "frontend"}
var last_frame: Dictionary = {}
var scene_owner: Node
var operation_pending := false
var title_cooldown := 0.0
var frontend_scene := "res://scenes/frontend_native.tscn"

func _ready() -> void:
	layer = 100
	process_priority = 100
	if not ClassDB.class_exists("Ps2RenderHost"):
		set_process(false)
		return
	host = ClassDB.instantiate("Ps2RenderHost")
	var surface := Node2D.new()
	add_child(surface)
	host.attach(surface.get_canvas_item())
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("user://debug"))
	info_enabled = "--info" in OS.get_cmdline_user_args()
	var test_session := false
	for argument in OS.get_cmdline_user_args():
		test_session = test_session or argument in ["--viewer-smoke", "--car-render-smoke", "--material-gpu-smoke"] or argument.begins_with("--expect-debug=")
	# Headless test viewport constraints must not overwrite the user's viewer layout/history.
	var settings := "user://debug/smoke-imgui.ini" if test_session else "user://debug/imgui.ini"
	host.console_init(ProjectSettings.globalize_path(settings), info_enabled)
	if "--viewer-smoke" in OS.get_cmdline_user_args():
		var runner := load("res://scripts/debug/viewer_smoke.gd").new() as Node
		add_child(runner)
	if "--car-render-smoke" in OS.get_cmdline_user_args():
		var runner := load("res://scripts/debug/car_render_smoke.gd").new() as Node
		add_child(runner)

func register_scene(owner: Node, kind: String) -> void:
	scene_owner = owner
	statistics = {"screen": kind}
	if host != null:
		var result: Dictionary = host.viewer_activate(kind)
		if not result.ok:
			write_log(str(result.error), true)

func unregister_scene(owner: Node) -> void:
	if scene_owner == owner:
		scene_owner = null

func set_info(value: bool) -> void:
	info_enabled = value
	if host != null and host.console_available():
		host.console_execute("info on" if value else "info off")

func write_log(text: String, error: bool = false) -> void:
	if host != null:
		host.console_log(text, error)
	print(text)

func is_console_open() -> bool:
	return host != null and host.console_visible()

func handle_input(event: InputEvent) -> bool:
	if host == null:
		return false
	var before := is_console_open()
	var consumed: bool = host.console_input(event)
	var after := is_console_open()
	if before != after:
		if after:
			Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
			_release_camera()
		console_toggled.emit(after)
	if consumed:
		get_viewport().set_input_as_handled()
	return consumed

func request(action: String, source: String = "") -> bool:
	if host == null:
		return false
	var result: Dictionary = host.viewer_request(action, source)
	if not result.ok:
		write_log(str(result.error), true)
	return bool(result.ok)

func _release_camera() -> void:
	if is_instance_valid(scene_owner) and scene_owner.has_method("release_camera"):
		scene_owner.release_camera()

func _process(delta: float) -> void:
	if host == null:
		return
	frame_ms = delta * 1000 if frame_ms == 0 else lerpf(frame_ms, delta * 1000, 0.1)
	var size := get_viewport().get_visible_rect().size
	title_cooldown -= delta
	if title_cooldown <= 0 and DisplayServer.get_name() != "headless":
		title_cooldown = 0.25
		get_window().title = "HP2 PS2 Native Port - %d FPS / %.2f ms" % [Engine.get_frames_per_second(), frame_ms] if host.is_debug_build() else "HP2 PS2 Native Port"
	statistics["fps"] = Engine.get_frames_per_second()
	statistics["ms"] = frame_ms
	var snapshot: Dictionary = {"kind": "frontend"}
	if is_instance_valid(scene_owner) and scene_owner.has_method("viewer_ui_snapshot"):
		snapshot = scene_owner.viewer_ui_snapshot()
	host.viewer_ui_snapshot(snapshot)
	# This shared ImGui frame owns FPS, console and viewer controls in every Debug build.
	last_frame = host.console_frame(size.x, size.y, delta, statistics)
	info_enabled = bool(last_frame.get("info", info_enabled))
	if not last_frame.get("ok", false):
		push_error(str(last_frame.get("error", "Debug overlay failed")))
	if not operation_pending:
		var ui_action: Dictionary = host.viewer_ui_next()
		if ui_action.has("action"):
			operation_pending = true
			_dispatch_ui_action.call_deferred(ui_action)
		else:
			var pending: Dictionary = host.viewer_next()
			if pending.has("action"):
				operation_pending = true
				_dispatch_request.call_deferred(pending)

func input_captured() -> bool:
	return is_console_open() or (host != null and host.viewer_ui_capture())

func _dispatch_ui_action(action: Dictionary) -> void:
	operation_pending = false
	var name := str(action.get("action", ""))
	if name in ["car", "track", "close", "load", "clear", "fixture", "reset"]:
		request(name, str(action.get("source", "")))
	elif is_instance_valid(scene_owner) and scene_owner.has_method("viewer_ui_action"):
		scene_owner.viewer_ui_action(action)

func _dispatch_request(pending: Dictionary) -> void:
	operation_pending = false
	var action := str(pending.action)
	if action in ["car", "track", "close"]:
		if action == "close" and str(host.viewer_state().kind) == "frontend":
			write_log("Already in the frontend")
			return
		_release_camera()
		Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
		var scene := frontend_scene if action == "close" else "res://scenes/debug/%s_viewer.tscn" % action
		var error := get_tree().change_scene_to_file(scene)
		if error != OK:
			write_log("Scene transition failed: %s (%d)" % [scene, error], true)
		return
	var result: Dictionary = host.viewer_apply(action, str(pending.get("source", "")))
	if not result.ok:
		write_log(str(result.error), true)
		return
	if is_instance_valid(scene_owner) and scene_owner.has_method("viewer_action"):
		scene_owner.viewer_action(action)
	var state: Dictionary = host.viewer_state()
	if not str(state.diagnostic).is_empty():
		write_log(str(state.diagnostic), true)

func _notification(what: int) -> void:
	if what == NOTIFICATION_APPLICATION_FOCUS_OUT and host != null:
		host.console_focus_lost()
		_release_camera()

func _exit_tree() -> void:
	if host != null:
		host.release()
