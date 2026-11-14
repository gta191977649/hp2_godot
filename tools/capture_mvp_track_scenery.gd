extends SceneTree

var started := Time.get_ticks_msec()
var captured := false

func _initialize() -> void:
    call_deferred("change_scene_to_file", "res://native_game.tscn")

func _process(_delta: float) -> bool:
    if Time.get_ticks_msec() - started > 50000:
        quit(1)
    if captured:
        return false
    var race := root.get_node_or_null("NativeGame/RacePresentation")
    if race and race.telemetry().get("race_state") == 3 and race.telemetry().get("step", 0) >= 35:
        captured = true
        _capture.call_deferred(race)
    return false

func _capture(race: Node) -> void:
    var state: Dictionary = race.telemetry()
    var track: int = state.track
    var route: PackedVector3Array = race.photo_route()
    if route.size() < 20:
        push_error("No scenery route for track %s" % track)
        quit(1)
        return
    race.set_process(false)
    RenderingServer.viewport_set_disable_2d(root.get_viewport_rid(), true)
    var scene := race.get_node("RaceScene")
    for node in scene.get_children():
        if node is SubViewport and node.name == &"RearViewTarget":
            node.render_target_update_mode = SubViewport.UPDATE_DISABLED
    var camera := root.get_camera_3d()
    var output := OS.get_environment("HP2_PHOTO_DIR")
    var takes := [
        [8 + (track % 6) * 3, 24.0, 11.0, 54.0],
        [25 + (track % 6) * 2, -30.0, 18.0, 57.0],
        [49 + (track % 6), 44.0, 29.0, 60.0],
        [66 + (track % 6) * 2, -46.0, 35.0, 59.0],
        [84 + (track % 6), 27.0, 16.0, 54.0],
    ]
    var long_lens := OS.get_environment("HP2_PHOTO_LENS") == "long"
    if long_lens:
        # Route sample, lateral distance, elevation, vertical FOV, setback.
        # Alternate along-road compression with views across bends.
        takes = [
            [8 + (track % 6) * 3, 18.0, 10.0, 19.0, 70.0],
            [15 + (track % 6), -42.0, 18.0, 23.0, 85.0],
            [25 + (track % 6) * 2, -32.0, 14.0, 21.0, 90.0],
            [34 + (track % 6), 38.0, 26.0, 25.0, 102.0],
            [49 + (track % 6), 48.0, 31.0, 24.0, 78.0],
            [56 + (track % 6), -63.0, 27.0, 22.0, 70.0],
            [66 + (track % 6) * 2, -49.0, 35.0, 23.0, 100.0],
            [77 + (track % 6), 53.0, 33.0, 24.0, 78.0],
            [84 + (track % 6), 28.0, 19.0, 20.0, 71.0],
        ]
    var surface := OS.get_environment("HP2_PHOTO_SURFACE")
    if OS.get_environment("HP2_PHOTO_WATER") == "1":
        surface = "water"
    if surface in ["water", "shaft"]:
        var metadata := "hp2_road_water" if surface == "water" else "hp2_light_shaft"
        for node in scene.get_children():
            if not node is MeshInstance3D or not node.mesh or not node.mesh.get_meta(metadata, false):
                continue
            var bounds: AABB = node.mesh.get_aabb()
            var center: Vector3 = node.global_transform * bounds.get_center()
            var radius := clampf(bounds.size.length() * 0.7, 3.0, 18.0)
            camera.look_at_from_position(center + Vector3(0.8, 0.55, 0.8) * radius, center)
            await process_frame
            await RenderingServer.frame_post_draw
            var visible := root.get_texture().get_image()
            node.hide()
            await process_frame
            await RenderingServer.frame_post_draw
            var hidden := root.get_texture().get_image()
            node.show()
            var difference := _pixel_difference(visible, hidden)
            if difference < 12:
                continue
            visible.save_png(output.path_join("%s-%02d.png" % [surface, track]))
            hidden.save_png(output.path_join("%s-%02d-hidden.png" % [surface, track]))
            print("SCENERY_SURFACE ", surface, " track=", track, " changed=", difference)
            quit(0)
            return
        push_error("Could not isolate %s for track %s" % [surface, track])
        quit(1)
        return
    var best: Image
    var best_score := -INF
    var selection := -1
    var requested := OS.get_environment("HP2_PHOTO_TAKE")
    for take in takes:
        var index: int = take[0]
        if not requested.is_empty() and index != requested.to_int():
            continue
        if route[index] == Vector3.ZERO:
            continue
        var road := route[index]
        var advance := Vector3.ZERO
        for next in range(index+1, mini(index+5, route.size())):
            if route[next] != Vector3.ZERO:
                advance = route[next] - road
                if Vector2(advance.x, advance.z).length_squared() >= 2.0:
                    break
        if Vector2(advance.x, advance.z).length_squared() < 2.0:
            continue
        var tangent := Vector3(advance.x, 0, advance.z).normalized()
        var side := tangent.cross(Vector3.UP)
        camera.fov = take[3]
        var eye := road - tangent * (float(take[4]) if long_lens else 23.0) +            + side * float(take[1]) + Vector3.UP * float(take[2])
        var target := road + advance * (clampf(advance.length() * 0.7, 24.0, 80.0) / advance.length()) +            + Vector3.UP * (5.5 if long_lens else 4.0)
        if long_lens:
            # Place the bend to one side, leaving room in its direction.
            target += side * (8.0 if take[1] > 0.0 else -8.0)
        camera.look_at_from_position(eye, target)
        await process_frame
        await RenderingServer.frame_post_draw
        await process_frame
        await RenderingServer.frame_post_draw
        var image := root.get_texture().get_image()
        var score := _score(image)
        if not output.is_empty() and OS.get_environment("HP2_PHOTO_TAKES") == "1":
            image.save_png(output.path_join("%02d-take-%02d.png" % [track, index]))
        if score > best_score:
            best_score = score
            best = image
            selection = index
    if best == null or best.save_png(output.path_join("track-%02d.png" % track)) != OK:
        push_error("Could not save scenery for track %s" % track)
        quit(1)
        return
    print("SCENERY_PHOTO ", track, " take=", selection, " resolution=", best.get_size(), " score=", best_score)
    quit(0)

func _pixel_difference(first: Image, second: Image) -> int:
    var count := 0
    for y in range(0, first.get_height(), 4):
        for x in range(0, first.get_width(), 4):
            var d := first.get_pixel(x, y) - second.get_pixel(x, y)
            if absf(d.r) + absf(d.g) + absf(d.b) > 0.06:
                count += 1
    return count

func _score(image: Image) -> float:
    var size := image.get_size()
    var brightness := 0.0
    var variance := 0.0
    var chroma := 0.0
    var clipping := 0.0
    var count := 0.0
    for y in range(size.y / 10, size.y * 9 / 10, 20):
        for x in range(size.x / 10, size.x * 9 / 10, 20):
            var c := image.get_pixel(x, y)
            var value := c.get_luminance()
            brightness += value
            variance += value * value
            chroma += maxf(c.r, maxf(c.g, c.b)) - minf(c.r, minf(c.g, c.b))
            if value < 0.028 or value > 0.98:
                clipping += 1.0
            count += 1.0
    var mean := brightness / count
    return 1.2 * sqrt(maxf(0.0, variance / count - mean * mean)) + chroma / count +        - absf(mean - 0.43) - clipping / count * 1.25
