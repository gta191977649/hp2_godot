extends Node3D
# Asset-free GPU numeric checks for the exact shared GS fusion/framebuffer helpers.
func _ready() -> void:
	_run.call_deferred()

func _function(source: String, name: String) -> String:
	var start := source.find(name)
	assert(start >= 0)
	var opening := source.find("{", start)
	var balance := 1
	var end := opening + 1
	while balance > 0 and end < source.length():
		if source[end] == "{":
			balance += 1
		elif source[end] == "}":
			balance -= 1
		end += 1
	return source.substr(start, end - start)

func _material(helper: String, effects: String, source: String, additive: bool) -> ShaderMaterial:
	var shader := Shader.new()
	shader.code = "shader_type spatial;\nrender_mode unshaded, cull_disabled, fog_disabled, shadows_disabled, depth_draw_never, " + ("blend_add" if additive else "blend_mix") + ";\n" + helper + "\n" + effects + "\nvoid fragment(){" + source + "}\n"
	var material := ShaderMaterial.new()
	material.shader = shader
	return material

func _fail(reason: String) -> void:
	push_error(reason)
	get_tree().quit(1)

func _run() -> void:
	if DisplayServer.get_name() == "headless":
		get_tree().quit(77)
		return
	var overlay := get_node_or_null("/root/DebugTools")
	if overlay != null:
		overlay.set("visible", false)
		overlay.set_process(false)
	var base := FileAccess.get_file_as_string(ProjectSettings.globalize_path("res://../src/platform/shaders/car_base.gdshader"))
	var track := FileAccess.get_file_as_string(ProjectSettings.globalize_path("res://../src/platform/shaders/track_effects.glsl"))
	if base.is_empty() or track.is_empty():
		_fail("Shared shader sources are unavailable for numeric probe")
		return
	var helper := _function(base, "vec3 hp2_framebuffer_rgb")
	var effects := _function(track, "int hp2_track_blend_channel") + "\n" + _function(track, "ivec3 hp2_track_fused40_increment") + "\n" + _function(track, "ivec3 hp2_track_fused40_copy")
	var viewport := SubViewport.new()
	viewport.size = Vector2i(16, 16)
	viewport.own_world_3d = true
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	add_child(viewport)
	var camera := Camera3D.new()
	camera.projection = Camera3D.PROJECTION_ORTHOGONAL
	camera.size = 2
	camera.position.z = 3
	viewport.add_child(camera)
	camera.current = true
	var background := MeshInstance3D.new()
	var quad := QuadMesh.new()
	quad.size = Vector2(4, 4)
	background.mesh = quad
	background.position.z = -0.1
	viewport.add_child(background)
	var foreground := MeshInstance3D.new()
	foreground.mesh = quad
	viewport.add_child(foreground)
	var cases := [
		{"base": 100, "specular": 40, "alpha": 192, "destination": 20, "additive": true, "expected": 230},
		{"base": 100, "specular": 40, "alpha": 192, "destination": 20, "additive": false, "expected": 160},
		{"base": 250, "specular": 40, "alpha": 192, "destination": 20, "additive": false, "expected": 255},
		{"base": 200, "specular": 60, "alpha": 192, "destination": 20, "additive": true, "expected": 255},
		{"base": 100, "specular": 40, "alpha": 64, "destination": 20, "additive": true, "expected": 90},
		{"base": 100, "specular": 0, "alpha": 192, "destination": 20, "additive": true, "plain_add": true, "expected": 170},
		{"base": 100, "specular": 0, "alpha": 64, "destination": 20, "additive": true, "plain_add": true, "expected": 70}]
	for sample in cases:
		background.material_override = _material(helper, "", "ALBEDO=hp2_framebuffer_rgb(vec3(%f/255.0));ALPHA=1.0;" % float(sample.destination), false)
		var operation := "hp2_track_fused40_increment" if sample.additive else "hp2_track_fused40_copy"
		var fragment := "ivec3 fused=%s(ivec3(%d),ivec3(%d),%d);ALBEDO=hp2_framebuffer_rgb(clamp(vec3(fused),vec3(0.0),vec3(255.0))/255.0);ALPHA=1.0;" % [operation, sample.base, sample.specular, sample.alpha]
		if sample.get("plain_add", false):
			fragment = "int increment=hp2_track_blend_channel(%d,0,%d,0,72);ALBEDO=hp2_framebuffer_rgb(vec3(clamp(float(increment),0.0,255.0)/255.0));ALPHA=1.0;" % [sample.base, sample.alpha]
		foreground.material_override = _material(helper, effects, fragment, bool(sample.additive))
		foreground.material_override.render_priority = 1
		await get_tree().process_frame
		await RenderingServer.frame_post_draw
		await get_tree().process_frame
		await RenderingServer.frame_post_draw
		var image := viewport.get_texture().get_image()
		var pixel := image.get_pixel(8, 8)
		var observed := Vector3i(roundi(pixel.r * 255), roundi(pixel.g * 255), roundi(pixel.b * 255))
		print("TRACK_FUSION_GPU_SAMPLE ", sample, " observed=", observed)
		for channel in [observed.x, observed.y, observed.z]:
			if absi(channel - int(sample.expected)) > 1:
				_fail("Shared fusion shader GPU byte-domain mismatch: " + str(sample) + " observed=" + str(observed))
				return
	# Ordered transparent queue adapter: explicit ranks dominate model origins.
	background.visible = false
	foreground.visible = false
	var ordered: Array[MeshInstance3D] = []
	for value in [200, 100]:
		var mesh := MeshInstance3D.new()
		mesh.mesh = quad
		mesh.material_override = _material(helper, "", "ALBEDO=hp2_framebuffer_rgb(vec3(%f/255.0));ALPHA=1.0;" % float(value), false)
		mesh.material_override.render_priority = -64
		mesh.set_sorting_use_aabb_center(false)
		viewport.add_child(mesh)
		ordered.append(mesh)
	for shift in [false, true]:
		for index in 2:
			var mesh: MeshInstance3D = ordered[index]
			mesh.position.z = (0.7 if index == 0 else -0.9) if shift else 0.0
			var origin_depth := -camera.to_local(mesh.global_position).z
			var rank := 200.0 if index == 0 else 100.0
			mesh.set_sorting_offset(origin_depth - rank)
		await get_tree().process_frame
		await RenderingServer.frame_post_draw
		await get_tree().process_frame
		await RenderingServer.frame_post_draw
		var ordered_pixel := viewport.get_texture().get_image().get_pixel(8, 8)
		var result := Vector3i(roundi(ordered_pixel.r * 255), roundi(ordered_pixel.g * 255), roundi(ordered_pixel.b * 255))
		if absi(result.x - 100) > 1 or absi(result.y - 100) > 1 or absi(result.z - 100) > 1:
			_fail("Explicit transparent ranks changed with model origin: shifted=" + str(shift) + " pixel=" + str(result))
			return
		print("TRACK_ORDER_GPU_SAMPLE shifted=", shift, " observed=", result)
	print("TRACK_FUSION_GPU_OK")
	get_tree().quit(0)
