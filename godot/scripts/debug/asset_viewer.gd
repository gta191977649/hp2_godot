extends Node3D
# Port-side viewing environment only. All geometry, units and colors here are synthetic.
@export_enum("car", "track") var viewer_kind := "car"
var camera: Camera3D
var fixture: Node3D
var state_label: Label
var source_edit: LineEdit
var content_label: Label
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
	_build_panel()
	reset_camera()
	refresh_state()
	DebugTools.write_log("%sViewer: synthetic fixture only; original resource loader is not implemented" % viewer_kind.capitalize())
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--viewer-capture="):
			_capture.call_deferred(argument.trim_prefix("--viewer-capture="))

func _capture(path: String) -> void:
	await get_tree().create_timer(0.5).timeout
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
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
	_mesh(self, plane, Vector3(0, -0.04, 0), Color(0.09, 0.12, 0.16))
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

func _label(text: String, font_size: int = 14) -> Label:
	var label := Label.new()
	label.text = text
	label.add_theme_font_size_override("font_size", font_size)
	label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	return label

func _button(row: HBoxContainer, text: String, action: String) -> void:
	var button := Button.new()
	button.text = text
	button.pressed.connect(func() -> void: DebugTools.request(action))
	row.add_child(button)

func _build_panel() -> void:
	var hud := CanvasLayer.new()
	hud.layer = 5
	add_child(hud)
	var panel := PanelContainer.new()
	panel.position = Vector2(16, 44)
	panel.custom_minimum_size.x = 350
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.025, 0.04, 0.065, 0.94)
	style.set_content_margin_all(14)
	style.set_corner_radius_all(6)
	panel.add_theme_stylebox_override("panel", style)
	hud.add_child(panel)
	var column := VBoxContainer.new()
	column.add_theme_constant_override("separation", 10)
	panel.add_child(column)
	column.add_child(_label("%sViewer" % viewer_kind.capitalize(), 24))
	var notice := _label("Port-side debug scaffold\nNo original game resource is loaded.")
	notice.modulate = Color(1, 0.8, 0.4)
	column.add_child(notice)
	content_label = _label("Synthetic fixture")
	column.add_child(content_label)
	source_edit = LineEdit.new()
	source_edit.placeholder_text = "ZZDATA-relative resource path"
	source_edit.tooltip_text = "Request only: the original resource loader is not implemented."
	source_edit.text_submitted.connect(_request_load)
	column.add_child(source_edit)
	var load_row := HBoxContainer.new()
	column.add_child(load_row)
	var load_button := Button.new()
	load_button.text = "Load request"
	load_button.pressed.connect(func() -> void: _request_load(source_edit.text))
	load_row.add_child(load_button)
	_button(load_row, "Clear", "clear")
	_button(load_row, "Fixture", "fixture")
	var camera_row := HBoxContainer.new()
	column.add_child(camera_row)
	_button(camera_row, "Reset Camera", "reset")
	_button(camera_row, "Back to Frontend", "close")
	var switch_row := HBoxContainer.new()
	column.add_child(switch_row)
	_button(switch_row, "CarViewer", "car")
	_button(switch_row, "TrackViewer", "track")
	state_label = _label("")
	column.add_child(state_label)
	var controls := "LMB drag: orbit | RMB drag: pan\nWheel: zoom | F: frame | R: reset" if viewer_kind == "car" else "Hold RMB: look + WASD / Q E move\nShift: faster | Wheel: speed\nF: frame | R: reset"
	column.add_child(_label(controls + "\nF8: console | TAB: console completion"))

func _request_load(source: String) -> void:
	DebugTools.request("load", source)
	source_edit.release_focus()

func refresh_state() -> void:
	if fixture == null:
		return
	var state: Dictionary = DebugTools.host.viewer_state()
	fixture.visible = bool(state.fixture)
	content_label.text = "Synthetic fixture (%d meshes)" % fixture_meshes if fixture.visible else "Empty viewer"
	state_label.text = "State: %s\nRequest: %s" % [state.status, state.source if not str(state.source).is_empty() else "(none)"]
	if not str(state.diagnostic).is_empty():
		state_label.text += "\n" + str(state.diagnostic)
	state_label.text += "\nCamera: %s" % camera.position.snapped(Vector3.ONE * 0.01)
	if viewer_kind == "track":
		state_label.text += "\nSpeed: %.1f port units/s" % move_speed

func viewer_action(action: String) -> void:
	if action == "reset":
		reset_camera()
	refresh_state()

func reset_camera() -> void:
	release_camera()
	orbit_center = Vector3(0, 1, 0)
	orbit_yaw = 0.6
	orbit_pitch = 0.35
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

func frame_target() -> void:
	# Only the synthetic fixture bounds are known in this scaffold.
	reset_camera()
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
	DebugTools.handle_input(event)
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
	if DebugTools.is_console_open() or camera == null:
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
	if DebugTools.is_console_open():
		release_camera()
	elif looking:
		var direction := Vector3(float(held.has(KEY_D)) - float(held.has(KEY_A)), float(held.has(KEY_E)) - float(held.has(KEY_Q)), float(held.has(KEY_S)) - float(held.has(KEY_W)))
		camera.position += camera.basis * direction.normalized() * move_speed * (4 if held.has(KEY_SHIFT) else 1) * delta
	DebugTools.host.viewer_camera("position=%s rotation=%s speed=%.1f" % [camera.position, camera.rotation_degrees, move_speed])
	DebugTools.statistics = {"screen": "%sViewer" % viewer_kind.capitalize()}
	refresh_state()

func _exit_tree() -> void:
	release_camera()
	DebugTools.unregister_scene(self)
