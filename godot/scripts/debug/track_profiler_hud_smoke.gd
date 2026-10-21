extends SceneTree
func _initialize() -> void:_run.call_deferred()
func _click(key: String) -> void:
    var overlay := root.get_node("DebugTools")
    var target: Vector2=overlay.last_frame.profile_debug.targets[key]*float(overlay.last_frame.ui_scale)
    var motion := InputEventMouseMotion.new()
    motion.position=target
    overlay.host.console_input(motion)
    await process_frame
    var button := InputEventMouseButton.new()
    button.button_index=MOUSE_BUTTON_LEFT
    button.position=target
    button.pressed=true
    overlay.host.console_input(button)
    await process_frame
    button.pressed=false
    overlay.host.console_input(button)
    for frame in 3:await process_frame
func _run() -> void:
    root.size=Vector2i(1280,800)
    var profiler := root.get_node("ProfileDebug")
    var scope: RefCounted=profiler.scope("Smoke.example_function")
    scope=null
    if not profiler.function_current.has("Smoke.example_function"):
        push_error("Function scopes did not record on scope exit")
        quit(1)
        return
    for frame in 12:
        await process_frame
        if DisplayServer.get_name()!="headless":await RenderingServer.frame_post_draw
    var overlay := root.get_node("DebugTools")
    var panel: Dictionary=overlay.last_frame.get("profile_debug",{})
    if not panel.get("visible",false) or int(panel.get("history",0))<2:
        push_error("Global ImGui profile panel did not collect live frames: "+str(panel))
        quit(1)
        return
    if profiler.latest.is_empty() or not profiler.latest.has("process_cpu_ms"):
        push_error("Global profiling incorrectly requires a loaded track")
        quit(1)
        return
    await _click("pause")
    if not overlay.last_frame.profile_debug.paused:
        push_error("ImGui pause did not freeze the live graph")
        quit(1)
        return
    var frozen: int=overlay.last_frame.profile_debug.history
    for frame in 3:await process_frame
    if overlay.last_frame.profile_debug.history!=frozen:
        push_error("Paused ImGui graph kept growing")
        quit(1)
        return
    await _click("clear")
    if overlay.last_frame.profile_debug.history!=0:
        push_error("ImGui clear did not reset graph history")
        quit(1)
        return
    await _click("pause")
    await _click("capture")
    if not profiler.capture_active or profiler.output.is_empty():
        push_error("ImGui export did not start a new recording")
        quit(1)
        return
    await _click("functions")
    if int(overlay.last_frame.profile_debug.get("function_rows",0))<1:
        push_error("ImGui function hotspot view did not draw instrumented functions")
        quit(1)
        return
    if DisplayServer.get_name()!="headless":
        var output := ProjectSettings.globalize_path("res://../local/task0070")
        DirAccess.make_dir_recursive_absolute(output)
        root.get_texture().get_image().save_png(output.path_join("profile-debug-imgui.png"))
    print("PROFILE_DEBUG_IMGUI_OK global sampling, function scope and native ImGui presentation")
    quit(0)
