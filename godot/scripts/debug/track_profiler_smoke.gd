extends SceneTree
func _initialize() -> void:
    _run.call_deferred()
func _run() -> void:
    var api := preload("res://scripts/debug/render_api.gd")
    for row in [["opengl3","OpenGL"],["d3d12","DX12"],["d3d11","DX11"],["vulkan","Vulkan"],["metal","Metal"]]:
        if api.label(row[0])!=row[1]:
            push_error("Incorrect active rendering API label")
            quit(1)
            return
    if api.label("opengl3_angle","ANGLE Direct3D11")!="OpenGL / DX11":
        push_error("ANGLE backend label mismatch")
        quit(1)
        return
    await process_frame
    var profiler := root.get_node("ProfileDebug")
    if not profiler.enabled or not Performance.has_custom_monitor("HP2/sun_readback_ms"):
        push_error("HP2 Performance monitors missing")
        quit(1)
        return
    if not root.title.ends_with("("+api.current()+")"):
        push_error("Actual window title missing active driver: "+root.title)
        quit(1)
        return
    if not profiler.samples.is_empty():
        push_error("Empty scene must not become rendering evidence")
        quit(1)
        return
    print("TRACK_PROFILER_SMOKE_OK title=",root.title," monitors=",profiler.monitors.size())
    quit(0)
