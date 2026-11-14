extends SceneTree

var track := 12
var route := false
var firetruck := false
var deadline := 0
var race: Node
var started := -1
var nodes := {}
var targets: Array[GPUParticles3D] = []
var checking := false
var samples: Array[float] = []
var previous := 0
var births_started := false
var warmed := 0

func _initialize() -> void:
    for arg in OS.get_cmdline_user_args():
        if arg.begins_with("--race-track="):
            track = arg.trim_prefix("--race-track=").to_int()
    firetruck = OS.get_environment("HP2_MVP_TEST_TRACK_EFFECTS") == "firetruck"
    route = firetruck or OS.get_environment("HP2_MVP_TEST_TRACK_EFFECTS") == "1"
    deadline = Time.get_ticks_msec() + 45000
    call_deferred("change_scene_to_file", "res://native_game.tscn")

func _process(_delta: float) -> bool:
    if Time.get_ticks_msec() > deadline:
        push_error("Particle smoke timed out")
        quit(1)
        return true
    if checking:
        return false
    race = root.get_node_or_null("NativeGame/RacePresentation")
    if not race:
        return false
    var state: Dictionary = race.telemetry()
    if state.get("debug_console_open", false):
        var event := InputEventKey.new()
        event.physical_keycode = KEY_QUOTELEFT
        event.keycode = KEY_QUOTELEFT
        event.pressed = true
        Input.parse_input_event(event)
        event = event.duplicate()
        event.pressed = false
        Input.parse_input_event(event)
        return false
    var effects: Node = race.race_scene().get_node_or_null("RaceEffects") if race.race_scene() else null
    if state.get("race_state", -1) != 3 or not effects:
        return false
    var step: int = state.step
    if started < 0:
        started = step
        if not route:
            Input.action_press("hp2_accelerate")
        for node in effects.get_children():
            if node is GPUParticles3D:
                nodes[node.get_instance_id()] = true
    targets.clear()
    for node in effects.get_children():
        if not node is GPUParticles3D:
            continue
        if not nodes.has(node.get_instance_id()):
            push_error("A particle system was allocated during the race")
            quit(1)
            return true
        if not node.visible:
            continue
        var definition: int = node.get_meta("source_effect_key", 0)
        if route:
            if firetruck and definition in [_hash("XE_FireTruck_WaterA"), _hash("XE_FireTruck_WaterB"),
                _hash("XE_FireTruck_WaterC"), _hash("XE_FireTruck_WaterD")]:
                if node.material_override.render_priority != 7:
                    push_error("Fire truck water lost its authored alpha priority")
                    quit(1)
                    return true
                targets.push_back(node)
            if not firetruck and track == 12 and definition in [_hash("XE_FFire_FlmL_01"), _hash("XE_FFire_FlmXL_01"),
                _hash("XE_FFire_SmkBLK_01"), _hash("XE_FFire_SmkWHT_01"), _hash("XE_FFire_Sparks_01")]:
                if (definition in [_hash("XE_FFire_SmkBLK_01"), _hash("XE_FFire_SmkWHT_01")]
                    and node.material_override.render_priority != 6):
                    push_error("Forest smoke lost its authored alpha priority")
                    quit(1)
                    return true
                targets.push_back(node)
            if track == 21 and definition in [_hash("XE_Sandstorm_XL_01"), _hash("XE_SandDebris_XL_01")]:
                targets.push_back(node)
            if track == 61 and definition in [_hash("XE_WaterFall_Cloud_01"), _hash("XE_WaterFall_Mist_01")]:
                targets.push_back(node)
        elif births_started and node.get_meta("source_effect_group", 0) == _hash("CARFX_NITRO"):
            targets.push_back(node)
        elif not births_started and node.name == "TailpipeExhaust":
            targets.push_back(node)

    if route or births_started:
        if targets.size() >= (2 if route else 1):
            if warmed == 0:
                warmed = step
            if step >= warmed + (95 if route else 4):
                var now := Time.get_ticks_usec()
                if previous > 0:
                    samples.push_back((now - previous) / 1000.0)
                previous = now
                if samples.size() >= (100 if route else 8):
                    checking = true
                    _check_pixels.call_deferred()
    elif step >= started + 100 and not targets.is_empty():
        checking = true
        _check_pixels.call_deferred()
    return false

func _check_pixels() -> void:
    race.set_process(false)
    var canvas_layers: Array[CanvasItem] = []
    if route:
        for node in race.get_children():
            if node is CanvasItem and node.visible:
                canvas_layers.push_back(node)
                node.hide()
    await process_frame
    await RenderingServer.frame_post_draw
    var shown := root.get_texture().get_image()
    for target in targets:
        target.hide()
    await process_frame
    await RenderingServer.frame_post_draw
    var hidden := root.get_texture().get_image()
    for target in targets:
        target.show()
    for layer in canvas_layers:
        layer.show()
    var count := 0
    var size := shown.get_size()
    for y in range(0, size.y, 2):
        for x in range(0, size.x, 2):
            var d := shown.get_pixel(x, y) - hidden.get_pixel(x, y)
            if absf(d.r) + absf(d.g) + absf(d.b) > 0.045:
                count += 1
    var label := "firetruck" if firetruck else "track" if route else "nitro" if births_started else "exhaust"
    if not samples.is_empty():
        samples.sort()
        print("PARTICLE_FRAME_MS count=", samples.size(), " p50=", samples[samples.size()/2],
            " p99=", samples[int((samples.size()-1)*0.99)], " max=", samples.back())
    print("PARTICLE_PIXELS ", label, " track=", track, " nodes=", targets.size(),
        " warm_pool=", nodes.size(), " changed=", count, " resolution=", size)
    if count > 5:
        var path := OS.get_environment("HP2_PARTICLE_CAPTURE_DIR")
        if not path.is_empty():
            shown.save_png(path.path_join("%s-%s.png" % [track, label]))
            hidden.save_png(path.path_join("%s-%s-hidden.png" % [track, label]))
    if count <= 5:
        warmed = 0
        previous = 0
        samples.clear()
        started = int(race.telemetry().step) - 85
    elif route or births_started:
        quit(0)
        return
    else:
        births_started = true
        Input.action_press("hp2_nitro")
    race.set_process(true)
    checking = false

func _hash(label: String) -> int:
    var value := 0xffffffff
    for byte in label.to_ascii_buffer():
        value = (value * 33 + byte) & 0xffffffff
    return value
