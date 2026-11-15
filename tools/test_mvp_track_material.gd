extends SceneTree

func _initialize() -> void:
    run.call_deferred()

func run() -> void:
    var camera := Camera3D.new()
    camera.fov = 90
    camera.cull_mask = 1
    root.add_child(camera)
    camera.make_current()
    var rgba := PackedByteArray()
    for i in range(16):
        rgba.append_array(PackedByteArray([96, 110, 128, 128]))
    for i in range(4):
        rgba.append_array(PackedByteArray([32, 64, 96, 128]))
    rgba.append_array(PackedByteArray([128, 128, 128, 128]))
    var image := Image.create_from_data(4, 4, true, Image.FORMAT_RGBA8, rgba)
    var material := ShaderMaterial.new()
    material.shader = load("res://shader/track_material_copy_static_write.gdshader")
    var tex := ImageTexture.create_from_image(image)
    material.set_shader_parameter("gs_texture_linear", tex)
    material.set_shader_parameter("gs_texture_nearest", tex)
    material.set_shader_parameter("gs_filter", Vector4i(2, 0, 2, 0))
    material.set_shader_parameter("gs_test", true)
    material.set_shader_parameter("repeat_axes", Vector2i.ZERO)
    for offset in [-1.0, 1.0]:
        var data := []
        data.resize(Mesh.ARRAY_MAX)
        data[Mesh.ARRAY_VERTEX] = PackedVector3Array([
            Vector3(offset-0.9, -0.8, -2), Vector3(offset+0.9, -0.8, -2),
            Vector3(offset-0.9, 0.8, -2), Vector3(offset+0.9, 0.8, -2)])
        data[Mesh.ARRAY_TEX_UV] = PackedVector2Array([Vector2.ZERO, Vector2.RIGHT, Vector2.DOWN, Vector2.ONE])
        var tint := Color8(192,144,64,128)
        data[Mesh.ARRAY_COLOR] = PackedColorArray([tint,tint,tint,tint])
        var bias := 0.0 if offset < 0.0 else 8.0
        data[Mesh.ARRAY_CUSTOM2] = PackedFloat32Array([bias,bias,bias,bias])
        data[Mesh.ARRAY_INDEX] = PackedInt32Array([0,1,2,1,3,2])
        var mesh := ArrayMesh.new()
        mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, data, [], {}, Mesh.ARRAY_CUSTOM_R_FLOAT << Mesh.ARRAY_FORMAT_CUSTOM2_SHIFT)
        mesh.surface_set_material(0, material)
        var instance := MeshInstance3D.new()
        instance.mesh = mesh
        root.add_child(instance)
    for i in range(4):
        await process_frame
        await RenderingServer.frame_post_draw
    var frame := root.get_texture().get_image()
    var center := frame.get_size() / 2
    var x := int(frame.get_height() / 4.0)
    var a := frame.get_pixel(center.x-x,center.y)
    var b := frame.get_pixel(center.x+x,center.y)
    var left := Vector3i(a.r8,a.g8,a.b8)
    var right := Vector3i(b.r8,b.g8,b.b8)
    print("GS_TRACK_RGB left=", left, " right=", right)
    if (left-Vector3i(144,123,64)).length() > 3 or (right-Vector3i(48,72,48)).length() > 3:
        quit(1)
        return
    var meshes := root.get_children().filter(func(node: Node) -> bool: return node is MeshInstance3D)
    var underlay := water_layer(meshes[0], "mix_write")
    var highlight := water_layer(meshes[1], "add_read")
    for i in range(3):
        await process_frame
        await RenderingServer.frame_post_draw
    frame = root.get_texture().get_image()
    a = frame.get_pixel(center.x-x, center.y)
    b = frame.get_pixel(center.x+x, center.y)
    left = Vector3i(a.r8, a.g8, a.b8)
    right = Vector3i(b.r8, b.g8, b.b8)
    print("GS_ROAD_WATER underlay=", left, " mask=", right)
    if (left-Vector3i(88,93,80)).length() > 3 or (right-Vector3i(64,104,96)).length() > 3:
        quit(1)
        return
    var sky := ShaderMaterial.new()
    sky.shader = load("res://shader/track_sky_mix.gdshader")
    sky.set_shader_parameter("albedo_texture", tex)
    sky.render_priority = Material.RENDER_PRIORITY_MIN
    meshes[0].material_override = sky
    for i in range(3):
        await process_frame
        await RenderingServer.frame_post_draw
    a = root.get_texture().get_image().get_pixel(center.x-x, center.y)
    var sky_mix := Vector3i(a.r8, a.g8, a.b8)
    var expected := Color8(144,123,64).srgb_to_linear().lerp(
        Color8(32,64,96).srgb_to_linear(), 0.5).linear_to_srgb()
    print("GS_ROAD_WATER over_sky=", sky_mix)
    if (sky_mix-Vector3i(expected.r8, expected.g8, expected.b8)).length() > 3:
        quit(1)
        return
    var wet_material := underlay.mesh.surface_get_material(0) as ShaderMaterial
    var clean_sky := Image.create_from_data(1, 1, false, Image.FORMAT_RGBA8,
        PackedByteArray([120,160,200,255]))
    wet_material.set_shader_parameter("gs_water_sky", ImageTexture.create_from_image(clean_sky))
    wet_material.set_shader_parameter("gs_has_water_sky", true)
    meshes[0].material_override = null
    meshes[0].position.z = -0.5
    for i in range(3):
        await process_frame
        await RenderingServer.frame_post_draw
    a = root.get_texture().get_image().get_pixel(center.x-x, center.y)
    var clean_mix := Vector3i(a.r8, a.g8, a.b8)
    print("GS_ROAD_WATER clean_sky=", clean_mix)
    if (clean_mix-Vector3i(76,112,148)).length() > 3:
        quit(1)
        return
    # The road texture blends over the clean sky before the mask and the
    # 0x320 falloff add their byte contributions to that same destination.
    wet_material.set_shader_parameter("gs_has_wet_mask", true)
    var mask_tex: Texture2D = wet_material.get_shader_parameter("gs_texture_linear")
    wet_material.set_shader_parameter("gs_wet_mask_linear", mask_tex)
    wet_material.set_shader_parameter("gs_wet_mask_nearest", mask_tex)
    wet_material.set_shader_parameter("gs_road_alpha", Vector4(1, 0, 128, 0))
    wet_material.set_shader_parameter("gs_specular", ImageTexture.create_from_image(Image.create_from_data(
        1, 1, false, Image.FORMAT_RGBA8, PackedByteArray([8,12,16,255]))))
    for shine in [false, true]:
        RenderingServer.global_shader_parameter_set("hp2_roadshine", shine)
        for i in range(3):
            await process_frame
            await RenderingServer.frame_post_draw
        a = root.get_texture().get_image().get_pixel(center.x-x, center.y)
        var paired := Vector3i(a.r8, a.g8, a.b8)
        var expected_pair := Vector3i(96,150,204) if shine else Vector3i(92,144,196)
        print("GS_ROAD_WATER paired, roadshine=", shine, " rgb=", paired)
        if (paired-expected_pair).length() > 3:
            quit(1)
            return
    wet_material.set_shader_parameter("gs_has_wet_mask", false)
    var biased := water_layer(meshes[0], "add_read_biased")
    var biased_material: ShaderMaterial = biased.mesh.surface_get_material(0)
    biased_material.render_priority = 4
    biased_material.set_shader_parameter("gs_bias", 1)
    biased_material.set_shader_parameter("gs_has_puddle_base", true)
    biased_material.set_shader_parameter("gs_puddle_base_linear", mask_tex)
    biased_material.set_shader_parameter("gs_puddle_base_nearest", mask_tex)
    biased_material.set_shader_parameter("gs_has_water_sky", true)
    biased_material.set_shader_parameter("gs_water_sky", wet_material.get_shader_parameter("gs_water_sky"))
    for i in range(3):
        await process_frame
        await RenderingServer.frame_post_draw
    a = root.get_texture().get_image().get_pixel(center.x-x, center.y)
    var biased_mix := Vector3i(a.r8, a.g8, a.b8)
    print("GS_ROAD_WATER biased mask over wet asphalt=", biased_mix)
    if (biased_mix-Vector3i(92,144,196)).length() > 3:
        quit(1)
        return
    # Move the opaque receiver in front of pack 0, first within pack 1's
    # depth offset and then in front of both wet draws.
    for receiver_z in [0.001, 0.01]:
        meshes[0].position.z = receiver_z
        for i in range(3):
            await process_frame
            await RenderingServer.frame_post_draw
        a = root.get_texture().get_image().get_pixel(center.x-x, center.y)
        var receiver := Vector3i(a.r8, a.g8, a.b8)
        var expected_receiver := Vector3i(160,155,112) if receiver_z < 0.01 else Vector3i(144,123,64)
        print("GS_ROAD_WATER biased receiver z=", receiver_z, " rgb=", receiver)
        if (receiver-expected_receiver).length() > 3:
            quit(1)
            return
    biased.queue_free()
    meshes[0].position.z = 0.0
    underlay.queue_free()
    highlight.queue_free()
    for node in root.get_children():
        if node is MeshInstance3D:
            node.material_override = sky
    for i in range(3):
        await process_frame
        await RenderingServer.frame_post_draw
    var pixel := root.get_texture().get_image().get_pixel(center.x-x,center.y)
    var sky_color := Vector3i(pixel.r8,pixel.g8,pixel.b8)
    print("GS_SKY_RGB ", sky_color)
    quit(0 if (sky_color-Vector3i(144,123,64)).length() <= 3 else 1)

func water_layer(road: MeshInstance3D, shader_name: String) -> MeshInstance3D:
    var arrays := road.mesh.surface_get_arrays(0)
    var tint := Color8(128, 128, 128, 128)
    arrays[Mesh.ARRAY_COLOR] = PackedColorArray([tint, tint, tint, tint])
    arrays[Mesh.ARRAY_CUSTOM2] = PackedFloat32Array([0.0, 0.0, 0.0, 0.0])
    var mask_vertex := PackedFloat32Array([128.0 / 255.0, 128.0 / 255.0, 128.0 / 255.0, 0.0])
    if shader_name == "add_read_biased":
        mask_vertex[3] = 128.0 / 255.0
    var mask_vertices := PackedFloat32Array()
    for vertex in range(4):
        mask_vertices.append_array(mask_vertex)
    arrays[Mesh.ARRAY_CUSTOM3] = mask_vertices
    arrays[Mesh.ARRAY_CUSTOM0] = PackedFloat32Array([0,0,0, 1,0,0, 0,1,0, 1,1,0])
    var mesh := ArrayMesh.new()
    mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays, [], {},
        (Mesh.ARRAY_CUSTOM_RGB_FLOAT << Mesh.ARRAY_FORMAT_CUSTOM0_SHIFT) |
        (Mesh.ARRAY_CUSTOM_R_FLOAT << Mesh.ARRAY_FORMAT_CUSTOM2_SHIFT) |
        (Mesh.ARRAY_CUSTOM_RGBA_FLOAT << Mesh.ARRAY_FORMAT_CUSTOM3_SHIFT))
    var material := ShaderMaterial.new()
    material.shader = load("res://shader/road_water_%s.gdshader" % shader_name)
    var image := Image.create_from_data(1, 1, false, Image.FORMAT_RGBA8, PackedByteArray([32, 64, 96, 64]))
    var tex := ImageTexture.create_from_image(image)
    material.set_shader_parameter("gs_texture_linear", tex)
    material.set_shader_parameter("gs_texture_nearest", tex)
    material.set_shader_parameter("repeat_axes", Vector2i.ZERO)
    mesh.surface_set_material(0, material)
    var instance := MeshInstance3D.new()
    instance.mesh = mesh
    root.add_child(instance)
    return instance
