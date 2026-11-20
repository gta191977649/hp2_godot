extends SceneTree

# Asset-independent Forward+ regression: exercise the production glass shader
# against transparent smoke/additive flame draws on both sides of its priority.
var viewport: SubViewport
var glass: MeshInstance3D
var effect: MeshInstance3D
var material: ShaderMaterial
var failures := 0

func _initialize() -> void:
    call_deferred("run")

func sample() -> Color:
    await process_frame
    await RenderingServer.frame_post_draw
    return viewport.get_texture().get_image().get_pixel(64, 64)

func check(condition: bool, message: String) -> void:
    if not condition:
        failures += 1
        push_error(message)

func quad(z: float, draw: Material) -> MeshInstance3D:
    var node := MeshInstance3D.new()
    var arrays := []
    arrays.resize(Mesh.ARRAY_MAX)
    arrays[Mesh.ARRAY_VERTEX] = PackedVector3Array([
        Vector3(-2, -2, 0), Vector3(2, -2, 0), Vector3(2, 2, 0), Vector3(-2, 2, 0)])
    arrays[Mesh.ARRAY_TEX_UV] = PackedVector2Array([
        Vector2(0, 1), Vector2(1, 1), Vector2(1, 0), Vector2(0, 0)])
    arrays[Mesh.ARRAY_INDEX] = PackedInt32Array([0, 2, 1, 0, 3, 2])
    arrays[Mesh.ARRAY_CUSTOM0] = PackedFloat32Array([
        0, 0, 1, 32767, 0, 0, 1, 32767, 0, 0, 1, 32767, 0, 0, 1, 32767])
    var mesh := ArrayMesh.new()
    mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays, [], {},
        Mesh.ARRAY_CUSTOM_RGBA_FLOAT << Mesh.ARRAY_FORMAT_CUSTOM0_SHIFT)
    node.mesh = mesh
    node.material_override = draw
    node.position.z = z
    viewport.add_child(node)
    return node

func run() -> void:
    viewport = SubViewport.new()
    viewport.size = Vector2i(128, 128)
    viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
    viewport.own_world_3d = true
    root.add_child(viewport)
    var camera := Camera3D.new()
    camera.projection = Camera3D.PROJECTION_ORTHOGONAL
    camera.size = 3
    viewport.add_child(camera)
    camera.make_current()
    var background := StandardMaterial3D.new()
    background.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
    background.albedo_color = Color(0.35, 0.4, 0.45)
    quad(-6, background)
    material = ShaderMaterial.new()
    material.shader = load("res://shader/car_paint_glass.gdshader")
    material.render_priority = 1
    glass = quad(-3, material)

    # The former glass output is a useful reference for unchanged no-FX color.
    var legacy := ShaderMaterial.new()
    legacy.render_priority = 1
    var legacy_shader := Shader.new()
    var code := FileAccess.get_file_as_string("res://shader/car_paint.gdshaderinc")
    code = code.replace('#include "gs_byte_texture.gdshaderinc"',
        '#include "res://shader/gs_byte_texture.gdshaderinc"')
    code = code.replace("ALBEDO = max(ALBEDO - backdrop * transmission, vec3(0.0));", "")
    code = code.replace("ALPHA = 1.0 - transmission;", "ALPHA = 1.0;")
    legacy_shader.code = "shader_type spatial;\nrender_mode unshaded, cull_disabled, depth_draw_never, blend_premul_alpha;\n#define CAR_GLASS\n" + code
    legacy.shader = legacy_shader

    var fx := StandardMaterial3D.new()
    fx.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
    fx.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
    fx.depth_draw_mode = BaseMaterial3D.DEPTH_DRAW_DISABLED
    effect = quad(-4, fx)
    for alpha in [0, 16, 64, 128]:
        var image := Image.create_empty(1, 1, false, Image.FORMAT_RGBA8)
        image.fill(Color8(64, 80, 96, alpha))
        var texture := ImageTexture.create_from_image(image)
        material.set_shader_parameter("base_image", texture)
        legacy.set_shader_parameter("base_image", texture)
        effect.hide()
        glass.material_override = legacy
        var reference := await sample()
        glass.material_override = material
        var baseline := await sample()
        check(Vector3(reference.r, reference.g, reference.b).distance_to(
            Vector3(baseline.r, baseline.g, baseline.b)) < 0.015,
            "No-FX glass color changed at alpha %s" % alpha)
        for additive in [false, true]:
            fx.blend_mode = BaseMaterial3D.BLEND_MODE_ADD if additive else BaseMaterial3D.BLEND_MODE_MIX
            fx.albedo_color = Color(1, 0.5, 0.15, 0.65)
            for foreground in [false, true]:
                fx.render_priority = 2 if foreground else 0
                effect.position.z = -2 if foreground else -4
                effect.show()
                var shown := await sample()
                var delta := Vector3(shown.r - baseline.r, shown.g - baseline.g, shown.b - baseline.b).length()
                var should_show: bool = foreground or alpha < 128
                check(delta > 0.025 if should_show else delta < 0.015,
                    "FX coverage failed: alpha=%s add=%s front=%s delta=%s" % [alpha, additive, foreground, delta])
                print("GLASS_FX alpha=", alpha, " additive=", additive, " foreground=", foreground, " delta=", delta)
                # The previous output must demonstrate the original erasure.
                if not foreground and alpha == 16:
                    glass.material_override = legacy
                    var erased := await sample()
                    check(Vector3(erased.r - reference.r, erased.g - reference.g,
                        erased.b - reference.b).length() < 0.015, "Legacy fixture did not reproduce erasure")
                    glass.material_override = material
                effect.hide()
    print("GLASS_EFFECT_ORDER ", "PASS" if failures == 0 else "FAIL", " failures=", failures)
    quit(0 if failures == 0 else 1)
