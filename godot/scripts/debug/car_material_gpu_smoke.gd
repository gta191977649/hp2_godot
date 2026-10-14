extends Node
# Port-side GPU numeric oracle. Run with the Compatibility backend and a real renderer.
# Uses the actual uploaded vehicle shader, with the independent native VU fixture inputs.
func _ready() -> void:
	_run.call_deferred()

func _check(value: bool, message: String) -> void:
	if not value:
		push_error("CAR_MATERIAL_GPU_FAILED: " + message)
		get_tree().quit(1)
		assert(value, message)

func _run() -> void:
	_check(RenderingServer.get_current_rendering_method() == "gl_compatibility", "Compatibility required")
	# Exercise the shared runtime backend, not the debug wrapper.
	var car: Node3D = ClassDB.instantiate("Ps2VehicleRenderer")
	add_child(car)
	var loaded: Dictionary = car.load_car("CARS/911TURBO/GEOMETRY.BIN")
	_check(loaded.ok, str(loaded.get("error", "Original shader source load")))
	car.visible = false
	var opaque: ShaderMaterial
	var transparent: ShaderMaterial
	var paint: ShaderMaterial
	var reflection: ShaderMaterial
	for child in car.get_children():
		if not child is MeshInstance3D:
			continue
		for index in child.mesh.get_surface_count():
			var material: Material = child.get_active_material(index)
			if material is ShaderMaterial:
				if material.next_pass is ShaderMaterial and reflection == null:
					reflection = material.next_pass.duplicate()
				var code: String = material.shader.code
				if code.contains("ALPHA=combined.a/128.0"):
					transparent = material.duplicate()
				elif not code.contains("ALPHA="):
					opaque = material.duplicate()
					if material.get_shader_parameter("flag_0x4"):
						paint = material.duplicate()
	_check(opaque != null and transparent != null, "Actual opaque/source-over shaders uploaded")
	_check(paint != null and paint.shader.code.contains("depth_draw_always"), "Original paint +0x50 writes depth independently of +0x74 TEST selection")
	_check(reflection != null and reflection.shader.code.contains("depth_draw_never"), "Actual original environment pass suppresses depth writes")
	opaque.next_pass = null
	transparent.next_pass = null
	paint.next_pass = null
	var viewport := SubViewport.new()
	viewport.size = Vector2i(64, 64)
	viewport.own_world_3d = true
	viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	add_child(viewport)
	var environment := WorldEnvironment.new()
	environment.environment = Environment.new()
	environment.environment.background_mode = Environment.BG_COLOR
	environment.environment.background_color = Color.BLACK
	viewport.add_child(environment)
	var camera := Camera3D.new()
	camera.projection = Camera3D.PROJECTION_ORTHOGONAL
	camera.size = 2
	camera.position = Vector3(0, 0, 3)
	viewport.add_child(camera)
	camera.current = true
	var arrays: Array = []
	arrays.resize(Mesh.ARRAY_MAX)
	arrays[Mesh.ARRAY_VERTEX] = PackedVector3Array([Vector3(-1, -1, 0), Vector3(1, -1, 0), Vector3(0, 1, 0)])
	arrays[Mesh.ARRAY_TEX_UV] = PackedVector2Array([Vector2(0.5, 0.5), Vector2(0.5, 0.5), Vector2(0.5, 0.5)])
	var packed := Color(248.0 / 255, 128.0 / 255, 64.0 / 255, 0)
	arrays[Mesh.ARRAY_COLOR] = PackedColorArray([packed, packed, packed])
	arrays[Mesh.ARRAY_CUSTOM0] = PackedFloat32Array([1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0])
	var mesh := ArrayMesh.new()
	mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays, [], {}, Mesh.ARRAY_CUSTOM_RGBA_FLOAT << Mesh.ARRAY_FORMAT_CUSTOM0_SHIFT)
	var instance := MeshInstance3D.new()
	instance.mesh = mesh
	viewport.add_child(instance)
	# CPU independent-instruction fixture: VU RGBA=(67,28,19,128).
	# With textureRGB=(64,128,192), GS MODULATE gives RGB=(33,28,28).
	var cases := [
		{"blend": false, "alpha": 128, "flag": false, "ambient": 32, "expected": Vector3i(33, 28, 28)},
		{"blend": true, "alpha": 64, "flag": false, "ambient": 32, "expected": Vector3i(16, 14, 14)},
		{"blend": false, "alpha": 0, "flag": false, "ambient": 32, "expected": Vector3i.ZERO},
		{"blend": false, "alpha": 128, "flag": true, "ambient": 32, "expected": Vector3i.ZERO},
		{"blend": false, "alpha": 128, "flag": true, "ambient": 33, "expected": Vector3i(33, 28, 28)}]
	for sample in cases:
		var blend := bool(sample.blend)
		var material: ShaderMaterial = transparent if blend else opaque
		material.set_shader_parameter("base_lit", true)
		material.set_shader_parameter("flag_0x4", sample.flag)
		material.set_shader_parameter("source_camera", Vector3(0, 0, 5))
		var blocks := [Vector4(0, 1065353216.0, 0.1, 1.0 / 255), Vector4(0.7, 0.5, 0.3, 99), Vector4(0.2, 0.3, 0.4, 66), Vector4(2, -1065353216.0, 32, 16)]
		if sample.flag:
			blocks[3] = Vector4(0, 1065353216.0, sample.ambient, 16)
		for i in 4:
			material.set_shader_parameter("material_" + str(350 + i), blocks[i])
		var names := ["35c", "35d", "35e", "35f", "360", "361", "362"]
		var lights := [Vector4(64, 32, 16, 128), Vector4(32, 64, 96, 128), Vector4(16, 32, 64, 128), Vector4(2, 3, 4, 0), Vector4(1, 0, 0, 0), Vector4(0, 1, 0, 0), Vector4(0, 0, 1, 0)]
		for i in 7:
			material.set_shader_parameter("light_" + names[i], lights[i])
		var raw_alpha := int(sample.alpha)
		var image := Image.create_from_data(1, 1, false, Image.FORMAT_RGBA8, PackedByteArray([64, 128, 192, raw_alpha]))
		material.set_shader_parameter("base_texture", ImageTexture.create_from_image(image))
		instance.material_override = material
		for i in 4:
			await get_tree().process_frame
			RenderingServer.force_draw(false)
		var pixel := viewport.get_texture().get_image().get_pixel(32, 32)
		var actual := Vector3i(roundi(pixel.r * 255), roundi(pixel.g * 255), roundi(pixel.b * 255))
		var expected: Vector3i = sample.expected
		_check(absi(actual.x - expected.x) <= 1 and absi(actual.y - expected.y) <= 1 and absi(actual.z - expected.z) <= 1, "Final GPU RGB %s != %s (blend=%s)" % [actual, expected, blend])
		print("CAR_MATERIAL_GPU_PIXEL: blend=", blend, " flag4=", sample.flag, " raw_alpha=", raw_alpha, " actual=", actual, " expected=", expected, " tolerance=1 encoded framebuffer unit")
	# A later transparent surface behind paint must not show through the bonnet.
	for uniform in opaque.shader.get_shader_uniform_list():
		paint.set_shader_parameter(uniform.name, opaque.get_shader_parameter(uniform.name))
	instance.material_override = paint
	var behind := MeshInstance3D.new()
	behind.mesh = mesh
	behind.position.z = -0.5
	var behind_material: ShaderMaterial = transparent.duplicate()
	behind_material.set_shader_parameter("base_texture", ImageTexture.create_from_image(Image.create_from_data(1, 1, false, Image.FORMAT_RGBA8, PackedByteArray([255, 255, 255, 128]))))
	behind.material_override = behind_material
	viewport.add_child(behind)
	for i in 4:
		await get_tree().process_frame
		RenderingServer.force_draw(false)
	var occluded := viewport.get_texture().get_image().get_pixel(32, 32)
	var occlusion_rgb := Vector3i(roundi(occluded.r * 255), roundi(occluded.g * 255), roundi(occluded.b * 255))
	_check(occlusion_rgb == Vector3i(33, 28, 28), "Paint must occlude a later source-over surface: " + str(occlusion_rgb))
	print("CAR_MATERIAL_GPU_DEPTH: actual=", occlusion_rgb, " paint occludes the later surface")
	behind.hide()
	# E-0035-R03 independent VU fixture: reflected=(1,0,0), identity environment.
	# Only the fixture input is substituted; the production environment arithmetic is used.
	var fixed_code: String = reflection.shader.code.replace("vec3 reflected = vu_reflection(source_position, source_normal);", "vec3 reflected = vec3(1.0,0.0,0.0);")
	var fixed_shader := Shader.new()
	fixed_shader.code = fixed_code
	reflection.shader = fixed_shader
	reflection.set_shader_parameter("environment_row0", Vector3.RIGHT)
	reflection.set_shader_parameter("environment_row1", Vector3.UP)
	reflection.set_shader_parameter("environment_row2", Vector3.BACK)
	reflection.set_shader_parameter("flag_0x4", false)
	reflection.set_shader_parameter("material_351", Vector4(0,0,0,64))
	reflection.set_shader_parameter("material_352", Vector4.ZERO)
	reflection.set_shader_parameter("base_texture", ImageTexture.create_from_image(Image.create_from_data(1, 1, false, Image.FORMAT_RGBA8, PackedByteArray([64,128,192,128]))))
	opaque.next_pass = reflection
	instance.material_override = opaque
	for i in 4:
		await get_tree().process_frame
		RenderingServer.force_draw(false)
	var env_pixel := viewport.get_texture().get_image().get_pixel(32,32)
	var env_rgb := Vector3i(roundi(env_pixel.r*255), roundi(env_pixel.g*255), roundi(env_pixel.b*255))
	_check(absi(env_rgb.x-65)<=1 and absi(env_rgb.y-92)<=1 and absi(env_rgb.z-124)<=1, "Original context2 additive RGB: " + str(env_rgb))
	for sample in [{"alpha":0.0,"expected":Vector3i(33,28,28)},{"alpha":128.0,"expected":Vector3i(97,156,220)}]:
		reflection.set_shader_parameter("material_351", Vector4(0,0,0,sample.alpha))
		for i in 4:
			await get_tree().process_frame
			RenderingServer.force_draw(false)
		var pixel: Color = viewport.get_texture().get_image().get_pixel(32,32)
		var rgb := Vector3i(roundi(pixel.r*255),roundi(pixel.g*255),roundi(pixel.b*255))
		var expected: Vector3i = sample.expected
		_check(absi(rgb.x-expected.x)<=1 and absi(rgb.y-expected.y)<=1 and absi(rgb.z-expected.z)<=1,"Environment alpha endpoint " + str(rgb))
	# A standalone environment pass must leave depth available to a later surface behind it.
	opaque.next_pass = null
	instance.material_override = reflection
	behind_material.set_render_priority(4)
	behind.show()
	for i in 4:
		await get_tree().process_frame
		RenderingServer.force_draw(false)
	var depth_pixel: Color = viewport.get_texture().get_image().get_pixel(32,32)
	_check(absi(roundi(depth_pixel.r*255)-133)<=1 and absi(roundi(depth_pixel.g*255)-55)<=1 and absi(roundi(depth_pixel.b*255)-37)<=1,"Environment must not occlude later surface: " + str(depth_pixel))
	# Conversely the environment must respect existing opaque depth in front of it.
	behind.hide()
	behind_material.set_render_priority(0)
	var depth_blocker: ShaderMaterial = paint.duplicate()
	behind.material_override = depth_blocker
	behind.position.z = 0.5
	behind.show()
	for i in 4:
		await get_tree().process_frame
		RenderingServer.force_draw(false)
	var blocked_pixel: Color = viewport.get_texture().get_image().get_pixel(32,32)
	_check(absi(roundi(blocked_pixel.r*255)-33)<=1 and absi(roundi(blocked_pixel.g*255)-28)<=1 and absi(roundi(blocked_pixel.b*255)-28)<=1,"Environment must retain depth testing: " + str(blocked_pixel))
	behind.hide()
	var diagnostic_shader := Shader.new()
	diagnostic_shader.code = fixed_code.replace("ALBEDO=hp2_framebuffer_rgb(combined.rgb/255.0);", "ALBEDO=hp2_framebuffer_rgb(vec3(UV,0.0));").replace("ALPHA=combined.a/128.0;", "ALPHA=1.0;")
	var diagnostic: ShaderMaterial = reflection.duplicate()
	diagnostic.shader = diagnostic_shader
	instance.material_override = diagnostic
	for i in 4:
		await get_tree().process_frame
		RenderingServer.force_draw(false)
	var uv_pixel := viewport.get_texture().get_image().get_pixel(32,32)
	_check(absi(roundi(uv_pixel.r*255)-39)<=1 and absi(roundi(uv_pixel.g*255)-128)<=1, "Original VU environment ST: " + str(uv_pixel))
	print("CAR_ENVIRONMENT_GPU_OK: VU ST, alpha64, original additive context2; actual=", env_rgb)
	car.queue_free()
	viewport.queue_free()
	await get_tree().process_frame
	print("CAR_MATERIAL_GPU_OK: actual vehicle shader, VU coefficients, texture MODULATE, copy/source-over")
	get_tree().quit(0)
