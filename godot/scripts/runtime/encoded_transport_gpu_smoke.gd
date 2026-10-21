extends Node3D
# TASK0071 port-side numeric transport/presentation fixture, not PS2 verification.
func _ready() -> void:
    run.call_deferred()

func run() -> void:
    if DisplayServer.get_name() == "headless":
        get_tree().quit(77)
        return
    var overlay := get_node("/root/DebugTools")
    overlay.hide()
    overlay.set_process(false)
    var raw := SubViewport.new()
    raw.size = Vector2i(16,16)
    raw.own_world_3d = true
    raw.use_hdr_2d = RenderingServer.get_current_rendering_method() != "gl_compatibility"
    raw.render_target_update_mode = SubViewport.UPDATE_ALWAYS
    add_child(raw)
    var camera := Camera3D.new()
    camera.projection = Camera3D.PROJECTION_ORTHOGONAL
    camera.size = 2
    camera.position.z = 3
    raw.add_child(camera)
    var text := FileAccess.get_file_as_string("res://../src/platform/shaders/car_base.gdshader")
    var begin := text.find("vec3 hp2_framebuffer_rgb")
    var end := text.find("float vu_bit_affine", begin)
    var helper := text.substr(begin, end-begin)
    var foreground := MeshInstance3D.new()
    foreground.mesh = QuadMesh.new()
    foreground.mesh.size = Vector2(4,4)
    var background := foreground.duplicate() as MeshInstance3D
    raw.add_child(background)
    raw.add_child(foreground)
    var display := SubViewport.new()
    display.size = raw.size
    display.disable_3d = true
    display.render_target_update_mode = SubViewport.UPDATE_ALWAYS
    add_child(display)
    var output := TextureRect.new()
    output.size = Vector2(display.size)
    output.texture = raw.get_texture()
    display.add_child(output)
    for sample in [
        {"source":33.0,"destination":0.0,"alpha":1.0,"add":false,"expected":33},
        {"source":128.0,"destination":64.0,"alpha":0.5,"add":false,"expected":96},
        {"source":80.0,"destination":20.0,"alpha":0.5,"add":true,"expected":60},
        {"source":200.0,"destination":200.0,"alpha":1.0,"add":true,"expected":255}]:
        for pair in [[background,float(sample.destination),false,1.0],[foreground,float(sample.source),bool(sample.add),float(sample.alpha)]]:
            var shader := Shader.new()
            shader.code = "shader_type spatial;render_mode unshaded,cull_disabled,depth_draw_never," + ("blend_add" if pair[2] else "blend_mix") + ";" + helper + "void fragment(){ALBEDO=hp2_framebuffer_rgb(vec3(" + str(pair[1]/255.0) + "));ALPHA=" + str(pair[3]) + ";}"
            var material := ShaderMaterial.new()
            material.shader = shader
            material.render_priority = 1 if pair[0] == foreground else 0
            pair[0].material_override = material
        for frame in 4:
            await get_tree().process_frame
            await RenderingServer.frame_post_draw
        var pixel := display.get_texture().get_image().get_pixel(8,8)
        var observed := roundi(pixel.r*255)
        if absi(observed-int(sample.expected)) > 1:
            push_error("Encoded transport/presentation mismatch: " + str(sample) + " actual=" + str(observed))
            get_tree().quit(1)
            return
        print("ENCODED_TRANSPORT_SAMPLE expected=",sample.expected," actual=",observed)
    print("ENCODED_TRANSPORT_GPU_OK: bounded copy/source-over/additive/resolve clamp; per-primitive HDR COLCLAMP remains outside this fixture")
    get_tree().quit(0)
