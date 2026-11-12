extends Node
# TASK-0107. Unified source-World transport/UI checks; no original equivalence.
const ENTRY := preload("res://scripts/runtime/world_debug_entry.gd")
var report := {"scope": "port World display/input/lifecycle", "restarts": 0, "roundtrips": 0, "routes": [], "original_equivalence": false, "native_resource_baseline_checked": false}

var active_footprint: Dictionary = {}
var closed_footprint: Dictionary = {}
var cleanup_checks := 0
var host_observations: Array[Dictionary] = []

func _footprint(state: Dictionary) -> Dictionary:
	var value: Dictionary = state.get("resource_footprint", {})
	for field in ["regions", "payload_bytes", "owned_allocations"]:
		if not value.has(field):
			return {}
	return value.duplicate(true)

func _audio_players(node: Node) -> int:
	var count := 1 if node is AudioStreamPlayer or node is AudioStreamPlayer2D or node is AudioStreamPlayer3D else 0
	for child in node.get_children():
		count += _audio_players(child)
	return count

func _host_summary() -> Dictionary:
	var summary := {"scope": "Godot host observations after 3 paused settle frames; first3 Ready samples excluded from trend", "samples": host_observations.size(), "warmup_samples": mini(3, host_observations.size()), "gpu_covered": DisplayServer.get_name() != "headless", "async_scope": "Owned World/track/actor ready flags only; native worker queue/thread/handle counters are external observations", "ranges": {}}
	for field in ["nodes", "resources", "audio_players", "gpu_texture_bytes", "gpu_buffer_bytes", "gpu_video_bytes"]:
		var values: Array[int] = []
		for row in host_observations:
			if row.has(field):
				values.append(int(row[field]))
		if values.is_empty():
			continue
		var minimum: int = values[0]
		var maximum: int = values[0]
		for value in values:
			minimum = mini(minimum, value)
			maximum = maxi(maximum, value)
		var steady := values.slice(mini(3, values.size()))
		var increasing := steady.size() >= 4
		var nondecreasing := steady.size() >= 4
		for index in range(1, steady.size()):
			increasing = increasing and steady[index] > steady[index - 1]
			nondecreasing = nondecreasing and steady[index] >= steady[index - 1]
		summary.ranges[field] = {"first": values.front(), "last": values.back(), "minimum": minimum, "maximum": maximum, "post_warmup_delta": 0 if steady.is_empty() else steady.back() - steady.front(), "strictly_increasing_after_warmup": increasing, "nondecreasing_after_warmup": nondecreasing}
	return summary

func _observe_host(scene: Node) -> void:
	var native: Dictionary = scene.simulation.get_world_state()
	var loading_actors := 0
	for node in scene.actor_nodes.values():
		if node.get_state().get("loading", false):
			loading_actors += 1
	var row := {"ready_sample": host_observations.size(), "step": scene.presented_step, "nodes": int(Performance.get_monitor(Performance.OBJECT_NODE_COUNT)), "resources": int(Performance.get_monitor(Performance.OBJECT_RESOURCE_COUNT)), "audio_players": _audio_players(get_tree().root), "world_loading": bool(native.get("loading", false)), "track_loading": bool(scene.track_renderer.get_state().get("loading", false)), "actor_loading_count": loading_actors}
	if DisplayServer.get_name() != "headless":
		row["gpu_texture_bytes"] = RenderingServer.get_rendering_info(RenderingServer.RENDERING_INFO_TEXTURE_MEM_USED)
		row["gpu_buffer_bytes"] = RenderingServer.get_rendering_info(RenderingServer.RENDERING_INFO_BUFFER_MEM_USED)
		row["gpu_video_bytes"] = RenderingServer.get_rendering_info(RenderingServer.RENDERING_INFO_VIDEO_MEM_USED)
	host_observations.append(row)
	report["host_ready_observations"] = host_observations
	report["host_baseline_summary"] = _host_summary()

func _check_ready_baseline(scene: Node) -> bool:
	scene.user_paused = true
	scene._sync_pause()
	for settle_frame in 3:
		await get_tree().process_frame
	_observe_host(scene)
	var value := _footprint(scene.simulation.get_world_state())
	if value.is_empty():
		_fail("Source World active resource footprint is unavailable")
		return false
	if active_footprint.is_empty():
		active_footprint = value
	elif value != active_footprint:
		_fail("Source World step0 resource footprint changed across identical restart: %s -> %s" % [active_footprint, value])
		return false
	report["active_source_footprint"] = active_footprint
	return true

func _check_source_close(scene: Node) -> bool:
	var result: Dictionary = scene.simulation.action("close")
	var state: Dictionary = scene.simulation.get_world_state()
	var value := _footprint(state)
	if not result.get("ok", false) or not state.get("source_cleanup_attempted", false) or not state.get("source_cleanup_ok", false) or not str(state.get("last_source_cleanup_diagnostic", "")).is_empty():
		_fail("Original source cleanup did not complete: " + str(state))
		return false
	if value.is_empty():
		_fail("Closed source World resource footprint is unavailable")
		return false
	if closed_footprint.is_empty():
		closed_footprint = value
	elif value != closed_footprint:
		_fail("Closed source World resource footprint changed: %s -> %s" % [closed_footprint, value])
		return false
	if scene.audio_display != null:
		scene.audio_display.silence()
		if scene.audio_display.queued_frames != 0 or scene.audio_display.player.playing:
			_fail("Host PCM queue/player retained audio after close")
			return false
	cleanup_checks += 1
	report["closed_source_footprint"] = closed_footprint
	report["source_cleanup_checks"] = cleanup_checks
	report["host_pcm_queue_drained"] = true
	return true

class FaultRuntime extends RefCounted:
	var real: RefCounted
	var calls := 0
	func _init(runtime: RefCounted) -> void:
		real = runtime
	func set_driving_input(throttle: float, brake: float, steering: float, handbrake: float, transmission: int, shift: int, neutral: bool, horn: bool = false) -> Dictionary:
		return real.set_driving_input(throttle, brake, steering, handbrake, transmission, shift, neutral, horn)
	func tick(_delta: float) -> Dictionary:
		calls += 1
		return {"ok": false, "status": "Blocked", "paused": true, "diagnostic": "Port test fault: publication barrier"}
	func action(command: String) -> Dictionary:
		return real.action(command)

func _ready() -> void:
	_run.call_deferred()

func _wait_world(scene: Node) -> bool:
	var deadline := Time.get_ticks_msec() + 180000
	while scene.phase in ["Loading", "LoadingActors"]:
		if Time.get_ticks_msec() > deadline:
			return false
		await get_tree().process_frame
	return scene.phase in ["Running", "Paused"] and not scene.last_world.is_empty()

func scene_state_error(state: Dictionary) -> String:
	return str({"ok": state.get("ok"), "loading": state.get("loading"), "error": state.get("error"), "phase": state.get("phase"), "source": state.get("source")})

func _wait_viewer(scene: Node) -> bool:
	var deadline := Time.get_ticks_msec() + 180000
	while true:
		var ready: bool = bool(scene.car_state.get("ok", false)) and (scene.viewer_kind == "car" or bool(scene.track_state.get("ok", false)))
		if ready and not scene.pending_load and not scene.track_pending and scene.world_restore_pending.is_empty() and not DebugTools.operation_pending:
			return true
		if Time.get_ticks_msec() > deadline:
			return false
		if not scene.pending_load and not scene.track_pending and not DebugTools.operation_pending:
			if not str(scene.car_state.get("error", "")).is_empty() or not str(scene.track_state.get("error", "")).is_empty():
				return false
		await get_tree().process_frame
	return false

func _fail(reason: String) -> void:
	report["ok"] = false
	report["error"] = reason
	_write_report()
	push_error(reason)
	get_tree().quit(1)

func _write_report() -> void:
	var path := ProjectSettings.globalize_path("res://../local/task0107/world-debug-smoke.json")
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--world-debug-report="):
			path = argument.trim_prefix("--world-debug-report=")
	DirAccess.make_dir_recursive_absolute(path.get_base_dir())
	var output := FileAccess.open(path, FileAccess.WRITE)
	if output != null:
		output.store_string(JSON.stringify(report, "  ") + "\n")

func _run() -> void:
	var scene: Node = get_tree().current_scene
	if not await _wait_world(scene):
		if str(scene.diagnostic).contains("not configured") or str(scene.diagnostic).begins_with("Configure HP2"):
			get_tree().quit(77)
			return
		_fail("World display did not become ready: " + str(scene.diagnostic))
		return
	if not await _check_ready_baseline(scene):
		return
	scene.set_process(false)
	scene.focused = true
	scene.user_paused = true
	scene._sync_pause()
	if scene.get_script().resource_path != "res://scripts/runtime/race_scene.gd" or scene.simulation.get_class() != "Ps2DrivingRuntime":
		_fail("Unified World entry/backend differs")
		return
	if not scene.simulation.has_method("get_world_state") or scene.last_world.actors.is_empty():
		_fail("World actors were not published")
		return
	for actor in scene.last_world.actors:
		var node: Node3D = scene.actor_nodes[scene._actor_key(scene.last_world, actor)]
		if not node.visible or node.transform != scene._actor_snapshot(actor).body:
			_fail("Actor renderer does not match source published pose")
			return
	var committed_step := int(scene.presented_step)
	for render_pass in 3:
		scene._present_frame()
	if int(scene.simulation.get_world_state().step) != committed_step:
		_fail("Display/reflection passes advanced source World")
		return
	if DisplayServer.get_name() != "headless":
		await get_tree().process_frame
		await RenderingServer.frame_post_draw
		var image := get_viewport().get_texture().get_image()
		if image.is_empty():
			_fail("World GPU viewport is empty")
			return
		image.save_png(ProjectSettings.globalize_path("res://../local/task0107/world-debug-gpu.png"))
	report["initial_actors"] = scene.last_world.actors.size()
	report["initial_step"] = committed_step
	scene.single_step()
	if scene.phase not in ["Paused", "LoadingActors"] or scene.presented_step <= committed_step:
		_fail("Single source frame did not commit: " + scene.diagnostic)
		return
	var key := InputEventKey.new()
	key.physical_keycode = KEY_W
	key.pressed = true
	scene.user_paused = false
	scene._sync_pause()
	scene._unhandled_input(key)
	scene.discard_elapsed = false
	var control_step := int(scene.presented_step)
	for index in 5:
		scene._process(1.0 / 60.0)
		if scene.phase not in ["Running", "Paused"]:
			_fail("Source input probe blocked before a committed frame: " + scene.diagnostic)
			return
	if scene.presented_step <= control_step or float(scene.simulation.get_state().get("throttle", 0)) != 1.0:
		_fail("Player input was not accepted by the one World session")
		return
	key.pressed = false
	scene._unhandled_input(key)
	if "--world-os-background" in OS.get_cmdline_user_args():
		report["focus_probe"] = "Skipped: explicit background fixture bypasses host focus gating; actual OS focus behavior is not covered"
	else:
		scene._notification(NOTIFICATION_APPLICATION_FOCUS_OUT)
		var paused_step := int(scene.presented_step)
		scene._process(1.0 / 60.0)
		if scene.presented_step != paused_step or not scene.input_adapter.held.is_empty():
			_fail("Focus loss advanced World or retained controls")
			return
		scene._notification(NOTIFICATION_APPLICATION_FOCUS_IN)
		report["focus_probe"] = "Passed: port focus notification paused World and cleared input"
	var actual_runtime: RefCounted = scene.simulation
	var saved_pose: Transform3D = scene.vehicle_renderer.transform
	var fault := FaultRuntime.new(actual_runtime)
	scene.simulation = fault
	scene.phase = "Running"
	scene.discard_elapsed = false
	scene._process(1.0 / 60.0)
	if scene.phase != "Blocked" or scene.vehicle_renderer.transform != saved_pose or fault.calls != 1:
		_fail("Blocked step changed the last complete display")
		return
	scene.toggle_pause()
	scene.single_step()
	scene._process(1.0 / 60.0)
	if scene.phase != "Blocked" or fault.calls != 1:
		_fail("Blocked partial state was resumed or stepped")
		return
	var frozen_camera: Transform3D = scene.camera.transform
	var mouse_down := InputEventMouseButton.new()
	mouse_down.button_index = MOUSE_BUTTON_RIGHT
	mouse_down.pressed = true
	var mouse_move := InputEventMouseMotion.new()
	mouse_move.relative = Vector2(30, 10)
	scene._unhandled_input(mouse_down)
	scene._unhandled_input(mouse_move)
	if scene.camera.transform != frozen_camera:
		_fail("Blocked World allowed debug camera mutation")
		return
	report["terminal_camera_frozen"] = true
	scene.simulation = actual_runtime
	report["guard_freeze"] = true
	scene._fail("Port test host presentation failure")
	var native_resume: Dictionary = actual_runtime.action("resume")
	if native_resume.get("ok", true) or scene.vehicle_renderer.transform != saved_pose:
		_fail("Host presentation failure left native World resumable or changed pose")
		return
	report["host_failure_native_terminal"] = true
	if "--world-debug-lifecycle-smoke" in OS.get_cmdline_user_args():
		if not await _roundtrips(scene):
			return
		scene = get_tree().current_scene
	var repeats := 30 if "--world-debug-lifecycle-smoke" in OS.get_cmdline_user_args() or "--world-debug-restarts-smoke" in OS.get_cmdline_user_args() else 1
	for index in repeats:
		if not _check_source_close(scene):
			return
		scene.set_process(true)
		scene.begin_session()
		if not await _wait_world(scene):
			_fail("World restart failed: " + scene.diagnostic)
			return
		if not await _check_ready_baseline(scene):
			return
		scene.user_paused = true
		scene._sync_pause()
		scene.set_process(false)
		report.restarts += 1
		_write_report()
	report["native_resource_baseline_checked"] = true
	report["native_resource_baseline_scope"] = "source Memory regions/payload/owned allocations at Ready step0 and after original close; host PCM queue drained; async/GPU resource counters not covered"
	report["ok"] = true
	_write_report()
	print("WORLD_DEBUG_SMOKE_OK ", JSON.stringify(report))
	get_tree().quit(0)

func _roundtrips(scene: Node) -> bool:
	for index in 10:
		var request: Dictionary = ENTRY.selection(get_tree())
		if not _check_source_close(scene):
			return false
		scene._release()
		var kind := "track" if index % 2 == 0 else "car"
		get_tree().change_scene_to_file("res://scenes/debug/%s_viewer.tscn" % kind)
		await get_tree().process_frame
		await get_tree().process_frame
		var viewer: Node = get_tree().current_scene
		while viewer.car_catalog.is_empty():
			await get_tree().process_frame
		viewer.viewer_ui_action({"action": "select_car", "car_id": int(request.car_id), "skin_id": int(request.skin_id)})
		if kind == "track":
			DebugTools.request("load", "TRACKS/TRACKB%d.LZC" % int(request.track))
		await get_tree().process_frame
		await get_tree().process_frame
		if not await _wait_viewer(viewer):
			_fail("Original viewer resource selection failed before roundtrip: car=%s track=%s" % [scene_state_error(viewer.car_state), scene_state_error(viewer.track_state)])
			return false
		var selected: Dictionary = viewer.world_debug_launch_request()
		var camera_pose: Transform3D = viewer.camera.transform
		if index % 3 == 2:
			DebugTools._dispatch_request({"action": "driving"})
			report.routes.append("F8 driving")
		else:
			viewer.viewer_ui_action({"action": "track_drive" if kind == "track" else "test_drive"})
			report.routes.append("Drive on track" if kind == "track" else "Test drive")
		await get_tree().process_frame
		await get_tree().process_frame
		scene = get_tree().current_scene
		if not await _wait_world(scene):
			_fail("Unified viewer route failed: " + scene.diagnostic)
			return false
		if scene.car_id != int(selected.car_id) or scene.skin_id != int(selected.skin_id) or scene.track_id != int(selected.track):
			_fail("Unified route changed selected car/skin/track")
			return false
		scene.set_process(false)
		scene.exit_session()
		await get_tree().process_frame
		await get_tree().process_frame
		viewer = get_tree().current_scene
		if not await _wait_viewer(viewer) or viewer.camera.transform != camera_pose:
			_fail("World return lost viewer resources/camera")
			return false
		report.roundtrips += 1
		_write_report()
		# Return to the one scene after every route so the caller can continue
		# restart checks after the final viewer roundtrip too.
		if index <= 9:
			viewer.viewer_ui_action({"action": "track_drive" if kind == "track" else "test_drive"})
			await get_tree().process_frame
			await get_tree().process_frame
			scene = get_tree().current_scene
			if not await _wait_world(scene):
				_fail("Roundtrip World re-entry failed")
				return false
	return true
