extends SceneTree
# Port-only untimed actual Test drive parity route; no original-runtime equivalence.
const SIZE := Vector2i(2880,1695)
const DT := 1.0/60.0
const CAPTURE_STEPS := [0,119,239,479,719]
var frame_limit := 720
var output := ""
var output_valid := false
var viewer: Node
var sessions: Array[Dictionary] = []
var captures: Array[Dictionary] = []
var timeline: Array[Dictionary] = []
var teardown: Array[Dictionary] = []
var failure := ""
var finished := false

func _initialize() -> void:
 _run.call_deferred()

func _write() -> bool:
 var file := FileAccess.open(output.path_join("run.json"),FileAccess.WRITE)
 if file==null:
  push_error("Driving parity report write failed: "+str(FileAccess.get_open_error()))
  return false
 file.store_string(JSON.stringify({"task":"TASK0072","fixture":"actual Test drive drive_spin parity; untimed","finished":finished,"error":failure,"engine":Engine.get_version_info(),"method":RenderingServer.get_current_rendering_method(),"driver":RenderingServer.get_current_rendering_driver_name(),"adapter":RenderingServer.get_video_adapter_name(),"requested_size":[SIZE.x,SIZE.y],"viewport_size":[root.size.x,root.size.y],"arguments":OS.get_cmdline_user_args(),"engine_arguments":OS.get_cmdline_args(),"track":11,"car":"CORVETTE","elapsed":DT,"main_steps":frame_limit,"diagnostic_short_run":frame_limit!=720,"captures_at":CAPTURE_STEPS,"sessions":sessions,"timeline":timeline,"captures":captures,"teardown":teardown,"limitations":"Port deterministic throttle0.35/steering0 and synthetic camera yaw; original route membership/equivalence unknown. Full recovered native timestep is recorded, never replaced. Images are raw texture bytes hashed before PNG conversion; no performance measurement."},"  "))
 return true

func _fail(message: String) -> void:
 failure=message
 push_error(message)
 if output_valid: _write()
 quit(1)

func _image(value: Image,path: String) -> Dictionary:
 if value==null or value.is_empty(): return {"error":"Missing driving parity image: "+path}
 var hash := HashingContext.new()
 hash.start(HashingContext.HASH_SHA256)
 hash.update(value.get_data())
 var result := {"sha256":hash.finish().hex_encode(),"width":value.get_width(),"height":value.get_height(),"format":value.get_format(),"raw_bytes":value.get_data().size(),"file":path}
 var saved := value.save_png(path)
 if saved!=OK: result.error="Driving parity image save failed: "+str(saved)+" "+path
 return result

func _hide_tools() -> void:
 for singleton in ["ProfileDebug","DebugTools"]:
  var node := root.get_node_or_null(singleton)
  if node==null: continue
  if node is CanvasLayer: node.hide()
  node.set_process(false)
  node.set_process_input(false)
  node.set_process_unhandled_input(false)
  if singleton=="ProfileDebug":
   node.enabled=false
   node.ui_enabled=false
   node.capture_active=false

func _start(session: int) -> bool:
 seed(0) # Host fixture PRNG only; recovered emitter/native state is untouched.
 var script: Script=load("res://scripts/runtime/track_performance_viewer.gd")
 if script==null:
  _fail("Driving parity wrapper could not load")
  return false
 viewer=script.new()
 viewer.benchmark_context="driving"
 viewer.benchmark_route="drive_spin"
 viewer.benchmark_viewport=SIZE
 viewer.benchmark_car_code="CORVETTE"
 viewer.track_id=11
 root.add_child(viewer)
 current_scene=viewer
 viewer.set_process(false)
 viewer.benchmark_start_allowed=true
 viewer.begin_session()
 var deadline := Time.get_ticks_msec()+180000
 while viewer.phase=="Loading" and Time.get_ticks_msec()<deadline:
  # Exactly the production loading path; no automatic Running process is allowed.
  viewer._process(0.0)
  await process_frame
  await RenderingServer.frame_post_draw
 if viewer.phase!="Running":
  _fail("Driving parity load/start failed: "+str(viewer.diagnostic))
  return false
 _hide_tools()
 viewer.track_renderer.set_diagnostics_level("summary")
 if root.size!=SIZE or Vector2i(root.get_visible_rect().size)!=SIZE:
  _fail("Driving parity viewport constrained: "+str(root.get_visible_rect().size))
  return false
 var controls: Dictionary=viewer.track_renderer.get_render_controls()
 if not bool(controls.get("render_ready",false)) or not bool(controls.get("debug_controls",false)):
  _fail("Driving parity requires a ready actual Debug Test drive")
  return false
 sessions.append({"session":session,"startup":viewer.startup_driving_state.duplicate(true),"controls":controls,"optimization":viewer.track_renderer.get_optimization_state(),"initial_step":viewer.driving.state.get("step")})
 return true

func _capture(session: int,index: int) -> bool:
 var renderer: Node3D=viewer.track_renderer
 var native: Dictionary=renderer.frame_summary()
 if not native.get("ok",false) or str(native.get("packet_digest","")).is_empty():
  _fail("Driving parity native digest unavailable at step%d"%index)
  return false
 var prefix := output.path_join("session-%d-step-%03d"%[session,index])
 var images := {"final":_image(root.get_texture().get_image(),prefix+"-final.png")}
 var targets: Dictionary=renderer.get_render_targets()
 for key in ["colour","native_depth"]:
  var texture: Texture2D=targets.get(key)
  images[key]=_image(texture.get_image() if texture!=null else null,prefix+"-"+key+".png")
 var faces: Array[Dictionary]=[]
 var face_nodes: Array[Node]=renderer.find_children("TrackReflectionFace*","SubViewport",true,false)
 for face_index in 6:
  var face: SubViewport
  for node in face_nodes:
   if str(node.name)=="TrackReflectionFace"+str(face_index): face=node as SubViewport;break
  if face==null or face.size!=Vector2i(64,64):
   _fail("Driving parity reflection face missing or wrong size: "+str(face_index))
   return false
  var face_camera: Camera3D=face.get_camera_3d()
  if face_camera==null:
   _fail("Driving parity reflection face camera missing: "+str(face_index))
   return false
  var submitted := false
  for view in native.get("lod",{}).get("native_culling_views",{}).values():
   if int(view.get("view",-1))==face_index+1: submitted=true
  if not submitted:
   faces.append({"face":face_index,"status":"not_submitted_yet"})
   continue
  var key := "face_"+str(face_index)
  images[key]=_image(face.get_texture().get_image(),prefix+"-"+key+".png")
  var pose := face_camera.global_transform
  var matrix: Array[float]=[]
  for column in [pose.basis.x,pose.basis.y,pose.basis.z,pose.origin]:
   for axis in 3: matrix.append(column[axis])
  faces.append({"face":face_index,"path":str(face.get_path()),"update_mode":int(face.render_target_update_mode),"transform_column_xyz":matrix,"transform_bits":var_to_bytes(pose).hex_encode(),"fov":face_camera.fov,"near":face_camera.near,"far":face_camera.far,"keep_aspect":face_camera.keep_aspect,"projection":face_camera.projection,"cull_mask":face_camera.cull_mask,"camera_projection_bits":var_to_bytes(face_camera.get_camera_projection()).hex_encode()})
 var environment: Dictionary=renderer.get_environment_capture()
 var atlas: Texture2D=environment.get("texture")
 if bool(environment.get("available",false)) and atlas!=null:
  images["atlas"]=_image(atlas.get_image(),prefix+"-atlas.png")
 for key in images:
  if images[key].has("error"):
   _fail(str(images[key].error))
   return false
 captures.append({"session":session,"index":index,"driving":viewer.last_driving_frame.duplicate(true),"native":native,"effects":renderer.get_effects(),"reflection_faces":faces,"captures":images,"environment_available":bool(environment.get("available",false)),"environment_matrix_bits":var_to_bytes(environment.get("hack_matrix_native")).hex_encode(),"post_draw_engine_frame":Engine.get_frames_drawn(),"memory":renderer.get_memory_state()})
 if not _write():
  _fail("Driving parity checkpoint report write failed")
  return false
 return true

func _steps(session: int,count: int,selected: Array) -> bool:
 for index in count:
  if DisplayServer.window_get_mode()==DisplayServer.WINDOW_MODE_MINIMIZED:
   _fail("Driving parity window minimized; rendering cannot be certified")
   return false
  var before := int(viewer.driving.state.get("step",-1))
  var state: Dictionary=viewer.benchmark_frame(DT)
  if not state.get("ok",false) or viewer.phase!="Running":
   _fail("Driving parity production frame failed at%d: %s"%[index,str(state)])
   return false
  var driving: Dictionary=viewer.last_driving_frame.duplicate(true)
  if int(driving.get("step_delta",-1)) not in [0,1] or int(driving.get("native_tick_calls",-1))!=1 or int(driving.get("step_before",-1))!=before or int(driving.get("step_after",-1))!=before+int(driving.get("step_delta",-1)) or int(driving.get("continuous_frame",-1))!=index:
   _fail("Driving parity step mismatch at%d: %s"%[index,JSON.stringify(driving)])
   return false
  timeline.append({"session":session,"index":index,"driving":driving})
  await process_frame
  await RenderingServer.frame_post_draw
  if selected.has(index) and not _capture(session,index): return false
  if index%120==119: print("TASK0072_DRIVING_PARITY_PROGRESS session=",session," steps=",index+1)
 return true

func _dispose(session: int) -> bool:
 var renderer: Node3D=viewer.track_renderer
 var vehicle: Node3D=viewer.vehicle_renderer
 viewer._release()
 renderer.clear_track()
 vehicle.clear_car()
 var cleared: Dictionary=renderer.get_state()
 teardown.append({"session":session,"cleared":cleared})
 if cleared.get("ok",false) or cleared.get("loading",true) or int(cleared.get("render_nodes",-1))!=0:
  _fail("Driving parity teardown retained track state: "+str(cleared))
  return false
 viewer.queue_free()
 await process_frame
 await RenderingServer.frame_post_draw
 viewer=null
 return true

func _run() -> void:
 if DisplayServer.get_name()=="headless":
  print("TASK0072_DRIVING_PARITY_SKIP headless")
  quit(77)
  return
 for argument in OS.get_cmdline_user_args():
  if argument.begins_with("--driving-parity-output="): output=argument.trim_prefix("--driving-parity-output=")
  elif argument.begins_with("--driving-parity-frame-limit="): frame_limit=int(argument.trim_prefix("--driving-parity-frame-limit="))
 output=output.replace("\\","/").simplify_path()
 if not output.is_absolute_path() or output.to_lower().find("/local/")<0:
  _fail("Driving parity requires an explicit absolute ignored local/ output directory")
  return
 if DirAccess.make_dir_recursive_absolute(output)!=OK:
  _fail("Cannot create driving parity output directory")
  return
 output_valid=true
 if RenderingServer.get_current_rendering_method()!="forward_plus" or RenderingServer.get_current_rendering_driver_name()!="vulkan":
  _fail("Driving parity requires actual Vulkan Forward+")
  return
 DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_WINDOWED)
 OS.low_processor_usage_mode=false
 root.size=SIZE
 root.content_scale_size=Vector2i.ZERO
 DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
 Engine.max_fps=0
 # Disable profiling before loading; keep DebugTools production startup available.
 var profiler := root.get_node_or_null("ProfileDebug")
 if profiler!=null:
  profiler.enabled=false
  profiler.ui_enabled=false
  profiler.capture_active=false
  profiler.set_process(false)
 if frame_limit<120 or frame_limit>720:
  _fail("Driving parity frame limit must be120..720")
  return
 if not await _start(0): return
 if not await _steps(0,frame_limit,CAPTURE_STEPS.filter(func(index):return index<frame_limit)): return
 if not await _dispose(0): return
 # A separately named short session validates full release/load; never restarts
 # a failed driving lifecycle or merges it into the720-frame route.
 if not await _start(1): return
 if not await _steps(1,3,[0,2]): return
 if not await _dispose(1): return
 finished=true
 if not _write():
  _fail("Driving parity final report write failed")
  return
 print("TASK0072_DRIVING_PARITY_OK ",frame_limit," continuous steps plus3 after reload; ",output)
 quit(0)
