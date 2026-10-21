extends Node
# Opt-in Godot Performance monitors and bounded engine viewport measurements.
# CPU wall times may include GPU waits; viewport GPU measurements are never inferred.
var enabled := false
var ui_enabled := false
var latest: Dictionary = {}
var latest_viewports: Array[Dictionary] = []
var samples: Array[Dictionary] = []
var renderers: Array[WeakRef] = []
var targets: Dictionary = {}
var monitors: Array[StringName] = []
var warm_frames := 60
var limit := 120
var output := ""
var seen_frames := 0
var exported := false
var previous_upload := -1
var previous_uniforms := -1
var last_generation := -1
var capture_active := true
var export_held := false # TASK0077: benchmark releases only after timed sampling.
var capture_completed_process_frame := -1
var latest_functions: Array[Dictionary] = []
var function_current: Dictionary = {}
var async_export := false # Independent candidate, default off until measured.
var export_identity_cache: Dictionary = {}
var export_thread: Thread
var export_job: Dictionary = {}
var export_queue: Array[Dictionary] = []
var export_request_sequence := 0
var capture_generation := 0
var export_status := "idle"
var export_error := ""
var export_completed_output := ""
var export_last_result: Dictionary = {}
var ui_bulk_upload := false # Port-side packed array transport candidate.
var compact_history := false # Port-side candidate; opt in until paired UI checks pass.
var timestamp_pending := false
var timestamp_frame := -1
var timestamp_rows: Dictionary = {}
var timestamp_status := "awaiting_render_thread_timestamps"
var sun_telemetry_enabled := false
var sun_telemetry_owner := 0
var sun_telemetry_generation := -1
var sun_telemetry_textures: Dictionary = {}
var sun_telemetry_pending := false
var sun_telemetry_request := 0
var sun_telemetry_last_request := -30
var sun_telemetry_available := false
var sun_telemetry_latest: Dictionary = {"uncovered":null,"source_frame":null,"source_frame_domain":"engine_process_frame","age_frames":null,"status":"disabled"}

# This telemetry is opt-in, readonly, and never feeds original Sun state. Keep
# only a WeakRef in the GPU callback, so a delayed read cannot retain its owner.
static func _read_sun_on_render_thread(observer: WeakRef, texture: RID, producer: int, generation: int, renderer_id: int, request: int) -> void:
    var rd := RenderingServer.get_rendering_device()
    var device_texture := RenderingServer.texture_get_rd_texture(texture) if rd!=null else RID()
    var done := func(bytes: PackedByteArray) -> void:
        var owner=observer.get_ref()
        if is_instance_valid(owner):owner._accept_sun_telemetry.call_deferred(bytes,texture,producer,generation,renderer_id,request)
    if rd==null or not device_texture.is_valid() or not rd.texture_is_valid(device_texture):
        done.call(PackedByteArray())
        return
    var error := rd.texture_get_data_async(device_texture,0,done)
    if error!=OK:done.call(PackedByteArray())

func _accept_sun_telemetry(bytes: PackedByteArray, texture: RID, producer: int, generation: int, renderer_id: int, request: int) -> void:
    if request!=sun_telemetry_request:return
    sun_telemetry_pending=false
    if renderer_id!=sun_telemetry_owner or generation!=sun_telemetry_generation or not sun_telemetry_available or not sun_telemetry_textures.has(texture.get_id()):return
    var count: Variant=null
    var status := "async_read_unavailable"
    # RG=count in every pixel; BA encodes the producer's32-bit process-frame
    # across each row's two pixels. Validate identity in the captured payload,
    # since the pingpong texture may have been reused before an async read ran.
    if bytes.size()==16 and producer>=0:
        var decoded := int(bytes[0])|(int(bytes[1])<<8)
        var consistent := true
        for pixel in range(1,4):
            if bytes[pixel*4]!=bytes[0] or bytes[pixel*4+1]!=bytes[1]:consistent=false
        for index in [2,3,6,7]:
            if bytes[index]!=bytes[index+8]:consistent=false
        var captured_producer := int(bytes[2])|(int(bytes[3])<<8)|(int(bytes[6])<<16)|(int(bytes[7])<<24)
        if captured_producer!=producer:status="async_source_frame_mismatch"
        elif not consistent:status="async_counter_payload_invalid"
        elif decoded<=1024:
            count=decoded
            status="async_readonly_diagnostic"
    sun_telemetry_latest={"uncovered":count,"source_frame":producer if count!=null else null,"source_frame_domain":"engine_process_frame","age_frames":maxi(0,Engine.get_process_frames()-producer) if count!=null else null,"generation":generation,"texture_rid":str(texture),"status":status}

func _sample_sun_telemetry(renderer: Node, sun: Dictionary) -> Dictionary:
    if not sun_telemetry_enabled:return sun_telemetry_latest
    var owner_id := renderer.get_instance_id() if renderer!=null else 0
    var generation := int(sun.get("telemetry_generation",-1))
    if owner_id!=sun_telemetry_owner or generation!=sun_telemetry_generation:
        sun_telemetry_owner=owner_id
        sun_telemetry_generation=generation
        sun_telemetry_textures={}
        sun_telemetry_pending=false
        sun_telemetry_request+=1
        sun_telemetry_latest={"uncovered":null,"source_frame":null,"source_frame_domain":"engine_process_frame","age_frames":null,"status":"awaiting_async_read"}
        sun_telemetry_last_request=-30
    var domain_valid := str(sun.get("counter_frame_domain",""))=="engine_process_frame"
    sun_telemetry_available=bool(sun.get("gpu_resident",false)) and bool(sun.get("counter_available",false)) and domain_valid
    var texture: RID=sun.get("counter_texture",RID())
    if not sun_telemetry_available or not texture.is_valid():
        sun_telemetry_latest={"uncovered":null,"source_frame":null,"source_frame_domain":"engine_process_frame","age_frames":null,"status":"counter_unavailable" if domain_valid else "counter_frame_domain_unavailable"}
        return sun_telemetry_latest
    sun_telemetry_textures[texture.get_id()]=true
    if not sun_telemetry_pending and Engine.get_process_frames()-sun_telemetry_last_request>=30:
        sun_telemetry_pending=true
        sun_telemetry_request+=1
        sun_telemetry_last_request=Engine.get_process_frames()
        RenderingServer.call_on_render_thread(_read_sun_on_render_thread.bind(weakref(self),texture,int(sun.get("counter_source_frame",-1)),generation,owner_id,sun_telemetry_request))
    var result := sun_telemetry_latest.duplicate()
    if result.get("source_frame",null)!=null:result["age_frames"]=maxi(0,Engine.get_process_frames()-int(result.source_frame))
    return result

# RenderingDevice timestamp arrays belong to the render thread. Read the completed
# frame and both viewport endpoints together, never the unversioned RS time cache.
# Godot ed1daf0bf renderer_viewport.cpp uses vp_begin_<RID> / vp_end_<RID>.
func _read_timestamps_on_render_thread() -> void:
    var rd := RenderingServer.get_rendering_device()
    var completed := -1
    var pairs: Dictionary = {}
    if rd != null:
        completed=rd.get_captured_timestamps_frame()
        for index in rd.get_captured_timestamps_count():
            var label := rd.get_captured_timestamp_name(index)
            if not (label.begins_with("vp_begin_") or label.begins_with("vp_end_")):continue
            var is_begin := label.begins_with("vp_begin_")
            var rid_id := label.trim_prefix("vp_begin_" if is_begin else "vp_end_")
            var pair: Dictionary=pairs.get(rid_id,{})
            pair["begin" if is_begin else "end"]=[rd.get_captured_timestamp_cpu_time(index),rd.get_captured_timestamp_gpu_time(index)]
            pairs[rid_id]=pair
    _accept_timestamps.call_deferred(completed,pairs)

func _accept_timestamps(completed: int, pairs: Dictionary) -> void:
    timestamp_pending=false
    timestamp_rows={}
    if completed < 0:
        timestamp_status="rendering_device_unavailable"
        return
    if completed <= timestamp_frame:
        timestamp_status="completed_timestamp_frame_not_advanced"
        return
    timestamp_frame=completed
    timestamp_status="completed_frame_missing_viewport_endpoints"
    for rid_id in pairs:
        var pair: Dictionary=pairs[rid_id]
        if not pair.has("begin") or not pair.has("end"):continue
        var first: Array=pair.begin
        var last: Array=pair.end
        var cpu_valid := int(first[0])>0 and int(last[0])>=int(first[0])
        var gpu_valid := int(first[1])>0 and int(last[1])>=int(first[1])
        timestamp_rows[rid_id]={"cpu_ms":(int(last[0])-int(first[0]))/1000.0 if cpu_valid else null,"gpu_ms":(int(last[1])-int(first[1]))/1000000.0 if gpu_valid else null,"source_frame":completed,"timing_status":"completed_timestamp_pair" if gpu_valid else "gpu_timestamp_unavailable"}
    if not timestamp_rows.is_empty():timestamp_status="completed_timestamp_pairs"

func _vector_data(value: Vector3) -> Array:
    return [value.x,value.y,value.z]

# Explicit JSON numbers retain the full camera orientation (Vector3 stringification
# rounds and is not a reproducible camera snapshot).
func _capture_context(renderer: Node, state: Dictionary) -> Dictionary:
    var context: Dictionary={"track":last_generation,"renderer_frame":state.get("lod",{}).get("visual_frame_counter",null),"controls":renderer.get_render_controls() if renderer!=null else {},"effective_near":state.get("effective_near",null),"effective_far":state.get("effective_far",null),"original_focal_h":state.get("original_focal_h",null),"packet_digest":state.get("packet_digest",""),"native_culling_views":state.get("lod",{}).get("native_culling_views",{}),"optimization_state":state.get("host_stats",{})}
    var scene := get_tree().current_scene
    var camera: Camera3D = scene.get_viewport().get_camera_3d() if scene!=null else null
    if camera!=null:
        var transform := camera.global_transform
        context["camera"]={"path":str(camera.get_path()),"position":_vector_data(transform.origin),"basis":[_vector_data(transform.basis.x),_vector_data(transform.basis.y),_vector_data(transform.basis.z)],"projection":camera.projection,"fov":camera.fov,"size":camera.size,"near":camera.near,"far":camera.far,"keep_aspect":camera.keep_aspect,"frustum_offset":[camera.frustum_offset.x,camera.frustum_offset.y],"h_offset":camera.h_offset,"v_offset":camera.v_offset}
    return context

func _source_identity(use_startup_hashes := false) -> Dictionary:
    var hashes: Dictionary=export_identity_cache.get("hashes_sha256",{}) if use_startup_hashes else {}
    if not use_startup_hashes:
        for path in ["res://scripts/debug/profile_debug.gd","res://project.godot","res://bin/ps2_port.dll","res://bin/ps2_port.so","res://bin/ps2_port.dylib"]:
            if FileAccess.file_exists(path):hashes[path]=FileAccess.get_sha256(path)
    return {"hashes_sha256":hashes,"hash_capture_phase":"profiler_startup" if use_startup_hashes else "synchronous_export","executable":OS.get_executable_path(),"engine_debug_build":OS.is_debug_build(),"extension_build":"identify_by_binary_sha256","arguments":OS.get_cmdline_args(),"user_arguments":OS.get_cmdline_user_args(),"compact_history":compact_history,"ui_bulk_upload":ui_bulk_upload,"async_export":async_export,"sun_gpu_telemetry":sun_telemetry_enabled,"vsync_mode":DisplayServer.window_get_vsync_mode(),"max_fps":Engine.max_fps}

# Worker validation inspects only owned Variant types/containers. It never calls
# an Object/Resource from a sample, even for string conversion or serialization.
static func _export_has_object(value: Variant) -> bool:
    if typeof(value)==TYPE_OBJECT:return true
    if value is Dictionary:
        for key in value:
            if _export_has_object(key) or _export_has_object(value[key]):return true
    elif value is Array:
        for item in value:
            if _export_has_object(item):return true
    return false

# This callable receives a detached snapshot, never a Node/renderer/camera.
# All FileAccess instances are created, used and closed on this worker only.
static func _write_export_snapshot(snapshot: Dictionary, path: String, temporary: String) -> Dictionary:
    if _export_has_object(snapshot):return {"ok":false,"path":path,"error":"Profile export snapshot contains an Object"}
    var error := DirAccess.make_dir_recursive_absolute(path.get_base_dir())
    if error!=OK:return {"ok":false,"path":path,"error":"Cannot create profile export directory (error %d)"%error}
    var file := FileAccess.open(temporary,FileAccess.WRITE)
    if file==null:return {"ok":false,"path":path,"error":"Cannot open profile export temporary file (error %d)"%FileAccess.get_open_error()}
    file.store_string(JSON.stringify(snapshot,"  "))
    file.flush()
    error=file.get_error()
    var bytes := file.get_position()
    file.close()
    if error==OK:error=DirAccess.rename_absolute(temporary,path)
    if error!=OK:
        DirAccess.remove_absolute(temporary)
        return {"ok":false,"path":path,"error":"Cannot commit profile export (error %d)"%error}
    return {"ok":true,"path":path,"bytes":bytes,"error":""}

func _pending_export_path() -> String:
    if not export_job.is_empty() and int(export_job.generation)==capture_generation:return str(export_job.path)
    for job in export_queue:
        if int(job.generation)==capture_generation:return str(job.path)
    return ""

func _complete_export_job(result: Dictionary) -> void:
    export_last_result=result.duplicate()
    if bool(result.get("ok",false)):
        export_completed_output=str(result.path)
        print("PROFILE_DEBUG_EXPORTED ",result.path)
    else:push_error("Track profile export failed: "+str(result.get("error","Unknown worker error")))
    if int(export_job.get("generation",-1))==capture_generation:
        exported=bool(result.get("ok",false))
        export_error=str(result.get("error",""))
        export_status="complete" if exported else "failed"
    export_job={}

func _start_export_job() -> void:
    if export_thread!=null or export_queue.is_empty():return
    export_job=export_queue.pop_front()
    export_thread=Thread.new()
    var error := export_thread.start(_write_export_snapshot.bind(export_job.snapshot,str(export_job.path),str(export_job.temporary)),Thread.PRIORITY_LOW)
    if error!=OK:
        export_thread=null
        _complete_export_job({"ok":false,"path":str(export_job.path),"error":"Cannot start profile export worker (error %d)"%error})
    elif int(export_job.generation)==capture_generation:export_status="writing"

func _poll_exports(drain := false) -> void:
    while true:
        if export_thread!=null:
            # Join only finished work during play. Shutdown explicitly drains its
            # owned jobs before the script/Thread objects can be destroyed.
            if not drain and export_thread.is_alive():return
            var result: Dictionary=export_thread.wait_to_finish()
            export_thread=null
            _complete_export_job(result)
        _start_export_job()
        if not drain or (export_thread==null and export_queue.is_empty()):return

class FunctionScope extends RefCounted:
    var observer: WeakRef
    var function_name: String
    var started: int
    func _init(owner: Node, label: String) -> void:
        observer=weakref(owner)
        function_name=label
        started=Time.get_ticks_usec()
    func _notification(what: int) -> void:
        if what==NOTIFICATION_PREDELETE:
            var owner=observer.get_ref()
            if is_instance_valid(owner):owner.record_function(function_name,(Time.get_ticks_usec()-started)/1000.0)

func scope(function_name: String) -> RefCounted:
    return FunctionScope.new(self,function_name) if enabled else null

func call_traced(function_name: String, callable: Callable, arguments: Array = []):
    var _profile_scope := scope(function_name)
    return callable.callv(arguments)

func record_function(function_name: String, milliseconds: float) -> void:
    var row: Dictionary=function_current.get(function_name,{"name":function_name,"ms":0.0,"calls":0,"max_ms":0.0,"source":"script scope (inclusive wall)"})
    row.ms+=milliseconds
    row.calls+=1
    row.max_ms=maxf(row.max_ms,milliseconds)
    function_current[function_name]=row

func begin_capture(warm := 0, frames := 120, hold_export := false) -> void:
    capture_generation+=1
    export_error=""
    export_status="idle"
    samples.clear()
    exported=false
    capture_active=true
    export_held=hold_export
    capture_completed_process_frame=-1
    seen_frames=0
    warm_frames=maxi(0,warm)
    limit=clampi(frames,1,600)

func _ready() -> void:
    var arguments := OS.get_cmdline_user_args()
    compact_history=bool(ProjectSettings.get_setting("hp2/profiling/compact_history",false))
    ui_bulk_upload=bool(ProjectSettings.get_setting("hp2/profiling/ui_bulk_upload",false))
    async_export=bool(ProjectSettings.get_setting("hp2/profiling/async_export",false))
    for argument in arguments:
        if argument.begins_with("--profile-compact-history="):
            var value := argument.trim_prefix("--profile-compact-history=")
            if value not in ["true","false"]:
                push_error("Profile compact history requires true or false")
                get_tree().quit(1)
                return
            compact_history=value=="true"
        elif argument.begins_with("--profile-ui-bulk-upload="):
            var value := argument.trim_prefix("--profile-ui-bulk-upload=")
            if value not in ["true","false"]:
                push_error("Profile bulk upload requires true or false")
                get_tree().quit(1)
                return
            ui_bulk_upload=value=="true"
        elif argument.begins_with("--profile-async-export="):
            var value := argument.trim_prefix("--profile-async-export=")
            if value not in ["true","false"]:
                push_error("Profile async export requires true or false")
                get_tree().quit(1)
                return
            async_export=value=="true"
    sun_telemetry_enabled="--sun-gpu-telemetry=true" in arguments
    enabled = "--profile-debug" in arguments or "--track-profile" in arguments or EngineDebugger.is_active()
    ui_enabled=("--profile-debug" in arguments or "--track-profile" in arguments) and "--profile-debug-no-ui" not in arguments and "--track-profile-no-ui" not in arguments
    for argument in arguments:
        if argument.begins_with("--track-profile-output="):output=argument.trim_prefix("--track-profile-output=")
        if argument.begins_with("--track-profile-frames="):limit=clampi(int(argument.trim_prefix("--track-profile-frames=")),1,600)
        if argument.begins_with("--profile-debug-output="):output=argument.trim_prefix("--profile-debug-output=")
        if argument.begins_with("--profile-debug-frames="):limit=clampi(int(argument.trim_prefix("--profile-debug-frames=")),1,600)
    if not enabled:
        set_process(false)
        return
    if async_export:export_identity_cache=_source_identity()
    process_priority=10000
    get_tree().node_added.connect(_node_added)
    _discover(get_tree().root)
    for key in ["process_cpu_ms","physics_cpu_ms","native_ms","host_ms","main_submit_ms","post_submit_ms","particle_submit_ms","particle_simulation_ms","particle_packing_ms","particle_upload_ms","sun_readback_ms","draw_calls","primitives","particle_uploaded_bytes","depth_uniform_reads","collector_ms"]:
        var id := StringName("HP2/"+key)
        if not Performance.has_custom_monitor(id):
            Performance.add_custom_monitor(id,_monitor.bind(key))
            monitors.append(id)
    print("PROFILE_DEBUG_ENABLED global engine/viewport/function monitors; bounded samples=",limit)

func ui_state() -> Dictionary:
    if not enabled or not ui_enabled:return {"enabled":false,"ui_bulk_upload":ui_bulk_upload}
    return {"enabled":true,"ui_bulk_upload":ui_bulk_upload,"compact_history":compact_history,"async_export":async_export,"export_status":export_status,"export_pending":export_thread!=null or not export_queue.is_empty(),"export_error":export_error,"exported":exported,"engine_frame":Engine.get_process_frames(),"metrics":latest,"viewports":latest_viewports,"functions":latest_functions,"recording":capture_active,"recorded":samples.size(),"limit":limit,"output":export_completed_output if async_export and not exported else output,"scene":get_tree().current_scene.scene_file_path if get_tree().current_scene!=null else ""}

func capture_next_frames() -> void:
    if export_held:return # An active benchmark owns the capture boundary.
    output=ProjectSettings.globalize_path("user://debug/profile-debug-"+str(Time.get_ticks_usec())+".json")
    begin_capture(0,120)

func _monitor(key: String) -> float:
    return float(latest.get(key,0))

func _discover(node: Node) -> void:
    _node_added(node)
    for child in node.get_children():_discover(child)

func _node_added(node: Node) -> void:
    if node.is_class("Ps2TrackRenderer"):renderers.append(weakref(node))
    if node is Viewport:
        var rid: RID=node.get_viewport_rid()
        targets[rid]=weakref(node)
        RenderingServer.viewport_set_measure_render_time(rid,true)

func _process(_delta: float) -> void:
    var begin := Time.get_ticks_usec()
    if async_export or export_thread!=null or not export_queue.is_empty():_poll_exports()
    var renderer: Node3D
    for reference in renderers:
        var node=reference.get_ref()
        if is_instance_valid(node) and node.is_visible_in_tree():
            renderer=node
            break
    var state: Dictionary=renderer.get_state() if renderer!=null else {}
    if int(state.get("track_id",-1))!=last_generation:
        last_generation=int(state.get("track_id",-1))
        previous_upload=-1
        previous_uniforms=-1
        seen_frames=0
    var host: Dictionary=state.get("host_stats",{})
    var sun_diagnostic := _sample_sun_telemetry(renderer,host.get("sun",{}))
    var particle: Dictionary=host.get("particles",{})
    var post: Dictionary=host.get("postprocess",{})
    var costs: Dictionary=state.get("costs",{})
    var upload := int(particle.get("uploaded_bytes",0))
    var uniforms := int(post.get("uniform_reads",0))
    latest={"frame_wall_ms":_delta*1000.0,"process_cpu_ms":Performance.get_monitor(Performance.TIME_PROCESS)*1000.0,"physics_cpu_ms":Performance.get_monitor(Performance.TIME_PHYSICS_PROCESS)*1000.0,"draw_calls":int(Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME)),"primitives":int(Performance.get_monitor(Performance.RENDER_TOTAL_PRIMITIVES_IN_FRAME))}
    latest["sun_uncovered_async"]=sun_diagnostic
    if state.get("ok",false):
        latest.merge({"native_ms":state.get("native_frame_ms",0),"host_ms":state.get("host_submit_ms",0),"main_submit_ms":costs.get("main_submit_ms",0),"post_submit_ms":costs.get("post_submit_ms",0),"particle_submit_ms":costs.get("particle_submit_ms",0),"particle_simulation_ms":particle.get("simulation_ms",0),"particle_packing_ms":particle.get("packing_ms",0),"particle_upload_ms":particle.get("upload_ms",0),"sun_readback_ms":costs.get("previous_sun_readback_ms",0),"particle_uploaded_bytes":maxi(0,upload-previous_upload) if previous_upload>=0 else 0,"depth_uniform_reads":maxi(0,uniforms-previous_uniforms) if previous_uniforms>=0 else 0})
    latest_functions.clear()
    for row in function_current.values():latest_functions.append(row.duplicate())
    function_current.clear()
    for pair in [["Runtime::prepare_frame","native_ms"],["Ps2TrackRenderer::submit_frame","main_submit_ms"],["TrackPostprocess::update","post_submit_ms"],["TrackParticleRenderer::update","particle_submit_ms"],["TrackSunRenderer::complete_readback","sun_readback_ms"]]:
        if latest.has(pair[1]):latest_functions.append({"name":pair[0],"ms":latest[pair[1]],"calls":1,"max_ms":latest[pair[1]],"source":"native section wall (readback is previous frame)"})
    latest_functions.sort_custom(func(a: Dictionary,b: Dictionary):return float(a.ms)>float(b.ms))
    previous_upload=upload
    previous_uniforms=uniforms
    seen_frames+=1
    var rows: Array[Dictionary]=[]
    for rid in targets.keys():
        var viewport: Viewport=targets[rid].get_ref()
        if not is_instance_valid(viewport):
            targets.erase(rid)
            continue
        var timing: Dictionary=timestamp_rows.get(str(rid.get_id()),{})
        var role := str(viewport.name)
        for target in post.get("targets",[]):
            if str(rid)==str(target.rid):role=str(target.role)
        var scheduled := true
        if viewport is SubViewport:scheduled=viewport.render_target_update_mode!=SubViewport.UPDATE_DISABLED
        rows.append({"role":role,"rid":str(rid),"size":[int(viewport.get_visible_rect().size.x),int(viewport.get_visible_rect().size.y)],"scheduled":scheduled,"cpu_ms":timing.get("cpu_ms",null),"gpu_ms":timing.get("gpu_ms",null),"source_frame":timing.get("source_frame",null),"timing_status":timing.get("timing_status",timestamp_status),"draws":viewport.get_render_info(Viewport.RENDER_INFO_TYPE_VISIBLE,Viewport.RENDER_INFO_DRAW_CALLS_IN_FRAME)})
    # A completed sample is consumed once. Reusing it would create false plateaus.
    timestamp_rows={}
    timestamp_status="awaiting_render_thread_timestamps"
    if not timestamp_pending:
        timestamp_pending=true
        RenderingServer.call_on_render_thread(_read_timestamps_on_render_thread)
    latest["collector_ms"]=(Time.get_ticks_usec()-begin)/1000.0
    latest["frame_wall_ms"]=_delta*1000.0
    latest_viewports=rows
    if capture_active and not exported and seen_frames>warm_frames:
        samples.append({"engine_frame":Engine.get_frames_drawn(),"track":last_generation,"scene":get_tree().current_scene.scene_file_path if get_tree().current_scene!=null else "", "context":_capture_context(renderer,state),"metrics":latest.duplicate(),"viewports":rows,"functions":latest_functions.duplicate(true)})
        if samples.size()>=limit:
            capture_active=false
            capture_completed_process_frame=Engine.get_process_frames()
            export_status="ready"
            if not export_held:export_profile()

func export_profile() -> String:
    if export_held:return "" # Includes manual UI requests while timing is active.
    if async_export:
        _poll_exports()
        var pending_path := _pending_export_path()
        if not pending_path.is_empty():return pending_path
        if exported:return output
    if samples.is_empty():return ""
    if output.is_empty():output=ProjectSettings.globalize_path("user://debug/profile-debug-"+str(Time.get_unix_time_from_system())+".json")
    var file: FileAccess
    if not async_export:
        DirAccess.make_dir_recursive_absolute(output.get_base_dir())
        file=FileAccess.open(output,FileAccess.WRITE)
        if file==null:
            push_error("Track profile export failed: "+output)
            return ""
    var data := {"schema":1,"engine":Engine.get_version_info(),"api":preload("res://scripts/debug/render_api.gd").current(),"driver":RenderingServer.get_current_rendering_driver_name(),"rendering_method":RenderingServer.get_current_rendering_method(),"adapter":RenderingServer.get_video_adapter_name(),"resolution":[get_window().size.x,get_window().size.y],"samples":samples,"notes":"Native CPU wall times may include waits. Viewport GPU values are engine timestamp measurements from earlier completed frames; zero/disabled is unknown. Viewports are separate workloads, not an additive total frame estimate. Byte/read metrics are cumulative-counter deltas. Profiling overhead is recorded as collector_ms."}
    data["schema"]=2
    data["source_identity"]=_source_identity(async_export)
    data["timing_source"]="render_thread_RenderingDevice_completed_timestamp_pairs"
    data["notes"]="CPU wall entries may overlap/include GPU waits. Viewport timestamp pairs are read together on the rendering thread and tagged with RenderingDevice completed source_frame; this is not engine_frame. A source frame is consumed once, missing endpoints or repeated completion frames are unavailable. Current scheduled flags describe collection time, not the delayed source frame. Viewports must never be summed as total frame GPU time. Byte/read metrics are cumulative deltas; collector_ms excludes deferred render-thread query and export. Camera/effect context belongs to the CPU sample, not delayed GPU source_frame."
    if async_export:
        if export_queue.size()>=4:
            export_error="Profile export queue is full"
            export_status="failed"
            capture_active=false
            push_error(export_error)
            return ""
        var started := Time.get_ticks_usec()
        var snapshot := data.duplicate(true)
        var snapshot_ms := (Time.get_ticks_usec()-started)/1000.0
        export_request_sequence+=1
        snapshot["export"]={"async":true,"snapshot_clone_ms":snapshot_ms,"snapshot_process_frame":Engine.get_process_frames(),"capture_generation":capture_generation,"request":export_request_sequence,"worker_priority":"low","source_hashes":"captured_at_profiler_startup","completion":"worker writes and atomically renames; exported becomes true only after main-thread completion polling"}
        snapshot.make_read_only()
        export_queue.append({"snapshot":snapshot,"path":output,"temporary":output+".tmp-%d-%d"%[OS.get_process_id(),export_request_sequence],"generation":capture_generation})
        capture_active=false
        exported=false
        export_status="queued"
        export_error=""
        _start_export_job()
        return "" if export_status=="failed" else output
    file.store_string(JSON.stringify(data,"  "))
    exported=true
    capture_active=false
    export_status="complete"
    print("PROFILE_DEBUG_EXPORTED ",output)
    return output

func _exit_tree() -> void:
    export_held=false # Shutdown is outside benchmark acceptance; preserve captures.
    if enabled and not exported:export_profile()
    _poll_exports(true)
    if not enabled:return
    for rid in targets:
        if is_instance_valid(targets[rid].get_ref()):RenderingServer.viewport_set_measure_render_time(rid,false)
    for id in monitors:Performance.remove_custom_monitor(id)
