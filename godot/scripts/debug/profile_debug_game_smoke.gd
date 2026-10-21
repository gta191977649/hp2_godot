extends SceneTree
func _initialize() -> void:_run.call_deferred()
func _run() -> void:
    root.size=Vector2i(1280,800)
    var scene: Node=load("res://scenes/race_scene.tscn").instantiate()
    root.add_child(scene)
    current_scene=scene
    var deadline := Time.get_ticks_msec()+120000
    while scene.phase=="Loading":
        if Time.get_ticks_msec()>deadline:
            push_error("Profile game fixture load timed out")
            quit(1)
            return
        await process_frame
    if scene.phase!="Running":
        push_error("Game fixture did not enter Running: "+str(scene.phase))
        quit(1)
        return
    var profiler := root.get_node("ProfileDebug")
    profiler.capture_next_frames()
    profiler.limit=20
    for frame in 40:
        await process_frame
        await RenderingServer.frame_post_draw
        if profiler.exported:break
    var names: Array=[]
    for sample in profiler.samples:
        for row in sample.functions:
            if not names.has(row.name):names.append(row.name)
    if "RaceScene._present_frame" not in names or "Ps2DrivingRuntime.tick" not in names:
        push_error("Running game function traces missing: "+str(names))
        quit(1)
        return
    var output := ProjectSettings.globalize_path("res://../local/task0070")
    DirAccess.make_dir_recursive_absolute(output)
    root.get_texture().get_image().save_png(output.path_join("profile-debug-game.png"))
    print("PROFILE_DEBUG_GAME_OK running game + live ImGui + native/script function scopes")
    quit(0)
