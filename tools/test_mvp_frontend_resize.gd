extends "test_frontend_mvp.gd"

const SIZES := [Vector2i(800, 600), Vector2i(1120, 630), Vector2i(1260, 540), Vector2i(540, 720)]
var direct := false
var track_picker := false
var resize_pending := false
var tested_screens := {}
var initial_mode: Window.Mode
var resize_failed := false
var loading_captured := false

func _initialize() -> void:
    direct = not OS.get_cmdline_user_args().is_empty()
    track_picker = OS.get_environment("HP2_MVP_RESIZE_TRACK_PICKER") == "1"
    initial_mode = root.mode
    super._initialize()

func _process(delta: float) -> bool:
    if resize_failed:
        return true
    if resize_pending:
        return false
    if not direct:
        if is_instance_valid(race):
            var state: Dictionary = race.call("telemetry")
            var page: String = state.get("screen", "")
            if state.get("loading", false) and not loading_captured:
                loading_captured = true
                _capture("loading")
            if track_picker:
                if stage == 6:
                    stage = 30
                    _tap(KEY_DOWN)
                    next_input = Time.get_ticks_msec() + 400
                    return false
                if stage == 30 and Time.get_ticks_msec() >= next_input:
                    stage = 31
                    _tap(KEY_ENTER)
                    return false
                if stage == 31 and page == "Challenge":
                    stage = 32
                    next_input = Time.get_ticks_msec() + 950
                    return false
                if stage == 32 and page == "Challenge" and Time.get_ticks_msec() >= next_input:
                    stage = 17
                    _tap(KEY_ENTER)
                    return false
                if tested_screens.has("select-track"):
                    print("HP2_MVP_RESIZE_PASS select-track")
                    quit(0)
                    return false
            var label: String = {
                "New Main Menu": "main-menu", "View Event Tree": "event-tree",
                "Select Car": "select-car", "Select Track": "select-track",
                "Pre Race Blurb": "pre-race-blurb", "Pre Race Stats": "pre-race-stats",
                "End Game Menu": "end-game-menu", "Race Results": "race-results",
            }.get(page, "")
            if page == "Cops On Mode Select" and stage == 29:
                var item: int = state.get("selected_item", 0)
                if item == 0x52e039f6:
                    stage = 14
                    _tap(KEY_ENTER)
                    return false
                if Time.get_ticks_msec() >= next_input:
                    _tap(KEY_DOWN if item < 0x52e039f6 else KEY_UP)
                    next_input = Time.get_ticks_msec() + 500
                return false
            var player := _find_video(root) if page == "Title Screen" and state.get("video_request") == 2 else null
            if player and player.stream_position >= 2.0:
                label = "intro-video"
            if page == "Title Screen" and stage == 11 and state.get("video_request") == 0:
                label = "title-screen"
            if not label.is_empty() and not tested_screens.has(label):
                _resize_sweep(label, label == "main-menu")
                return false
            if pursuit and route == null and state.get("race_state") == 3 and state.get("track") == 13:
                for required in ["intro-video", "title-screen", "main-menu", "event-tree",
                        "select-car", "pre-race-blurb", "pre-race-stats"]:
                    if not tested_screens.has(required):
                        return _fail("missing_resize_" + required)
                print("HP2_MVP_RESIZE_PASS pursuit ", tested_screens, " loading_capture=", loading_captured)
                quit(0)
                return false
        return super._process(delta)
    if Time.get_ticks_msec() > deadline:
        return _fail("direct_resize_timeout")
    if not is_instance_valid(race):
        race = root.get_node_or_null("NativeGame/RacePresentation")
        if is_instance_valid(race) and OS.get_environment("HP2_MVP_DEBUG_CONSOLE") == "1":
            _tap(KEY_QUOTELEFT)
        return false
    var sample: Dictionary = race.call("telemetry")
    var screen: String = sample.get("screen", "")
    if stage == 0 and sample.get("race_state") == 3 and sample.get("step", 0) > 20:
        stage = 1
        _key(KEY_W, true)
        _resize_sweep("direct-hud")
    elif stage == 1 and sample.get("speed", 0) > 12.0:
        stage = 2
        _key(KEY_W, false)
        _tap(KEY_ESCAPE)
    elif stage == 2 and screen == "Pause Menu":
        stage = 3
        _resize_sweep("direct-pause", true)
    elif stage == 3:
        stage = 4
        next_input = 0
    elif stage == 4 and sample.get("selected_item") == 0x52e039f4:
        stage = 5
        _tap(KEY_ENTER)
        next_input = Time.get_ticks_msec() + 650
    elif stage == 4 and Time.get_ticks_msec() >= next_input:
        _tap(KEY_DOWN if sample.get("selected_item", 0) < 0x52e039f4 else KEY_UP)
        next_input = Time.get_ticks_msec() + 650
    elif stage == 5 and Time.get_ticks_msec() >= next_input:
        stage = 6
        _resize_sweep("direct-confirm")
    elif stage == 6:
        stage = 7
        print("HP2_MVP_RESIZE_PASS direct ", tested_screens)
        quit(0)
    return false

func _capture(name: String) -> bool:
    if tested_screens.has(name):
        return true
    return await super._capture(name)

func _resize_sweep(name: String, maximize := false) -> bool:
    resize_pending = true
    var before: Dictionary = race.call("telemetry")
    var video: VideoStreamPlayer
    if name == "intro-video" or name == "main-menu":
        video = _find_video(root)
    var original_video_rect := Rect2(video.position, video.size) if video else Rect2()
    var original_video_position := video.stream_position if video else 0.0
    var canvas := race.get_node_or_null("FrontendCanvas") as Node2D
    if canvas == null:
        return _resize_error("canvas_" + name)
    var original_size := root.size
    root.mode = Window.MODE_WINDOWED
    var sizes := SIZES.duplicate()
    if name == "select-car" or name == "select-track":
        # Capture the loaded preview after the initial screen fade.
        await create_timer(0.6).timeout
        sizes.append(Vector2i(1280, 900))
    for wanted in sizes:
        root.size = wanted
        await process_frame
        await RenderingServer.frame_post_draw
        var image := root.get_texture().get_image()
        if root.size != wanted or image == null or image.get_size() != wanted:
            return _resize_error("extent_" + name)
        var fitted_scale := minf(wanted.x / 640.0, wanted.y / 480.0)
        if not canvas.scale.is_equal_approx(Vector2.ONE * fitted_scale) or not canvas.position.is_equal_approx(
                (Vector2(wanted) - Vector2(640, 480) * fitted_scale) * 0.5):
            return _resize_error("centered_aspect_" + name)
        if name == "direct-hud":
            var mirror := canvas.get_node_or_null("RearViewMirror") as TextureRect
            var target := race.get_node_or_null("RaceScene/RearViewTarget") as SubViewport
            if not mirror or not mirror.visible or not target or mirror.texture != target.get_texture():
                return _resize_error("rear_view_target")
            var mirror_rect := mirror.get_global_rect()
            if not is_equal_approx(mirror_rect.get_center().x, wanted.x * 0.5):
                return _resize_error("rear_view_center")
            var expected_height := maxi(64, ceili(64 * fitted_scale))
            if target.size != Vector2i(roundi(expected_height * 2.88), expected_height):
                return _resize_error("rear_view_projection_size")
        if name == "select-car" or name == "select-track":
            var display := canvas.get_node_or_null("PreviewDisplay1") as TextureRect
            var preview := display.get_node_or_null("PreviewViewport") as SubViewport if display else null
            var pixels := Vector2i((Vector2(640, 480) * fitted_scale).ceil())
            if not preview or not display.visible or preview.size != pixels or display.size != Vector2(640, 480):
                return _resize_error("preview_resolution_" + name)
            if name == "select-car" and wanted == sizes.back() and not _has_race_preview_materials(preview):
                return _resize_error("preview_race_materials")
            print("HP2_MVP_PREVIEW screen=", name, " window=", wanted, " render_target=", preview.size)
        if name == "intro-video" or name == "main-menu":
            if not video is VideoStreamPlayer or not video.visible or not is_equal_approx(video.size.aspect(), 4.0 / 3.0):
                return _resize_error("video_aspect")
            if name == "main-menu" and original_video_rect.has_area() and not Rect2(
                    video.position, video.size).is_equal_approx(original_video_rect):
                return _resize_error("video_panel_alignment")
        if image.save_png(screenshot_dir.path_join("%s-%dx%d.png" % [name, wanted.x, wanted.y])) != OK:
            return _resize_error("screenshot_" + name)
    if maximize:
        root.mode = Window.MODE_MAXIMIZED
        await create_timer(0.35).timeout
        await RenderingServer.frame_post_draw
        if root.mode != Window.MODE_MAXIMIZED:
            return _resize_error("maximize_" + name)
    root.mode = Window.MODE_WINDOWED
    root.size = original_size
    if initial_mode == Window.MODE_MAXIMIZED:
        root.mode = initial_mode
    await process_frame
    var after: Dictionary = race.call("telemetry")
    for key in ["screen", "generation", "video_request"]:
        if before.get(key) != after.get(key):
            return _resize_error("state_" + name + "_" + key)
    if video and video.stream_position < original_video_position:
        return _resize_error("video_restarted_" + name)
    tested_screens[name] = sizes
    print("HP2_MVP_RESIZE screen=", name, " sizes=", sizes)
    resize_pending = false
    return true

func _find_video(node: Node) -> VideoStreamPlayer:
    if node is VideoStreamPlayer:
        return node
    for child in node.get_children():
        var player := _find_video(child)
        if player:
            return player
    return null

func _has_race_preview_materials(preview: SubViewport) -> bool:
    var surfaces := 0
    var texture_keys := {}
    for node in preview.get_children():
        if node is Light3D:
            return false
        if not node is MeshInstance3D or not node.visible or not node.mesh:
            continue
        for i in range(node.mesh.get_surface_count()):
            var material := node.mesh.surface_get_material(i) as StandardMaterial3D
            if material and material.albedo_texture:
                texture_keys[material.albedo_texture.resource_name] = true
            if not material or material.shading_mode != BaseMaterial3D.SHADING_MODE_UNSHADED:
                return false
            var arrays: Array = node.mesh.surface_get_arrays(i)
            if not material.vertex_color_use_as_albedo or not material.vertex_color_is_srgb or arrays[Mesh.ARRAY_COLOR].is_empty():
                return false
            surfaces += 1
    print("HP2_MVP_PREVIEW unshaded_surfaces=", surfaces, " texture_keys=", texture_keys.keys())
    # The first Ultimate Racer event selects the Lotus Elise. Check its
    # resolved off-state headlamp and brake lamp texture resources.
    if pursuit and (not texture_keys.has("544547642") or not texture_keys.has("199674477")):
        return false
    return surfaces > 0

func _resize_error(reason: String) -> bool:
    resize_failed = true
    _fail(reason)
    return false
