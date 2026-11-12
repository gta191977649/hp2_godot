extends Node
# TASK-0107 opt-in read-only observer for the owned Python input test.
var output_path := ""
var sequence := 0
var elapsed := 0.0
var key_events: Array[Dictionary] = []
var capture_pending := false
var capture_elapsed := 0.0
var materials_written := false
var route_written := false
var road_course_written := false
var capture_on_request := false
var benchmark_socket: PacketPeerUDP

func _send_benchmark_pose(scene: Node) -> void:
	var body: Transform3D = scene.state.get("body", Transform3D.IDENTITY)
	var row := {
		"phase": scene.phase,
		"diagnostic": scene.diagnostic,
		"step": scene.presented_step,
		"position": _parameter(body.origin),
		"forward": _parameter(body.basis.z),
		"powertrain": scene.state.get("powertrain", {}),
		"race": scene.world_state.get("race", {}),
		"event_result": scene.event_result,
	}
	benchmark_socket.put_packet(JSON.stringify(row).to_utf8_buffer())

func _capture_due(delta: float) -> bool:
	if capture_pending:
		return false
	if capture_on_request:
		var request_path := output_path + ".capture"
		if not FileAccess.file_exists(request_path):
			return false
		DirAccess.remove_absolute(request_path)
		return true
	capture_elapsed += delta
	if capture_elapsed < 0.5:
		return false
	capture_elapsed = 0.0
	return true

func _parameter(value: Variant) -> Variant:
	if value is Vector4:
		return [value.x, value.y, value.z, value.w]
	if value is Vector3:
		return [value.x, value.y, value.z]
	return value

func _texture_stats(texture: Texture2D, cache: Dictionary) -> Dictionary:
	var id := texture.get_instance_id()
	if cache.has(id):
		return cache[id]
	var image := texture.get_image()
	if image == null or image.is_empty():
		return {"available": false}
	image = image.duplicate()
	if image.is_compressed():
		image.decompress()
	image.convert(Image.FORMAT_RGBA8)
	var bytes := image.get_data()
	var sums := [0, 0, 0, 0]
	var maximum := [0, 0, 0, 0]
	for offset in range(0, bytes.size(), 4):
		for channel in 4:
			sums[channel] += int(bytes[offset + channel])
			maximum[channel] = maxi(maximum[channel], bytes[offset + channel])
	var count := image.get_width() * image.get_height()
	var average: Array = []
	for channel in 4:
		average.append(float(sums[channel]) / count)
	var stats := {"available": true, "width": image.get_width(), "height": image.get_height(), "rgba8_average": average, "rgba8_maximum": maximum}
	cache[id] = stats
	return stats

func _material_rows(node: Node, rows: Array, textures: Dictionary) -> void:
	if node is MeshInstance3D and node.mesh != null:
		for surface in node.mesh.get_surface_count():
			var material: Material = node.get_active_material(surface)
			var pass_number := 0
			while material != null:
				if material is ShaderMaterial:
					var row := {"node": str(node.get_path()), "visible": node.is_visible_in_tree(), "surface": surface, "pass": pass_number, "parameters": {}}
					for name in ["base_lit", "flag_0x4", "reflection_pass", "frontend_skip_strip", "material_350", "material_351", "material_352", "material_353", "light_35c", "light_35d", "light_35e", "light_35f", "light_360", "light_361", "light_362", "source_camera"]:
						row.parameters[name] = _parameter(material.get_shader_parameter(name))
					var texture: Variant = material.get_shader_parameter("base_texture")
					if texture is Texture2D:
						row["texture"] = _texture_stats(texture, textures)
					rows.append(row)
				material = material.next_pass
				pass_number += 1
	for child in node.get_children():
		_material_rows(child, rows, textures)

func _write_materials(scene: Node) -> void:
	var rows: Array = []
	_material_rows(scene.vehicle_renderer, rows, {})
	var file := FileAccess.open(output_path + ".materials.json", FileAccess.WRITE)
	if file != null:
		file.store_string(JSON.stringify({"car": scene.car_code, "skin": scene.skin_id, "step": scene.presented_step, "rows": rows}, "  "))
		materials_written = true

func _capture() -> void:
	capture_pending = true
	# A static/occluded frontend may otherwise stop requesting draws. This asks
	# the GPU to draw already committed data; it never ticks native simulation.
	call_deferred("_capture_draw")
	await RenderingServer.frame_post_draw
	if is_inside_tree():
		var image := get_viewport().get_texture().get_image()
		if not image.is_empty():
			image.save_png(output_path + ".png")
			var capture_scene := get_tree().current_scene
			var metadata := {"sequence": sequence, "scene": capture_scene.scene_file_path if capture_scene != null else "", "source_loading_visible": false}
			if capture_scene != null and capture_scene.get_script() != null and capture_scene.get_script().resource_path == "res://scripts/runtime/race_scene.gd":
				metadata.source_loading_visible = capture_scene.source_loading_visible
			var capture_file := FileAccess.open(output_path + ".capture.json", FileAccess.WRITE)
			if capture_file != null: capture_file.store_string(JSON.stringify(metadata))
	capture_pending = false

func _capture_draw() -> void:
	if capture_pending and is_inside_tree():
		RenderingServer.force_draw(false)

func _input(event: InputEvent) -> void:
	if event is InputEventKey and not event.echo:
		key_events.append({"physical": event.physical_keycode, "key": event.keycode, "pressed": event.pressed})
		if key_events.size() > 16:
			key_events.pop_front()

func _ready() -> void:
	for argument in OS.get_cmdline_user_args():
		if argument == "--world-os-capture-on-request":
			capture_on_request = true
		if argument.begins_with("--world-os-state="):
			output_path = argument.trim_prefix("--world-os-state=")
		if argument.begins_with("--championship-pose-port="):
			benchmark_socket = PacketPeerUDP.new()
			var port := int(argument.trim_prefix("--championship-pose-port="))
			if benchmark_socket.connect_to_host("127.0.0.1", port) != OK:
				push_error("Championship benchmark pose socket failed")
				get_tree().quit(1)

func _process(delta: float) -> void:
	elapsed += delta
	if output_path.is_empty() or elapsed < 0.05:
		return
	elapsed = 0.0
	sequence += 1
	var scene := get_tree().current_scene
	if scene == null:
		return
	if benchmark_socket != null and ProfileDebug.capture_active and ProfileDebug.event_capture_started and scene.scene_file_path == "res://scenes/race_scene.tscn":
		_send_benchmark_pose(scene)
		return
	var _profile_scope := ProfileDebug.scope("External input test probe")
	var row := {"sequence": sequence, "pid": OS.get_process_id(), "scene": scene.scene_file_path, "focused": get_window().has_focus(), "nodes": Performance.get_monitor(Performance.OBJECT_NODE_COUNT), "key_events": key_events}
	if scene.get_script() != null and scene.get_script().resource_path == "res://scripts/frontend_native.gd" and scene.layout != null:
		row["phase"] = "Frontend"
		row["frontend"] = scene.layout.menu_state()
		row["runtime_error_count"] = scene.runtime_error_count
		var ui: Dictionary = scene.layout.ui_snapshot()
		row["frontend_modal"] = bool(ui.get("modal", false))
		row["frontend_popup_buttons"] = ui.get("widgets", []).filter(func(item): return bool(item.get("interactive", false)) and str(item.get("id", "")).begins_with("selection:popup:")).size()
		if _capture_due(delta):
			_capture()
	if scene.get_script() != null and scene.get_script().resource_path == "res://scripts/runtime/race_scene.gd":
		row["host_focused"] = scene.focused
		row["source_loading_visible"] = scene.source_loading_visible
		row["source_loading_presented"] = scene.source_loading_presented
		row["source_loading_presented_count"] = scene.source_loading_presented_count
		row["source_loading_status"] = scene.source_loading_status
		row["source_nis_draw_count"] = scene.last_world.get("nis_draws", []).size()
		row["source_ui_clip"] = _parameter(scene.last_world.get("source_ui_clip_bounds", Vector4.ZERO))
		if scene.source_ui_canvas != null:
			row["ui_canvas_clip"] = scene.source_ui_canvas.clip_contents
			row["ui_canvas_position"] = [scene.source_ui_canvas.position.x, scene.source_ui_canvas.position.y]
			row["ui_canvas_size"] = [scene.source_ui_canvas.size.x, scene.source_ui_canvas.size.y]
		row.merge({"phase": scene.phase, "diagnostic": scene.diagnostic, "step": scene.presented_step, "generation": scene.generation, "actors": scene.last_world.get("actors", []).size(), "track": scene.track_id, "car": scene.car_code, "skin": scene.skin_id, "held": scene.input_adapter.held, "tick_calls": scene.native_tick_calls, "console": DebugTools.input_captured()})
		if scene.simulation != null:
			var native: Dictionary = scene.simulation.get_state()
			for key in ["backend", "loading", "world_ready", "throttle", "brake", "steering", "handbrake", "powertrain", "runtime_mode", "runtime_warnings", "race", "rule_view", "source_ui_screen_count", "source_driver_info_count", "source_ui_revision", "source_ui_enabled", "source_autosave_requested", "source_attempt_pending", "source_attempt_requires_saved", "source_continuation_reload", "source_menu_screen", "source_menu_item", "source_menu_actions", "source_geometry_count", "source_menu_popup", "source_menu_paused", "source_return_requested"]:
				if native.has(key):
					row[key] = native[key]
				elif scene.world_state.has(key):
					row[key] = scene.world_state[key]
			if str(native.get("phase", "")) in ["Blocked", "Failed"]:
				row["runtime_warnings"] = scene.simulation.get_world_state().get("runtime_warnings", [])
		if scene.state.get("body") is Transform3D:
			var body: Transform3D = scene.state.body
			row["position"] = [body.origin.x, body.origin.y, body.origin.z]
			row["forward"] = [body.basis.z.x, body.basis.z.y, body.basis.z.z]
		row["command_commits"] = scene.input_adapter.command_commits
		if scene.last_world.has("camera"):
			row["camera"] = scene.last_world.camera.duplicate()
			for field in ["position", "target", "up"]:
				row.camera[field] = _parameter(row.camera[field])
			row["camera_fov"] = scene.camera.fov
			row["camera_keep_aspect"] = scene.camera.keep_aspect
			row["window_mode"] = get_window().mode
			row["viewport_size"] = [get_viewport().get_visible_rect().size.x, get_viewport().get_visible_rect().size.y]
			row["camera_position"] = _parameter(scene.camera.position)
			row["camera_manual"] = scene.camera_adapter.manual
			row["camera_orbit_center"] = _parameter(scene.camera_adapter.orbit_origin)
			row["camera_aim_target"] = _parameter(scene.camera_adapter.orbit_target)
			row["camera_forward"] = _parameter(-scene.camera.basis.z)
			row["camera_orbit_radius"] = scene.camera_adapter.orbit_radius
			row["camera_orbit_strength"] = scene.camera_adapter.return_strength
			row["camera_orbit_dragging"] = scene.camera_adapter.dragging
			row["vehicle_body_bounds"] = []
			for part in scene.vehicle_renderer.get_children():
				if part is MeshInstance3D and part.visible and int(part.get_meta("hp2_part_group", -1)) == 0:
					var mesh_bounds: AABB = part.global_transform * part.get_aabb()
					row.vehicle_body_bounds.append({"min": _parameter(mesh_bounds.position), "max": _parameter(mesh_bounds.end)})
					for batch in part.get_children():
						if batch is MeshInstance3D and batch.visible:
							var batch_bounds: AABB = batch.global_transform * batch.get_aabb()
							row.vehicle_body_bounds.append({"min": _parameter(batch_bounds.position), "max": _parameter(batch_bounds.end)})
		row["event_request"] = scene.event_request
		row["event_result"] = scene.event_result
		row["player_address"] = int(scene.last_world.get("player_car", 0))
		row["actor_positions"] = []
		for actor in scene.last_world.get("actors", []):
			var actor_state: Dictionary = actor.get("state", actor)
			if actor_state.get("body") is Transform3D:
				var actor_body: Transform3D = actor_state.body
				row.actor_positions.append({"address": int(actor.get("address", 0)), "car_id": int(actor.get("car_id", -1)), "position": _parameter(actor_body.origin), "forward": _parameter(actor_body.basis.z)})
		if scene.presented_step >= 0 and scene.simulation != null and scene.simulation.has_method("get_world_road_path"):
			row["road_path"] = scene.simulation.get_world_road_path()
		row["buttons"] = {}
		var visible_buttons: Array = scene.pause_button.get_parent().get_children()
		if scene.race_ui != null:
			visible_buttons = scene.race_ui.find_children("*", "Button", true, false)
		for button in visible_buttons:
			if button is Button and button.is_visible_in_tree() and not button.disabled:
				var center: Vector2 = button.get_global_rect().get_center()
				row.buttons[button.text] = [center.x, center.y]
		if scene.audio_display != null:
			row["pcm_submitted"] = scene.audio_display.submitted_frames
			row["pcm_nonzero"] = scene.audio_display.nonzero_frames
			row["pcm_queued"] = scene.audio_display.queued_frames
		if scene.presented_step >= 0 and not materials_written:
			_write_materials(scene)
		if scene.presented_step >= 0 and not route_written and scene.simulation != null and scene.simulation.has_method("get_world_route"):
			var points: Array = scene.simulation.get_world_route()
			if not points.is_empty():
				var route_file := FileAccess.open(output_path + ".route.json", FileAccess.WRITE)
				if route_file != null:
					route_file.store_string(JSON.stringify({"track": scene.track_id, "source_generation": scene.world_state.get("source_generation", 0), "coordinate_system": "read-only recovered primary QuickSpline native X,Y; third value PC planning arc distance; host ground X=Y,Z=X", "points": points}))
					route_written = true
		if scene.presented_step >= 0 and not road_course_written and scene.simulation != null and scene.simulation.has_method("get_world_road_course"):
			var course: Dictionary = scene.simulation.get_world_road_course()
			if bool(course.get("ok", false)) and not course.get("points", []).is_empty():
				var course_file := FileAccess.open(output_path + ".road-course.json", FileAccess.WRITE)
				if course_file != null:
					course["port_test_observation"] = true
					course["source_generation"] = scene.world_state.get("source_generation", 0)
					course["coordinate_system"] = "Original TrackRoute X,Y; third value PC XY arc distance; fourth original Z; then route/point indices. No game-state changes."
					course_file.store_string(JSON.stringify(course))
					road_course_written = true
		if (scene.presented_step >= 0 or scene.source_loading_visible) and _capture_due(delta):
			_capture()
	var file := FileAccess.open(output_path + ".tmp", FileAccess.WRITE)
	if file != null:
		file.store_string(JSON.stringify(row))
		file.close()
		DirAccess.rename_absolute(output_path + ".tmp", output_path)
