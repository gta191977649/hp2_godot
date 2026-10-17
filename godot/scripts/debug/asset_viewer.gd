extends Node3D
# Port-side viewing environment. Original car content is loaded by the native bridge.
@export_enum("car", "track") var viewer_kind := "car"
var camera: Camera3D
var fixture: Node3D
var orbit_center := Vector3(0, 1, 0)
var orbit_yaw := 0.6
var orbit_pitch := 0.35
var orbit_distance := 8.0
var move_speed := 16.0
var orbit_drag := false
var pan_drag := false
var looking := false
var held: Dictionary = {}
var updates := 0
var fixture_meshes := 0
var car_model: Node3D
var car_state: Dictionary = {}
var last_rendering := ""
var forced_lod := 0
var show_lods := false
var pending_load := false
var frame_on_load := true
var load_generation := -1
var max_load_frame_ms := 0.0
var wireframe := false
var show_bounds := false
var inspection := false
var preview_mode := false
# Port-side presentation speed, not an asserted original PS2 camera constant.
const PREVIEW_ORBIT_SPEED := 0.2094395102
var grid_node: MeshInstance3D
var floor_node: MeshInstance3D
var car_catalog: Array = []
var driving: RefCounted
var focused := true
var test_drive_pending := false

func _ready() -> void:
	if DebugTools.host == null or not DebugTools.host.is_debug_build():
		DebugTools.write_log("Debug viewers are disabled in Release builds", true)
		get_tree().quit(1)
		return
	DebugTools.register_scene(self, viewer_kind)
	if DisplayServer.get_name() != "headless":
		get_window().size = Vector2i(1280, 800)
	_build_environment()
	_build_fixture()
	if viewer_kind == "car":
		car_model = ClassDB.instantiate("Ps2CarViewer")
		add_child(car_model)
		car_catalog = car_model.get_catalog()
		driving = preload("res://scripts/debug/driving_lab.gd").new(self)
	reset_camera()
	refresh_state()
	DebugTools.write_log("CarViewer: ready for CARS/<model>/GEOMETRY.BIN" if viewer_kind == "car" else "TrackViewer: synthetic fixture; loader not implemented")
	var explicit_load := false
	if "--driving-smoke" in OS.get_cmdline_user_args() or "--driving-asset-smoke" in OS.get_cmdline_user_args() or "--driving-os-smoke" in OS.get_cmdline_user_args():
		explicit_load = true
		var smoke_runner := preload("res://scripts/debug/driving_smoke.gd").new()
		add_child(smoke_runner)
	if DebugTools.test_drive_requested:
		DebugTools.test_drive_requested = false
		test_drive_pending = true
	for argument in OS.get_cmdline_user_args():
		if argument == "--wheel-blur" and car_model != null:
			car_model.set_wheel_blur(true)
		if argument == "--car-lods" and car_model != null:
			show_lods = true
			car_model.set_show_lods(true)
		if argument == "--car-inspection" and car_model != null:
			inspection = true
			car_model.set_inspection(true)
		if argument.begins_with("--car-load=") and viewer_kind == "car":
			explicit_load = true
			DebugTools.request("load", argument.trim_prefix("--car-load="))
		if argument == "--preview" and viewer_kind == "car":
			set_preview_mode(true)
		if argument.begins_with("--viewer-capture="):
			_capture.call_deferred(argument.trim_prefix("--viewer-capture="))
	if viewer_kind == "car" and not explicit_load and "--viewer-smoke" not in OS.get_cmdline_user_args():
		_select_initial_car.call_deferred()

func _select_initial_car() -> void:
	for car in car_catalog:
		if not bool(car.available):
			continue
		for skin in car.skins:
			if int(skin.id) >= 0:
				viewer_ui_action({"action": "select_car", "car_id": int(car.id), "skin_id": int(skin.id)})
				return

func _capture(path: String) -> void:
	await get_tree().create_timer(0.5).timeout
	while pending_load:
		await get_tree().process_frame
	# Explicit capture also works while the OS has occluded/minimized this debug window.
	RenderingServer.viewport_set_update_mode(get_viewport().get_viewport_rid(), RenderingServer.VIEWPORT_UPDATE_ALWAYS)
	RenderingServer.force_draw(false)
	await get_tree().process_frame
	RenderingServer.force_draw(false)
	var result := get_viewport().get_texture().get_image().save_png(path)
	print("VIEWER_CAPTURE_OK: ", path, " result=", result)
	get_tree().quit(0 if result == OK else 1)

func _material(color: Color, unshaded: bool = false) -> StandardMaterial3D:
	var material := StandardMaterial3D.new()
	material.albedo_color = color
	material.roughness = 0.85
	if unshaded:
		material.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	return material

func _mesh(parent: Node3D, mesh: Mesh, at: Vector3, color: Color) -> MeshInstance3D:
	var instance := MeshInstance3D.new()
	instance.mesh = mesh
	instance.material_override = _material(color)
	parent.add_child(instance)
	instance.position = at
	return instance

func _box(size: Vector3, at: Vector3, color: Color) -> MeshInstance3D:
	var shape := BoxMesh.new()
	shape.size = size
	fixture_meshes += 1
	return _mesh(fixture, shape, at, color)

func _build_environment() -> void:
	var world := WorldEnvironment.new()
	var environment := Environment.new()
	environment.background_mode = Environment.BG_COLOR
	environment.background_color = Color(0.035, 0.05, 0.075)
	environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	environment.ambient_light_color = Color(0.65, 0.75, 0.9)
	environment.ambient_light_energy = 0.6
	world.environment = environment
	add_child(world)
	var light := DirectionalLight3D.new()
	light.rotation_degrees = Vector3(-55, -30, 0)
	light.light_energy = 1.6
	light.shadow_enabled = true
	add_child(light)
	var plane := PlaneMesh.new()
	var extent := 20 if viewer_kind == "car" else 100
	plane.size = Vector2(extent, extent)
	floor_node = _mesh(self, plane, Vector3(0, -0.04, 0), Color(0.09, 0.12, 0.16))
	var lines := ImmediateMesh.new()
	var line_material := _material(Color.WHITE, true)
	line_material.vertex_color_use_as_albedo = true
	lines.surface_begin(Mesh.PRIMITIVE_LINES, line_material)
	var step := 1 if viewer_kind == "car" else 5
	for index in range(-extent / 2, extent / 2 + 1, step):
		lines.surface_set_color(Color(0.18, 0.23, 0.29))
		lines.surface_add_vertex(Vector3(index, 0, -extent / 2))
		lines.surface_add_vertex(Vector3(index, 0, extent / 2))
		lines.surface_add_vertex(Vector3(-extent / 2, 0, index))
		lines.surface_add_vertex(Vector3(extent / 2, 0, index))
	for axis in [Vector3.RIGHT, Vector3.UP, Vector3.BACK]:
		lines.surface_set_color(Color(absf(axis.x), absf(axis.y), absf(axis.z)))
		lines.surface_add_vertex(Vector3(0, 0.02, 0))
		lines.surface_add_vertex(Vector3(0, 0.02, 0) + axis * 3)
	lines.surface_end()
	var grid := MeshInstance3D.new()
	grid_node = grid
	grid.mesh = lines
	add_child(grid)
	camera = Camera3D.new()
	camera.near = 0.05
	camera.far = 1000
	add_child(camera)
	camera.current = true

func _build_fixture() -> void:
	fixture = Node3D.new()
	fixture.name = "SyntheticFixture"
	add_child(fixture)
	if viewer_kind == "car":
		_box(Vector3(2.0, 0.7, 4.0), Vector3(0, 0.85, 0), Color(0.12, 0.65, 0.9))
		_box(Vector3(1.6, 0.65, 1.8), Vector3(0, 1.5, 0.1), Color(0.2, 0.35, 0.48))
		for x in [-1.05, 1.05]:
			for z in [-1.2, 1.2]:
				var wheel := CylinderMesh.new()
				wheel.top_radius = 0.45
				wheel.bottom_radius = 0.45
				wheel.height = 0.25
				var instance := _mesh(fixture, wheel, Vector3(x, 0.5, z), Color(0.04, 0.05, 0.06))
				instance.rotation_degrees.z = 90
				fixture_meshes += 1
	else:
		# Test layout, not a reconstruction of any shipped track or road semantics.
		_box(Vector3(8, 0.08, 36), Vector3(0, 0.05, 0), Color(0.2, 0.25, 0.31))
		_box(Vector3(24, 0.08, 8), Vector3(8, 0.06, -14), Color(0.2, 0.25, 0.31))
		for z in [-12, -4, 4, 12]:
			_box(Vector3(0.15, 0.08, 3), Vector3(0, 0.14, z), Color(0.85, 0.85, 0.55))
			for x in [-5, 5]:
				_box(Vector3(0.7, 1.4, 0.7), Vector3(x, 0.7, z), Color(0.9, 0.4, 0.12))
		_box(Vector3(12, 0.5, 0.5), Vector3(0, 5, 10), Color(0.12, 0.65, 0.9))
		for x in [-6, 6]:
			_box(Vector3(0.5, 5, 0.5), Vector3(x, 2.5, 10), Color(0.12, 0.65, 0.9))

func refresh_state() -> void:
	if fixture != null:
		fixture.visible = bool(DebugTools.host.viewer_state().fixture)

func viewer_ui_snapshot() -> Dictionary:
	if driving != null and driving.engaged():
		return driving.snapshot()
	var state: Dictionary = DebugTools.host.viewer_state().duplicate()
	state["kind"] = viewer_kind
	state["camera"] = "%s | speed %.1f" % [camera.position.snapped(Vector3.ONE * 0.01), move_speed]
	state["cars"] = car_catalog
	state["lod"] = forced_lod
	state["wireframe"] = wireframe
	state["bounds"] = show_bounds
	state["inspection"] = inspection
	state["show_lods"] = show_lods
	state["preview"] = preview_mode
	if driving != null and not driving.error.is_empty():
		state["diagnostic"] = str(state.get("diagnostic", "")) + "\n" + driving.error
	if car_model != null:
		var native: Dictionary = car_model.get_state()
		state["wheel_speed"] = float(native.get("wheel_speed", 80.0))
		state["wheel_blur"] = bool(native.get("wheel_blur", false))
		if bool(native.get("loading", false)):
			state["status"] = "%s... (previous car retained)" % str(native.get("phase", "Loading"))
		state["parts"] = native.get("parts", [])
		state["rendering"] = str(native.get("rendering", ""))
		if not str(native.get("summary", "")).is_empty():
			state["rendering"] += "\n" + str(native.summary)
		state["rendering"] += "\nCPU %.1f ms | upload %.1f ms | max slice %.1f ms\nGeometry reads %d / hits %d | texture decodes %d / hits %d" % [float(native.get("cpu_ms", 0)), float(native.get("upload_ms", 0)), float(native.get("max_upload_slice_ms", 0)), int(native.get("geometry_reads", 0)), int(native.get("geometry_hits", 0)), int(native.get("texture_decodes", 0)), int(native.get("texture_hits", 0))]
		state["rendering"] += "\nRead %.1f ms | geometry decode %.1f ms | pixels %.1f ms | binding %.1f ms" % [float(native.get("read_ms", 0)), float(native.get("geometry_decode_ms", 0)), float(native.get("texture_decode_ms", 0)), float(native.get("binding_ms", 0))]
		state["car_id"] = native.get("car_id", -1)
		state["skin_id"] = native.get("skin_id", -1)
		state["inspection"] = native.get("inspection", inspection)
		if not str(native.get("error", "")).is_empty():
			state["diagnostic"] = str(state.get("diagnostic", "")) + "\n" + str(native.error)
			if str(state.get("status", "")) == "Loaded":
				state["status"] = "Render failed"
	return state

func _queue_car(result: Dictionary, frame: bool = true) -> void:
	pending_load = bool(result.get("loading", false))
	max_load_frame_ms = 0
	frame_on_load = frame
	load_generation = int(result.get("generation", -1))
	if not pending_load:
		_publish_car_result(result, frame)

func _publish_car_result(result: Dictionary, frame: bool = true) -> void:
	car_state = result
	if bool(result.get("ok", false)):
		car_model.set_wireframe(wireframe)
		car_model.set_bounds(show_bounds)
		car_model.set_inspection(inspection)
		car_state = car_model.get_state()
	var detail := str(car_state.get("diagnostic", "")) if car_state.get("ok", false) else str(result.get("error", "Vehicle load failed"))
	DebugTools.host.viewer_loaded(bool(car_state.get("ok", false)), detail, str(car_state.get("summary", "")))
	if car_state.get("ok", false) and frame:
		frame_target()
	refresh_state()

func viewer_action(action: String) -> void:
	if driving != null and driving.engaged() and action in ["load", "clear", "fixture"]:
		var requested_source := str(DebugTools.host.viewer_state().source)
		driving.stop()
		DebugTools.host.viewer_apply(action, requested_source)
	if car_model != null:
		if action in ["clear", "fixture"]:
			pending_load = false
			car_model.clear_car()
			car_state.clear()
			last_rendering = ""
		if action == "load":
			_queue_car(car_model.request_car(str(DebugTools.host.viewer_state().source), -1, 1))
	if action == "reset":
		reset_camera()
		if car_state.get("ok", false):
			frame_target()
	refresh_state()

func viewer_ui_action(action: Dictionary) -> void:
	# Called deferred, after native ImGui drawing has completed.
	if car_model == null:
		return
	var command := str(action.get("action", ""))
	if command == "test_drive":
		driving.start()
		return
	if driving.engaged():
		if command.begins_with("drive_"):
			driving.action(action)
		return
	match command:
		"preview":
			set_preview_mode(bool(action.get("enabled", false)))
		"select_car":
			var selected_id := int(action.get("car_id", -1))
			var selected_path := ""
			for car in car_catalog:
				if int(car.id) == selected_id:
					selected_path = str(car.path)
					break
			var applied: Dictionary = DebugTools.host.viewer_apply("load", selected_path)
			if applied.get("ok", false):
				last_rendering = ""
				_queue_car(car_model.request_car(selected_path, selected_id, int(action.get("skin_id", -1))))
			else:
				DebugTools.write_log(str(applied.get("error", "Invalid catalog selection")), true)
		"select_skin":
			var applied: Dictionary = DebugTools.host.viewer_apply("load", str(DebugTools.host.viewer_state().source))
			if applied.get("ok", false):
				last_rendering = ""
				_queue_car(car_model.request_car(str(DebugTools.host.viewer_state().source), int(car_model.get_state().car_id), int(action.get("skin_id", -1))), false)
		"show_lods":
			show_lods = bool(action.get("enabled", false))
			_queue_car(car_model.set_show_lods(show_lods))
		"wheel_speed":
			car_state = car_model.set_wheel_speed(float(action.get("value", 80.0)))
		"wheel_blur":
			car_state = car_model.set_wheel_blur(bool(action.get("enabled", false)))
		"lod":
			forced_lod = int(action.get("lod", 0))
		"wireframe":
			wireframe = bool(action.get("enabled", false))
			car_model.set_wireframe(wireframe)
		"bounds":
			show_bounds = bool(action.get("enabled", false))
			car_model.set_bounds(show_bounds)
		"inspection":
			inspection = bool(action.get("enabled", false))
			car_model.set_inspection(inspection)
			car_state = car_model.get_state()
			if bool(car_state.get("loading", false)):
				_queue_car(car_state, frame_on_load)
		"part":
			car_model.set_part_visible(str(action.get("name", "")), bool(action.get("enabled", false)))
	refresh_state()

func reset_camera() -> void:
	release_camera()
	orbit_center = Vector3(0, 1, 0)
	orbit_yaw = 0.6
	orbit_pitch = 0.12 if preview_mode else 0.35
	orbit_distance = 8
	move_speed = 16
	if viewer_kind == "car":
		_update_orbit()
	else:
		camera.position = Vector3(18, 14, 22)
		camera.look_at(Vector3.ZERO)

func _update_orbit() -> void:
	camera.position = orbit_center + Vector3(sin(orbit_yaw) * cos(orbit_pitch), sin(orbit_pitch), cos(orbit_yaw) * cos(orbit_pitch)) * orbit_distance
	camera.look_at(orbit_center)

func set_preview_mode(enabled: bool) -> void:
	preview_mode = enabled and viewer_kind == "car"
	if grid_node != null:
		grid_node.visible = not preview_mode
	if floor_node != null:
		floor_node.visible = not preview_mode
	if camera != null:
		camera.fov = 35 if preview_mode else 75
		frame_target()

func frame_target() -> void:
	reset_camera()
	if car_state.get("ok", false):
		var bounds: AABB = car_state.bounds
		orbit_center = bounds.get_center()
		orbit_distance = maxf(2, bounds.size.length() * 1.3)
		_update_orbit()
	if viewer_kind == "track":
		camera.position = Vector3(26, 25, 32)
		camera.look_at(Vector3(3, 0, -2))

func release_camera() -> void:
	orbit_drag = false
	pan_drag = false
	looking = false
	held.clear()
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE

func _input(event: InputEvent) -> void:
	var in_drive: bool = driving != null and driving.engaged()
	# Relative camera motion belongs to the captured tool camera, not ImGui.
	if in_drive and driving.captures_pointer() and (event is InputEventMouseMotion or event is InputEventMouseButton):
		driving.handle_input(event)
		get_viewport().set_input_as_handled()
		return
	var consumed: bool = DebugTools.handle_input(event)
	if in_drive:
		driving.sync_lifecycle()
		driving.handle_input(event, consumed)
		get_viewport().set_input_as_handled()
		return
	# Release even when the pointer ends over a GUI control.
	if event is InputEventMouseButton and not event.pressed:
		if event.button_index == MOUSE_BUTTON_RIGHT:
			looking = false
			pan_drag = false
			Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
		if event.button_index == MOUSE_BUTTON_LEFT:
			orbit_drag = false
	if event is InputEventKey and not event.pressed:
		held.erase(event.keycode)

func _unhandled_input(event: InputEvent) -> void:
	if driving != null and driving.engaged():
		return
	# Mouse hover must isolate camera/mouse events without swallowing driving keys.
	var captured: bool = DebugTools.input_captured()
	if event is InputEventKey:
		captured = DebugTools.is_console_open() or DebugTools.host.viewer_keyboard_capture()
	if captured or not focused or camera == null:
		return
	if event is InputEventKey:
		if event.pressed:
			held[event.keycode] = true
		if event.pressed and not event.echo:
			if event.keycode == KEY_R:
				reset_camera()
			elif event.keycode == KEY_F:
				frame_target()
	elif event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_RIGHT and event.pressed:
			looking = viewer_kind == "track"
			pan_drag = viewer_kind == "car"
			if looking:
				Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
		elif event.button_index == MOUSE_BUTTON_LEFT and event.pressed:
			orbit_drag = viewer_kind == "car"
		elif event.pressed and event.button_index in [MOUSE_BUTTON_WHEEL_UP, MOUSE_BUTTON_WHEEL_DOWN]:
			var factor := 0.85 if event.button_index == MOUSE_BUTTON_WHEEL_UP else 1.0 / 0.85
			if viewer_kind == "car":
				orbit_distance = clampf(orbit_distance * factor, 1, 200)
				_update_orbit()
			else:
				move_speed = clampf(move_speed / factor, 1, 200)
	elif event is InputEventMouseMotion:
		if viewer_kind == "car":
			if orbit_drag:
				orbit_yaw -= event.relative.x * 0.01
				orbit_pitch = clampf(orbit_pitch + event.relative.y * 0.01, 0.05, 1.4)
			if pan_drag:
				orbit_center += (-camera.basis.x * event.relative.x + camera.basis.y * event.relative.y) * orbit_distance * 0.002
			if orbit_drag or pan_drag:
				_update_orbit()
		elif looking:
			camera.rotation.y -= event.relative.x * 0.003
			camera.rotation.x = clampf(camera.rotation.x - event.relative.y * 0.003, deg_to_rad(-85), deg_to_rad(85))

func _process(delta: float) -> void:
	if camera == null:
		return
	updates += 1
	if car_model != null and pending_load:
		max_load_frame_ms = maxf(max_load_frame_ms, delta * 1000)
		var loaded: Dictionary = car_model.pump_load()
		if not bool(loaded.get("loading", false)) and int(loaded.get("completed", -2)) == load_generation:
			pending_load = false
			var previews: Dictionary = loaded.get("previews", {})
			var preview_errors: Dictionary = loaded.get("preview_errors", {})
			for entry in car_catalog:
				if int(entry.id) == int(loaded.get("car_id", -1)):
					for skin in entry.skins:
						if previews.has(int(skin.id)):
							skin["preview"] = previews[int(skin.id)]
						elif preview_errors.has(int(skin.id)):
							skin["preview_error"] = preview_errors[int(skin.id)]
			_publish_car_result(loaded, frame_on_load)
	if test_drive_pending and not pending_load and car_state.get("ok", false):
		test_drive_pending = false
		driving.start()
	if driving != null and driving.engaged():
		driving.process(delta)
	elif DebugTools.input_captured():
		release_camera()
	elif looking:
		var direction := Vector3(float(held.has(KEY_D)) - float(held.has(KEY_A)), float(held.has(KEY_E)) - float(held.has(KEY_Q)), float(held.has(KEY_S)) - float(held.has(KEY_W)))
		camera.position += camera.basis * direction.normalized() * move_speed * (4 if held.has(KEY_SHIFT) else 1) * delta
	# Auto orbit is independent of dragging and UI/console capture; mouse offsets
	# adjust the same camera without suspending the presentation rotation.
	if viewer_kind == "car" and preview_mode and not (driving != null and driving.engaged()):
		orbit_yaw = fposmod(orbit_yaw + PREVIEW_ORBIT_SPEED * delta, TAU)
		_update_orbit()
	DebugTools.host.viewer_camera("position=%s rotation=%s speed=%.1f" % [camera.position, camera.rotation_degrees, move_speed])
	DebugTools.statistics = {"screen": "%sViewer" % viewer_kind.capitalize()}
	if car_model != null and car_state.get("ok", false):
		if not pending_load and (driving == null or not driving.engaged()):
			car_model.advance_wheels(delta)
		# Port-side mapping of Godot vertical FOV to pixel projection scale.
		var scale: float = get_viewport().get_visible_rect().size.y * 0.5 / tan(deg_to_rad(camera.fov) * 0.5)
		var rendered: Dictionary = car_model.update_view(camera.position, -camera.basis.z, scale, forced_lod)
		var description := str(rendered.get("summary", ""))
		if not rendered.get("ok", false):
			description += " error=" + str(rendered.get("error", "Rendering failed"))
		if description != last_rendering:
			DebugTools.host.viewer_rendering(description)
			last_rendering = description
	refresh_state()

func _notification(what: int) -> void:
	if what == NOTIFICATION_APPLICATION_FOCUS_OUT:
		focused = false
		release_camera()
	elif what == NOTIFICATION_APPLICATION_FOCUS_IN:
		focused = true
	if driving != null and driving.engaged():
		driving.sync_lifecycle()

func _exit_tree() -> void:
	release_camera()
	if car_model != null:
		car_model.clear_car()
	DebugTools.unregister_scene(self)
