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
	var viewport := SubViewport.new();viewport.use_hdr_2d=RenderingServer.get_current_rendering_method()!="gl_compatibility"
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
	# Real Track11 road material/normal/light context, CPU original VU oracle.
	var alpha_fixtures = JSON.parse_string(FileAccess.get_file_as_string(ProjectSettings.globalize_path("res://../reverse/task0052/roadshine-alpha-fixtures.json")))
	if not alpha_fixtures is Array or alpha_fixtures.size() < 6:
		_fail("Original road C3 fixtures unavailable")
		return
	var alpha_helpers := "uniform vec3 source_camera;\nuniform vec3 probe_position;\nuniform vec3 probe_normal;\nuniform vec4 probe_material0;\nuniform vec4 probe_material3;\nuniform vec3 light_direction;\n" + _function(base, "float vu_bit_affine") + "\n" + _function(base, "vec3 vu_reflection") + "\nfloat hp2_affine_bit_power(float v,float s,float b){return vu_bit_affine(v,s,b); }\n" + _function(track, "float hp2_track_alpha40")
	for fixture in alpha_fixtures:
		var material := _material(helper, alpha_helpers, "float alpha=hp2_track_alpha40(vu_reflection(probe_position,probe_normal),probe_material0,probe_material3,light_direction);ALBEDO=hp2_framebuffer_rgb(vec3(alpha/255.0));ALPHA=1.0;", false)
		material.set_shader_parameter("source_camera", Vector3(fixture.camera[0],fixture.camera[1],fixture.camera[2]))
		material.set_shader_parameter("probe_position", Vector3(fixture.position[0],fixture.position[1],fixture.position[2]))
		material.set_shader_parameter("probe_normal", Vector3(fixture.normal[0],fixture.normal[1],fixture.normal[2]))
		material.set_shader_parameter("probe_material0", Vector4(fixture.material[0],fixture.material[1],fixture.material[2],fixture.material[3]))
		material.set_shader_parameter("probe_material3", Vector4(fixture.material[12],fixture.material[13],fixture.material[14],fixture.material[15]))
		material.set_shader_parameter("light_direction", Vector3(fixture.light[16],fixture.light[20],fixture.light[24]))
		foreground.material_override = material
		await RenderingServer.frame_post_draw
		await RenderingServer.frame_post_draw
		var pixel := viewport.get_texture().get_image().get_pixel(8,8)
		var observed := roundi(pixel.r*255.0)
		print("TRACK_C3_GPU_SAMPLE solid=",fixture.solid," camera=",fixture.camera," expected=",fixture.alpha," observed=",observed)
		if observed != int(fixture.alpha):
			_fail("Shared C3 GPU alpha disagrees with original CPU arithmetic")
			return
	# Exercise production vertex C3, CUSTOM0 float4 normals and RGBA8 colour.
	# A uniform supplies the original native point while a host quad provides coverage.
	var vertex_code := base.replace("@RENDER_MODES@", "fog_disabled, shadows_disabled, depth_draw_never, blend_mix").replace("@SAMPLER_HINT@", "filter_linear")
	vertex_code = vertex_code.replace("@FRAGMENT_SOURCE@", "ALBEDO=hp2_framebuffer_rgb(vec3(gs_rgba.a/255.0));ALPHA=1.0;")
	var alpha_vertex := FileAccess.get_file_as_string(ProjectSettings.globalize_path("res://../src/platform/shaders/track_alpha_vertex.glsl"))
	vertex_code = vertex_code.replace("    if(reflection_pass){", alpha_vertex + "\n    if(reflection_pass){")
	vertex_code = vertex_code.replace("void vertex(){", "float hp2_affine_bit_power(float v,float s,float b){return vu_bit_affine(v,s,b); }\n" + _function(track, "float hp2_track_alpha40") + "\nvoid vertex(){")
	vertex_code = vertex_code.replace("void vertex(){", "uniform vec3 fixture_native_position;\nvoid vertex(){").replace("vec3 source_position = vec3(VERTEX.z, VERTEX.x, VERTEX.y);", "vec3 source_position = fixture_native_position;")
	var vertex_shader := Shader.new()
	vertex_shader.code = vertex_code
	for fixture in alpha_fixtures:
		var arrays := []
		arrays.resize(Mesh.ARRAY_MAX)
		arrays[Mesh.ARRAY_VERTEX] = PackedVector3Array([Vector3(-1,-1,0),Vector3(1,-1,0),Vector3(1,1,0),Vector3(-1,1,0)])
		arrays[Mesh.ARRAY_COLOR] = PackedColorArray([Color(248.0/255.0,120.0/255.0,16.0/255.0,128.0/255.0),Color.WHITE,Color.BLACK,Color.RED])
		arrays[Mesh.ARRAY_CUSTOM0] = PackedFloat32Array([fixture.normal[0],fixture.normal[1],fixture.normal[2],0,fixture.normal[0],fixture.normal[1],fixture.normal[2],0,fixture.normal[0],fixture.normal[1],fixture.normal[2],0,fixture.normal[0],fixture.normal[1],fixture.normal[2],0])
		arrays[Mesh.ARRAY_INDEX] = PackedInt32Array([0,1,2,0,2,3])
		var original_mesh := ArrayMesh.new()
		original_mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES,arrays,[],{},Mesh.ARRAY_CUSTOM_RGBA_FLOAT<<Mesh.ARRAY_FORMAT_CUSTOM0_SHIFT)
		foreground.mesh = original_mesh
		foreground.custom_aabb = AABB(Vector3(-1,-1,-1),Vector3(2,2,2))
		var material := ShaderMaterial.new()
		material.shader = vertex_shader
		material.set_shader_parameter("fixture_native_position",Vector3(fixture.position[0],fixture.position[1],fixture.position[2]))
		material.render_priority = 100
		material.render_priority = 100
		material.set_shader_parameter("source_camera",Vector3(fixture.camera[0],fixture.camera[1],fixture.camera[2]))
		for row in range(4):
			material.set_shader_parameter("material_"+str(350+row),Vector4(fixture.material[row*4],fixture.material[row*4+1],fixture.material[row*4+2],fixture.material[row*4+3]))
		for row in range(7):
			var key: String = "light_"+(["35c","35d","35e","35f","360","361","362"][row])
			material.set_shader_parameter(key,Vector4(fixture.light[row*4],fixture.light[row*4+1],fixture.light[row*4+2],fixture.light[row*4+3]))
		foreground.material_override = material
		await RenderingServer.frame_post_draw
		await RenderingServer.frame_post_draw
		var observed := roundi(viewport.get_texture().get_image().get_pixel(8,8).r*255.0)
		print("TRACK_C3_VERTEX_GPU_SAMPLE expected=",fixture.alpha," observed=",observed)
		if observed != int(fixture.alpha):
			_fail("Production vertex C3 transport disagrees with original CPU arithmetic")
			return
	foreground.mesh = quad
	var cases := [
		{"base": 100, "specular": 40, "alpha": 192, "destination": 20, "additive": true, "expected": 230},
		{"base": 100, "specular": 40, "alpha": 192, "destination": 20, "additive": false, "expected": 160},
		{"base": 100, "specular": 40, "alpha": 192, "destination": 20, "additive": true, "roadshine": false, "expected": 170},
		{"base": 100, "specular": 40, "alpha": 192, "destination": 20, "additive": false, "roadshine": false, "expected": 100},
		{"base": 250, "specular": 40, "alpha": 192, "destination": 20, "additive": false, "expected": 255},
		{"base": 200, "specular": 60, "alpha": 192, "destination": 20, "additive": true, "expected": 255},
		{"base": 100, "specular": 40, "alpha": 64, "destination": 20, "additive": true, "expected": 90},
		{"base": 100, "specular": 0, "alpha": 192, "destination": 20, "additive": true, "plain_add": true, "expected": 170},
		{"base": 100, "specular": 0, "alpha": 64, "destination": 20, "additive": true, "plain_add": true, "expected": 70}]
	for sample in cases:
		background.material_override = _material(helper, "", "ALBEDO=hp2_framebuffer_rgb(vec3(%f/255.0));ALPHA=1.0;" % float(sample.destination), false)
		var operation := "hp2_track_fused40_increment" if sample.additive else "hp2_track_fused40_copy"
		var fragment := "ivec3 fused=%s(ivec3(%d),ivec3(effect_roadshine_enabled?%d:0),%d);ALBEDO=hp2_framebuffer_rgb(clamp(vec3(fused),vec3(0.0),vec3(255.0))/255.0);ALPHA=1.0;" % [operation, sample.base, sample.specular, sample.alpha]
		if sample.get("plain_add", false):
			fragment = "int increment=hp2_track_blend_channel(%d,0,%d,0,72);ALBEDO=hp2_framebuffer_rgb(vec3(clamp(float(increment),0.0,255.0)/255.0));ALPHA=1.0;" % [sample.base, sample.alpha]
		foreground.material_override = _material(helper, "uniform bool effect_roadshine_enabled=true;\n" + effects, fragment, bool(sample.additive))
		foreground.material_override.set_shader_parameter("effect_roadshine_enabled", sample.get("roadshine", true))
		foreground.material_override.render_priority = 1
		await get_tree().process_frame
		await RenderingServer.frame_post_draw
		await get_tree().process_frame
		await RenderingServer.frame_post_draw
		var image := viewport.get_texture().get_image()
		image.convert(Image.FORMAT_RGBA8)
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
