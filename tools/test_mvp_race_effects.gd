extends SceneTree

var race: Node
var deadline: int
var highest_birth := 0
var highest_marks := 0
var lamps := 0
var sun := 0
var phase := 0
var restart_at := 0
var race_step := 0
var with_particles: Image
var captured_particles: Array[GPUParticles3D] = []
var next_capture_step := 0
var before_capture := 0
var terrain_groups := {}

func _initialize() -> void:
    for effect in ["XETT_ROAD_DR", "XETT_ROAD_SK", "XETT_DIRTRD_DR", "XETT_DIRTRD_SK",
            "XETT_GRASS_DR", "XETT_GRASS_SK", "XETT_SAND_DR", "XETT_SAND_SK"]:
        var group := 0xffffffff
        for byte in effect.to_ascii_buffer():
            group = (group * 33 + byte) & 0xffffffff
        terrain_groups[group] = effect
    deadline = Time.get_ticks_msec() + 45000
    call_deferred("change_scene_to_file", "res://native_game.tscn")

func _process(_delta: float) -> bool:
    if Time.get_ticks_msec() >= deadline:
        print("HP2_EFFECTS_TIMEOUT ", highest_birth, " ", highest_marks, " ", lamps, " ", sun, " phase=", phase)
        quit(1)
        return false
    if not race:
        race = root.get_node_or_null("NativeGame/RacePresentation")
        if race and race.telemetry().get("debug_console_open",false):
            key(KEY_QUOTELEFT,true)
            key(KEY_QUOTELEFT,false)
        return false
    var state: Dictionary = race.telemetry()
    if state.get("race_state") != 3:
        return false
    var effects: Node = race.race_scene().get_node_or_null("RaceEffects") if race.race_scene() else null
    if not effects:
        return false
    if phase == 0:
        key(KEY_W,true)
        race_step = state.step
        phase = 1
    if phase == 1 and state.step > race_step + 100:
        key(KEY_A,true)
        key(KEY_SPACE,true)
        restart_at = state.step
        phase = 2
    if phase == 2 and state.step > restart_at + 42:
        key(KEY_SPACE,false)
        key(KEY_A,false)
        phase = 3
    var lamp_count := 0
    var sun_count := 0
    var births := 0
    var marks := 0
    for node in effects.get_children():
        if not node is Node3D or not node.visible:
            continue
        if node is GPUParticles3D and _is_terrain_particle(node):
            births += 1
        elif node is MeshInstance3D:
            if node.mesh is ArrayMesh:
                marks += 1
            elif node.name.begins_with("SunFlare"):
                sun_count += 1
            else:
                lamp_count += 1
    lamps = maxi(lamps, lamp_count)
    sun = maxi(sun, sun_count)
    highest_birth = maxi(births,highest_birth)
    highest_marks = maxi(marks,highest_marks)
    if births > 0 and highest_marks > 0 and phase >= 2 and phase < 4 and state.step >= next_capture_step:
        print("HP2_EFFECTS_NODES particles=",highest_birth," marks=",highest_marks," lamps=",lamps," sun=",sun)
        before_capture = phase
        phase = 4
        race.set_process(false)
        RenderingServer.frame_post_draw.connect(_compare_particles.bind(effects), CONNECT_ONE_SHOT)
    return false

func _compare_particles(effects: Node) -> void:
    captured_particles.clear()
    with_particles = root.get_texture().get_image()
    capture("effects", with_particles)
    for node in effects.get_children():
        if node is GPUParticles3D and node.visible and _is_terrain_particle(node):
            captured_particles.push_back(node)
            node.hide()
    var groups := {}
    for node in captured_particles:
        var group := int(node.get_meta("source_effect_group"))
        groups[terrain_groups[group]] = true
    print("HP2_EFFECTS_TERRAIN_GROUPS ", groups.keys())
    _capture_hidden.call_deferred()

func _is_terrain_particle(node: GPUParticles3D) -> bool:
    return terrain_groups.has(node.get_meta("source_effect_group", 0))

func _capture_hidden() -> void:
    await process_frame
    await RenderingServer.frame_post_draw
    var without_particles := root.get_texture().get_image()
    capture("effects-hidden", without_particles)
    var changed := 0
    for y in range(100, without_particles.get_height() - 80):
        for x in range(0, without_particles.get_width()):
            var difference := with_particles.get_pixel(x, y) - without_particles.get_pixel(x, y)
            if absf(difference.r) + absf(difference.g) + absf(difference.b) > 0.06:
                changed += 1
    print("HP2_EFFECTS_PARTICLE_PIXELS_CHANGED ", changed)
    if changed >= 24:
        quit(0)
        return
    for node in captured_particles:
        node.show()
    next_capture_step = race.telemetry().step + 8
    phase = before_capture
    race.set_process(true)

func key(code: Key, pressed: bool) -> void:
    var event := InputEventKey.new()
    event.keycode = code
    event.physical_keycode = code
    event.pressed = pressed
    Input.parse_input_event(event)

func capture(name: String, image: Image) -> void:
    var dir := OS.get_environment("HP2_MVP_SCREENSHOT_DIR")
    if not dir.is_empty():
        image.save_png(dir.path_join(name + ".png"))
