extends SceneTree
# Port-only bounded capture/parity fixture; never a steady-state FPS benchmark.
var renderer: Node3D
var camera: Camera3D
var output := ""
var capture := false
func _initialize() -> void:
    _run.call_deferred()
func _fail(message: String) -> void:
    push_error(message)
    quit(1)
func _step(dt := 0.0) -> bool:
    renderer.set_visual_timestep(dt)
    var state: Dictionary = renderer.update_view(camera.global_position)
    if not state.get("ok",false):
        _fail("Transport frame failed: " + str(state))
        return false
    await process_frame
    await RenderingServer.frame_post_draw
    return true
func _save(label: String) -> void:
    var helper := preload("res://scripts/debug/track_render_diagnostics.gd").new()
    var report: Dictionary = helper.sample(root,camera,renderer,true)
    report["fixture"] = {"visual_steps":60,"visual_dt":1.0/60.0,"frozen_after_steps":true,"live_particle_checkpoint":false}
    report["summary"] = renderer.frame_summary()
    report["image_sha256"] = {}
    var images := {"final":root.get_texture().get_image()}
    for key in ["colour","native_depth"]:
        images[key] = renderer.get_render_targets()[key].get_image()
    for key in images:
        var image: Image = images[key]
        var hash := HashingContext.new()
        hash.start(HashingContext.HASH_SHA256)
        hash.update(image.get_data())
        report.image_sha256[key] = hash.finish().hex_encode()
        image.save_png(output.path_join(label+"-"+key+".png"))
    var file := FileAccess.open(output.path_join(label+".json"),FileAccess.WRITE)
    file.store_string(JSON.stringify(report,"  "))
func _run() -> void:
    var snapshot := ""
    var size := Vector2i(2880,1695)
    for arg in OS.get_cmdline_user_args():
        if arg.begins_with("--transport-output="):output=arg.trim_prefix("--transport-output=")
        if arg.begins_with("--transport-snapshot="):snapshot=arg.trim_prefix("--transport-snapshot=")
        if arg=="--transport-1080":size=Vector2i(1920,1080)
        if arg=="--transport-capture":capture=true
    if output.is_empty() or snapshot.is_empty():
        _fail("Transport fixture requires explicit output and snapshot")
        return
    DirAccess.make_dir_recursive_absolute(output)
    root.size=size
    root.content_scale_size=Vector2i.ZERO
    DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
    var overlay := root.get_node_or_null("DebugTools")
    if overlay:overlay.hide()
    camera=Camera3D.new()
    root.add_child(camera)
    camera.current=true
    renderer=ClassDB.instantiate("Ps2TrackRenderer")
    root.add_child(renderer)
    var parsed: Dictionary=JSON.parse_string(FileAccess.get_file_as_string(snapshot))
    var state: Dictionary=renderer.request_track(int(parsed.track))
    var deadline := Time.get_ticks_msec()+120000
    while state.get("loading",false):
        if Time.get_ticks_msec()>deadline:
            _fail("Transport load timeout")
            return
        state=renderer.pump_load()
        await process_frame
    if not state.get("ok",false):
        _fail("Transport load failed: "+str(state))
        return
    var helper := preload("res://scripts/debug/track_render_diagnostics.gd").new()
    print(helper.restore_snapshot(camera,renderer,snapshot))
    for argument in OS.get_cmdline_user_args():
        if argument.begins_with("--transport-disable="):
            renderer.set_effect_enabled(argument.trim_prefix("--transport-disable="),false)
    if renderer.has_method("set_diagnostics_level"):renderer.set_diagnostics_level("summary")
    for frame in 60:
        if not await _step(1.0/60.0):return
    for frame in 12:
        if not await _step():return
    if "--transport-profile" in OS.get_cmdline_user_args():
        var profiler := root.get_node("ProfileDebug")
        if not profiler.enabled:
            _fail("Transport profile requires --track-profile")
            return
        profiler.begin_capture(0,60)
        for frame in 100:
            if not await _step():return
            if profiler.exported:break
        if not profiler.exported:
            _fail("Transport bounded profiler did not complete")
            return
    if capture:
        var ready := FileAccess.open(output.path_join("ready.json"),FileAccess.WRITE)
        ready.store_string(JSON.stringify({"track":int(parsed.track),"size":[root.size.x,root.size.y],"visual_steps":60}))
        ready.close()
        deadline=Time.get_ticks_msec()+60000
        while not FileAccess.file_exists(output.path_join("captured.json")):
            if Time.get_ticks_msec()>deadline:
                _fail("Bounded capture handshake timeout")
                return
            if not await _step():return
    _save("pose")
    # Small movement/rotation also exercises cache invalidation without a long benchmark.
    camera.position+=Vector3(3,1,-5)
    camera.rotate_object_local(Vector3.UP,0.12)
    for frame in 6:
        if not await _step():return
    _save("nearby")
    renderer.clear_track()
    print("TRACK_TRANSPORT_CAPTURE_OK ",output)
    quit(0)
