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
    quit(0 if (left-Vector3i(144,123,64)).length() <= 3 and
        (right-Vector3i(48,72,48)).length() <= 3 else 1)
