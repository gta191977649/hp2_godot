extends RefCounted
# TASK0084: recovered native default pose with an explicit host mouse overlay.
enum Phase { OFF, STARTING, DRIVING, SUSPENDED, ERROR }
var driving_camera := preload("res://scripts/debug/driving_camera.gd").new()
var camera_state: Dictionary = {}
var driving_car_visible := true
var input_adapter := preload("res://scripts/debug/driving_input.gd").new()
var skid_renderer: RefCounted
var audio_display: Node
var skid_quad_count := 0
var last_input_frame: Dictionary = {}
# Port diagnostic only: one increment immediately before each native tick invocation.
var native_tick_calls := 0
var viewer: Node3D
var lab: Object
var phase := Phase.OFF
var active := false
var state: Dictionary = {}
var saved: Dictionary = {}
var error := ""
var capture_wanted := false
var discard_elapsed := false
var surface_visuals: Node3D
var surface_material: ShaderMaterial
var surface_key := ""

func _init(owner: Node3D) -> void:
	viewer = owner
	lab = ClassDB.instantiate("Ps2DrivingLab")

func engaged() -> bool:
	return phase != Phase.OFF

func _valid_step(next: Dictionary) -> bool:
	return bool(next.get("ok", false)) and str(next.get("status", "")) not in ["Blocked", "Failed"] and not bool(next.get("paused", true)) and bool(next.get("body_known", false)) and bool(next.get("wheels_known", false)) and int(next.get("step", 0)) > 0

func _reason(next: Dictionary) -> String:
	return str(next.get("error", next.get("diagnostic", "Native driving step failed")))

func _remember(native: Dictionary) -> void:
	saved = {"preview": viewer.preview_mode, "show_lods": viewer.show_lods,
		"source": str(DebugTools.host.viewer_state().source), "car_id": int(native.car_id), "skin_id": int(native.skin_id),
		"transform": viewer.car_model.transform, "car_visible": viewer.car_model.visible, "floor_position": viewer.floor_node.position,
		"floor_size": viewer.floor_node.mesh.size, "floor_material": viewer.floor_node.material_override, "grid_position": viewer.grid_node.position,
		"floor_visible": viewer.floor_node.visible, "grid_visible": viewer.grid_node.visible,
		"orbit_yaw": viewer.orbit_yaw, "orbit_pitch": viewer.orbit_pitch, "orbit_distance": viewer.orbit_distance,
		"orbit_center": viewer.orbit_center, "camera_transform": viewer.camera.transform, "fov": viewer.camera.fov, "near": viewer.camera.near, "keep_aspect": viewer.camera.keep_aspect}

func start() -> bool:
	if engaged() or viewer.pending_load:
		return false
	error = ""
	discard_elapsed = false
	var native: Dictionary = viewer.car_model.get_state()
	var code := ""
	for entry in viewer.car_catalog:
		if int(entry.id) == int(native.get("car_id", -1)):
			code = str(entry.code)
	if code.is_empty() or not viewer.car_state.get("ok", false):
		error = "Load an original car before Test drive"
		return false
	state = lab.load_config(code)
	if not state.get("configured", false):
		error = _reason(state)
		return false
	if viewer.viewer_kind == "track":
		state = lab.set_track(int(viewer.track_state.get("track_id", -1)))
		if not state.get("ok", false) or not state.get("real_track", false):
			error = _reason(state)
			return false
	state = lab.action("run")
	native_tick_calls += 1
	state = lab.tick()
	if not _valid_step(state):
		error = _reason(state)
		return false
	_remember(native)
	driving_camera.reset()
	camera_state.clear()
	viewer.release_camera()
	viewer.preview_mode = false
	driving_car_visible = true
	viewer.car_model.visible = driving_car_visible
	viewer.grid_node.visible = viewer.viewer_kind != "track"
	viewer.floor_node.visible = viewer.viewer_kind != "track"
	viewer.floor_node.mesh.size = Vector2(4000, 4000)
	viewer.floor_node.position.y = 0
	phase = Phase.STARTING
	if viewer.show_lods:
		viewer.show_lods = false
		viewer._queue_car(viewer.car_model.set_show_lods(false), false)
	if not viewer.pending_load:
		_finish_start()
	return engaged()

func _finish_start() -> void:
	if not viewer.car_state.get("ok", false):
		_start_failed("Original model could not prepare Test drive")
		return
	var result: Dictionary = viewer.car_model.apply_driving_snapshot(state)
	if not result.get("ok", false):
		_start_failed(str(result.get("error", "Initial driving pose failed")))
		return
	_present_surface()
	skid_renderer = preload("res://scripts/debug/driving_skids.gd").new(viewer)
	audio_display = preload("res://scripts/runtime/driving_audio.gd").new()
	viewer.add_child(audio_display)
	var audio_result: Dictionary = audio_display.configure(lab)
	if not audio_result.get("ok", false):
		_start_failed(str(audio_result.get("error", "Original vehicle sound unavailable")))
		return
	active = true
	phase = Phase.DRIVING
	capture_wanted = true
	_update_camera(0)
	if phase != Phase.DRIVING: return
	sync_lifecycle()
	_sync_pointer()

func _start_failed(reason: String) -> void:
	stop()
	error = reason
	DebugTools.write_log(error, true)

func stop() -> void:
	if not engaged():
		return
	lab.action("pause")
	if audio_display != null:
		audio_display.silence()
		audio_display.queue_free()
		audio_display = null
	lab.clear_particles()
	if viewer.track_model != null:
		viewer.track_model.set_driving_effect_source(null)
	active = false
	driving_camera.reset()
	phase = Phase.OFF
	capture_wanted = false
	viewer.release_camera()
	input_adapter.clear()
	lab.action("restart")
	viewer.car_model.transform = saved.transform
	viewer.car_model.visible = bool(saved.car_visible)
	viewer.floor_node.position = saved.floor_position
	_clear_surface()
	if skid_renderer != null:
		skid_renderer.clear()
		skid_renderer = null
	skid_quad_count = 0
	viewer.floor_node.mesh.size = saved.floor_size
	viewer.floor_node.material_override = saved.floor_material
	viewer.floor_node.visible = saved.floor_visible
	viewer.grid_node.visible = saved.grid_visible
	viewer.grid_node.position = saved.grid_position
	viewer.show_lods = bool(saved.show_lods)
	# The same cached renderer restores preview attachment bindings on commit.
	if viewer.viewer_kind == "car":
		DebugTools.host.viewer_apply("load", saved.source)
		viewer.car_model.set_show_lods(viewer.show_lods)
		viewer._queue_car(viewer.car_model.request_car(saved.source, saved.car_id, saved.skin_id), false)
	else:
		viewer.car_model.visible = false
	viewer.preview_mode = bool(saved.preview)
	viewer.orbit_yaw = saved.orbit_yaw
	viewer.orbit_pitch = saved.orbit_pitch
	viewer.orbit_distance = saved.orbit_distance
	viewer.orbit_center = saved.orbit_center
	viewer.camera.keep_aspect = saved.keep_aspect
	viewer.camera.fov = saved.fov
	viewer.camera.near = saved.near
	viewer.camera.transform = saved.camera_transform
	error = ""

func _fail(reason: String) -> void:
	if phase == Phase.ERROR:
		return
	input_adapter.clear()
	driving_camera.clear_held_view()
	error = reason if not reason.is_empty() else "Native driving stopped unexpectedly"
	lab.action("pause")
	phase = Phase.ERROR
	capture_wanted = false
	viewer.release_camera()
	DebugTools.write_log(error, true)

func captures_pointer() -> bool:
	return phase == Phase.DRIVING and capture_wanted and viewer.focused and not DebugTools.is_console_open()

func _sync_pointer() -> void:
	var wanted: bool = captures_pointer()
	var mode := Input.MOUSE_MODE_CAPTURED if wanted else Input.MOUSE_MODE_VISIBLE
	if Input.mouse_mode != mode:
		Input.mouse_mode = mode

func sync_lifecycle() -> bool:
	# Returns true on a transition: the caller discards that frame's elapsed time.
	if phase not in [Phase.DRIVING, Phase.SUSPENDED]:
		return false
	var suspend: bool = not viewer.focused or DebugTools.is_console_open()
	if suspend and phase == Phase.DRIVING:
		state = lab.action("pause")
		input_adapter.clear()
		driving_camera.clear_held_view()
		viewer.release_camera()
		phase = Phase.SUSPENDED
		discard_elapsed = true
		return true
	if not suspend and phase == Phase.SUSPENDED:
		viewer.held.clear()
		input_adapter.clear()
		discard_elapsed = true
		state = lab.action("run")
		if state.get("paused", true) or str(state.get("status", "")) in ["Failed", "Blocked"]:
			_fail(_reason(state))
		else:
			phase = Phase.DRIVING
			_sync_pointer()
		return true
	return false

func controls() -> Vector3:
	var value: Dictionary = input_adapter.frame()
	return Vector3(value.throttle, value.brake, value.steering)

func handle_input(event: InputEvent, consumed: bool = false) -> void:
	# A release must clear a held lookback even if a widget/focus change consumes it.
	if event.is_action_released("hp2_drive_camera_lookback"):
		driving_camera.set_look_back(false)
	var keyboard_consumed := consumed and (event is InputEventKey or event is InputEventAction)
	input_adapter.handle_input(event, keyboard_consumed or phase != Phase.DRIVING or not viewer.focused)
	if consumed or phase != Phase.DRIVING or not viewer.focused:
		if keyboard_consumed or phase != Phase.DRIVING or not viewer.focused:
			driving_camera.clear_held_view()
		return
	if event.is_action_pressed("hp2_drive_release_cursor", false):
		capture_wanted = false
		viewer.release_camera()
	elif event.is_action_pressed("hp2_drive_camera_view", false):
		var result: Dictionary = lab.cycle_driving_camera_pov()
		if not result.get("ok", false):
			_fail(str(result.get("error", "Original driving POV cycle failed")))
			return
		driving_camera.clear_mouse()
		_update_camera(0)
	elif event.is_action_pressed("hp2_drive_camera_lookback", false):
		driving_camera.set_look_back(true)
	elif event is InputEventMouseMotion and captures_pointer():
		driving_camera.mouse_motion(event.relative)
	elif event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		capture_wanted = true
		_sync_pointer()

func _update_camera(delta: float) -> void:
	camera_state = lab.update_driving_camera(delta, driving_camera.look_back)
	if not camera_state.get("ok", false):
		_fail(str(camera_state.get("error", "Recovered driving camera failed")))
		return
	driving_camera.apply(viewer.camera, camera_state, delta)
	# In this single-car host, the vehicle renderer is the main car/flare subtree.
	# Its environment capture consumes separate world faces, not this car subtree.
	viewer.car_model.visible = driving_car_visible and not bool(camera_state.hide_car)
	var target: Vector3 = camera_state.target
	var tile := Vector3(floorf(target.x / 10) * 10, 0, floorf(target.z / 10) * 10)
	viewer.floor_node.position = tile
	viewer.grid_node.position = tile + Vector3(0, 0.002, 0)

func process(delta: float) -> void:
	if phase == Phase.STARTING:
		if not viewer.pending_load:
			_finish_start()
		return
	if not active:
		return
	if sync_lifecycle():
		return
	if phase != Phase.DRIVING:
		return
	_sync_pointer()
	if discard_elapsed:
		discard_elapsed = false
		return
	if DebugTools.host.viewer_keyboard_capture():
		driving_camera.clear_held_view()
		viewer.held.clear()
		# Intentional GUI commands survive the widget's keyboard capture. Physical
		# held/edge inputs do not. Lifecycle suspension still clears both sources.
		input_adapter.clear(true)
	var value: Dictionary = input_adapter.frame()
	last_input_frame = value.duplicate()
	var next: Dictionary = lab.set_driving_input(value.throttle, value.brake, value.steering,
		value.handbrake, value.transmission, value.shift, value.neutral, value.horn)
	if not next.get("ok", false):
		_fail(_reason(next))
		return
	var previous_step := int(state.get("step", 0))
	native_tick_calls += 1
	state = lab.tick(delta)
	if int(state.get("step", 0)) > previous_step:
		input_adapter.committed()
	if not _valid_step(state):
		_fail(_reason(state))
		return
	var result: Dictionary = viewer.car_model.apply_driving_snapshot(state)
	if not result.get("ok", false):
		_fail(str(result.get("error", "Driving pose publication failed")))
		return
	_present_surface()
	_update_camera(delta)
	if phase != Phase.DRIVING: return
	_present_skids()
	if viewer.viewer_kind == "car":
		var particles: Dictionary = lab.present_particles(viewer, viewer.camera)
		state["particles"] = particles
		if not particles.get("ok", false):
			_fail(str(particles.get("error", "Original wheel particles failed")))

func _present_skids() -> void:
	if skid_renderer == null:
		return
	var camera: Camera3D = viewer.camera
	var clip: Projection = camera.get_camera_projection() * Projection(camera.global_transform.affine_inverse())
	# Consume the actual projection after the original horizontal-FOV policy.
	var H := viewer.get_viewport().get_visible_rect().size.y * camera.get_camera_projection().y.y * 0.5
	var rendered: Dictionary = lab.render_skids(camera.global_transform, clip, H)
	if not rendered.get("ok", false):
		_fail(str(rendered.get("error", "Original skid rendering failed")))
		return
	skid_renderer.publish(rendered.get("commands", []))
	skid_quad_count = skid_renderer.count

# TASK-0045: shared native descriptor drives presentation. The shader's world Z is
# native X; following the camera/floor never moves the material boundaries.
func _clear_surface() -> void:
	if is_instance_valid(surface_visuals):
		surface_visuals.queue_free()
	surface_visuals = null
	surface_material = null
	surface_key = ""

func _present_surface() -> void:
	if viewer.viewer_kind == "track":
		return
	if not bool(state.get("surface_known", false)):
		return
	var settings: Dictionary = state.surface
	var catalog: Array = state.get("surface_catalog", [])
	var bounds: Array = state.get("surface_boundaries", [])
	var ids: Array = state.get("surface_terrain_ids", [])
	var colors: Array = state.get("surface_colors", [])
	if catalog.size() != 5 or bounds.size() != 4 or ids.size() != 5 or colors.size() != 5:
		return
	var key := "%d:%d" % [int(settings.mode), int(settings.terrain)]
	if key == surface_key:
		return
	_clear_surface()
	surface_key = key
	var shader := Shader.new()
	shader.code = """shader_type spatial;
render_mode unshaded;
varying float world_z;
uniform bool zones = false;
uniform int uniform_index = 0;
uniform vec4 boundaries;
uniform vec4 color0 : source_color;
uniform vec4 color1 : source_color;
uniform vec4 color2 : source_color;
uniform vec4 color3 : source_color;
uniform vec4 color4 : source_color;
void vertex(){world_z=(MODEL_MATRIX*vec4(VERTEX,1.0)).z;}
void fragment(){
 int zone=uniform_index;
 if(zones){zone=0;if(world_z>=boundaries.x)zone=1;if(world_z>=boundaries.y)zone=2;if(world_z>=boundaries.z)zone=3;if(world_z>=boundaries.w)zone=4;}
 vec4 chosen=color0;if(zone==1)chosen=color1;if(zone==2)chosen=color2;if(zone==3)chosen=color3;if(zone==4)chosen=color4;
 ALBEDO=chosen.rgb;
}
"""
	surface_material = ShaderMaterial.new()
	surface_material.shader = shader
	surface_material.set_shader_parameter("zones", int(settings.mode) == 1)
	surface_material.set_shader_parameter("uniform_index", ids.find(int(settings.terrain)))
	surface_material.set_shader_parameter("boundaries", Vector4(float(bounds[0]), float(bounds[1]), float(bounds[2]), float(bounds[3])))
	for i in 5:
		surface_material.set_shader_parameter("color%d" % i, colors[i])
	viewer.floor_node.material_override = surface_material
	surface_visuals = Node3D.new()
	surface_visuals.name = "ToolSurfaceLabels"
	viewer.add_child(surface_visuals)
	surface_visuals.top_level = true
	surface_visuals.global_transform = Transform3D.IDENTITY
	for i in 5:
		if int(settings.mode) == 0 and int(ids[i]) != int(settings.terrain):
			continue
		var label := Label3D.new()
		var material_name := ""
		for entry in catalog:
			if int(entry.id) == int(ids[i]):
				material_name = str(entry.name)
		label.text = "%s #%d\nTool surface" % [material_name, int(ids[i])]
		label.billboard = BaseMaterial3D.BILLBOARD_ENABLED
		label.no_depth_test = false
		label.font_size = 24
		label.pixel_size = 0.005
		label.modulate = colors[i]
		var z := 0.0
		if int(settings.mode) == 1:
			var lower := 0.0 if i == 0 else float(bounds[i - 1])
			var upper := float(bounds[i]) if i < 4 else float(bounds[3]) + (float(bounds[3]) - float(bounds[2]))
			z = (lower + upper) * 0.5
		label.position = Vector3(-5, 0.4, z)
		surface_visuals.add_child(label)

func action(request: Dictionary) -> void:
	var command := str(request.get("action", ""))
	if command == "drive_exit":
		stop()
	elif command == "drive_transmission" and engaged():
		input_adapter.request({"transmission": int(request.get("lod", 0))}, true)
	elif command == "drive_neutral" and engaged():
		input_adapter.request({"neutral": true}, true)
	elif command == "drive_surface" and engaged():
		# UiAction lod/car_id carry this explicit port-side mode/material payload.
		var result: Dictionary = lab.set_surface(int(request.get("lod", 0)), int(request.get("car_id", 1)))
		if not result.get("ok", false):
			DebugTools.write_log(_reason(result), true)
		else:
			state = result # Still the last committed surface until the next successful tick.

func snapshot() -> Dictionary:
	return {"kind": "driving", "real_track": viewer.viewer_kind == "track", "track_name": viewer.track_state.get("track_name", ""), "status": "Preparing Test drive" if phase == Phase.STARTING else "",
		"diagnostic": error, "driving_hints": input_adapter.hints(), "powertrain": state.get("powertrain", {}), "powertrain_known": state.get("powertrain_known", false), "driving_telemetry": "", "driving_recording": false,
		"surface_known": state.get("surface_known", false), "surface": state.get("surface", {}),
		"surface_catalog": state.get("surface_catalog", []), "wheels": state.get("wheels", [])}
