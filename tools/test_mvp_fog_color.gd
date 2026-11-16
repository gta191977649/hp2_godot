extends SceneTree

const FOREGROUND := Color8(160, 55, 95)
const WEATHER := Color8(70, 105, 155)

func _initialize() -> void:
    verify.call_deferred()

func linear_channel(value: float) -> float:
    return value / 12.92 if value <= 0.04045 else pow((value + 0.055) / 1.055, 2.4)

func verify() -> void:
    var viewport := SubViewport.new()
    viewport.size = Vector2i(3, 1)
    viewport.disable_3d = true
    viewport.render_target_update_mode = SubViewport.UPDATE_ALWAYS
    root.add_child(viewport)
    var rect := ColorRect.new()
    rect.size = Vector2(3, 1)
    viewport.add_child(rect)
    var draw := ShaderMaterial.new()
    draw.shader = load("res://shader/race_post_fog.gdshader")
    rect.material = draw
    var color := Image.create_empty(3, 1, false, Image.FORMAT_RGBAF)
    color.fill(Color(linear_channel(FOREGROUND.r), linear_channel(FOREGROUND.g),
        linear_channel(FOREGROUND.b), 1.0))
    var depth := Image.create_empty(3, 1, false, Image.FORMAT_RF)
    for x in range(3):
        # Slots 255 (near), 64 (middle distance), and 0 (clear / far).
        depth.set_pixel(x, 0, Color([1.0, 64.5 / 2048.0, 0.0][x], 0, 0))
    draw.set_shader_parameter("scene_color", ImageTexture.create_from_image(color))
    draw.set_shader_parameter("scene_depth", ImageTexture.create_from_image(depth))
    draw.set_shader_parameter("fog", Vector4(WEATHER.r, WEATHER.g, WEATHER.b, 0.5))
    for mode in [0, 1]:
        var palette := Image.create_empty(256, 1, false, Image.FORMAT_RGBAF)
        for slot in range(256):
            var density := clampf((255.0 - slot - 125.0) / 130.0, 0.0, 1.0)
            palette.set_pixel(slot, 0, Color(WEATHER, floorf(maxf(0.01, density) * 64.0) / 128.0)
                if mode == 0 else Color(0, 0, 0, floorf((1.0 - density) * 64.0) / 128.0))
        draw.set_shader_parameter("fog_palette", ImageTexture.create_from_image(palette))
        draw.set_shader_parameter("fog_params", Vector4(125, 1, 524288, mode))
        for use_palette in [false, true]:
            draw.set_shader_parameter("use_fog_palette", use_palette)
            await process_frame
            await RenderingServer.frame_post_draw
            var image := viewport.get_texture().get_image()
            var expected := [FOREGROUND, Color8(137, 67, 110), Color8(115, 80, 125)] if mode == 0 else [
                Color8(80, 27, 47), Color8(38, 13, 23), Color8(0, 0, 0)]
            for x in range(3):
                var got := image.get_pixel(x, 0)
                for channel in range(3):
                    if absf(got[channel] - expected[x][channel]) > 1.1 / 255.0:
                        push_error("Fog mode %d palette=%s pixel %d: got %s, expected %s" % [
                            mode, use_palette, x, got, expected[x]])
                        quit(1)
                        return
    print("RACE_FOG_COLOR_PASS foreground unchanged, distance tint and GS darken match")
    quit()
