extends SceneTree

func _initialize() -> void:
    run.call_deferred()

func run() -> void:
    change_scene_to_file("res://native_game.tscn")
    var deadline := Time.get_ticks_msec() + 20000
    var race: Node
    while Time.get_ticks_msec() < deadline:
        await process_frame
        race = root.get_node_or_null("NativeGame/RacePresentation")
        if race and race.telemetry().get("race_state") == 3:
            break
    if not race or not race.race_scene():
        fail("race scene unavailable")
        return
    race.set_process(false)
    var scene: Node3D = race.race_scene()
    var viewport := scene.get_viewport()
    var camera := viewport.get_camera_3d()
    var route: PackedVector3Array = race.photo_route()
    var zones := {}
    var found := false
    for i in range(1, route.size() - 1, 4):
        var eye := route[i] + Vector3.UP * 8.0
        var direction := route[i + 1] - route[i - 1]
        direction.y = 0
        if direction.length_squared() < 0.01:
            continue
        var heading := direction.normalized()
        camera.look_at_from_position(eye, eye + heading - Vector3.UP * 0.18)
        race.test_update_scenery_camera(true)
        var front: Dictionary = race.telemetry()
        if front.scenery_fallback:
            continue
        zones[front.scenery_sections] = true
        if found or front.scenery_tree_nodes < 1 or front.scenery_candidates < 1:
            continue
        var before := camera.global_transform
        var front_image := await capture(viewport)
        camera.look_at_from_position(eye, eye - heading - Vector3.UP * 0.18)
        race.test_update_scenery_camera(true)
        var back: Dictionary = race.telemetry()
        var back_image := await capture(viewport)
        if front.scenery_candidates == back.scenery_candidates:
            continue
        if front_image.get_data() == back_image.get_data():
            fail("camera rotation produced identical frames")
            return
        camera.global_transform = before
        race.test_update_scenery_camera(false)
        var disabled: Dictionary = race.telemetry()
        if not disabled.scenery_fallback or disabled.scenery_sections <= front.scenery_sections:
            fail("disabling zone culling did not restore section coverage")
            return
        await capture(viewport)
        race.test_update_scenery_camera(true)
        if race.telemetry().scenery_sections != front.scenery_sections:
            fail("re-enabling zone culling did not restore the original selection")
            return
        var folder := OS.get_environment("HP2_GRID_CAPTURE_DIR")
        if not folder.is_empty():
            front_image.save_png(folder.path_join("road-forward.png"))
            back_image.save_png(folder.path_join("road-backward.png"))
        print("SCENERY_ROTATION front=", front.scenery_candidates, " back=", back.scenery_candidates,
            " zone_sections=", front.scenery_sections, " disabled_sections=", disabled.scenery_sections)
        found = true
    camera.position = Vector3(10000, 1000, 10000)
    race.test_update_scenery_camera(true)
    if not race.telemetry().scenery_fallback or not found or zones.size() < 2:
        fail("camera fixtures did not cover rotation, zone changes and the zone-free fallback")
        return
    print("SCENERY_VISIBILITY_PASS sections=", zones.keys(), " fallback=", race.telemetry().scenery_sections)
    quit(0)

func capture(viewport: Viewport) -> Image:
    await process_frame
    await RenderingServer.frame_post_draw
    return viewport.get_texture().get_image()

func fail(reason: String) -> void:
    push_error(reason)
    quit(1)
