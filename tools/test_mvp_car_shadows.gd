extends SceneTree

func _initialize() -> void:
    call_deferred("run")

func run() -> void:
    change_scene_to_file("res://native_game.tscn")
    var deadline := Time.get_ticks_msec() + 20000
    while Time.get_ticks_msec() < deadline:
        await process_frame
        var race := root.get_node_or_null("NativeGame/RacePresentation")
        if not race or race.telemetry().get("race_state", -1) != 3:
            continue
        var scene: Node3D = race.race_scene()
        var effects := scene.get_node_or_null("RaceEffects")
        var patches: Array[MeshInstance3D] = []
        var priorities := {}
        for node in effects.get_children():
            if node is MeshInstance3D and node.visible and String(node.name).contains("CarShadow"):
                if node.layers == 2:
                    patches.push_back(node)
                    priorities[node.material_override.render_priority] = true
        if not priorities.has(4):
            continue
        race.set_process(false)
        var viewport := scene.get_viewport()
        var shown := await capture(viewport)
        for node in patches:
            node.hide()
        var hidden := await capture(viewport)
        var darker := 0
        for y in range(0, shown.get_height(), 2):
            for x in range(0, shown.get_width(), 2):
                var difference := hidden.get_pixel(x, y) - shown.get_pixel(x, y)
                if difference.r + difference.g + difference.b > 0.045:
                    darker += 1
        print("CAR_SHADOW_PIXELS batches=", patches.size(), " priorities=", priorities, " darker=", darker)
        quit(0 if darker > 25 else 1)
        return
    push_error("Projected car shadow did not publish an authored ramp")
    quit(1)

func capture(viewport: Viewport) -> Image:
    await process_frame
    await RenderingServer.frame_post_draw
    return viewport.get_texture().get_image()
