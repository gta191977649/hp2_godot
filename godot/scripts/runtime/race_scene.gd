extends Node3D
# TASK-0107. One port debug scene for a shared, source-derived World session.
const ENTRY := preload("res://scripts/runtime/world_debug_entry.gd")
const EVENT_ENTRY := preload("res://scripts/runtime/event_entry.gd")
var event_request: Dictionary = {}
var event_result: Dictionary = {}
var race_ui: CanvasLayer
var source_ui_resources: RefCounted
var source_ui_host: RefCounted
var source_ui_canvas: Control
# TASK-0113. Read-only observation of the actual authored loading presentation.
var source_loading_visible := false
var source_loading_presented := false
var source_loading_presented_count := 0
var source_loading_status: Dictionary = {}
var loading_work_started := false
var mirror_viewport: SubViewport
var mirror_camera: Camera3D
var mirror_overlay_host: RefCounted
var race_render_profile := "fidelity"
var mirror_refresh_usec := -1
var mirror_output_size := Vector2.ZERO
var mirror_refresh_count := 0
var mirror_hold_count := 0
const OUTSIDE_FOV_BANGLE := 0x32DC
var track_id := 11
var car_code := "ELISE"
var car_id := -1
var skin_id := -1
var reverse := false
var track_renderer: Node3D
var nis_renderer: Node3D
var vehicle_renderer: Node3D
var catalog_renderer: Node3D
var simulation: RefCounted
var input_adapter := preload("res://scripts/debug/driving_input.gd").new()
var camera_adapter := preload("res://scripts/debug/driving_camera.gd").new()
var skid_renderer: RefCounted
var audio_display: Node
var camera: Camera3D
var status_label: Label
var debug_panel: VBoxContainer
var pause_button: Button
var step_button: Button
var state: Dictionary = {}
var world_state: Dictionary = {}
var last_world: Dictionary = {}
var pending_world: Dictionary = {}
var track_state: Dictionary = {}
var car_state: Dictionary = {}
var actor_nodes: Dictionary = {}
var actor_resources: Dictionary = {}
var car_catalog: Array = []
var phase := "Empty"
var diagnostic := ""
var focused := true
var discard_elapsed := false
var user_paused := false
var single_step_pending := false
var native_tick_calls := 0
var generation := 0
var orbit_dragging := false
var presented_step := -1
var presented_ui_revision := -1
var source_save_pending := false
var source_save_failed := false
var attempt_pending := false
var attempt_baseline: Dictionary = {}
var menu_input_adapter := preload("res://scripts/runtime/frontend_input.gd").new()
var windowed_mode := Window.MODE_WINDOWED

func _ready() -> void:
	RenderingServer.frame_post_draw.connect(_loading_frame_presented)
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--world-os-state=") and not get_tree().has_meta("world_window_probe"):
			get_tree().set_meta("world_window_probe", true)
			get_tree().root.add_child.call_deferred(preload("res://scripts/runtime/world_window_probe.gd").new())
	event_request = EVENT_ENTRY.consume(get_tree())
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--race-render-profile="):
			race_render_profile = argument.trim_prefix("--race-render-profile=")
	if race_render_profile not in ["fidelity", "performance60"]:
		push_error("Unknown race render profile: " + race_render_profile)
		get_tree().quit(1)
		return
	if event_request.is_empty():
		race_render_profile = "fidelity"
	if not event_request.is_empty(): menu_input_adapter.initialize()
	var request: Dictionary = ENTRY.consume(get_tree()) if event_request.is_empty() else event_request
	track_id = int(request.track)
	car_id = int(request.car_id)
	skin_id = int(request.skin_id)
	car_code = str(request.get("car_code", ""))
	reverse = bool(request.reverse)
	camera = Camera3D.new()
	camera.name = "WorldPlayerCamera"
	camera.near = 0.05
	camera.far = 20000
	camera.keep_aspect = Camera3D.KEEP_WIDTH
	camera.fov = float(OUTSIDE_FOV_BANGLE) * 360.0 / 65536.0
	add_child(camera)
	camera.current = true
	camera_adapter.widescreen_hor_plus = true
	var canvas := CanvasLayer.new()
	canvas.layer = 10
	add_child(canvas)
	source_ui_canvas = Control.new()
	source_ui_canvas.mouse_filter = Control.MOUSE_FILTER_IGNORE
	source_ui_canvas.clip_contents = true
	canvas.add_child(source_ui_canvas)
	source_ui_resources = ClassDB.instantiate("Ps2FrontEndLayout")
	var loaded_ui: Dictionary = source_ui_resources.load("GLOBAL/INGAMEB.BUN")
	if not loaded_ui.get("ok", false):
		_fail(_reason(loaded_ui))
		return
	source_ui_resources.load_fonts(PackedStringArray(["GLOBAL/GLOBALB.BUN", "GLOBAL/INGAMEB.BUN", "FRONTEND/FRONTB.BUN"]))
	source_ui_host = ClassDB.instantiate("Ps2RenderHost")
	source_ui_host.attach(source_ui_canvas.get_canvas_item())
	var panel := VBoxContainer.new()
	debug_panel = panel
	panel.position = Vector2(16, 16)
	canvas.add_child(panel)
	status_label = Label.new()
	status_label.add_theme_font_size_override("font_size", 14)
	panel.add_child(status_label)
	var actions := HBoxContainer.new()
	panel.add_child(actions)
	actions.visible = event_request.is_empty()
	pause_button = Button.new()
	pause_button.focus_mode = Control.FOCUS_NONE
	pause_button.text = "Pause (P)"
	pause_button.pressed.connect(toggle_pause)
	pause_button.add_theme_font_size_override("font_size", 14)
	actions.add_child(pause_button)
	step_button = Button.new()
	step_button.focus_mode = Control.FOCUS_NONE
	step_button.text = "Step (.)"
	step_button.pressed.connect(single_step)
	step_button.add_theme_font_size_override("font_size", 14)
	actions.add_child(step_button)
	var restart := Button.new()
	restart.focus_mode = Control.FOCUS_NONE
	restart.text = "Restart (R)"
	restart.pressed.connect(begin_session)
	restart.add_theme_font_size_override("font_size", 14)
	actions.add_child(restart)
	var exit_button := Button.new()
	exit_button.focus_mode = Control.FOCUS_NONE
	exit_button.text = "Return (Esc)"
	exit_button.pressed.connect(exit_session)
	exit_button.add_theme_font_size_override("font_size", 14)
	actions.add_child(exit_button)
	if not event_request.is_empty():
		race_ui = preload("res://scripts/runtime/race_ui.gd").new()
		add_child(race_ui)
		race_ui.pause_requested.connect(toggle_pause)
		race_ui.resume_requested.connect(resume_session)
		race_ui.return_requested.connect(exit_session)
		panel.hide()
	for required in ["Ps2TrackRenderer", "Ps2VehicleRenderer", "Ps2DrivingRuntime", "Ps2NisRenderer"]:
		if not ClassDB.class_exists(required):
			_fail("Required World runtime class is unavailable: " + required)
			return
	nis_renderer = ClassDB.instantiate("Ps2NisRenderer")
	add_child(nis_renderer)
	track_renderer = ClassDB.instantiate("Ps2TrackRenderer")
	if not event_request.is_empty() and track_renderer.has_method("set_race_godot_quality"):
		var godot_sampling := true
		var fxaa := race_render_profile != "performance60"
		for argument in OS.get_cmdline_user_args():
			if argument in ["--race-godot-sampling=true", "--race-godot-sampling=false"]:
				godot_sampling = argument.ends_with("true")
			if argument in ["--race-fxaa=true", "--race-fxaa=false"]:
				fxaa = argument.ends_with("true")
		track_renderer.set_race_godot_quality(godot_sampling, fxaa)
	if race_render_profile == "performance60":
		if not track_renderer.has_method("set_race_performance_raster"):
			push_error("The performance60 profile requires rebuilding the Godot extension")
			get_tree().quit(1)
			return
		track_renderer.set_race_performance_raster(true)
	track_renderer.set_camera_mode(1)
	track_renderer.name = "SharedTrackRenderer"
	add_child(track_renderer)
	catalog_renderer = ClassDB.instantiate("Ps2VehicleRenderer")
	catalog_renderer.name = "WorldPlayerRenderer"
	catalog_renderer.visible = false
	add_child(catalog_renderer)
	vehicle_renderer = catalog_renderer
	car_catalog = catalog_renderer.get_catalog()
	DebugTools.scene_owner = self
	DebugTools.statistics["screen"] = "world_debug"
	begin_session()
	if ("--world-debug-smoke" in OS.get_cmdline_user_args() or "--race-runtime-smoke" in OS.get_cmdline_user_args() or "--world-debug-lifecycle-smoke" in OS.get_cmdline_user_args() or "--world-debug-restarts-smoke" in OS.get_cmdline_user_args()) and not get_tree().has_meta("world_debug_smoke_started"):
		get_tree().set_meta("world_debug_smoke_started", true)
		get_tree().root.add_child.call_deferred(preload("res://scripts/runtime/race_scene_smoke.gd").new())

func _reason(result: Dictionary) -> String:
	var reason := str(result.get("error", result.get("diagnostic", "")))
	if reason.is_empty(): return "Native operation returned an unsuccessful response (phase=%s, keys=%s)" % [str(result.get("phase", "")), str(result.keys())]
	return reason

func _native_phase(result: Dictionary) -> String:
	return str(result.get("phase", result.get("status", ""))).to_lower()

func _terminal(result: Dictionary) -> bool:
	return _native_phase(result) in ["blocked", "failed"]

func _release() -> void:
	if source_loading_visible and source_ui_resources != null:
		source_ui_resources.end_loading_screen()
	source_loading_visible = false
	source_loading_presented = false
	loading_work_started = false
	generation += 1
	input_adapter.clear()
	if source_ui_host != null: source_ui_host.release()
	if track_renderer != null and track_renderer.has_method("set_secondary_view"): track_renderer.set_secondary_view(null, 0)
	if mirror_overlay_host != null:
		mirror_overlay_host.release()
		mirror_overlay_host = null
	if mirror_viewport != null:
		mirror_viewport.queue_free()
		mirror_viewport = null
		mirror_camera = null
	mirror_refresh_usec = -1
	mirror_output_size = Vector2.ZERO
	if audio_display != null:
		audio_display.silence()
		audio_display.queue_free()
		audio_display = null
	if track_renderer != null:
		track_renderer.set_driving_effect_source(null)
	if simulation != null:
		var closed: Dictionary = simulation.action("close")
		if not closed.get("ok", true):
			DebugTools.write_log("World close: " + _reason(closed), true)
		if simulation.has_method("get_world_state"):
			var cleanup: Dictionary = simulation.get_world_state()
			if cleanup.get("source_cleanup_attempted", false):
				var cleanup_ok: bool = bool(cleanup.get("source_cleanup_ok", false))
				DebugTools.write_log("World source cleanup: " + ("complete" if cleanup_ok else str(cleanup.get("last_source_cleanup_diagnostic", "failed without diagnostic"))), not cleanup_ok)
		simulation = null
	if skid_renderer != null:
		skid_renderer.clear()
		skid_renderer = null
	for node in actor_nodes.values():
		node.clear_car()
		if node != catalog_renderer:
			node.queue_free()
	actor_nodes.clear()
	if nis_renderer != null: nis_renderer.apply_packets([])
	actor_resources.clear()
	if catalog_renderer != null:
		catalog_renderer.clear_car()
		catalog_renderer.visible = false
	vehicle_renderer = catalog_renderer
	if track_renderer != null:
		track_renderer.clear_track()
	state.clear()
	world_state.clear()
	last_world.clear()
	pending_world.clear()
	track_state.clear()
	car_state.clear()
	presented_step = -1
	presented_ui_revision = -1
	source_save_pending = false
	source_save_failed = false
	attempt_pending = false
	attempt_baseline.clear()
	camera_adapter.reset()
	orbit_dragging = false
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE

func begin_session() -> void:
	if track_renderer == null or catalog_renderer == null:
		return
	if not event_result.is_empty():
		exit_session()
		return
	_release()
	if source_ui_host != null: source_ui_host.attach(source_ui_canvas.get_canvas_item())
	diagnostic = ""
	user_paused = false
	single_step_pending = false
	discard_elapsed = true
	phase = "Loading"
	ENTRY.remember(get_tree(), {"track": track_id, "car_id": car_id, "skin_id": skin_id, "car_code": car_code, "reverse": reverse})
	simulation = ClassDB.instantiate("Ps2DrivingRuntime")
	if not simulation.has_method("configure_world") or not simulation.has_method("get_world_state"):
		_fail("The loaded DLL does not provide the unified source World runtime")
		return
	if event_request.is_empty():
		state = simulation.configure_world(car_id, skin_id, track_id, reverse, car_code)
	else:
		var launch := event_request.duplicate(true)
		# Source World publishes its actual selected Player POV for HUD predicates.
		state = simulation.configure_event_world(launch)
	if _terminal(state) or (state.has("ok") and not state.ok):
		_fail(_reason(state), _native_phase(state) == "blocked")
		return
	if not await _begin_source_loading(): return
	# configure_world records choices; run submits source loading asynchronously.
	# The bridge opens ReadyPaused so asset upload cannot advance source time.
	state = simulation.action("run")
	if _terminal(state) or not state.get("ok", false):
		_fail(_reason(state), _native_phase(state) == "blocked")
		return
	track_state = track_renderer.request_track(track_id)
	if not track_state.get("loading", false) and not track_state.get("ok", false):
		_fail(_reason(track_state))
		return
	skid_renderer = preload("res://scripts/debug/driving_skids.gd").new(self)
	audio_display = preload("res://scripts/runtime/world_audio.gd").new()
	add_child(audio_display)
	_update_status()

func _begin_source_loading() -> bool:
	# Explicit PC first-frame barrier before actual World/track jobs. Source
	# authored two-second pulse is not a loading minimum or progress estimate.
	phase = "Loading"
	loading_work_started = false
	source_loading_presented = false
	var loading: Dictionary = source_ui_resources.begin_loading_screen(int(event_request.get("tree_type", -1)), int(event_request.get("transmission", 0)), int(event_request.get("language", 0)))
	if not loading.get("ok", false):
		_fail(_reason(loading))
		return false
	source_loading_status = loading
	source_loading_visible = true
	# Only the source loading document is visible during preparation. Legacy
	# laboratory/pause controls resume independently once loading ends.
	if race_ui != null: race_ui.hide()
	if debug_panel != null: debug_panel.hide()
	var launch_generation := generation
	if not _draw_source_loading(0.0): return false
	await RenderingServer.frame_post_draw
	if not is_inside_tree() or generation != launch_generation or not source_loading_visible:
		return false
	source_loading_presented = true
	loading_work_started = true
	return true

func _loading_frame_presented() -> void:
	if source_loading_visible:
		source_loading_presented = true
		source_loading_presented_count += 1

func _draw_source_loading(elapsed: float) -> bool:
	if not source_loading_visible: return true
	var updated: Dictionary = source_ui_resources.tick_loading_screen(elapsed)
	if not updated.get("ok", false):
		_fail(_reason(updated))
		return false
	source_loading_status = updated
	var clip: Array = updated.get("clip_bounds", [])
	if clip.size() != 4:
		_fail("Original loading viewport clip is absent")
		return false
	var size := get_viewport().get_visible_rect().size
	_layout_source_ui_canvas(size, Vector4(float(clip[0]), float(clip[1]), float(clip[2]), float(clip[3])))
	var drawn: Dictionary = source_ui_host.draw_loading(source_ui_resources, size.x, size.y)
	if not drawn.get("ok", false):
		_fail(_reason(drawn))
		return false
	source_loading_status["draw"] = drawn
	return true

func _finish_source_loading() -> bool:
	if not source_loading_visible: return true
	var ended: Dictionary = source_ui_resources.end_loading_screen()
	if not ended.get("ok", false):
		_fail(_reason(ended))
		return false
	source_loading_status = ended
	source_loading_visible = false
	return true

func _actor_key(frame: Dictionary, actor: Dictionary) -> String:
	return "%d:%d:%d" % [generation, int(frame.get("generation", 0)), int(actor.get("address", 0))]

func _actor_snapshot(actor: Dictionary) -> Dictionary:
	return actor.get("state", actor)

func _validate_actor(actor: Dictionary) -> String:
	var snapshot := _actor_snapshot(actor)
	if int(actor.get("address", 0)) == 0 or not snapshot.get("body_known", false) or not snapshot.get("wheels_known", false):
		return "World actor has no complete source identity/body/wheels"
	if not snapshot.get("body") is Transform3D or not snapshot.body.is_finite() or is_zero_approx(snapshot.body.basis.determinant()):
		return "World actor body transform is invalid"
	var wheels: Array = snapshot.get("wheels", [])
	if wheels.size() != 4:
		return "World actor requires four published source wheels"
	for wheel in wheels:
		for key in ["tire", "brake", "blur"]:
			if not wheel.get(key) is Transform3D or not wheel[key].is_finite():
				return "World wheel transform is invalid: " + key
		for key in ["phase", "angular_velocity", "blur_alpha"]:
			if not wheel.has(key) or not is_finite(float(wheel[key])):
				return "World wheel scalar is unavailable: " + key
	return ""

func _resource_for(actor: Dictionary) -> Dictionary:
	for item in car_catalog:
		if int(item.id) == int(actor.get("car_id", -1)):
			return item
	return {}

func _prepare_actors(frame: Dictionary) -> bool:
	var actors: Array = frame.get("actors", [])
	if actors.is_empty() or int(frame.get("player_car", 0)) == 0:
		_fail("Ready World did not publish its actual Player and actors")
		return false
	var complete := true
	for actor in actors:
		var invalid := _validate_actor(actor)
		if not invalid.is_empty():
			_fail(invalid)
			return false
		var key := _actor_key(frame, actor)
		var signature := Vector2i(int(actor.get("car_id", -1)), int(actor.get("skin_id", -1)))
		if not actor_nodes.has(key):
			var node: Node3D = catalog_renderer if int(actor.address) == int(frame.player_car) else ClassDB.instantiate("Ps2VehicleRenderer")
			if node != catalog_renderer:
				node.name = "SourceCar_%X" % int(actor.address)
				add_child(node)
			node.visible = false
			actor_nodes[key] = node
		if actor_resources.get(key) != signature:
			var item := _resource_for(actor)
			if item.is_empty() or not item.get("available", false):
				_fail("Source World vehicle resource unavailable: " + str(actor.get("car_id", -1)))
				return false
			var requested: Dictionary = actor_nodes[key].request_car(str(item.path), signature.x, signature.y)
			actor_resources[key] = signature
			if not requested.get("loading", false) and not requested.get("ok", false):
				_fail(_reason(requested))
				return false
		var load_started := Time.get_ticks_usec()
		var loaded: Dictionary = actor_nodes[key].pump_load()
		if phase in ["Loading", "LoadingActors"]:
			ProfileDebug.event_loading_scope("Vehicle load pump", load_started)
		if loaded.get("loading", false):
			complete = false
		elif not loaded.get("ok", false):
			_fail(_reason(loaded))
			return false
	return complete

func _restore_visuals() -> void:
	for node in actor_nodes.values():
		node.visible = false
	for actor in last_world.get("actors", []):
		var key := _actor_key(last_world, actor)
		if actor_nodes.has(key):
			actor_nodes[key].apply_driving_snapshot(_actor_snapshot(actor))
			actor_nodes[key].visible = actor.get("visible", true)
	if nis_renderer != null:
		nis_renderer.apply_packets(last_world.get("nis_draws", []))
	if track_renderer.has_method("apply_world_objects"):
		track_renderer.apply_world_objects(last_world.get("objects", []), last_world.get("scenery_flags", []))
	if track_renderer.has_method("set_world_vehicle_visuals"):
		track_renderer.set_world_vehicle_visuals(_world_vehicle_visuals(last_world))

func _world_vehicle_visuals(frame: Dictionary) -> Array:
	var visual_states: Array = []
	for actor in frame.get("actors", []):
		var visual: Dictionary = actor_nodes[_actor_key(frame, actor)].get_visual_state()
		visual["world_address"] = int(actor.address)
		visual["world_generation"] = int(frame.generation)
		visual_states.append(visual)
	return visual_states

func _set_actor_layers(node: Node, mask: int) -> void:
	if node is GeometryInstance3D:
		node.layers = mask
	for child in node.get_children():
		_set_actor_layers(child, mask)

func _prepare_mirror(frame: Dictionary) -> bool:
	var source: Dictionary = frame.get("mirror_camera", {})
	if source.is_empty():
		track_renderer.set_secondary_view(null, 0)
		if mirror_viewport != null: mirror_viewport.render_target_update_mode = SubViewport.UPDATE_DISABLED
		mirror_refresh_usec = -1
		return true
	if mirror_viewport == null:
		mirror_viewport = SubViewport.new()
		mirror_viewport.name = "SourceRearViewTarget"
		# Source RVM_MASK writes RTT alpha for the final DisplayRVMs sample.
		mirror_viewport.transparent_bg = true
		mirror_viewport.world_3d = get_world_3d()
		mirror_viewport.own_world_3d = false
		add_child(mirror_viewport)
		mirror_camera = Camera3D.new()
		mirror_viewport.add_child(mirror_camera)
		mirror_camera.current = true
		var canvas := CanvasLayer.new()
		mirror_viewport.add_child(canvas)
		var overlay := Control.new()
		overlay.mouse_filter = Control.MOUSE_FILTER_IGNORE
		canvas.add_child(overlay)
		mirror_overlay_host = ClassDB.instantiate("Ps2RenderHost")
		mirror_overlay_host.attach(overlay.get_canvas_item())
	# PC raster policy preserves the original anisotropic projection, then
	# samples it into the original DisplayRVMs rectangle. Native target stays256x64.
	var target_size := Vector2i(roundi(float(source.projection_aspect) * 100.0), 100)
	var output_size := get_viewport().get_visible_rect().size
	var now := Time.get_ticks_usec()
	var performance := race_render_profile == "performance60"
	if performance and mirror_refresh_usec >= 0 and now - mirror_refresh_usec < 33333 \
			and mirror_output_size == output_size and mirror_viewport.size == target_size:
		mirror_hold_count += 1
		return true
	mirror_refresh_usec = now
	mirror_output_size = output_size
	mirror_refresh_count += 1
	mirror_viewport.size = target_size
	mirror_viewport.render_target_update_mode = SubViewport.UPDATE_ONCE if performance else SubViewport.UPDATE_ALWAYS
	mirror_camera.position = source.position
	mirror_camera.look_at(source.target, source.up)
	mirror_camera.near = float(source.near_z)
	mirror_camera.far = float(source.far_z)
	mirror_camera.keep_aspect = Camera3D.KEEP_HEIGHT
	mirror_camera.fov = float(source.vertical_fov)
	mirror_camera.cull_mask = 128 | 256 | (0 if bool(source.hide_car) else 1)
	track_renderer.set_secondary_view(mirror_camera, int(source.fov_bangle))
	var overlay_drawn: Dictionary = mirror_overlay_host.draw_mirror_overlay(simulation, source_ui_resources, mirror_viewport.size.x, mirror_viewport.size.y)
	if not overlay_drawn.get("ok", false):
		_fail(_reason(overlay_drawn))
		return false
	source_ui_host.set_world_mirror(mirror_viewport.get_texture())
	return true

func _commit_frame(frame: Dictionary) -> bool:
	# Source frame is complete. Roll back only host poses if a GPU binding rejects it.
	if not _prepare_mirror(frame): return false
	if source_ui_host != null:
		var prepared: Dictionary = source_ui_host.prepare_world_ui(simulation, source_ui_resources)
		if not prepared.get("ok", false):
			_fail(_reason(prepared))
			return false
	var active: Dictionary = {}
	for actor in frame.get("actors", []):
		var key := _actor_key(frame, actor)
		var applied: Dictionary = actor_nodes[key].apply_driving_snapshot(_actor_snapshot(actor))
		if not applied.get("ok", false):
			_restore_visuals()
			_fail(_reason(applied))
			return false
		# Host debug capture policy: reuse the committed main-view mesh/LOD.
		# Other actors enter shared capture layers; the player excludes itself.
		_set_actor_layers(actor_nodes[key], 1 if int(actor.address) == int(frame.player_car) else 0x17f)
		active[key] = actor.get("visible", true)
	var nis_result: Dictionary = nis_renderer.apply_packets(frame.get("nis_draws", []))
	if not nis_result.get("ok", false):
		_restore_visuals()
		_fail(_reason(nis_result))
		return false
	var objects: Array = frame.get("objects", [])
	if track_renderer.has_method("apply_world_objects"):
		var result: Dictionary = track_renderer.apply_world_objects(objects, frame.get("scenery_flags", []))
		if not result.get("ok", false):
			_restore_visuals()
			_fail(_reason(result))
			return false
	elif not objects.is_empty() and not frame.get("world_objects_presented", false):
		_restore_visuals()
		_fail("Source World dynamic-object display binding is unavailable")
		return false
	if not track_renderer.has_method("set_world_vehicle_visuals"):
		_restore_visuals()
		_fail("Source World participant display binding is unavailable")
		return false
	var shadows: Dictionary = track_renderer.set_world_vehicle_visuals(_world_vehicle_visuals(frame))
	if not shadows.get("ok", false):
		_restore_visuals()
		_fail(_reason(shadows))
		return false
	if not simulation.has_method("commit_presentation"):
		_restore_visuals()
		_fail("Source World presentation commit barrier is unavailable")
		return false
	var committed: Dictionary = simulation.commit_presentation(int(frame.get("step", -1)), int(frame.get("source_ui_revision", -1)))
	if not committed.get("ok", false):
		_restore_visuals()
		_fail(_reason(committed), _native_phase(committed) == "blocked")
		return false
	for key in actor_nodes.keys():
		actor_nodes[key].visible = active.get(key, false)
		if not active.has(key):
			# A new native generation reuses the dedicated Player renderer.
			# Removing its old key must not release the new generation's car.
			var retained := false
			for active_key in active:
				if actor_nodes.get(active_key) == actor_nodes[key]: retained = true
			if not retained:
				actor_nodes[key].clear_car()
				if actor_nodes[key] != catalog_renderer:
					actor_nodes[key].queue_free()
			actor_nodes.erase(key)
			actor_resources.erase(key)
	last_world = frame.duplicate(true)
	world_state = frame
	presented_step = int(frame.get("step", 0))
	presented_ui_revision = int(frame.get("source_ui_revision", 0))
	for actor in frame.get("actors", []):
		if int(actor.address) == int(frame.player_car):
			vehicle_renderer = actor_nodes[_actor_key(frame, actor)]
			state = _actor_snapshot(actor).duplicate(true)
			car_state = vehicle_renderer.get_state()
			car_id = int(actor.get("car_id", car_id))
			skin_id = int(actor.get("skin_id", skin_id))
			car_code = str(actor.get("car_code", car_code))
			break
	# Persist the source-resolved catalog identities after a code/default launch.
	# Subsequent viewer selection must not retain the original -1 request IDs.
	ENTRY.remember(get_tree(), {"track": track_id, "car_id": car_id, "skin_id": skin_id, "car_code": car_code, "reverse": reverse})
	if simulation.has_method("take_world_pcm"):
		audio_display.publish(simulation.take_world_pcm())
	_accept_event_result(frame)
	return true

func _accept_event_result(frame: Dictionary) -> void:
	if event_request.is_empty():
		return
	var outcome: Dictionary = frame.get("event_result", {})
	if not outcome.get("ready", false):
		return
	if has_meta(EVENT_ENTRY.COMMITTED_RESULT) and get_meta(EVENT_ENTRY.COMMITTED_RESULT) != outcome:
		_fail("Source result changed after its private save acknowledgement")
		return
	event_result = outcome.duplicate(true)
	input_adapter.clear()
	phase = "Results"
	if audio_display != null:
		audio_display.silence()
	world_state = frame
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	_present_frame()

# Source menu updates share the presentation barrier without resubmitting actors.
func _commit_menu_frame() -> bool:
	var next: Dictionary = simulation.get_world_state()
	if _terminal(next) or not next.get("ok", false):
		_fail(_reason(next), _native_phase(next) == "blocked")
		return false
	if bool(next.get("source_return_requested", false)):
		exit_session()
		return false
	if int(next.get("source_generation", 0)) != int(last_world.get("source_generation", 0)):
		if not _prepare_actors(next):
			pending_world = next
			phase = "LoadingActors"
			return false
		if not _commit_frame(next): return false
	else:
		if not _prepare_mirror(next): return false
		var prepared: Dictionary = source_ui_host.prepare_world_ui(simulation, source_ui_resources)
		if not prepared.get("ok", false):
			_fail(_reason(prepared))
			return false
		var committed: Dictionary = simulation.commit_presentation(int(next.step), int(next.source_ui_revision))
		if not committed.get("ok", false):
			_fail(_reason(committed))
			return false
		last_world = next.duplicate(true)
		world_state = next
		presented_ui_revision = int(next.source_ui_revision)
	_accept_event_result(next)
	if bool(next.get("source_attempt_pending", false)) and not attempt_pending:
		attempt_pending = true
		_acknowledge_attempt.call_deferred()
	if bool(next.get("source_autosave_requested", false)) and not source_save_pending and not source_save_failed:
		source_save_pending = true
		_save_source_result.call_deferred()
	phase = str(next.get("phase", phase))
	if phase == "Running":
		user_paused = false
		discard_elapsed = true
		audio_display.resume_stream()
	else:
		audio_display.silence()
	_present_frame()
	_update_status()
	return true

# The source has already selected its next attempt. This only reseals storage.
func _acknowledge_attempt() -> void:
	var frontend: Node = get_tree().get_meta(EVENT_ENTRY.FRONTEND, null)
	if not is_instance_valid(frontend):
		_fail("Retained frontend is absent during source continuation")
		return
	if bool(world_state.get("source_attempt_requires_saved", false)):
		if EVENT_ENTRY.save_result(self, event_result) != OK:
			attempt_pending = false
			_fail(diagnostic)
			return
	if attempt_baseline.is_empty():
		attempt_baseline = frontend.layout.continue_event_attempt(int(event_request.generation), bool(world_state.get("source_attempt_requires_saved", false)))
		if not attempt_baseline.get("ok", false):
			_fail(_reason(attempt_baseline))
			return
	if bool(world_state.get("source_continuation_reload", false)):
		_clear_input()
		if not await _begin_source_loading(): return
	var acknowledged: Dictionary = simulation.acknowledge_event_attempt(attempt_baseline)
	if _terminal(acknowledged) or not acknowledged.get("ok", false):
		_fail(_reason(acknowledged), _native_phase(acknowledged) == "blocked")
		return
	event_request.generation = int(attempt_baseline.generation)
	attempt_baseline.clear()
	attempt_pending = false
	event_result.clear()
	if has_meta(EVENT_ENTRY.COMMITTED_RESULT): remove_meta(EVENT_ENTRY.COMMITTED_RESULT)
	if str(acknowledged.get("phase", "")) == "Loading":
		_clear_input()
		audio_display.silence()
		if mirror_overlay_host != null:
			mirror_overlay_host.release()
			mirror_overlay_host = null
		track_renderer.set_secondary_view(null, 0)
		if mirror_viewport != null:
			mirror_viewport.queue_free()
			mirror_viewport = null
			mirror_camera = null
		source_ui_host.release()
		source_ui_host.attach(source_ui_canvas.get_canvas_item())
		if source_loading_visible and not _draw_source_loading(0.0): return
		for node in actor_nodes.values():
			node.clear_car()
			if node != catalog_renderer: node.queue_free()
		actor_nodes.clear()
		if nis_renderer != null: nis_renderer.apply_packets([])
		actor_resources.clear()
		track_renderer.clear_track()
		track_id = int(acknowledged.track_id)
		car_id = int(acknowledged.car_id)
		skin_id = int(acknowledged.skin_id)
		track_state = track_renderer.request_track(track_id)
		last_world.clear()
		world_state.clear()
		presented_step = -1
		presented_ui_revision = -1
		phase = "Loading"
	else:
		_commit_menu_frame()

# Save after the authored saving popup has reached an actual GPU frame.
func _save_source_result() -> void:
	var owner_generation := generation
	await RenderingServer.frame_post_draw
	if not is_inside_tree() or owner_generation != generation: return
	var saved := EVENT_ENTRY.save_result(self, event_result)
	if saved != OK:
		source_save_pending = false
		source_save_failed = true
		_update_status()
		return
	var acknowledged: Dictionary = simulation.action("acknowledge_autosave")
	source_save_pending = false
	source_save_failed = false
	if _terminal(acknowledged) or not acknowledged.get("ok", false):
		_fail(_reason(acknowledged), _native_phase(acknowledged) == "blocked")
		return
	_commit_menu_frame()

func _source_menu_input(event: InputEvent) -> bool:
	if event_request.is_empty() or simulation == null or DebugTools.input_captured(): return false
	if phase in ["Blocked", "Failed"]: return false
	var key_press: bool = event is InputEventKey and event.pressed and not event.echo
	if key_press and event.physical_keycode in [KEY_F8, KEY_F11]: return false
	if source_save_failed:
		if key_press and event.physical_keycode in [KEY_ENTER, KEY_KP_ENTER]: resume_session()
		return true
	if key_press and event.physical_keycode in [KEY_ESCAPE, KEY_P]:
		if Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
			release_camera()
		elif phase == "Running":
			_clear_input()
			simulation.action("open_pause")
			_commit_menu_frame()
		elif int(world_state.get("source_menu_screen", 0)) != 0:
			simulation.menu_input(11 if event.physical_keycode == KEY_ESCAPE else 7, true, 0)
			_commit_menu_frame()
		return true
	if int(world_state.get("source_menu_screen", 0)) == 0: return false
	for request in menu_input_adapter.handle(event):
		simulation.menu_input(int(request.id), bool(request.pressed), int(request.port))
		if not _commit_menu_frame(): break
	return true

func _sync_pause() -> void:
	if phase not in ["Running", "Paused"] or simulation == null:
		return
	if not event_request.is_empty() and bool(world_state.get("source_menu_paused", false)):
		return
	var suspend := user_paused or not focused or DebugTools.input_captured()
	if suspend and phase == "Running":
		_clear_input()
		simulation.action("pause")
		phase = "Paused"
		audio_display.silence()
		discard_elapsed = true
	elif not suspend and phase == "Paused":
		var result: Dictionary = simulation.action("resume")
		if _terminal(result):
			_fail(_reason(result), _native_phase(result) == "blocked")
			return
		phase = "Running"
		audio_display.resume_stream()
		discard_elapsed = true

var profile_input: Dictionary = {}
var profile_process_started := 0

func _process(delta: float) -> void:
	if ProfileDebug.event_benchmark:
		profile_process_started = Time.get_ticks_usec()
		profile_input = {}
	var _profile_scope := ProfileDebug.scope("WorldDebug._process")
	if simulation == null:
		return
	var visual_dt := 0.0
	if phase == "Loading":
		if source_loading_visible and not _draw_source_loading(delta): return
		if not loading_work_started: return
		state = simulation.get_state()
		var load_started := Time.get_ticks_usec()
		track_state = track_renderer.pump_load()
		ProfileDebug.event_loading_scope("Track load pump", load_started)
		if _terminal(state):
			_fail(_reason(state), _native_phase(state) == "blocked")
			return
		if track_state.get("loading", false) or _native_phase(state) in ["loading", "empty", "opening"]:
			_update_status()
			return
		if not track_state.get("ok", false):
			_fail(_reason(track_state))
			return
		pending_world = simulation.get_world_state()
		if _terminal(pending_world):
			_fail(_reason(pending_world), _native_phase(pending_world) == "blocked")
			return
		if not _prepare_actors(pending_world):
			_update_status()
			return
		if not _commit_frame(pending_world):
			return
		if not _finish_source_loading(): return
		phase = "Paused"
		discard_elapsed = true
		_sync_pause()
		_present_frame()
		return
	if phase == "LoadingActors":
		if source_loading_visible and not _draw_source_loading(delta): return
		if _prepare_actors(pending_world) and _commit_frame(pending_world):
			if not _finish_source_loading(): return
			if bool(pending_world.get("source_attempt_pending", false)):
				_commit_menu_frame()
			else:
				phase = "Paused"
				_sync_pause()
		_update_status()
		return
	_sync_pause()
	if not event_request.is_empty() and phase in ["Paused", "Results"] and int(world_state.get("source_menu_screen", 0)) != 0:
		if focused and not DebugTools.input_captured():
			for request in menu_input_adapter.tick(delta):
				simulation.menu_input(int(request.id), bool(request.pressed), int(request.port))
			var updated: Dictionary = simulation.tick_ui(delta)
			if _terminal(updated) or not updated.get("ok", false):
				_fail(_reason(updated), _native_phase(updated) == "blocked")
				return
			if not _commit_menu_frame(): return
	if phase == "Running":
		if discard_elapsed:
			discard_elapsed = false
		else:
			var controls: Dictionary = input_adapter.frame()
			var accepted: Dictionary = simulation.set_driving_input(controls.throttle, controls.brake, controls.steering, controls.handbrake, controls.transmission, controls.shift, controls.neutral, controls.horn)
			if not accepted.get("ok", false):
				_fail(_reason(accepted), _native_phase(accepted) == "blocked")
				return
			var before := presented_step
			native_tick_calls += 1
			if ProfileDebug.event_benchmark:
				profile_input = controls
			var tick_started := Time.get_ticks_usec()
			var result: Dictionary = simulation.tick(delta)
			ProfileDebug.event_scope("World tick (including bridge result)", tick_started)
			if ProfileDebug.event_benchmark and ProfileDebug.capture_active:
				if result.has("profile_world_advance_ms"):
					ProfileDebug.record_function("World native advance", result.profile_world_advance_ms)
					ProfileDebug.record_function("World bridge result", result.profile_world_bridge_result_ms)
					if result.has("profile_world_replay_ms"):
						ProfileDebug.record_function("World Replay", result.profile_world_replay_ms)
						ProfileDebug.record_function("World DoTimestep", result.profile_world_timestep_ms)
						ProfileDebug.record_function("World publish", result.profile_world_publish_ms)
			if _terminal(result) or not result.get("ok", false):
				_fail(_reason(result), _native_phase(result) == "blocked")
				return
			var read_started := Time.get_ticks_usec()
			var next: Dictionary = simulation.get_world_state()
			ProfileDebug.event_scope("World state read", read_started)
			if int(next.get("step", before)) > before or int(next.get("source_ui_revision", -1)) != presented_ui_revision:
				input_adapter.committed()
				if not _prepare_actors(next):
					if phase not in ["Failed", "Blocked"]:
						pending_world = next
						simulation.action("pause")
						phase = "LoadingActors"
						audio_display.silence()
					return
				var commit_started := Time.get_ticks_usec()
				var committed := _commit_frame(next)
				ProfileDebug.event_scope("World state commit", commit_started)
				if not committed:
					return
				visual_dt = float(next.get("world_dt", next.get("dt", 0)))
			_accept_event_result(next)
	if phase in ["Running", "Paused", "Results"]:
		_present_frame(visual_dt if phase == "Running" else 0.0, delta if phase in ["Running", "Paused"] else 0.0)
	_update_status()

func _vehicle_orbit_center(fallback: Vector3) -> Vector3:
	# Port visual-bounds pivot, derived from the actual displayed body meshes.
	var found := false
	var bounds := AABB()
	for part in vehicle_renderer.get_children():
		if part is MeshInstance3D and part.visible and int(part.get_meta("hp2_part_group", -1)) == 0:
			var actual: AABB = part.global_transform * part.get_aabb()
			bounds = bounds.merge(actual) if found else actual
			found = true
			# Extra surface batches inherit the same original body-part pose.
			for batch in part.get_children():
				if batch is MeshInstance3D and batch.visible:
					bounds = bounds.merge(batch.global_transform * batch.get_aabb())
	return bounds.get_center() if found else fallback

func _update_camera(elapsed: float = 0.0) -> void:
	if vehicle_renderer == null or last_world.is_empty():
		return
	var source: Dictionary = last_world.get("camera", {})
	if source.is_empty():
		_fail("Committed source Player camera is unavailable")
		return
	# TASK-0114 host observation only: take the actual committed Player body
	# pivot, never the source camera's speed-dependent look-ahead target.
	var observation := source.duplicate()
	for actor in last_world.get("actors", []):
		if int(actor.address) == int(last_world.player_car):
			var body: Transform3D = _actor_snapshot(actor).body
			observation["orbit_center"] = _vehicle_orbit_center(body.origin)
			observation["orbit_back"] = -body.basis.z
			break
	camera_adapter.apply(camera, observation, elapsed)
	# Layer 1 is the Player only; other actors retain additional capture bits.
	camera.cull_mask = 0xffffe if bool(source.get("hide_car", false)) and not camera_adapter.manual else 0xfffff

func _present_frame(visual_delta: float = 0.0, camera_elapsed: float = 0.0) -> void:
	if last_world.is_empty() or vehicle_renderer == null:
		return
	_update_camera(camera_elapsed)
	track_renderer.set_visual_state(vehicle_renderer.get_visual_state())
	track_renderer.set_driving_effect_source(simulation)
	track_renderer.set_visual_timestep(visual_delta)
	var track_started := Time.get_ticks_usec()
	var view: Dictionary = track_renderer.update_view(camera.position)
	ProfileDebug.event_scope("Track view (including reflection and depth)", track_started)
	if not view.get("ok", false):
		_fail(_reason(view))
		return
	track_state = view
	var vehicle_started := Time.get_ticks_usec()
	for actor in last_world.get("actors", []):
		var node: Node3D = actor_nodes[_actor_key(last_world, actor)]
		node.set_environment_capture(track_renderer.get_environment_capture())
		var result: Dictionary = node.update_view(camera.position, -camera.basis.z, float(view.original_focal_h), -1)
		if not result.get("ok", false):
			_fail(_reason(result))
			return
	ProfileDebug.event_scope("Vehicle view", vehicle_started)
	var clip: Projection = camera.get_camera_projection() * Projection(camera.global_transform.affine_inverse())
	var skids: Dictionary = simulation.render_skids(camera.global_transform, clip, get_viewport().get_visible_rect().size.y)
	if not skids.get("ok", false):
		_fail(_reason(skids), _native_phase(skids) == "blocked")
		return
	skid_renderer.publish(skids.get("commands", []))
	if source_ui_host != null:
		var size := get_viewport().get_visible_rect().size
		var ui_clip: Vector4 = last_world.get("source_ui_clip_bounds", Vector4.ZERO)
		if ui_clip.z <= ui_clip.x or ui_clip.w <= ui_clip.y:
			_fail("Original Flailer viewport clip is absent")
			return
		_layout_source_ui_canvas(size, ui_clip)
		var hud_started := Time.get_ticks_usec()
		var drawn: Dictionary = source_ui_host.draw_world(simulation, source_ui_resources, size.x, size.y)
		ProfileDebug.event_scope("HUD draw", hud_started)
		if not drawn.get("ok", false): _fail(_reason(drawn))

func _layout_source_ui_canvas(size: Vector2, clip: Vector4) -> void:
	# Host placement only; clip comes from the committed original Flailer frame.
	var fit := minf(size.x / 640.0, size.y / 480.0)
	source_ui_canvas.position = (size - Vector2(640.0, 480.0) * fit) * 0.5 + Vector2(clip.x, clip.y) * fit
	source_ui_canvas.size = Vector2(clip.z - clip.x, clip.w - clip.y) * fit

func _clear_input() -> void:
	input_adapter.clear()
	menu_input_adapter.release_all()
	camera_adapter.clear_held_view()
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	if simulation != null:
		simulation.set_driving_input(0.0, 0.0, 0.0, 0.0, -1, 0, false, false)
		if not last_world.is_empty(): simulation.update_driving_camera(0.0, false)

func _fail(reason: String, blocked: bool = false) -> void:
	DebugTools.write_log("Race interrupted: " + reason, true)
	print_stack()
	orbit_dragging = false
	diagnostic = reason
	phase = "Blocked" if blocked else "Failed"
	input_adapter.clear()
	if simulation != null:
		# Source guards already own their original diagnostic. Host publication
		# failures must also make the native adapter terminal, not merely paused.
		if not blocked and simulation.has_method("report_presentation_failure"):
			simulation.report_presentation_failure(reason)
		else:
			simulation.action("pause")
	if track_renderer != null:
		track_renderer.set_driving_effect_source(null)
		track_renderer.set_visual_timestep(0.0)
	if audio_display != null:
		audio_display.silence()
	_update_status()

func toggle_pause() -> void:
	if not event_request.is_empty() and simulation != null:
		_clear_input()
		if bool(world_state.get("source_menu_paused", false)): simulation.menu_input(7, true, 0)
		else: simulation.action("open_pause")
		_commit_menu_frame()
		return
	if phase not in ["Running", "Paused"]:
		return
	user_paused = not user_paused
	_sync_pause()

func resume_session() -> void:
	if source_save_failed:
		source_save_failed = false
		source_save_pending = true
		_save_source_result.call_deferred()
		return
	if not event_request.is_empty():
		toggle_pause()
		return
	if phase != "Paused":
		return
	user_paused = false
	_sync_pause()

func single_step() -> void:
	if phase not in ["Running", "Paused"]:
		return
	user_paused = true
	_sync_pause()
	var controls: Dictionary = input_adapter.frame()
	var accepted: Dictionary = simulation.set_driving_input(controls.throttle, controls.brake, controls.steering, controls.handbrake, controls.transmission, controls.shift, controls.neutral, controls.horn)
	if not accepted.get("ok", false):
		_fail(_reason(accepted), _native_phase(accepted) == "blocked")
		return
	var before := presented_step
	native_tick_calls += 1
	var result: Dictionary = simulation.action("step")
	if _terminal(result) or not result.get("ok", false):
		_fail(_reason(result), _native_phase(result) == "blocked")
		return
	var next: Dictionary = simulation.get_world_state()
	if int(next.get("step", before)) > before or int(next.get("source_ui_revision", -1)) != presented_ui_revision:
		input_adapter.committed()
		if _prepare_actors(next):
			_commit_frame(next)
		else:
			pending_world = next
			if phase not in ["Failed", "Blocked"]:
				phase = "LoadingActors"
	if phase == "Paused":
		_present_frame()
	_update_status()

func _update_status() -> void:
	var loading_only := source_loading_visible and phase not in ["Failed", "Blocked"]
	if debug_panel != null: debug_panel.visible = event_request.is_empty() and not loading_only
	if race_ui != null: race_ui.visible = not loading_only
	if status_label == null:
		return
	if status_label == null:
		return
	if not event_request.is_empty():
		if race_ui != null:
			var display := world_state.duplicate(false)
			display.status = phase
			display.diagnostic = diagnostic
			display.event_result = event_result
			display.source_save_failed = source_save_failed
			display.player_finished = bool(world_state.get("race", {}).get("player_finished", false))
			display.controls_text = "W/S Accelerate / brake / reverse · A/D Steer · Space Handbrake · Q/E Gears · P / Esc Pause · F8 Console"
			var powertrain: Dictionary = state.get("powertrain", {})
			if powertrain.has("speed"):
				display.speed_kmh = absf(float(powertrain.speed)) * 3.6
			race_ui.present(display)
	else:
		var mode_label := "Minimal FreeRun" if str(world_state.get("runtime_mode", "strict")) == "minimal_driving" else "Strict World"
		status_label.text = "%s | Track %d | %s / skin %d\n%s | Committed step %d | Source actors %d\nW/S throttle/brake | A/D steer | Space handbrake | Q/E shift\nP pause | . single step | R restart | Esc return | RMB debug orbit\nF8 console: missing-feature warnings\n%s" % [mode_label, track_id, car_code, skin_id, phase, presented_step, last_world.get("actors", []).size(), diagnostic]
	pause_button.disabled = phase not in ["Running", "Paused"]
	pause_button.text = "Resume (P)" if phase == "Paused" else "Pause (P)"
	step_button.disabled = phase not in ["Running", "Paused"]

func _input(event: InputEvent) -> void:
	# GUI focus may consume a release after a driving press was accepted.
	# Release continuous controls before routing to menus or focused Controls.
	if event.is_released(): input_adapter.handle_input(event)
	if _source_menu_input(event):
		get_viewport().set_input_as_handled()
		return
	# Escape is session navigation when the console is closed. Handle it before
	# optional viewer/GUI capture, including stale focus from the previous page.
	if event is InputEventKey and event.pressed and not event.echo and event.physical_keycode == KEY_ESCAPE and not DebugTools.is_console_open():
		get_viewport().set_input_as_handled()
		if Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
			release_camera()
			return
		exit_session()
		return
	if DebugTools.handle_input(event):
		_clear_input()
		_sync_pause()
		return

func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo:
		if event.physical_keycode == KEY_F11:
			var window := get_window()
			if window.mode in [Window.MODE_FULLSCREEN, Window.MODE_EXCLUSIVE_FULLSCREEN]:
				window.mode = windowed_mode
			else:
				windowed_mode = window.mode
				window.mode = Window.MODE_FULLSCREEN
			get_viewport().set_input_as_handled()
			return
		if event.physical_keycode == KEY_ESCAPE:
			get_viewport().set_input_as_handled()
			exit_session()
			return
		if event.physical_keycode == KEY_R and event_request.is_empty():
			begin_session()
			return
		if event.physical_keycode == KEY_P:
			toggle_pause()
			return
		if event.physical_keycode == KEY_PERIOD and event_request.is_empty():
			single_step()
			return
	if phase not in ["Running", "Paused"]:
		return
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_RIGHT:
		orbit_dragging = event.pressed and focused and not DebugTools.input_captured()
		camera_adapter.set_dragging(orbit_dragging)
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT and event.pressed and focused and not DebugTools.input_captured():
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
	if event.is_action_pressed("hp2_drive_camera_view", false) and focused and not DebugTools.input_captured():
		camera_adapter.clear_mouse()
		simulation.cycle_driving_camera_pov()
	if event.is_action("hp2_drive_camera_lookback") and focused and not DebugTools.input_captured():
		camera_adapter.set_look_back(event.is_action_pressed("hp2_drive_camera_lookback"))
		simulation.update_driving_camera(0.0, camera_adapter.look_back)
	if event is InputEventMouseMotion and focused and not DebugTools.input_captured() and (orbit_dragging or Input.mouse_mode == Input.MOUSE_MODE_CAPTURED):
		camera_adapter.mouse_motion(event.relative)
		_update_camera()
	if phase in ["Running", "Paused"]:
		input_adapter.handle_input(event, not focused or DebugTools.input_captured())

func _notification(what: int) -> void:
	# Explicit Python background-message fixture bypasses only host focus gating.
	if "--world-os-background" in OS.get_cmdline_user_args() and what in [NOTIFICATION_APPLICATION_FOCUS_OUT, NOTIFICATION_APPLICATION_FOCUS_IN, NOTIFICATION_WM_WINDOW_FOCUS_OUT, NOTIFICATION_WM_WINDOW_FOCUS_IN]:
		return
	if what in [NOTIFICATION_APPLICATION_FOCUS_OUT, NOTIFICATION_WM_WINDOW_FOCUS_OUT]:
		focused = false
		_clear_input()
		_sync_pause()
	elif what in [NOTIFICATION_APPLICATION_FOCUS_IN, NOTIFICATION_WM_WINDOW_FOCUS_IN]:
		focused = true
		discard_elapsed = true
		_sync_pause()

func release_camera() -> void:
	orbit_dragging = false
	camera_adapter.clear_held_view()
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	if simulation != null and not last_world.is_empty(): simulation.update_driving_camera(0.0, false)

func exit_session() -> void:
	if not event_request.is_empty():
		var event_error := EVENT_ENTRY.return_to_frontend(self, event_result)
		if event_error != OK:
			diagnostic = "Event return failed: %s (%d)" % [diagnostic, event_error]
			_update_status()
		return
	ENTRY.remember(get_tree(), {"track": track_id, "car_id": car_id, "skin_id": skin_id, "car_code": car_code, "reverse": reverse})
	_release()
	phase = "Empty"
	var error := ENTRY.return_to_origin(self)
	if error != OK:
		diagnostic = "World return failed: %d" % error
		_update_status()

func _exit_tree() -> void:
	DebugTools.unregister_scene(self)
	_release()

func viewer_ui_snapshot() -> Dictionary:
	if not event_request.is_empty():
		return {"kind": "frontend"} # Keep F8 console, omit viewer-only controls.
	var result: Dictionary = preload("res://scripts/runtime/track_controls.gd").snapshot(track_renderer)
	result["kind"] = "track_runtime"
	result["world_debug"] = true
	result["real_track"] = true
	result["status"] = phase
	result["diagnostic"] = diagnostic
	result["track_id"] = track_id
	result["world_step"] = presented_step
	result["world_actor_count"] = last_world.get("actors", []).size()
	return result

func viewer_ui_action(request: Dictionary) -> void:
	match str(request.get("action", "")):
		"drive_exit": exit_session()
		"drive_transmission": input_adapter.request({"transmission": int(request.get("lod", 0))}, true)
		"drive_neutral": input_adapter.request({"neutral": true}, true)
		"drive_surface": diagnostic = "Physical surface overrides are unavailable in the source World"
		_: preload("res://scripts/runtime/track_controls.gd").action(track_renderer, request)
