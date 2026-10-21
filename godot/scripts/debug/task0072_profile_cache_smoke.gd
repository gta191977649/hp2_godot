extends SceneTree
# Port-only native ImGui history contract; works headless after isolated DLL build.
var host: Object
var packet: Dictionary
var result: Dictionary
var failed := false
func check(condition: bool, message: String) -> void:
    if not condition:
        failed=true
        push_error(message)
func _initialize() -> void:_run.call_deferred()
func _packet(index: int) -> Dictionary:
    return {"enabled":true,"compact_history":true,"verify_history":true,"engine_frame":1000000+index,"metrics":{"frame_wall_ms":index+0.5,"process_cpu_ms":index+1.5,"host_ms":index+2.5,"sun_readback_ms":index+3.5,"draw_calls":100+index,"primitives":200+index,"collector_ms":0.1},"viewports":[{"role":"colour","gpu_ms":null if index%2==0 else 4.5},{"role":"native_depth","gpu_ms":5.5},{"role":"history_0","gpu_ms":6.5},{"role":"history_1","gpu_ms":7.5},{"role":"root","gpu_ms":8.5}],"functions":[{"name":"Fixture.owned","ms":2.0,"calls":1,"max_ms":2.0,"source":"injected contract"}]}
func _tick() -> void:
    var frame: Dictionary=host.console_frame(1280,800,1.0/60.0,{"profile_debug":packet,"ui_scale":1.0})
    check(frame.get("ok",false),"Native profile cache fixture frame failed")
    result=frame.get("profile_debug",{})
func _click(key: String) -> void:
    if not result.get("targets",{}).has(key):
        check(false,"Profile cache fixture target missing: "+key)
        return
    var position: Vector2=result.targets[key]
    var motion := InputEventMouseMotion.new()
    motion.position=position
    host.console_input(motion)
    _tick()
    var button := InputEventMouseButton.new()
    button.button_index=MOUSE_BUTTON_LEFT
    button.position=position
    button.pressed=true
    host.console_input(button)
    _tick()
    button.pressed=false
    host.console_input(button)
    _tick()
func _run() -> void:
    await process_frame
    var overlay := root.get_node("DebugTools")
    overlay.set_process(false)
    root.get_node("ProfileDebug").set_process(false)
    host=overlay.host
    if host==null:
        check(false,"Profile cache fixture requires Debug native UI host")
        quit(1)
        return
    for index in 245:
        packet=_packet(index)
        _tick()
    check(result.history==240 and result.graph_cache_size==240,"Numeric history and snapshots must pop together at240")
    check(result.first_graph[0]==5.5 and result.first_graph[4]==4.5,"Numeric oldest history/pop or GPU role mapping differs")
    check(result.selected_graph==[244.5,245.5,246.5,247.5,null,5.5,6.5,7.5,8.5,344.0,444.0],"Latest cached values or N/A graph gap differ")
    _click("pause")
    check(result.paused,"Pause must freeze numeric and function history")
    packet=_packet(246)
    _tick()
    check(result.history==240 and result.selected_graph[0]==244.5,"Paused numeric history advanced")
    _click("inspect")
    check(result.selected_graph[0]==5.5+result.selected,"Inspect selection lost cached historical values")
    check(result.selected!=239,"Inspect slider did not change selected frame")
    _click("functions")
    check(result.get("function_rows",0)==1,"Compact native history lost function entries")
    _click("clear")
    check(result.history==0 and result.graph_cache_size==0,"Clear must reset both histories while paused")
    _click("pause")
    packet=_packet(250)
    _tick()
    check(result.history==1 and result.graph_cache_size==1 and result.selected_graph[0]==250.5,"Resume must append one complete native sample")
    _click("pause")
    packet.metrics.frame_wall_ms=999.0
    _tick()
    check(result.selected_graph[0]==250.5,"Native history aliased the live metrics dictionary")
    packet.compact_history=false
    _tick()
    check(result.graph_cache_size==0 and result.history==1,"Baseline path must retire only numeric cache")
    packet.compact_history=true
    _tick()
    check(result.graph_cache_size==1 and result.selected_graph[0]==250.5,"Switching to compact path must rebuild frozen numeric history")
    print("TASK0072_PROFILE_CACHE_", "FAILED" if failed else "OK", " cap/pop, gaps, ownership, pause/inspect, functions, clear/resume and switch transition")
    quit(1 if failed else 0)

