extends RefCounted
# TASK0071 host-only CarViewer presentation. Keep GS numeric RGB through blending,
# then let the LDR canvas clamp it without an additional sRGB transfer.
var target: SubViewport
var camera: Camera3D
var source: Camera3D
var canvas: CanvasLayer
var output: TextureRect
var source_mask := 0

func attach(owner: Node3D, source_camera: Camera3D) -> void:
    if RenderingServer.get_current_rendering_method() == "gl_compatibility" or DisplayServer.get_name() == "headless":
        return
    source = source_camera
    source_mask = source.cull_mask
    target = SubViewport.new()
    target.name = "CarEncodedColour"
    target.use_hdr_2d = true
    target.canvas_cull_mask = 0
    target.world_3d = source.get_world_3d()
    target.render_target_update_mode = SubViewport.UPDATE_ALWAYS
    owner.add_child(target)
    camera = Camera3D.new()
    target.add_child(camera)
    camera.current = true
    canvas = CanvasLayer.new()
    canvas.layer = 1
    owner.add_child(canvas)
    output = TextureRect.new()
    output.mouse_filter = Control.MOUSE_FILTER_IGNORE
    output.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
    output.texture = target.get_texture()
    var shader := Shader.new()
    shader.code = "shader_type canvas_item;render_mode unshaded,blend_disabled;void fragment(){COLOR=vec4(clamp(texture(TEXTURE,UV).rgb,vec3(0),vec3(1)),1.0);}"
    var material := ShaderMaterial.new()
    material.shader = shader
    output.material = material
    canvas.add_child(output)
    source.cull_mask = 0
    update()

func update() -> void:
    if not is_instance_valid(target):
        return
    var size := Vector2i(source.get_viewport().get_visible_rect().size)
    if target.size != size:
        target.size = size
        output.size = Vector2(size)
    camera.projection = source.projection
    camera.keep_aspect = source.keep_aspect
    camera.fov = source.fov
    camera.size = source.size
    camera.near = source.near
    camera.far = source.far
    camera.h_offset = source.h_offset
    camera.v_offset = source.v_offset
    camera.frustum_offset = source.frustum_offset
    camera.environment = source.environment
    camera.cull_mask = source_mask
    camera.global_transform = source.global_transform
