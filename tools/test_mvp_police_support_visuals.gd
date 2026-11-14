extends SceneTree

const MODELS := ["ROADBLOCK_BARREL", "SPIKEBELT_00", "TRACK_HELICOPTER", "SMOKEBOMB"]
var seen := {}
var seen_fx := {}
var deadline: int
var next_input := 0
var start_support := Vector2i(-1, -1)
var race: Node
var capturing := false

func _initialize() -> void:
    deadline = Time.get_ticks_msec() + 115000
    call_deferred("change_scene_to_file", "res://native_game.tscn")

func _process(_delta: float) -> bool:
    if Time.get_ticks_msec() >= deadline:
        push_error("Police support visual coverage timed out: %s %s" % [seen, seen_fx])
        quit(1)
        return true
    if capturing:
        return false
    race = root.get_node_or_null("NativeGame/RacePresentation")
    if not race:
        return false
    var sample: Dictionary = race.telemetry()
    if sample.get("race_state") != 3 or not sample.get("police_hud", false):
        return false
    if sample.get("debug_console_open", false):
        if Time.get_ticks_msec() >= next_input:
            _tap(KEY_QUOTELEFT)
            next_input = Time.get_ticks_msec() + 500
        return false
    if start_support.x < 0:
        start_support = Vector2i(sample["police_roadblocks"], sample["police_helicopters"])
        _key(KEY_W, true)

    var objects: Node = race.race_scene().get_node_or_null("WorldObjects") if race.race_scene() else null
    var effects: Node = race.race_scene().get_node_or_null("RaceEffects") if race.race_scene() else null
    if effects:
        for child in effects.get_children():
            if child is GPUParticles3D and child.has_meta("source_effect_group"):
                var group: int = child.get_meta("source_effect_group")
                for label in ["SMOKEBOMBEXPLD", "XEM_HELI_DUST", "ROCKETEXPLD"]:
                    if group == _hash(label) and not seen_fx.has(label):
                        seen_fx[label] = true
                        print("SUPPORT_GPU ", label)
    if objects:
        for child in objects.get_children():
            var mesh := child as MeshInstance3D
            if not mesh or not mesh.visible:
                continue
            var label := String(mesh.name).rsplit("_", true, 1)[0]
            if label in MODELS and not seen.has(label):
                if not mesh.mesh or mesh.mesh.get_surface_count() == 0:
                    push_error("Missing authored geometry for " + label)
                    quit(1)
                    return true
                _check_pixels.call_deferred(mesh, label)
                capturing = true
                return false

    if seen.size() == MODELS.size() and seen_fx.has("SMOKEBOMBEXPLD"):
        print("SUPPORT_VISUAL_PASS meshes=", seen, " effects=", seen_fx)
        quit(0)
        return true
    if Time.get_ticks_msec() >= next_input:
        if not sample["police_target_active"]:
            _tap(KEY_H)
        elif sample["police_roadblocks"] == start_support.x:
            _tap(KEY_R)
        elif sample["police_helicopters"] == start_support.y:
            _tap(KEY_T)
        next_input = Time.get_ticks_msec() + 1000
    return false

func _check_pixels(mesh: MeshInstance3D, label: String) -> void:
    race.set_process(false)
    var source_camera: Camera3D = race.race_scene().get_viewport().get_camera_3d()
    var camera := Camera3D.new()
    camera.far = source_camera.far
    race.race_scene().add_child(camera)
    camera.make_current()
    var bounds := mesh.get_aabb()
    var center := mesh.to_global(bounds.get_center())
    var distance := maxf(0.8, bounds.size.length() * 1.35)
    camera.look_at_from_position(center + Vector3(0.45, 0.55, 1.0) * distance, center)
    await process_frame
    await RenderingServer.frame_post_draw
    var visible_frame := root.get_texture().get_image()
    mesh.hide()
    await process_frame
    await RenderingServer.frame_post_draw
    var hidden_frame := root.get_texture().get_image()
    mesh.show()
    source_camera.make_current()
    camera.queue_free()
    race.set_process(true)
    var changed := 0
    var size := Vector2(visible_frame.get_size())
    var begin := Vector2i(size * 0.4)
    var end := Vector2i(size * 0.6)
    for y in range(begin.y, end.y):
        for x in range(begin.x, end.x):
            var d := visible_frame.get_pixel(x, y) - hidden_frame.get_pixel(x, y)
            if absf(d.r) + absf(d.g) + absf(d.b) > 0.12:
                changed += 1
    if changed > 24:
        seen[label] = changed
        print("SUPPORT_MESH_PIXELS ", label, " ", changed, " surfaces=", mesh.mesh.get_surface_count())
        var output := OS.get_environment("HP2_SUPPORT_CAPTURE_DIR")
        if not output.is_empty():
            visible_frame.save_png(output.path_join(label + ".png"))
    capturing = false

func _hash(text: String) -> int:
    var value := 0xffffffff
    for byte in text.to_ascii_buffer():
        value = (value * 33 + byte) & 0xffffffff
    return value

func _tap(key: Key) -> void:
    _key(key, true)
    await create_timer(0.15).timeout
    _key(key, false)

func _key(key: Key, down: bool) -> void:
    var event := InputEventKey.new()
    event.physical_keycode = key
    event.keycode = key
    event.pressed = down
    Input.parse_input_event(event)
