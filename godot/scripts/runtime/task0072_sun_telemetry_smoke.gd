extends Node3D
# Port-only real RenderingDevice async telemetry lifecycle. No synchronous texture
# reads, images, polling callback waits, or original-game verification claim.
const CLEAR := Vector3(-732,300,-98)
const BLOCKED := Vector3(-732.5417,-94.604,-98.08092)
const SUN := Vector3(650,1300,-3900)
var renderer: Node3D
var camera: Camera3D
var profiler: Node
var output := ""
var blocked := false
var rows: Array = []
var errors: Array[String] = []
var completed := false
var initial_generation := -1
var reload_generation := -1
func _ready() -> void:_run.call_deferred()
func _write() -> bool:
    var report := {"task":"TASK0072","fixture":"production Sun async stamped telemetry","complete":completed and errors.is_empty(),"errors":errors,"engine":Engine.get_version_info(),"driver":RenderingServer.get_current_rendering_driver_name(),"method":RenderingServer.get_current_rendering_method(),"arguments":OS.get_cmdline_user_args(),"initial_generation":initial_generation,"reload_generation":reload_generation,"blocked_requested":blocked,"source_frame_domain":"engine_process_frame","notes":"Bounded original Track11 clear/below-road anchors; counter reads use only profiler asynchronous telemetry. Fixture waits for ordinary frames, never a callback or synchronous read. This is not a performance or original-equivalence result.","frames":rows}
    var file := FileAccess.open(output,FileAccess.WRITE)
    if file:
        file.store_string(JSON.stringify(report,"  "))
        return true
    errors.append("Cannot write Sun telemetry report: "+output)
    push_error(errors.back())
    return false
func _fail(message: String) -> void:
    errors.append(message)
    _write()
    push_error(message)
    get_tree().quit(1)
func _load_track() -> bool:
    var state: Dictionary=renderer.request_track(11)
    var deadline := Time.get_ticks_msec()+120000
    while state.get("loading",false):
        if Time.get_ticks_msec()>deadline:
            _fail("Sun async telemetry Track11 load timed out")
            return false
        state=renderer.pump_load()
        await get_tree().process_frame
    if not state.get("ok",false):
        _fail("Sun async telemetry Track11 load failed: "+str(state))
        return false
    renderer.restore_render_defaults()
    renderer.set_effect_enabled("sun",true)
    return true
func _collect(label: String, pose: Vector3, expected: int, frame_limit: int, transition: int, require_new_generation := false) -> bool:
    camera.position=pose
    camera.look_at(SUN)
    var valid := 0
    for index in frame_limit:
        renderer.set_visual_timestep(0.0)
        var submitted: Dictionary=renderer.update_view(camera.position)
        if not submitted.get("ok",false):
            _fail("Sun async original frame submission failed: "+str(submitted))
            return false
        await get_tree().process_frame
        await RenderingServer.frame_post_draw
        var sun: Dictionary=renderer.get_optimization_state().get("sun",{})
        var diagnostic: Dictionary=profiler.latest.get("sun_uncovered_async",{}).duplicate()
        rows.append({"phase":label,"index":index,"process_frame":Engine.get_process_frames(),"sun":sun,"async":diagnostic})
        if not bool(sun.get("gpu_resident",false)) or bool(sun.get("gpu_compare",false)) or int(sun.get("readback_bytes",-1))!=0 or sun.get("uncovered")!=null:
            _fail("Async telemetry requires resident Sun with zero normal synchronous readback: "+str(sun))
            return false
        var generation := int(sun.get("telemetry_generation",-1))
        if label=="initial_clear":initial_generation=generation
        if require_new_generation:
            reload_generation=generation
            if generation==initial_generation:
                _fail("Reloaded native Sun renderer reused its telemetry generation")
                return false
        if diagnostic.get("uncovered",null)!=null:
            if str(diagnostic.get("status",""))!="async_readonly_diagnostic" or str(diagnostic.get("source_frame_domain",""))!="engine_process_frame" or int(diagnostic.get("age_frames",-1))<0:
                _fail("Valid async count lacks stamped source/domain/age: "+str(diagnostic))
                return false
            if int(diagnostic.get("generation",-1))!=generation:
                _fail("Async count from retired native Sun generation was accepted")
                return false
            var source := int(diagnostic.get("source_frame",-1))
            if source>Engine.get_process_frames():
                _fail("Async counter source frame lies in the future")
                return false
            # Earlier valid diagnostics may remain visible during a pose change;
            # acceptance requires a payload stamped strictly after the transition.
            if source>transition and int(diagnostic.uncovered)==expected:valid+=1
        if index%30==0:print("TASK0072_SUN_TELEMETRY_FRAME ",label," ",index," ",JSON.stringify(diagnostic))
    if valid==0:
        _fail("No fresh stamped async count=%d in %d bounded %s frames"%[expected,frame_limit,label])
        return false
    if not _write():
        get_tree().quit(1)
        return false
    return true
func _run() -> void:
    if DisplayServer.get_name()=="headless":
        get_tree().quit(77)
        return
    output=ProjectSettings.globalize_path("res://../local/task0072/sun-telemetry.json")
    for argument in OS.get_cmdline_user_args():
        if argument.begins_with("--sun-telemetry-output="):output=argument.trim_prefix("--sun-telemetry-output=")
        if argument=="--sun-telemetry-blocked=true":blocked=true
    output=output.replace("\\","/").simplify_path()
    if not output.is_absolute_path() or output.to_lower().find("/local/")<0 or not output.ends_with(".json"):
        push_error("Sun telemetry requires an absolute ignored local/ JSON output")
        get_tree().quit(1)
        return
    if DirAccess.make_dir_recursive_absolute(output.get_base_dir())!=OK:
        push_error("Cannot create Sun telemetry report directory")
        get_tree().quit(1)
        return
    profiler=get_node_or_null("/root/ProfileDebug")
    if profiler==null or not profiler.enabled or profiler.ui_enabled or not profiler.sun_telemetry_enabled:
        _fail("Use --profile-debug --profile-debug-no-ui --sun-gpu-telemetry=true with this fixture")
        return
    # Ordinary profiling/async sampling stays live. The fixture owns its report.
    profiler.capture_active=false
    var overlay := get_node_or_null("/root/DebugTools")
    if overlay is CanvasLayer:overlay.hide()
    get_window().size=Vector2i(960,540)
    DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
    Engine.max_fps=0
    camera=Camera3D.new()
    camera.keep_aspect=Camera3D.KEEP_WIDTH
    camera.fov=float(0x32dc)*360.0/65536.0
    add_child(camera)
    camera.current=true
    renderer=ClassDB.instantiate("Ps2TrackRenderer")
    add_child(renderer)
    if renderer.get_catalog().is_empty():
        _fail("Sun telemetry requires configured original Track11 assets")
        return
    if not await _load_track():return
    if not await _collect("initial_clear",CLEAR,1024,160,Engine.get_process_frames()):return
    var node_identity := renderer.get_instance_id()
    renderer.clear_track()
    await get_tree().process_frame
    await RenderingServer.frame_post_draw
    if profiler.sun_telemetry_latest.get("uncovered",null)!=null:
        _fail("Track clear retained retired async Sun count")
        return
    var reload_transition := Engine.get_process_frames()
    if not await _load_track():return
    if renderer.get_instance_id()!=node_identity:
        _fail("Telemetry reload fixture must preserve the Godot renderer node")
        return
    if not await _collect("reloaded_clear",CLEAR,1024,160,reload_transition,true):return
    if blocked and not await _collect("road_blocked",BLOCKED,0,300,Engine.get_process_frames()):return
    completed=true
    if not _write():
        get_tree().quit(1)
        return
    print("TASK0072_SUN_TELEMETRY_GPU_OK ",output)
    get_tree().quit(0)
