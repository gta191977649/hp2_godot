extends SceneTree
# TASK0071 extends the port-side benchmark; no PS2 equivalence claim.
const VISUAL_DT := 1.0 / 60.0
const FLAGS := ["resource_reuse", "particle_mesh_reuse", "instancing", "occlusion", "opaque_queue"]
var output := "user://track-performance.json"
var performance_context := "race"
var selected_car := "ELISE"
var car_explicit := false
var track_ids: Array[int] = []
var sample_count := 600
var duration_seconds := 0.0
var warm_count := 120
var repeat_count := 3
var profile := "baseline"
var mode := ""
var overrides: Dictionary = {}
var measured_viewports: Dictionary = {}
var results: Array = []
var all_catalog_tracks := false
var isolate_reflection := false
var scenario_selection := "default"
var particle_backend := "native"
var scenarios: Array[String] = []
var viewport_size := Vector2i(1920,1080)
var explicit_width := 0
var requested_height := 1080
var route_kind := "legacy"
var heading_degrees := 0.0
var performance_pose := "start"
var native_build := "unknown"
var port_ui_optimized_debug := false
var render_selection_optimized_debug := false
var render_host_optimized_debug := false
var window_minimized := false
var warm_signal_path := ""
var fixed_camera := false
var snapshot_path := ""
var snapshot_sha256 := ""
var profile_ui := false
var profiler_output_base := ""
var environment_capture := false # TASK0076 policy; opt-in pressure fixture only.
var run_identity: Dictionary = {}
var candidate_flags := {"track-submit-math":false,"track-vehicle-depth-staging":false,"track-residency-cache":false,"track-capture-world-batches":false,"track-depth-discovery-cache":false,"track-dynamic-plan-cache":false,"profile-async-export":false,"profile-ui-bulk-upload":false,"track-sun-gpu-resident":false,"track-sun-gpu-compare":false,"profile-compact-history":false,"sun-gpu-telemetry":false,"track-particle-transport-cache":true,"track-particle-state-cache":true,"track-post-consumer-cache":true,"track-sun-reduction":false,"track-sun-reduction-compare":false,"track-source-3d-skip":false}

func _scenario_settings(scenario: String) -> Dictionary:
 var settings := {"render_props":true,"fog":true,"particles":true,"sun":true,"environment":environment_capture,"dof":true}
 if scenario.begins_with("p") and scenario.length()==6:
  settings.render_props=scenario[1]=="1"
  settings.fog=scenario[3]=="1"
  settings.particles=scenario[5]=="1"
 elif scenario.ends_with("_off"):
  settings[scenario.trim_suffix("_off")]=false
 return settings

func _apply_scenario(renderer: Node, scenario: String) -> String:
 renderer.restore_render_defaults()
 if scenario=="default":
  renderer.set_effect_enabled("environment",environment_capture)
  return ""
 var controls: Dictionary=renderer.get_render_controls()
 if not controls.get("debug_controls",false) and not controls.get("performance_controls",false):
  return "Release scenario overrides require --track-performance-analysis"
 var settings := _scenario_settings(scenario)
 renderer.set_render_props(settings.render_props)
 for effect in ["fog","particles","sun","environment","dof"]:
  renderer.set_effect_enabled(effect,settings[effect])
 return ""

func _capture_name(track_id: int, repeat_index: int, scenario: String, suffix: String="") -> String:
 var tag := "" if scenario=="default" else "-"+scenario
 if performance_pose!="start": tag+="-"+performance_pose
 return output.get_base_dir().path_join("track-%d-repeat-%d%s%s.png"%[track_id,repeat_index,tag,suffix])

func _initialize() -> void:
 _run.call_deferred()

func _viewports(node: Node) -> void:
 if node is Viewport:
  var rid: RID=node.get_viewport_rid()
  measured_viewports[rid]=node
  RenderingServer.viewport_set_measure_render_time(rid,true)
 for child in node.get_children():
  _viewports(child)

func _percentile(values: Array, fraction: float) -> float:
 values.sort()
 return float(values[clampi(int(ceil(fraction*values.size()))-1,0,values.size()-1)])

func _summary(samples: Array) -> Dictionary:
 var result := {}
 var fields: Array=["wall_ms","prepare_ms","native_ms","submit_ms","viewport_cpu_ms","viewport_gpu_ms","draw_calls","triangles","active_view_count","video_memory_bytes"]
 for sample in samples:
  for key in sample:
   if str(key).begins_with("cost_") and not fields.has(key): fields.append(key)
 for field in fields:
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
 var document := {"task":"TASK0072" if viewport_size==Vector2i(2880,1695) else "TASK0071","route_kind":route_kind,"context":performance_context,"selected_car":selected_car,"external_camera_input":false if performance_context in ["viewer","driving"] else null,"duration_seconds":duration_seconds,"long_run_stress":duration_seconds>0,"long_run_storage":"Compact per-frame numeric counters/costs/driving evidence; warmup timeline only; first requested sample_count profiler frames" if duration_seconds>0 else "Full bounded matrix samples","driving_resolution_protocol":_driving_protocol(),"driving_route_validation":"Port deterministic driving route; pending actual GPU/paired validation","context_provenance":"Actual AssetViewer driving_lab Test drive; continuous native physics and production car/reflection/skids; semantic throttle0.35,steer0" if performance_context=="driving" else "Actual AssetViewer TrackViewer free browsing; hidden native car retained; driving inactive; no environment capture origin" if performance_context=="viewer" else "Runtime race scene; paused physics; live vehicle environment capture","heading_degrees":heading_degrees,"pose_provenance":"Synthetic port-side camera; screenshot position only, original basis unknown" if performance_pose=="user_capture" else "Existing port-side benchmark pose","native_resolution_protocol":performance_context=="viewer" and not window_minimized and viewport_size==Vector2i(2880,1695) and warm_count==120 and sample_count==600 and repeat_count==3 and native_build=="debug" and profile_ui and bool(root.get_node("ProfileDebug").enabled) and RenderingServer.get_current_rendering_method()=="forward_plus" and RenderingServer.get_current_rendering_driver_name()=="vulkan" and track_ids==[11] and route_kind in ["fixed","dense","short","short_spin","rotate"],"fixed_camera":fixed_camera,"snapshot_path":snapshot_path,"snapshot_sha256":snapshot_sha256,"profiler_enabled":bool(root.get_node("ProfileDebug").enabled),"profiler_ui":profile_ui,"viewport":[viewport_size.x,viewport_size.y],"pose":performance_pose,"warm_signal_path":warm_signal_path,"selected_track_ids":track_ids,"scenario_selection":scenario_selection,"scenarios":scenarios,"rendering_method":RenderingServer.get_current_rendering_method(),"rendering_driver":RenderingServer.get_current_rendering_driver_name(),"adapter":RenderingServer.get_video_adapter_name(),"adapter_vendor":RenderingServer.get_video_adapter_vendor(),"os":OS.get_name(),"build":native_build,"port_ui_optimized_debug":port_ui_optimized_debug,"render_selection_optimized_debug":render_selection_optimized_debug,"render_host_optimized_debug":render_host_optimized_debug,"engine_debug":OS.has_feature("debug"),"render_prepare_workers":int(_user_arg("--render-prepare-workers=","0")),"render_thread_requested":_engine_render_thread(),"candidate_flags":candidate_flags,"window_minimized":window_minimized,"window_timing_policy":"Minimized windows abort a route; no minimized timing accepted. Window focus is not forced.","vsync":false,"primary_metric":"wall_ms","route":route_kind+"; continuous actual Test drive; elapsed1/60; native dt recorded" if performance_context=="driving" else (route_kind+"; fixed pose" if fixed_camera else route_kind+"; deterministic port camera route; physics paused"),"visual_dt":VISUAL_DT,"warm_frames":warm_count,"sample_frames":sample_count,"repeats":repeat_count,"acceptance_protocol":performance_context=="race" and not window_minimized and viewport_size==Vector2i(1920,1080) and warm_count==120 and sample_count==600 and repeat_count==3,"all_catalog_tracks":all_catalog_tracks,"isolate_reflection_capture":isolate_reflection,"profile":profile,"mode":mode,"overrides":overrides,"packet_parity":"No digest computation requested in timed loops; empty packet_digest is unavailable. Run separate parity checks.","gpu_timing":"TASK72 direct CPU/GPU viewport query metrics are unavailable without source-frame validation; raw_unverified fields are diagnostic only. TASK71 legacy delayed root readings remain non-additive and are not whole-frame totals. Consult aligned ProfileDebug exports for independently validated timestamps.","engine":Engine.get_version_info(),"tracks":results}
 document["schema"]=3
 document["particle_backend"]=particle_backend
 document["run_identity"]=run_identity
 document["environment_capture_requested"]=environment_capture
 document["measurement_protocol"]="task0077-export-separated-v1"
 file.store_string(JSON.stringify(document,"  "))

func _user_arg(prefix: String,fallback: String) -> String:
 for arg in OS.get_cmdline_user_args():
  if arg.begins_with(prefix):return arg.trim_prefix(prefix)
 return fallback

func _engine_render_thread() -> String:
 # Godot consumes --render-thread before exposing its remaining engine arguments.
 return _user_arg("--render-thread-requested=","project")

func _driving_protocol() -> bool:
 return duration_seconds==0 and performance_context=="driving" and route_kind in ["drive","drive_spin"] and not window_minimized and viewport_size==Vector2i(2880,1695) and warm_count==120 and sample_count==600 and repeat_count==3 and native_build=="debug" and profile_ui and bool(root.get_node("ProfileDebug").enabled) and RenderingServer.get_current_rendering_method()=="forward_plus" and RenderingServer.get_current_rendering_driver_name()=="vulkan" and track_ids==[11]

func _pipeline_counts() -> Dictionary:
 # Exact monitor constants from pinned Godot4.7.2 generated Performance headers.
 return {"canvas":int(Performance.get_monitor(Performance.PIPELINE_COMPILATIONS_CANVAS)),"mesh":int(Performance.get_monitor(Performance.PIPELINE_COMPILATIONS_MESH)),"surface":int(Performance.get_monitor(Performance.PIPELINE_COMPILATIONS_SURFACE)),"draw":int(Performance.get_monitor(Performance.PIPELINE_COMPILATIONS_DRAW)),"specialization":int(Performance.get_monitor(Performance.PIPELINE_COMPILATIONS_SPECIALIZATION))}

static func pipeline_delta(before: Dictionary, after: Dictionary) -> Dictionary:
 if before.is_empty() or after.is_empty(): return {"status":"unavailable","delta":null}
 var values := {}
 for key in before:
  if not after.has(key) or int(after[key])<int(before[key]): return {"status":"counter_reset_or_missing","before":before,"after":after,"delta":null}
  values[key]=int(after[key])-int(before[key])
 return {"status":"monotonic_engine_counters","before":before,"after":after,"delta":values}

static func accumulate_driving_evidence(summary: Dictionary, evidence: Dictionary) -> void:
 var position: Array=evidence.position_host
 if summary.is_empty():
  summary.merge({"frames":0,"position_min_host":position.duplicate(),"position_max_host":position.duplicate(),"all_contacts_known_frames":0,"zero_known_contacts_frames":0,"unknown_contact_frames":0,"contact_count_histogram":{},"terrain_samples":{},"speed_min":null,"speed_max":null,"route_membership":"Not established; finite physics and contact counts do not prove the vehicle stayed on the intended road"})
 summary.frames+=1
 for axis in 3:
  summary.position_min_host[axis]=minf(float(summary.position_min_host[axis]),float(position[axis]))
  summary.position_max_host[axis]=maxf(float(summary.position_max_host[axis]),float(position[axis]))
 var known := 0
 var contacts := 0
 for wheel in evidence.get("wheels",[]):
  if wheel.get("contact")!=null:
   known+=1
   if bool(wheel.contact): contacts+=1
  if wheel.get("terrain")!=null:
   var key := str(wheel.terrain)
   summary.terrain_samples[key]=int(summary.terrain_samples.get(key,0))+1
 if known==4:
  summary.all_contacts_known_frames+=1
  if contacts==0: summary.zero_known_contacts_frames+=1
  var key := str(contacts)
  summary.contact_count_histogram[key]=int(summary.contact_count_histogram.get(key,0))+1
 else: summary.unknown_contact_frames+=1
 var speed=evidence.get("speed")
 if speed!=null and is_finite(float(speed)):
  summary.speed_min=float(speed) if summary.speed_min==null else minf(float(summary.speed_min),float(speed))
  summary.speed_max=float(speed) if summary.speed_max==null else maxf(float(summary.speed_max),float(speed))

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
 var script: Script=load("res://scripts/runtime/track_performance_viewer.gd" if performance_context in ["viewer","driving"] else "res://scripts/runtime/track_performance_scene.gd")
 if script==null:return null
 var scene: Node=script.new()
 if performance_context in ["viewer","driving"]:
  scene.benchmark_context=performance_context
  scene.benchmark_route=route_kind
  scene.benchmark_viewport=viewport_size
  scene.benchmark_car_code=selected_car
 root.add_child(scene)
 current_scene=scene
 scene.set_process(false)
 if scene.track_renderer!=null and scene.track_renderer.has_method("get_render_controls"):
  var controls: Dictionary=scene.track_renderer.get_render_controls()
  port_ui_optimized_debug=bool(controls.get("port_ui_optimized_debug",false))
  render_selection_optimized_debug=bool(controls.get("render_selection_optimized_debug",false))
  render_host_optimized_debug=bool(controls.get("render_host_optimized_debug",false))
  native_build="debug" if controls.get("debug_controls",false) else "release"
 return scene

func _dispose(scene: Node) -> void:
 scene._release()
 scene.queue_free()
 await process_frame
 if DisplayServer.window_get_mode()!=DisplayServer.WINDOW_MODE_MINIMIZED:
  await RenderingServer.frame_post_draw

func _finish_profile_export(profiler: Node, row: Dictionary) -> void:
 if not profiler.enabled: return
 profiler.capture_active=false
 # No snapshot, source hashing, serialization or export dispatch before this
 # boundary. A stress run may have reached the Profile limit much earlier.
 if not row.has("profile_export_started_usec"):
  row.profile_export_started_usec=Time.get_ticks_usec()
  row.profile_export_was_held=bool(profiler.export_held)
  row.profile_capture_completed_process_frame=profiler.capture_completed_process_frame
  profiler.export_held=false
  var dispatch_begin := Time.get_ticks_usec()
  profiler.export_profile()
  row.profile_export_dispatch_ms=(Time.get_ticks_usec()-dispatch_begin)/1000.0
 var begin := Time.get_ticks_msec()
 var deadline := begin+30000
 # Completion waits are outside all timed frames and after end-of-route captures.
 # Poll only finished jobs; never call the blocking shutdown drain here.
 while not profiler.exported and str(profiler.export_status) in ["queued","writing"] and Time.get_ticks_msec()<deadline:
  await process_frame
  profiler._poll_exports()
 row.profile_async_export=bool(profiler.async_export)
 row.profile_export_status=str(profiler.export_status)
 row.profile_export_error=str(profiler.export_error)
 row.profile_export_pending=profiler.export_thread!=null or not profiler.export_queue.is_empty()
 row.profile_export_wait_ms=Time.get_ticks_msec()-begin
 row.profile_exported=bool(profiler.exported)
 row.profile_output=profiler.output
 row.profile_samples=profiler.samples.size()
 row.profile_export_total_ms=(Time.get_ticks_usec()-int(row.profile_export_started_usec))/1000.0
 row.export_outside_measurement=int(row.profile_export_started_usec)>=int(row.measurement_end_usec) and bool(row.profile_export_was_held)
 if not row.export_outside_measurement or row.profile_samples!=mini(sample_count,600):
  row.timing_valid=false
  row.error="Profile capture count/export boundary mismatch"
 if not profiler.exported:
  row.error=str(row.get("error",""))+" Profile export incomplete/failed after untimed completion wait: "+str(profiler.export_status)+" "+str(profiler.export_error)
  row.timing_valid=false
  row.frame_budget_met=false
  row.long_run_complete=false

func _repeat(track_id: int, repeat_index: int, scenario: String) -> Dictionary:
 var profiler := root.get_node("ProfileDebug")
 if profiler.enabled: profiler.capture_active=false
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
 var scenario_error := _apply_scenario(renderer,scenario)
 if not scenario_error.is_empty():
  await _dispose(benchmark)
  return {"track":track_id,"repeat":repeat_index,"scenario":scenario,"error":scenario_error}
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
 if performance_context=="race":
  benchmark.simulation.action("pause")
  for child in benchmark.get_children():
   if child is CanvasLayer: child.hide()
 var camera: Camera3D=benchmark.camera
 benchmark._update_camera()
 # Port-side diagnostic pose from the existing original-particle GPU fixture.
 if performance_context!="driving" and performance_pose=="forest_fire":
  camera.position=Vector3(170,-48,-640)
  camera.look_at(Vector3(190.068,-53.8522,-654.923))
 if performance_context!="driving" and performance_pose=="user_capture":
  # User supplied position only; heading is synthetic, never original camera evidence.
  camera.position=Vector3(-455.37,-34.04,-238.34)
  var heading := deg_to_rad(heading_degrees)
  camera.look_at(camera.position+Vector3(sin(heading),0,cos(heading))*12.0)
 if not snapshot_path.is_empty():
  var restored: String=preload("res://scripts/debug/track_render_diagnostics.gd").new().restore_snapshot(camera,renderer,snapshot_path)
  if not restored.begins_with("Restored diagnostic"):
   await _dispose(benchmark)
   return {"track":track_id,"repeat":repeat_index,"error":restored}
  # Explicit policy also overrides restored environment capture permission.
  scenario_error=_apply_scenario(renderer,scenario)
  if not scenario_error.is_empty():
   await _dispose(benchmark)
   return {"track":track_id,"repeat":repeat_index,"error":scenario_error}
 var actual_viewport_size := Vector2i(root.get_visible_rect().size)
 if viewport_size==Vector2i(2880,1695) and actual_viewport_size!=viewport_size:
  await _dispose(benchmark)
  return {"track":track_id,"repeat":repeat_index,"error":"Native-resolution viewport was constrained: "+str(actual_viewport_size),"actual_viewport":[actual_viewport_size.x,actual_viewport_size.y]}
 var base_transform := camera.transform
 var base_position := camera.position
 var forward := -camera.basis.z
 var right := camera.basis.x
 var target := Vector3(190.068,-53.8522,-654.923) if performance_pose=="forest_fire" else base_position+forward*12.0
 var pre_measurement_snapshot := ""
 var driving_timeline: Array=[]
 var driving_summary: Dictionary={}
 var warm_end_step := -1
 var measurement_started_usec := 0
 var samples: Array=[]
 var route_error := ""
 measured_viewports.clear()
 var frame := 0
 while frame<warm_count+sample_count or (duration_seconds>0 and measurement_started_usec>0 and float(Time.get_ticks_usec()-measurement_started_usec)/1000000.0<duration_seconds):
  if DisplayServer.window_get_mode()==DisplayServer.WINDOW_MODE_MINIMIZED:
   window_minimized=true
   route_error="benchmark window minimized; timing invalid"
   break
  if frame==warm_count-2: _viewports(root)
  if frame==warm_count: measurement_started_usec=Time.get_ticks_usec()
  if frame==warm_count and profiler.enabled:
   profiler.output=profiler_output_base.get_basename()+"-track-%d-%s-repeat-%d.json"%[track_id,scenario,repeat_index]
   profiler.begin_capture(0,sample_count,true)
  var begin := Time.get_ticks_usec()
  # Traverse the complete route during warmup; do not compile later views only
  # after sampling begins. Both phases cover the same camera path.
  var route_frame := frame if frame<warm_count else frame-warm_count
  var route_frames := warm_count if frame<warm_count else sample_count
  var phase := float(route_frame)/float(route_frames-1)
  var advance := forward*(1.0-cos(phase*TAU))*6.0
  if performance_context=="driving":
   pass # Production driving_lab owns both car and camera continuously across phases.
  elif fixed_camera:
   camera.transform=base_transform
  elif route_kind=="rotate":
   camera.transform=base_transform
   camera.rotate_object_local(Vector3.UP,phase*TAU*2.0)
  elif route_kind=="dense":
   # In-place angular sweep probes nearby view density without changing visibility rules.
   camera.transform=base_transform
   camera.rotate_object_local(Vector3.UP,deg_to_rad(25.0)*sin(phase*TAU))
  else:
   camera.position=base_position+right*sin(phase*TAU)*6.0+advance
   camera.look_at(target+advance)
   if route_kind=="short_spin":
    # Port-side stress fixture: traverse the existing closed short route while
    # turning through two full revolutions. This does not simulate driving.
    camera.basis=base_transform.basis
    camera.rotate_object_local(Vector3.UP,phase*TAU*2.0)
  var state: Dictionary
  var pipeline_before := _pipeline_counts()
  if performance_context in ["viewer","driving"]:
   state=benchmark.benchmark_frame(VISUAL_DT)
  else:
   renderer.set_visual_state(benchmark.vehicle_renderer.get_visual_state())
   renderer.set_visual_timestep(VISUAL_DT)
   state=renderer.update_view(camera.position)
   if state.get("ok",false):
    benchmark.vehicle_renderer.set_environment_capture(renderer.get_environment_capture())
    var car_state: Dictionary=benchmark.vehicle_renderer.update_view(camera.position,-camera.basis.z,float(state.original_focal_h),-1)
    if not car_state.get("ok",false):
     route_error=str(car_state.get("error","Original vehicle failed"))
     break
  if not state.get("ok",false):
   route_error=str(state.get("error","Original renderer failed"))
   break
  var prepare_ms := float(Time.get_ticks_usec()-begin)/1000.0
  var active: Array=state.get("active_viewport_rids",[])
  await process_frame
  if DisplayServer.window_get_mode()==DisplayServer.WINDOW_MODE_MINIMIZED:
   window_minimized=true
   route_error="benchmark window minimized; timing invalid"
   break
  await RenderingServer.frame_post_draw
  if DisplayServer.window_get_mode()==DisplayServer.WINDOW_MODE_MINIMIZED:
   window_minimized=true
   route_error="benchmark window minimized; timing invalid"
   break
  var completed_wall_ms := float(Time.get_ticks_usec()-begin)/1000.0
  var pipeline_sample := pipeline_delta(pipeline_before,_pipeline_counts())
  var driving_evidence: Dictionary={}
  if performance_context=="driving":
   driving_evidence=benchmark.last_driving_frame.duplicate(true)
   accumulate_driving_evidence(driving_summary,driving_evidence)
   driving_evidence["stage"]="warm" if frame<warm_count else "measured"
   driving_evidence["pipeline"]=pipeline_sample
   if duration_seconds==0 or frame<warm_count: driving_timeline.append(driving_evidence)
   if frame==warm_count-1: warm_end_step=int(driving_evidence.step_after)
  if frame>=warm_count:
   var wall_ms := completed_wall_ms
   var root_rid := root.get_viewport_rid()
   var cpu := RenderingServer.viewport_get_measured_render_time_cpu(root_rid)
   var gpu := RenderingServer.viewport_get_measured_render_time_gpu(root_rid)
   var valid := false # Direct queries have no source-frame provenance at any size.
   var seen := {}
   var viewport_rows: Array=[]
   for rid in active:
    if typeof(rid)!=TYPE_RID or not measured_viewports.has(rid): continue
    if seen.has(rid): continue
    seen[rid]=true
    var c := RenderingServer.viewport_get_measured_render_time_cpu(rid)
    var g := RenderingServer.viewport_get_measured_render_time_gpu(rid)
    var timer_valid := false # Keep raw readings diagnostic-only at every size.
    var viewport: Viewport=measured_viewports[rid]
    var view_size := viewport.get_visible_rect().size
    viewport_rows.append({"path":str(viewport.get_path()),"size":[view_size.x,view_size.y],"cpu_ms":c if timer_valid else null,"gpu_ms":g if timer_valid else null,"timer_valid":timer_valid,"raw_unverified_cpu_ms":c,"raw_unverified_gpu_ms":g,"visible_draws":viewport.get_render_info(Viewport.RENDER_INFO_TYPE_VISIBLE,Viewport.RENDER_INFO_DRAW_CALLS_IN_FRAME),"canvas_draws":viewport.get_render_info(Viewport.RENDER_INFO_TYPE_CANVAS,Viewport.RENDER_INFO_DRAW_CALLS_IN_FRAME),"shadow_draws":viewport.get_render_info(Viewport.RENDER_INFO_TYPE_SHADOW,Viewport.RENDER_INFO_DRAW_CALLS_IN_FRAME)})
   # Read post-draw counters too: the Sun completion callback runs during drawing.
   var sampled_state: Dictionary=renderer.get_state()
   var sample := {"pipeline":pipeline_sample,"context":performance_context,"viewer_process_updates":benchmark.updates if performance_context in ["viewer","driving"] else null,"driving":driving_evidence,"wall_ms":wall_ms,"prepare_ms":prepare_ms,"native_ms":state.native_frame_ms,"submit_ms":state.host_submit_ms,"viewport_cpu_ms":cpu if valid else null,"viewport_gpu_ms":gpu if valid else null,"gpu_timer_valid":valid,"raw_unverified_viewport_cpu_ms":cpu,"raw_unverified_viewport_gpu_ms":gpu,"gpu_query_source_verified":false,"viewport_timing_scope":"root viewport only; delayed, not whole frame","active_view_count":seen.size(),"active_viewports":viewport_rows,"view_submissions":state.get("view_submissions",[]),"host_stats":sampled_state.get("host_stats",{}),"native_selection":sampled_state.get("lod",{}).get("native_culling_views",{}),"zone":sampled_state.get("zone"),"selected_sections":sampled_state.get("sections",[]),"considered_instances":sampled_state.get("considered_instances"),"selected_instances":sampled_state.get("visible_instances"),"triangles":Performance.get_monitor(Performance.RENDER_TOTAL_PRIMITIVES_IN_FRAME),"draw_calls":Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME),"video_memory_bytes":Performance.get_monitor(Performance.RENDER_VIDEO_MEM_USED),"commands":state.commands,"packet_digest":state.get("packet_digest",""),"frame":frame,"visual_time":(frame+1)*VISUAL_DT}
   var particle_stats: Dictionary=sample.get("host_stats",{}).get("particles",{})
   sample["particle_stats"]=particle_stats
   sample["particle_active"]=sampled_state.get("particles_active",0)
   for key in ["simulation_ms","packet_ms","packing_ms","upload_ms"]:
    if particle_stats.has(key): sample["cost_particle_"+key]=particle_stats[key]
   var costs: Dictionary=sampled_state.get("costs",{})
   for key in costs:
    if typeof(costs[key]) in [TYPE_FLOAT,TYPE_INT]: sample["cost_"+str(key)]=costs[key]
   if duration_seconds>0:
    # Stress runs retain full numeric frame evidence without repeating large host
    # resource dictionaries and viewport/submission lists for tens of thousands of frames.
    for key in ["host_stats","active_viewports","view_submissions"]: sample.erase(key)
   samples.append(sample)
   if samples.size()%120==0:
    print("TRACK_PERFORMANCE_PROGRESS track=",track_id," scenario=",scenario," repeat=",repeat_index," samples=",samples.size(),"/",sample_count)
  elif frame==warm_count-1:
   # Untimed, full pose/settings evidence before capture begins. No historical-state replay claim.
   var diagnostic: Dictionary=preload("res://scripts/debug/track_render_diagnostics.gd").new().sample(benchmark,camera,renderer,true)
   diagnostic["benchmark"]={"driving_startup":benchmark.startup_driving_state if performance_context=="driving" else {},"driving":driving_evidence,"warm_end_step":warm_end_step,"warm_measurement_boundary":"Continuous; no teleport, reset, input restart or camera route restart" if performance_context=="driving" else "Port camera path repeats after warmup","context":performance_context,"selected_car":selected_car,"route_kind":route_kind,"heading_degrees":heading_degrees,"pose":performance_pose,"pose_provenance":"Synthetic port-side pose; user position only; screenshot basis unknown" if performance_pose=="user_capture" else "Port benchmark pose","viewport":[viewport_size.x,viewport_size.y],"vsync_mode":DisplayServer.window_get_vsync_mode(),"max_fps":Engine.max_fps,"native_build":native_build,"candidate_flags":candidate_flags,"profile_ui":profile_ui}
   pre_measurement_snapshot=output.get_basename()+"-track-%d-%s-repeat-%d-snapshot.json"%[track_id,scenario,repeat_index]
   var snapshot_file := FileAccess.open(pre_measurement_snapshot,FileAccess.WRITE)
   if snapshot_file==null:
    route_error="Cannot write pre-measurement camera snapshot: "+pre_measurement_snapshot
    break
   snapshot_file.store_string(JSON.stringify(diagnostic,"  "))
   snapshot_file.close()
   if not warm_signal_path.is_empty():
    var signal_file := FileAccess.open(warm_signal_path,FileAccess.WRITE)
    if signal_file==null:
     route_error="Cannot write benchmark warm signal: "+warm_signal_path
     break
    signal_file.store_string(JSON.stringify({"track":track_id,"scenario":scenario,"repeat":repeat_index,"warm_frames":warm_count,"pose":performance_pose}))
    signal_file.close()
   print("TRACK_PERFORMANCE_WARMED track=",track_id," scenario=",scenario," repeat=",repeat_index," warm_frames=",warm_count)
  frame+=1
 var measurement_end_usec := Time.get_ticks_usec()
 if profiler.enabled:profiler.capture_active=false
 var measured_duration_seconds := float(measurement_end_usec-measurement_started_usec)/1000000.0 if measurement_started_usec>0 else 0.0
 var final_state: Dictionary=renderer.get_state()
 # Untimed end-of-route evidence; never compute the full digest inside samples.
 var logical_frame: Dictionary=renderer.frame_summary()
 var pose_bits := var_to_bytes([camera.global_transform,benchmark.vehicle_renderer.global_transform]).hex_encode()
 var row := {"driving_startup":benchmark.startup_driving_state if performance_context=="driving" else {},"driving_summary":driving_summary,"duration_seconds":duration_seconds,"measured_duration_seconds":measured_duration_seconds,"long_run_complete":duration_seconds>0 and route_error.is_empty() and samples.size()>=sample_count and measured_duration_seconds>=duration_seconds,"driving_timeline":driving_timeline,"warm_end_step":warm_end_step,"driving_elapsed":VISUAL_DT if performance_context=="driving" else null,"port_ui_optimized_debug":port_ui_optimized_debug,"render_selection_optimized_debug":render_selection_optimized_debug,"render_host_optimized_debug":render_host_optimized_debug,"context":performance_context,"selected_car":selected_car,"actual_viewport":[actual_viewport_size.x,actual_viewport_size.y],"pre_measurement_snapshot":pre_measurement_snapshot,"route_kind":route_kind,"heading_degrees":heading_degrees,"final_frame":logical_frame,"vehicle_code":selected_car,"pose_bits":pose_bits,"track":track_id,"repeat":repeat_index,"scenario":scenario,"pose":performance_pose,"window_minimized":window_minimized,"timing_valid":route_error.is_empty(),"scenario_settings":_scenario_settings(scenario),"load_ms":load_ms,"loading_max_frame_ms":load_max_ms,"completed_samples":samples.size(),"configuration":renderer.get_optimization_state() if renderer.has_method("get_optimization_state") else configuration,"summary":_summary(samples),"effects":renderer.get_effects(),"diagnostic":final_state.get("diagnostic",""),"submissions":final_state.get("view_submissions",[]),"host_stats":final_state.get("host_stats",{}),"costs":final_state.get("costs",{}),"particle_emitter_count":final_state.get("particle_emitter_count"),"particle_definition_count":final_state.get("particle_definition_count"),"samples":samples}
 row.measurement_end_usec=measurement_end_usec
 row.measurement_boundary="TASK0077: Profile export held until timed loop completes; export and captures excluded"
 if not samples.is_empty():
  row.frame_budget_met=route_error.is_empty() and samples.size()==sample_count and float(row.summary.wall_ms.median)<=1000.0/60.0 and float(row.summary.wall_ms.p95)<=1000.0/60.0
 if profiler.enabled:
  profiler.capture_active=false
  row.profile_output=profiler.output
  row.profile_samples=profiler.samples.size()
  row.profile_exported=profiler.exported
 if not route_error.is_empty(): row.error=route_error
 if window_minimized:
  # A minimized window may not emit frame_post_draw; do not await/read targets.
  await _finish_profile_export(profiler,row)
  await _dispose(benchmark)
  return row
 var capture := _capture_name(track_id,repeat_index,scenario)
 root.get_texture().get_image().save_png(capture)
 row.capture=capture
 if renderer.has_method("get_render_targets"):
  var targets: Dictionary=renderer.get_render_targets()
  for key in ["colour","native_depth"]:
   var texture: Texture2D=targets.get(key)
   if texture!=null:texture.get_image().save_png(_capture_name(track_id,repeat_index,scenario,"-"+key))
 # Untimed isolation evidence; original capture above retains all original effects.
 var environment: Dictionary=renderer.get_environment_capture()
 if environment.get("available",false):
  row.capture_matrix_bits=var_to_bytes(environment.get("hack_matrix_native")).hex_encode()
  var atlas: Texture2D=environment.get("texture")
  if atlas!=null:atlas.get_image().save_png(_capture_name(track_id,repeat_index,scenario,"-atlas"))
 if isolate_reflection:
  benchmark.vehicle_renderer.set_environment_capture({"enabled":false})
  renderer.set_visual_timestep(0.0)
  renderer.update_view(camera.position)
  await process_frame
  await RenderingServer.frame_post_draw
  root.get_texture().get_image().save_png(_capture_name(track_id,repeat_index,scenario,"-car-reflection-off"))
 await _finish_profile_export(profiler,row)
 await _dispose(benchmark)
 return row

func _run() -> void:
 var identity_path := _user_arg("--performance-identity=","")
 if not identity_path.is_empty():
  var identity_value=JSON.parse_string(FileAccess.get_file_as_string(identity_path))
  if not identity_value is Dictionary or identity_value.get("schema")!=1:
   push_error("Missing/invalid benchmark identity")
   quit(1)
   return
  run_identity=identity_value
 if DisplayServer.get_name()=="headless":
  push_error("Track performance requires a real GPU window")
  quit(1)
  return
 particle_backend=str(ProjectSettings.get_setting("hp2/rendering/particle_backend","native"))
 for argument in OS.get_cmdline_user_args():
  if argument.begins_with("--particle-backend="): particle_backend=argument.trim_prefix("--particle-backend=")
 candidate_flags["track-sun-gpu-resident"]=bool(ProjectSettings.get_setting("hp2/rendering/sun_gpu_resident",false))
 candidate_flags["profile-ui-bulk-upload"]=bool(root.get_node("ProfileDebug").ui_bulk_upload)
 candidate_flags["profile-compact-history"]=bool(root.get_node("ProfileDebug").compact_history)
 candidate_flags["profile-async-export"]=bool(root.get_node("ProfileDebug").async_export)
 for argument in OS.get_cmdline_user_args():
  for flag in candidate_flags:
   if argument=="--"+flag: candidate_flags[flag]=true
   elif argument.begins_with("--"+flag+"="): candidate_flags[flag]=argument.trim_prefix("--"+flag+"=").to_lower() in ["true","1","on"]
  if argument.begins_with("--performance-context="): performance_context=argument.trim_prefix("--performance-context=")
  elif argument.begins_with("--car="):
   selected_car=argument.trim_prefix("--car=")
   car_explicit=true
  elif argument.begins_with("--performance-output="): output=argument.trim_prefix("--performance-output=")
  elif argument.begins_with("--performance-track="): track_ids.append(int(argument.trim_prefix("--performance-track=")))
  elif argument.begins_with("--performance-duration-seconds="): duration_seconds=float(argument.trim_prefix("--performance-duration-seconds="))
  elif argument.begins_with("--performance-samples="): sample_count=maxi(10,int(argument.trim_prefix("--performance-samples=")))
  elif argument.begins_with("--performance-environment-capture="):
   var capture_value := argument.trim_prefix("--performance-environment-capture=")
   if capture_value not in ["true","false"]:
    push_error("Environment capture requires true or false")
    quit(1)
    return
   environment_capture=capture_value=="true"
  elif argument.begins_with("--performance-warm="): warm_count=maxi(3,int(argument.trim_prefix("--performance-warm=")))
  elif argument.begins_with("--performance-repeats="): repeat_count=maxi(1,int(argument.trim_prefix("--performance-repeats=")))
  elif argument.begins_with("--track-optimization-profile="): profile=argument.trim_prefix("--track-optimization-profile=")
  elif argument.begins_with("--performance-warm-signal="): warm_signal_path=argument.trim_prefix("--performance-warm-signal=")
  elif argument=="--performance-fixed": fixed_camera=true
  elif argument=="--performance-profile-ui": profile_ui=true
  elif argument.begins_with("--performance-snapshot="): snapshot_path=argument.trim_prefix("--performance-snapshot=")
  elif argument.begins_with("--performance-pose="): performance_pose=argument.trim_prefix("--performance-pose=")
  elif argument.begins_with("--performance-scenario="): scenario_selection=argument.trim_prefix("--performance-scenario=")
  elif argument.begins_with("--performance-width="): explicit_width=int(argument.trim_prefix("--performance-width="))
  elif argument.begins_with("--performance-height="): requested_height=int(argument.trim_prefix("--performance-height="))
  elif argument.begins_with("--performance-route="): route_kind=argument.trim_prefix("--performance-route=")
  elif argument.begins_with("--performance-heading="): heading_degrees=float(argument.trim_prefix("--performance-heading="))
  elif argument=="--performance-isolate-reflection": isolate_reflection=true
  elif argument.begins_with("--performance-mode="): mode=argument.trim_prefix("--performance-mode=")
  else:
   for flag in FLAGS:
    var prefix: String= "--"+flag.replace("_","-")+"="
    if argument.begins_with(prefix): overrides[flag]=argument.trim_prefix(prefix).to_lower() in ["true","1","on"]
 if performance_context in ["viewer","driving"] and not car_explicit: selected_car="CORVETTE"
 if performance_context not in ["race","viewer","driving"]:
  push_error("Performance context must be race, viewer or driving")
  quit(1)
  return
 if performance_context in ["viewer","driving"] and ("--viewer-smoke" in OS.get_cmdline_user_args() or isolate_reflection):
  push_error("Viewer benchmark cannot run the separate viewer-smoke runner or race reflection isolation")
  quit(1)
  return
 if not is_finite(duration_seconds) or duration_seconds<0 or duration_seconds>3600 or (duration_seconds>0 and performance_context!="driving"):
  push_error("Duration must be0..3600 seconds and nonzero is supported only for actual driving")
  quit(1)
  return
 if performance_context=="driving" and route_kind=="legacy": route_kind="drive"
 if performance_context=="driving" and (route_kind not in ["drive","drive_spin"] or performance_pose!="start" or not snapshot_path.is_empty() or fixed_camera or heading_degrees!=0):
  push_error("Driving context requires drive/drive_spin, start pose, no snapshot/fixed/heading override")
  quit(1)
  return
 if performance_context!="driving" and route_kind in ["drive","drive_spin"]:
  push_error("Drive routes require actual driving context")
  quit(1)
  return
 if requested_height not in [720,1080,1695] or explicit_width<0 or explicit_width>8192 or (explicit_width>0 and explicit_width<64):
  push_error("Performance dimensions require height720/1080/1695 and optional width64..8192")
  quit(1)
  return
 if requested_height==1695 and explicit_width!=2880:
  push_error("Native1695 height requires explicit --performance-width=2880")
  quit(1)
  return
 viewport_size=Vector2i(explicit_width if explicit_width>0 else (1280 if requested_height==720 else 1920),requested_height)
 if route_kind not in ["legacy","fixed","dense","short","short_spin","rotate","drive","drive_spin"] or not is_finite(heading_degrees):
  push_error("Unknown performance route or nonfinite heading")
  quit(1)
  return
 if route_kind=="fixed": fixed_camera=true
 if fixed_camera and route_kind in ["dense","short","short_spin","rotate"]:
  push_error("--performance-fixed conflicts with moving/angular route")
  quit(1)
  return
 if profile not in ["baseline","optimized"] or mode not in ["","baseline","optimized","reuse","multimesh","occlusion","opaque"]:
  push_error("Unknown optimization profile/mode")
  quit(1)
  return
 if not warm_signal_path.is_empty() and not warm_signal_path.is_absolute_path():
  push_error("Performance warm signal requires an absolute path")
  quit(1)
  return
 if not snapshot_path.is_empty():
  var snapshot=JSON.parse_string(FileAccess.get_file_as_string(snapshot_path))
  if not snapshot is Dictionary or snapshot.get("schema",0)!=1 or not snapshot.has("camera") or not snapshot.has("track"):
   push_error("Performance snapshot requires a schema-1 camera/track diagnostic")
   quit(1)
   return
  snapshot_sha256=FileAccess.get_sha256(snapshot_path)
  if track_ids.is_empty(): track_ids.append(int(snapshot.track))
  for selected_track in track_ids:
   if selected_track!=int(snapshot.track):
    push_error("Performance snapshot track differs from selected track")
    quit(1)
    return
 if performance_pose not in ["start","forest_fire","user_capture"]:
  push_error("Unknown performance pose: "+performance_pose)
  quit(1)
  return
 if performance_pose in ["forest_fire","user_capture"]:
  if track_ids.is_empty(): track_ids.append(11)
  for selected_track in track_ids:
   if selected_track!=11:
    push_error("The selected diagnostic performance pose requires Track11")
    quit(1)
    return
 var matrix: Array[String]=[]
 for props in 2:
  for fog in 2:
   for particles in 2:
    matrix.append("p%df%da%d"%[props,fog,particles])
 var allowed: Array[String]=["default","sun_off","environment_off","dof_off","particles_off"]
 allowed.append_array(matrix)
 if scenario_selection=="all":
  scenarios.assign(matrix+["sun_off","environment_off","dof_off"])
 elif scenario_selection in allowed: scenarios.append(scenario_selection)
 else:
  push_error("Unknown performance scenario: "+scenario_selection)
  quit(1)
  return
 DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_WINDOWED)
 OS.low_processor_usage_mode=false
 Engine.max_fps=0
 root.size=viewport_size
 DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
 var overlay := root.get_node_or_null("DebugTools")
 var profiler := root.get_node("ProfileDebug")
 if profile_ui and (not profiler.enabled or not profiler.ui_enabled):
  push_error("--performance-profile-ui requires --profile-debug without --profile-debug-no-ui")
  quit(1)
  return
 if profiler.enabled:
  profiler.capture_active=false
  profiler.ui_enabled=profile_ui
  profiler_output_base=profiler.output if not profiler.output.is_empty() else output.get_basename()+"-profile.json"
 if overlay!=null and not profile_ui:
  overlay.visible=false
  overlay.set_process(false)
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
  for scenario in scenarios:
   for repeat_index in repeat_count:
    var row: Dictionary=await _repeat(track_id,repeat_index,scenario)
    row.scenario=scenario
    results.append(row)
    if row.has("error"): failures+=1
    _write()
    if window_minimized:
     push_error("benchmark window minimized; timing invalid")
     quit(1)
     return
    print("TRACK_PERFORMANCE ",track_id," scenario=",scenario," repeat=",repeat_index," ",row.get("summary",row.get("error")))
 print("TRACK_PERFORMANCE_DONE failures=",failures," ",output)
 quit(0 if failures==0 else 1)

