extends SceneTree
# TASK0056 deterministic host benchmark; wall time is primary, not PS2 equivalence.
const VISUAL_DT := 1.0 / 60.0
const FLAGS := ["resource_reuse", "particle_mesh_reuse", "instancing", "occlusion", "opaque_queue"]
var output := "user://track-performance.json"
var track_ids: Array[int] = []
var sample_count := 600
var warm_count := 120
var repeat_count := 3
var profile := "baseline"
var mode := ""
var overrides: Dictionary = {}
var measured_viewports: Dictionary = {}
var results: Array = []
var all_catalog_tracks := false
var isolate_reflection := false
var driving := false

func _initialize() -> void:
 _run.call_deferred()

func _viewports(node: Node) -> void:
 if node is Viewport:
  var rid: RID=node.get_viewport_rid()
  measured_viewports[rid]=true
  RenderingServer.viewport_set_measure_render_time(rid,true)
 for child in node.get_children():
  _viewports(child)

func _percentile(values: Array, fraction: float) -> float:
 values.sort()
 return float(values[clampi(int(ceil(fraction*values.size()))-1,0,values.size()-1)])

func _summary(samples: Array) -> Dictionary:
 var result := {}
 for field in ["wall_ms","prepare_ms","native_ms","submit_ms","viewport_cpu_ms","viewport_gpu_ms","draw_calls","triangles","static_memory_bytes","video_memory_bytes"]:
  var values: Array=[]
  for sample in samples:
   if sample.get(field)!=null: values.append(sample[field])
  if values.is_empty(): result[field]={"available":false}
  else: result[field]={"available":true,"valid_samples":values.size(),"median":_percentile(values.duplicate(),0.5),"p95":_percentile(values.duplicate(),0.95),"p99":_percentile(values.duplicate(),0.99),"max":values.max()}
 return result

func _write() -> void:
 var file := FileAccess.open(output,FileAccess.WRITE)
 if file==null:
  push_error("Cannot write performance output: "+output)
  return
 file.store_string(JSON.stringify({"task":"TASK0056","viewport":[1920,1080],"vsync":false,"primary_metric":"wall_ms","route":"native driving with constant throttle0.35" if driving else "fixed start pose; deterministic lateral/forward camera loop","visual_dt":VISUAL_DT,"warm_frames":warm_count,"sample_frames":sample_count,"repeats":repeat_count,"exhaustive_all30_120_600_3":all_catalog_tracks and warm_count==120 and sample_count==600 and repeat_count==3,"all_catalog_tracks":all_catalog_tracks,"isolate_reflection_capture":isolate_reflection,"profile":profile,"mode":mode,"overrides":overrides,"packet_parity":"No digest computation requested in timed loops; empty packet_digest is unavailable. Run separate parity checks.","gpu_timing":"Only explicit per-frame active_viewport_rids; unavailable without fresh scheduling metadata or valid GPU timer. Never sums inactive retained timers.","engine":Engine.get_version_info(),"tracks":results},"  "))

func _options() -> Dictionary:
 var selected := mode if not mode.is_empty() else profile
 var options := {}
 for flag in FLAGS: options[flag]=false
 if selected in ["optimized","reuse","multimesh","occlusion","opaque"]:
  options.resource_reuse=true
  options.particle_mesh_reuse=true
 if selected=="multimesh": options.instancing=true
 if selected=="occlusion": options.occlusion=true
 if selected=="opaque": options.opaque_queue=true
 options.merge(overrides,true)
 return options

func _new_scene() -> Node:
 var script: Script=load("res://scripts/runtime/track_performance_scene.gd")
 if script==null:return null
 var scene: Node=script.new()
 root.add_child(scene)
 current_scene=scene
 scene.set_process(false)
 return scene

func _dispose(scene: Node) -> void:
 scene._release()
 scene.queue_free()
 await process_frame
 await RenderingServer.frame_post_draw

func _repeat(track_id: int, repeat_index: int) -> Dictionary:
 # Fresh original session resets emitter seeds, reflection schedule and history.
 seed(0) # Host PRNG reset; original emitter seeds reset by the new native session.
 var benchmark: Node=_new_scene()
 if benchmark==null:return {"track":track_id,"repeat":repeat_index,"error":"Benchmark runtime script failed to load"}
 var renderer: Node=benchmark.track_renderer
 var options := _options()
 var configuration: Dictionary={"legacy_frozen54":true,"effective":options}
 if renderer.has_method("configure_optimizations"):
  configuration=renderer.configure_optimizations(options)
  if configuration.has("ok") and not configuration.ok:
   await _dispose(benchmark)
   return {"track":track_id,"repeat":repeat_index,"error":"Optimization configuration rejected: "+str(configuration)}
 elif options.values().has(true) or profile!="baseline" or (not mode.is_empty() and mode!="baseline"):
  await _dispose(benchmark)
  return {"track":track_id,"repeat":repeat_index,"error":"Frozen TASK0054 DLL supports baseline measurements only"}
 benchmark.track_id=track_id
 benchmark.benchmark_start_allowed=true
 var load_begin := Time.get_ticks_usec()
 benchmark.begin_session()
 benchmark.set_process(true)
 var load_max_ms := 0.0
 var deadline := Time.get_ticks_msec()+180000
 while benchmark.phase=="Loading" and Time.get_ticks_msec()<deadline:
  var begin := Time.get_ticks_usec()
  await process_frame
  load_max_ms=maxf(load_max_ms,float(Time.get_ticks_usec()-begin)/1000.0)
 if benchmark.phase!="Running":
  var failed := {"track":track_id,"repeat":repeat_index,"error":benchmark.diagnostic,"load_ms":float(Time.get_ticks_usec()-load_begin)/1000.0}
  await _dispose(benchmark)
  return failed
 var load_ms := float(Time.get_ticks_usec()-load_begin)/1000.0
 benchmark.set_process(false)
 if not driving: benchmark.simulation.action("pause")
 for child in benchmark.get_children():
  if child is CanvasLayer: child.hide()
 renderer.restore_render_defaults()
 var camera: Camera3D=benchmark.camera
 benchmark._update_camera()
 var base_position := camera.position
 var forward := -camera.basis.z
 var right := camera.basis.x
 var target := base_position+forward*12.0
 var samples: Array=[]
 var route_error := ""
 measured_viewports.clear()
 for frame in warm_count+sample_count:
  if frame==warm_count-2: _viewports(root)
  var begin := Time.get_ticks_usec()
  # Traverse the complete route during warmup; do not compile later views only
  # after sampling begins. Both phases cover the same camera path.
  var route_frame := frame if frame<warm_count else frame-warm_count
  var route_frames := warm_count if frame<warm_count else sample_count
  if driving:
   if not benchmark.focused:
    benchmark.focused=true
    benchmark.simulation.action("run")
   var accepted: Dictionary=benchmark.simulation.set_driving_input(0.35,0,0,0,-1,0,false)
   if not accepted.get("ok",false):
    route_error=str(accepted)
    break
   benchmark.state=benchmark.simulation.tick(VISUAL_DT)
   if not benchmark._valid(benchmark.state):
    route_error=str(benchmark.state)
    break
   benchmark.vehicle_renderer.apply_driving_snapshot(benchmark.state)
   benchmark._present_frame(VISUAL_DT)
   if benchmark.phase!="Running":
    route_error=benchmark.diagnostic
    break
  else:
   var phase := float(route_frame)/float(route_frames-1)
   var advance := forward*(1.0-cos(phase*TAU))*6.0
   camera.position=base_position+right*sin(phase*TAU)*6.0+advance
   camera.look_at(target+advance)
   renderer.set_visual_state(benchmark.vehicle_renderer.get_visual_state())
   renderer.set_visual_timestep(VISUAL_DT)
   var state: Dictionary=renderer.update_view(camera.position)
   if not state.get("ok",false):
    route_error=str(state.get("error","Original renderer failed"))
    break
   benchmark.vehicle_renderer.set_environment_capture(renderer.get_environment_capture())
   var car_state: Dictionary=benchmark.vehicle_renderer.update_view(camera.position,-camera.basis.z,float(state.original_focal_h),-1)
   if not car_state.get("ok",false):
    route_error=str(car_state.get("error","Original vehicle failed"))
    break
  var state: Dictionary=benchmark.track_state if driving else renderer.get_state()
  var prepare_ms := float(Time.get_ticks_usec()-begin)/1000.0
  var active: Array=state.get("active_viewport_rids",[])
  await process_frame
  await RenderingServer.frame_post_draw
  if frame>=warm_count:
   var wall_ms := float(Time.get_ticks_usec()-begin)/1000.0
   var cpu := 0.0
   var gpu := 0.0
   var valid := not active.is_empty()
   var seen := {}
   for rid in active:
    if typeof(rid)!=TYPE_RID or not measured_viewports.has(rid): valid=false;break
    if seen.has(rid): continue
    seen[rid]=true
    var c := RenderingServer.viewport_get_measured_render_time_cpu(rid)
    var g := RenderingServer.viewport_get_measured_render_time_gpu(rid)
    if not is_finite(c) or not is_finite(g) or g<=0: valid=false;break
    cpu+=c
    gpu+=g
   samples.append({"wall_ms":wall_ms,"prepare_ms":prepare_ms,"native_ms":state.native_frame_ms,"submit_ms":state.host_submit_ms,"viewport_cpu_ms":cpu if valid else null,"viewport_gpu_ms":gpu if valid else null,"gpu_timer_valid":valid,"active_view_count":active.size(),"draw_calls":Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME),"triangles":Performance.get_monitor(Performance.RENDER_TOTAL_PRIMITIVES_IN_FRAME),"video_memory_bytes":Performance.get_monitor(Performance.RENDER_VIDEO_MEM_USED),"static_memory_bytes":Performance.get_monitor(Performance.MEMORY_STATIC),"commands":state.commands,"packet_digest":state.get("packet_digest",""),"frame":frame,"visual_time":(frame+1)*VISUAL_DT})
 var final_state: Dictionary=renderer.get_state()
 # Untimed end-of-route evidence; never compute the full digest inside samples.
 var logical_frame: Dictionary=renderer.frame_summary()
 var pose_bits := var_to_bytes([camera.global_transform,benchmark.vehicle_renderer.global_transform]).hex_encode()
 var row := {"final_frame":logical_frame,"lod":renderer.get_lod_state(),"driving":driving,"physics_step":benchmark.state.get("step",0),"pose_bits":pose_bits,"track":track_id,"repeat":repeat_index,"load_ms":load_ms,"loading_max_frame_ms":load_max_ms,"completed_samples":samples.size(),"configuration":renderer.get_optimization_state() if renderer.has_method("get_optimization_state") else configuration,"summary":_summary(samples),"effects":renderer.get_effects(),"diagnostic":final_state.get("diagnostic",""),"submissions":final_state.get("view_submissions",[]),"host_stats":final_state.get("host_stats",{}),"samples":samples}
 if not route_error.is_empty(): row.error=route_error
 var capture := output.get_base_dir().path_join("track-%d-repeat-%d.png"%[track_id,repeat_index])
 root.get_texture().get_image().save_png(capture)
 row.capture=capture
 if renderer.has_method("get_render_targets"):
  var targets: Dictionary=renderer.get_render_targets()
  for key in ["colour","native_depth"]:
   var texture: Texture2D=targets.get(key)
   if texture!=null:texture.get_image().save_png(output.get_base_dir().path_join("track-%d-repeat-%d-%s.png"%[track_id,repeat_index,key]))
 # Untimed isolation evidence; original capture above retains all original effects.
 var environment: Dictionary=renderer.get_environment_capture()
 if environment.get("available",false):
  row.capture_matrix_bits=var_to_bytes(environment.get("hack_matrix_native")).hex_encode()
  var atlas: Texture2D=environment.get("texture")
  if atlas!=null:atlas.get_image().save_png(output.get_base_dir().path_join("track-%d-repeat-%d-atlas.png"%[track_id,repeat_index]))
 if isolate_reflection:
  benchmark.vehicle_renderer.set_environment_capture({"enabled":false})
  renderer.set_visual_timestep(0.0)
  renderer.update_view(camera.position)
  await process_frame
  await RenderingServer.frame_post_draw
  root.get_texture().get_image().save_png(output.get_base_dir().path_join("track-%d-repeat-%d-car-reflection-off.png"%[track_id,repeat_index]))
 await _dispose(benchmark)
 return row

func _run() -> void:
 if DisplayServer.get_name()=="headless":
  push_error("Track performance requires a real GPU window")
  quit(1)
  return
 for argument in OS.get_cmdline_user_args():
  if argument.begins_with("--performance-output="): output=argument.trim_prefix("--performance-output=")
  elif argument.begins_with("--performance-track="): track_ids.append(int(argument.trim_prefix("--performance-track=")))
  elif argument.begins_with("--performance-samples="): sample_count=maxi(10,int(argument.trim_prefix("--performance-samples=")))
  elif argument.begins_with("--performance-warm="): warm_count=maxi(3,int(argument.trim_prefix("--performance-warm=")))
  elif argument.begins_with("--performance-repeats="): repeat_count=maxi(1,int(argument.trim_prefix("--performance-repeats=")))
  elif argument.begins_with("--track-optimization-profile="): profile=argument.trim_prefix("--track-optimization-profile=")
  elif argument=="--performance-isolate-reflection": isolate_reflection=true
  elif argument=="--performance-driving": driving=true
  elif argument.begins_with("--performance-mode="): mode=argument.trim_prefix("--performance-mode=")
  else:
   for flag in FLAGS:
    var prefix: String= "--"+flag.replace("_","-")+"="
    if argument.begins_with(prefix): overrides[flag]=argument.trim_prefix(prefix).to_lower() in ["true","1","on"]
 if profile not in ["baseline","optimized"] or mode not in ["","baseline","optimized","reuse","multimesh","occlusion","opaque"]:
  push_error("Unknown optimization profile/mode")
  quit(1)
  return
 root.size=Vector2i(1920,1080)
 DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
 var overlay := root.get_node_or_null("DebugTools")
 if overlay!=null: overlay.visible=false;overlay.set_process(false)
 if track_ids.is_empty():
  all_catalog_tracks=true
  var catalogue: Node=_new_scene()
  if catalogue==null:
   push_error("Benchmark runtime script failed to load")
   quit(1)
   return
  for entry in catalogue.track_renderer.get_catalog(): track_ids.append(int(entry.id))
  await _dispose(catalogue)
 var failures := 0
 for track_id in track_ids:
  for repeat_index in repeat_count:
   var row: Dictionary=await _repeat(track_id,repeat_index)
   results.append(row)
   if row.has("error"): failures+=1
   _write()
   print("TRACK_PERFORMANCE ",track_id," repeat=",repeat_index," ",row.get("summary",row.get("error")))
 print("TRACK_PERFORMANCE_DONE failures=",failures," ",output)
 quit(0 if failures==0 else 1)

