extends SceneTree

var deadline := Time.get_ticks_msec() + 45000
var checking := false

func _initialize() -> void:
    call_deferred("change_scene_to_file", "res://native_game.tscn")

func _process(_delta: float) -> bool:
    if Time.get_ticks_msec() > deadline:
        push_error("Postprocessing smoke timed out")
        quit(1)
    if checking:
        return false
    var race := root.get_node_or_null("NativeGame/RacePresentation")
    if race and race.telemetry().get("race_state") == 3 and race.telemetry().get("step", 0) > 70:
        checking = true
        _check.call_deferred(race)
    return false

func _check(race: Node) -> void:
    race.set_process(false)
    RenderingServer.viewport_set_disable_2d(root.get_viewport_rid(), true)
    var camera := root.get_camera_3d()
    var effect: CompositorEffect = camera.compositor.compositor_effects[0]
    var native := await _frame()
    effect.enabled = false
    var unprocessed := await _frame()
    print("RACE_POST_PIXELS authored_weather=", _difference(native, unprocessed))
    var path := OS.get_environment("HP2_PHOTO_DIR")
    for mode in [1, 2, 4, 3]:
        camera.position += Vector3.RIGHT * 0.35
        effect.enabled = false
        var before := await _frame()
        effect.call("set_test_mode", mode)
        await _frame()
        if mode == 3:
            camera.position += Vector3.RIGHT * 1.0
        var after := await _frame()
        if mode == 3:
            effect.enabled = false
            before = await _frame()
        var changed := _difference(before, after)
        print("RACE_POST_PIXELS mode=", mode, " changed=", changed)
        if not path.is_empty():
            before.save_png(path.path_join("post-%s-before.png" % mode))
            after.save_png(path.path_join("post-%s-after.png" % mode))
        if changed < 20:
            push_error("Missing post effect %s" % mode)
            quit(1)
            return
    quit(0)

func _frame() -> Image:
    await process_frame
    await RenderingServer.frame_post_draw
    return root.get_texture().get_image()

func _difference(a: Image, b: Image) -> int:
    var count := 0
    for y in range(0, a.get_height(), 3):
        for x in range(0, a.get_width(), 3):
            var d := a.get_pixel(x,y) - b.get_pixel(x,y)
            if absf(d.r) + absf(d.g) + absf(d.b) > 0.045:
                count += 1
    return count
