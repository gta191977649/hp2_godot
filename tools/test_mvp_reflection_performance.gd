extends SceneTree

const DEMO := 8
const POSE_STEP := 282
const WARMUP := 120
const SAMPLES := 360
const TEST_FOV := 59.0

var race: Node
var deadline := Time.get_ticks_msec() + 210000
var mode := OS.get_environment("HP2_MVP_REFLECTION_CHECK")
var process_started := 0
var draw_started := 0

const PIPELINE_MONITORS := [
    Performance.PIPELINE_COMPILATIONS_CANVAS, Performance.PIPELINE_COMPILATIONS_MESH,
    Performance.PIPELINE_COMPILATIONS_SURFACE, Performance.PIPELINE_COMPILATIONS_DRAW,
    Performance.PIPELINE_COMPILATIONS_SPECIALIZATION,
]

func _initialize() -> void:
    run.call_deferred()

func _process(_delta: float) -> bool:
    if Time.get_ticks_msec() > deadline:
        fail("timed out")
    return false

func fail(message: String) -> void:
    push_error("Reflection check: %s" % message)
    quit(1)

func run() -> void:
    if OS.get_environment("HP2_MVP_TEST_PROFILE") != "1":
        fail("HP2_MVP_TEST_PROFILE=1 is required")
        return
    change_scene_to_file("res://native_game.tscn")
    while not is_instance_valid(race):
        await process_frame
        race = root.get_node_or_null("NativeGame/RacePresentation")
    if mode == "demo44":
        root.mode = Window.MODE_MAXIMIZED
        await measure_demo44()
        return
    var state: Dictionary
    while true:
        await process_frame
        state = race.telemetry()
        if state.get("race_state") == DEMO and state.get("step", 0) >= POSE_STEP:
            break
    if state.track != 13 or not state.radial_blur_active:
        fail("expected the Scenic Drive AI demo with radial blur")
        return
    var scene: Node3D = race.race_scene()
    var camera := scene.get_viewport().get_camera_3d()
    var atlas := scene.get_node("CarReflectionAtlas") as SubViewport
    var faces: Array[SubViewport] = []
    for i in range(6):
        var face := atlas.get_node("CarReflectionFace%d" % i) as SubViewport
        if face.size != Vector2i(64, 64) or face.world_3d != atlas.get_node("CarReflectionFace0").world_3d:
            fail("reflection faces did not share their 64x64 world")
            return
        faces.append(face)
    var shared := OS.get_environment("HP2_MVP_PROFILE_SHARED_REFLECTION_WORLD") == "1"
    if (faces[0].world_3d == camera.get_world_3d()) != shared or atlas.size != Vector2i(256, 256):
        fail("unexpected reflection world or atlas extent")
        return
    print("REFLECTION_START track=", state.track, " step=", state.step, " size=", root.size,
        " refresh=", DisplayServer.screen_get_refresh_rate(), " vsync=", DisplayServer.window_get_vsync_mode(),
        " camera=", camera.global_transform, " origin=", faces[0].get_camera_3d().global_position)
    if mode == "demo":
        await watch_demo(scene, camera, atlas)
    else:
        race.set_process(false)
        # Pin the primary camera and face origin. The car roster is seeded at entry.
        # The sampled face orientations come from the donor schedule at step 282.
        camera.look_at_from_position(Vector3(-307.7053, -56.48249, -302.916),
            Vector3(-308.624171, -56.500217, -302.522054), Vector3.UP)
        camera.fov = TEST_FOV
        for face in faces:
            var face_camera := face.get_camera_3d()
            face_camera.global_position = Vector3(-312.5822, -58.29121, -300.7836)
        print("REFLECTION_FIXED camera=", camera.global_transform, " fov=", camera.fov,
            " origin=", faces[0].get_camera_3d().global_position)
        await fixed_view(atlas, faces)

func draw_faces(atlas: SubViewport, faces: Array[SubViewport], frame: int) -> Array[int]:
    var due: Array[int] = []
    if frame % 2 == 0:
        due.assign([0, 5, 1 + (frame / 2) % 4])
        for index in due:
            faces[index].render_target_update_mode = SubViewport.UPDATE_ONCE
        atlas.render_target_update_mode = SubViewport.UPDATE_ONCE
    return due

func fixed_view(atlas: SubViewport, faces: Array[SubViewport]) -> void:
    for index in range(WARMUP):
        draw_faces(atlas, faces, index)
        await process_frame
    var samples: Array[float] = []
    var faces_per_frame := PackedInt32Array()
    var draw_calls: Array[int] = []
    var start := Time.get_ticks_usec()
    var previous := start
    print("REFLECTION_BEGIN ", Engine.get_process_frames())
    for index in range(SAMPLES):
        var due := draw_faces(atlas, faces, index)
        await process_frame
        var now := Time.get_ticks_usec()
        samples.push_back((now - previous) / 1000.0)
        previous = now
        var draws := 0
        for face in due:
            draws += faces[face].get_render_info(Viewport.RENDER_INFO_TYPE_VISIBLE,
                Viewport.RENDER_INFO_DRAW_CALLS_IN_FRAME)
        draw_calls.push_back(draws)
        faces_per_frame.append(due.size())
    samples.sort()
    var active_draws := draw_calls.filter(func(draws: int) -> bool: return draws > 0)
    active_draws.sort()
    print("REFLECTION_END ", Engine.get_process_frames(), " P50/P95/P99=", percentiles(samples),
        " avg_fps=", SAMPLES * 1000000.0 / (previous - start), " faces_per_frame=", faces_per_frame.count(3) * 3.0 / SAMPLES,
        " reflection_draws_p50_active=", active_draws[active_draws.size() / 2] if not active_draws.is_empty() else 0,
        " >=15ms=", samples.filter(func(ms: float) -> bool: return ms >= 15.0).size())
    if active_draws.is_empty():
        fail("the reflection viewports did not render scenery")
        return
    quit(0)

func watch_demo(scene: Node3D, camera: Camera3D, atlas: SubViewport) -> void:
    var changes := 0
    var cuts := 0
    var previous_position := camera.global_position
    var previous_image := PackedByteArray()
    var step := 0
    while true:
        await process_frame
        var state: Dictionary = race.telemetry()
        if state.get("screen") == "Title Screen" and not is_instance_valid(scene):
            break
        if state.get("race_state") != DEMO or state.get("step", 0) < step + 120:
            continue
        step = state.step
        if not state.radial_blur_active:
            fail("radial blur stopped during the active demo")
            return
        await RenderingServer.frame_post_draw
        var image := atlas.get_texture().get_image()
        var pixels := image.get_data()
        if pixels != previous_image and not previous_image.is_empty():
            changes += 1
        if camera.global_position.distance_to(previous_position) > 20.0:
            cuts += 1
        previous_position = camera.global_position
        previous_image = pixels
    if is_instance_valid(atlas) or changes < 3 or cuts < 3:
        fail("demo did not exercise reflection updates, moving views and retirement")
        return
    print("REFLECTION_DEMO_PASS last_step=", step, " atlas_changes=", changes, " camera_moves=", cuts)
    quit(0)

func percentiles(samples: Array[float]) -> Array[float]:
    return [samples[int((samples.size() - 1) * 0.50)], samples[int((samples.size() - 1) * 0.95)],
        samples[int((samples.size() - 1) * 0.99)]]

func mark_process_start() -> void:
    process_started = Time.get_ticks_usec()

func mark_draw_start() -> void:
    draw_started = Time.get_ticks_usec()

func measure_demo44() -> void:
    var started := Time.get_ticks_usec()
    var vsync := OS.get_environment("HP2_MVP_DEMO_VSYNC").to_int()
    var previous := 0
    var first_tick := -1.0
    var extent := Vector2i.ZERO
    var camera_cut := 0
    var old_eye := Vector3.ZERO
    var old_forward := Vector3.ZERO
    var rows := []
    var previous_pipelines := PackedInt32Array()
    previous_pipelines.resize(PIPELINE_MONITORS.size())
    var diagnostics := OS.get_environment("HP2_MVP_DEMO_DIAGNOSTICS") == "1"
    if diagnostics:
        process_frame.connect(mark_process_start)
        RenderingServer.frame_pre_draw.connect(mark_draw_start)
    while true:
        await RenderingServer.frame_post_draw
        var now := Time.get_ticks_usec()
        var state: Dictionary = race.telemetry()
        if state.get("race_state") != DEMO:
            if first_tick >= 0.0:
                if race.race_scene() != null:
                    continue
                break
            continue
        if state.track != 44 or not state.radial_blur_active:
            fail("expected the live Track 44 demo with radial blur")
            return
        var scene: Node3D = race.race_scene()
        var view := scene.get_viewport() as SubViewport
        var camera := view.get_camera_3d()
        var tick: float = state.animation_clock.x
        if first_tick < 0.0:
            first_tick = tick
            extent = view.size
            print("DEMO44_START client=", root.size, " viewport=", extent, " window_mode=", root.mode,
                " refresh=", DisplayServer.screen_get_refresh_rate(), " vsync=", DisplayServer.window_get_vsync_mode(),
                " time_to_demo_ms=", (now - started) / 1000.0)
            if DisplayServer.window_get_vsync_mode() != vsync:
                fail("unexpected VSync mode")
                return
        if root.mode != Window.MODE_MAXIMIZED or view.size != extent or root.size != extent or view.scaling_3d_scale != 1.0:
            fail("window mode or native extent changed during measurement")
            return
        if view.screen_space_aa != Viewport.SCREEN_SPACE_AA_FXAA:
            fail("expected Godot's FXAA preset")
            return
        var eye := camera.global_position
        var forward := -camera.global_basis.z
        if previous > 0 and (eye.distance_to(old_eye) > 10.0 or forward.dot(old_forward) < 0.85):
            camera_cut += 1
        if previous > 0 and tick - first_tick >= 120.0:
            var row := [Engine.get_process_frames(), (tick - first_tick) / 60.0, (now - previous) / 1000.0,
                camera_cut, view.get_render_info(Viewport.RENDER_INFO_TYPE_VISIBLE, Viewport.RENDER_INFO_DRAW_CALLS_IN_FRAME),
                view.get_render_info(Viewport.RENDER_INFO_TYPE_VISIBLE, Viewport.RENDER_INFO_PRIMITIVES_IN_FRAME),
                state.get("scenery_batch_updates", 0)]
            if diagnostics:
                row.append([(draw_started - process_started) / 1000.0, (now - draw_started) / 1000.0])
                var compilations := []
                for index in range(PIPELINE_MONITORS.size()):
                    var count := int(Performance.get_monitor(PIPELINE_MONITORS[index]))
                    compilations.append(count - previous_pipelines[index])
                    previous_pipelines[index] = count
                row.append(compilations)
            rows.append(row)
            # The first 35 source seconds include several reproducible stalls.
            if diagnostics and tick - first_tick >= 35.0 * 60.0:
                break
        previous = now
        old_eye = eye
        old_forward = forward
    if rows.size() < 300:
        fail("incomplete Track 44 sample")
        return
    var output := FileAccess.open(OS.get_environment("HP2_MVP_DEMO_TRACE"), FileAccess.WRITE)
    if output == null:
        fail("could not write frame trace")
        return
    output.store_string(JSON.stringify(rows))
    output.close()
    if diagnostics:
        process_frame.disconnect(mark_process_start)
        RenderingServer.frame_pre_draw.disconnect(mark_draw_start)
        # Leave through the game so it retires the live viewport and atlas.
        Input.action_press("hp2_pause")
        while race.race_scene() != null:
            await process_frame
        Input.action_release("hp2_pause")
    print("DEMO44_PASS frames=", rows.size(), " camera_cuts=", camera_cut)
    quit.call_deferred()
