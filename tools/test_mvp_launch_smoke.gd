extends SceneTree

const GRID := 1
const COUNTDOWN := 2
const LIVE := 3
const SAMPLE_SECONDS := 5.0
const ROAD_EFFECTS := ["XETT_ROAD_SK", "XETT_ROAD_SP", "XEBO_ROAD"]

var race: Node
var deadline := Time.get_ticks_msec() + 45000
var variant := OS.get_environment("HP2_LAUNCH_VARIANT")
var output := OS.get_environment("HP2_LAUNCH_OUTPUT")
var capture_tick := OS.get_environment("HP2_LAUNCH_CAPTURE_TICK").to_int()
var hold_tick := OS.get_environment("HP2_LAUNCH_HOLD_TICK").to_int()
var keys := {}
var effects: Array[GPUParticles3D] = []
var trimmed: Array[ShaderMaterial] = []
var rows: Array = []
var seen := {}
var confirm_at := 0
var start_tick := -1.0
var previous := 0
var tick := 0.0
var phase := 0
var step := 0
var advancing := false
var finished := false
var holding := false

func _initialize() -> void:
    if OS.get_environment("HP2_MVP_TEST_PROFILE") != "1" or output.is_empty():
        fail("The launch fixture needs an isolated profile and output directory")
        return
    for label in ROAD_EFFECTS:
        var key := 0xffffffff
        for byte in label.to_ascii_buffer():
            key = (key * 33 + byte) & 0xffffffff
        keys[key] = label
    RenderingServer.frame_post_draw.connect(sample_frame)
    change_scene_to_file.call_deferred("res://native_game.tscn")

func _process(_delta: float) -> bool:
    if finished or holding:
        return false
    var now := Time.get_ticks_msec()
    if now > deadline:
        fail("Timed out at %s" % [race.telemetry() if race else {}])
        return false
    if not race:
        race = root.get_node_or_null("NativeGame/RacePresentation")
        return false
    var state: Dictionary = race.telemetry()
    phase = state.get("race_state", 0)
    if phase == GRID and state.get("screen") == "Pre Race Stats" and now >= confirm_at:
        confirm_at = now + 500
        send_confirm()
    if phase != COUNTDOWN and phase != LIVE:
        return false
    if not advancing:
        var scene: Node = race.race_scene()
        var owner := scene.get_node("RaceEffects")
        for node in owner.get_children():
            if node is GPUParticles3D and keys.has(road_effect_key(node)):
                effects.push_back(node)
                var material := node.material_override as ShaderMaterial
                if material.get_shader_parameter("trim_tyre_quad") == true:
                    trimmed.push_back(material)
                    if variant == "full-quad":
                        material.set_shader_parameter("trim_tyre_quad", false)
                if variant == "no-tyre":
                    node.layers = 0
        var exhaust := owner.get_node_or_null("TailpipeExhaust") as GPUParticles3D
        if variant == "no-exhaust" and exhaust:
            exhaust.layers = 0
        var viewport := scene.get_viewport() as SubViewport
        if viewport.screen_space_aa != Viewport.SCREEN_SPACE_AA_FXAA:
            fail("Launch fixture requires FXAA")
            return false
        if variant == "full-quad" and trimmed.is_empty():
            fail("Full-quad control did not find any trimmed launch materials")
            return false
        print("LAUNCH_DISPLAY track=", state.track, " mode=", root.mode, " client=", root.size,
            " viewport=", viewport.get_visible_rect().size, " vsync=", DisplayServer.window_get_vsync_mode(),
            " refresh=", DisplayServer.screen_get_refresh_rate(root.current_screen), " cap=", Engine.max_fps,
            " trim_materials=", trimmed.size())
        send_key(KEY_UP, true)
        advancing = true
    var clock: Vector4 = state.animation_clock
    tick = clock.z
    step = state.step
    if phase == LIVE and start_tick < 0.0:
        start_tick = tick
    if capture_tick > 0 and phase == LIVE and tick >= start_tick + capture_tick:
        finished = true
        compare_pixels.call_deferred()
    elif hold_tick > 0 and phase == LIVE and tick >= start_tick + hold_tick:
        finished = true
        hold_pose.call_deferred()
    elif phase == LIVE and tick >= start_tick + SAMPLE_SECONDS * 60.0:
        finished = true
        save_trace.call_deferred()
    return false

func sample_frame() -> void:
    if finished or not advancing:
        return
    var now := Time.get_ticks_usec()
    var active := 0
    for node in effects:
        if node.visible:
            active += 1
            var label: String = keys[road_effect_key(node)]
            seen[label] = seen.get(label, 0) + 1
    if previous:
        rows.push_back([Engine.get_process_frames(), step, tick, phase, (now - previous) / 1000.0, active])
    previous = now

func hold_pose() -> void:
    race.set_process(false)
    holding = true
    send_key(KEY_UP, false)
    if not effects.any(func(node): return node.visible):
        fail("The requested pose has no active road tyre emitters")
        return
    print("LAUNCH_HOLD step=", step, " ticks_after_in_progress=", tick - start_tick)
    for _frame in range(36):
        await RenderingServer.frame_post_draw
    rows.clear()
    seen.clear()
    previous = 0
    finished = false
    for _frame in range(241):
        await RenderingServer.frame_post_draw
    finished = true
    save_trace()

func save_trace() -> void:
    send_key(KEY_UP, false)
    if rows.size() < 120 or seen.is_empty():
        fail("Launch did not exercise the road tyre effects. Frames=%d matched_nodes=%d speed=%s"
            % [rows.size(), effects.size(), race.telemetry().get("speed")])
        return
    var trace := FileAccess.open(output.path_join("frames.json"), FileAccess.WRITE)
    if not trace:
        fail("Could not open the launch trace")
        return
    # Keep the existing trace field. The in-progress session starts during the countdown.
    trace.store_string(JSON.stringify({"go": start_tick, "rows": rows, "effects": seen}))
    trace.close()
    print("LAUNCH_PASS frames=", rows.size(), " effects=", seen)
    quit()

func compare_pixels() -> void:
    race.set_process(false)
    var scene: Node = race.race_scene()
    var viewport: SubViewport = scene.get_viewport()
    var rear := scene.get_node_or_null("RearViewTarget") as SubViewport
    await RenderingServer.frame_post_draw
    var shown := viewport.get_texture().get_image()
    var show_rear := rear and rear.render_target_update_mode != SubViewport.UPDATE_DISABLED
    if show_rear:
        rear.get_texture().get_image().save_png(output.path_join("rear-shown.png"))
    for material in trimmed:
        material.set_shader_parameter("trim_tyre_quad", false)
    await process_frame
    await RenderingServer.frame_post_draw
    var full_quad := viewport.get_texture().get_image()
    if show_rear:
        rear.get_texture().get_image().save_png(output.path_join("rear-full-quad.png"))
    for node in effects:
        node.layers = 0
    await process_frame
    await RenderingServer.frame_post_draw
    var hidden := viewport.get_texture().get_image()
    if show_rear:
        rear.get_texture().get_image().save_png(output.path_join("rear-hidden.png"))
    var changed := 0
    for y in range(0, shown.get_height(), 2):
        for x in range(0, shown.get_width(), 2):
            var diff := shown.get_pixel(x, y) - hidden.get_pixel(x, y)
            if absf(diff.r) + absf(diff.g) + absf(diff.b) > 3.0 / 255.0:
                changed += 1
    print("LAUNCH_PIXELS step=", step, " ticks_after_in_progress=", tick - start_tick,
        " changed_samples=", changed)
    if changed < 6 or shown.save_png(output.path_join("shown.png")) != OK or \
            full_quad.save_png(output.path_join("full-quad.png")) != OK or \
            hidden.save_png(output.path_join("hidden.png")) != OK:
        fail("Could not confirm drawn tyre smoke")
        return
    send_key(KEY_UP, false)
    quit()

func send_confirm() -> void:
    send_key(KEY_ENTER, true)
    await create_timer(0.08).timeout
    send_key(KEY_ENTER, false)

func road_effect_key(node: Node) -> int:
    var group: int = node.get_meta("source_effect_group", 0)
    return group if keys.has(group) else node.get_meta("source_effect_key", 0)

func send_key(key: Key, held: bool) -> void:
    var event := InputEventKey.new()
    event.physical_keycode = key
    event.keycode = key
    event.pressed = held
    Input.parse_input_event(event)

func fail(reason: String) -> void:
    finished = true
    send_key(KEY_UP, false)
    push_error("LAUNCH_FAILURE " + reason)
    quit(1)
