extends SceneTree

func _initialize() -> void:
    call_deferred("run")

func run() -> void:
    change_scene_to_file("res://native_game.tscn")
    var race: Node
    var flare: MeshInstance3D
    var deadline := Time.get_ticks_msec() + 30000
    while Time.get_ticks_msec() < deadline:
        await process_frame
        race = root.get_node_or_null("NativeGame/RacePresentation")
        if race:
            flare = race.get_node_or_null("RaceScene/RaceEffects/SunFlare0")
            if flare and flare.visible:
                break
    if not flare or not flare.visible:
        push_error("Authored Track 12 sun unavailable")
        quit(1)
        return
    race.set_process(false)
    for node in race.get_children():
        if node is CanvasItem:
            node.hide()
    var camera := root.get_camera_3d()
    var scene := flare.get_parent().get_parent()
    var sky := scene.get_node_or_null("TrackSky") as MeshInstance3D
    if not sky:
        push_error("Authored Track 12 sky unavailable")
        quit(1)
        return
    for node in scene.get_children():
        if node is Node3D and node != camera and node != flare.get_parent():
            node.hide()
    for node in flare.get_parent().get_children():
        if node is Node3D and node != flare:
            node.hide()
    var mat := flare.material_override as ShaderMaterial
    # An asymmetric anchor exposes both mirrored drawing and mirrored depth lookup.
    var source_image := Image.create_empty(2, 2, false, Image.FORMAT_RGBA8)
    source_image.fill(Color8(128, 128, 128, 128))
    mat.set_shader_parameter("flare_0", ImageTexture.create_from_image(source_image))
    var uv := Vector2(0.42, 0.26)
    var rects := PackedVector4Array()
    var colors := PackedVector4Array()
    var states := PackedVector4Array()
    rects.resize(12)
    colors.resize(12)
    states.resize(12)
    rects[0] = Vector4(uv.x, uv.y, 0.12, 0.13)
    colors[0] = Vector4(128, 128, 128, 128)
    states[0] = Vector4(1, 0, 0, 20) # Add and sample sky.
    mat.set_shader_parameter("layer_count", 1)
    mat.set_shader_parameter("disc_layer", -1)
    mat.set_shader_parameter("layer_rect", rects)
    mat.set_shader_parameter("layer_color", colors)
    mat.set_shader_parameter("layer_state", states)
    mat.set_shader_parameter("sun_uv", uv)
    mat.set_shader_parameter("bounds_uv", Vector4(0.30, 0.13, 0.54, 0.39))
    mat.set_shader_parameter("source_scale", root.size.y * 1.0714286566 / 448.0)
    mat.set_shader_parameter("sample_sky", true)
    var wall := MeshInstance3D.new()
    var cube := BoxMesh.new()
    cube.size = Vector3(1.3,1.3,0.1)
    wall.mesh = cube
    var black := StandardMaterial3D.new()
    black.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
    black.albedo_color = Color.BLACK
    wall.material_override = black
    scene.add_child(wall)
    wall.global_basis = camera.global_basis
    var size := Vector2(root.size)
    var center := Vector2i(uv*size)
    var mirror := Vector2i(Vector2(uv.x, 1.0 - uv.y) * size)
    wall.hide()
    var open := await pixels(center,mirror)
    sky.show()
    var with_sky := await pixels(center,mirror)
    sky.hide()
    wall.global_position = camera.project_position(Vector2(mirror),5.0)
    wall.show()
    var wrong_side := await pixels(center,mirror)
    wall.global_position = camera.project_position(Vector2(center),5.0)
    var blocked := await pixels(center,mirror)
    mat.set_shader_parameter("sample_sky",false)
    states[0].w = 12 # Add and test disc depth.
    mat.set_shader_parameter("layer_state", states)
    var disc := await pixels(center,mirror)
    # Depth-free alpha foliage is composited over the sun by Godot's render order.
    black.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
    black.depth_draw_mode = BaseMaterial3D.DEPTH_DRAW_DISABLED
    black.albedo_color = Color(0, 0, 0, 0.8)
    var foliage := await pixels(center, mirror)
    print("SUN_ALIGNMENT open=",open," sky=",with_sky," mirrored_blocker=",wrong_side,
        " blocker=",blocked," disc=",disc," alpha_foliage=",foliage)
    # Check ordered blend on an opaque background. GS bytes: 64 + 128 = 192,
    # then 192 + floor((0 - 192) * 64 / 128) = 96.
    black.transparency = BaseMaterial3D.TRANSPARENCY_DISABLED
    black.depth_draw_mode = BaseMaterial3D.DEPTH_DRAW_OPAQUE_ONLY
    black.albedo_color = Color8(64, 64, 64)
    rects[1] = rects[0]
    colors[1] = colors[0]
    colors[0] = Vector4(0, 0, 0, 64)
    states[0].w = 0
    states[1] = Vector4(1, 0, 0, 4)
    mat.set_shader_parameter("layer_count", 2)
    mat.set_shader_parameter("layer_rect", rects)
    mat.set_shader_parameter("layer_color", colors)
    mat.set_shader_parameter("layer_state", states)
    var composed := await pixels(center, mirror)
    # On this texture a raw 192 alpha must survive until GS modulation.
    source_image.fill(Color8(128, 128, 128, 192))
    mat.set_shader_parameter("flare_0", ImageTexture.create_from_image(source_image))
    colors[0] = Vector4(128, 128, 128, 64)
    states[0].w = 4
    mat.set_shader_parameter("layer_state", states)
    mat.set_shader_parameter("layer_count", 1)
    mat.set_shader_parameter("layer_color", colors)
    var extended_alpha := await pixels(center, mirror)
    print("SUN_GS ordered=", composed, " expected=96, extended_alpha=", extended_alpha, " expected=160")
    quit(0 if absf(open.x * 255.0 - 128.0) <= 2.0 and open.y < 0.1
        and with_sky.x > open.x and wrong_side.x > 0.4 and blocked.x < 0.1
        and disc.x < 0.1 and foliage.x > 0.05 and foliage.x < open.x
        and absf(composed.x * 255.0 - 96.0) <= 2.0
        and absf(extended_alpha.x * 255.0 - 160.0) <= 2.0 else 1)

func pixels(center: Vector2i, mirror: Vector2i) -> Vector2:
    await process_frame
    await RenderingServer.frame_post_draw
    var image := root.get_texture().get_image()
    return Vector2(image.get_pixelv(center).r,image.get_pixelv(mirror).r)
