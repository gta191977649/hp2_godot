extends Node3D
# TASK0055 host optimization parity. No recovered behavior or native frame mutation.
var baseline: Node3D
var candidate: Node3D
var camera: Camera3D
var mode := "optimized"
var selected := {}
func _ready() -> void:_run.call_deferred()
func _fail(reason: String) -> void:
 push_error(reason)
 get_tree().quit(1)
func _options() -> Dictionary:
 var result := {"resource_reuse":false,"particle_mesh_reuse":false,"instancing":false,"occlusion":false,"opaque_queue":false,"world_batches":false}
 if mode=="reuse" or mode=="optimized" or mode=="world_reuse":
  result.resource_reuse=true
  result.particle_mesh_reuse=true
 if mode=="multimesh":result.instancing=true
 if mode=="occlusion":result.occlusion=true
 if mode=="opaque":result.opaque_queue=true
 if mode in ["world","world_reuse"]:result.world_batches=true
 return result
func _load(number: int) -> bool:
 var a: Dictionary=baseline.request_track(number)
 var b: Dictionary=candidate.request_track(number)
 var deadline := Time.get_ticks_msec()+120000
 while a.get("loading",false) or b.get("loading",false):
  if Time.get_ticks_msec()>deadline:return false
  a=baseline.pump_load()
  b=candidate.pump_load()
  await get_tree().process_frame
 return a.get("ok",false) and b.get("ok",false)
func _step(renderer: Node3D,dt: float=0) -> Dictionary:
 renderer.set_visual_timestep(dt)
 return renderer.update_view(camera.position)
func _capture(renderer: Node3D) -> Dictionary:
 if renderer==baseline:
  candidate.hide()
  baseline.show()
 else:
  baseline.hide()
  candidate.show()
 for index in 6:
  var state := _step(renderer)
  if not state.get("ok",false):return {"error":str(state)}
  await get_tree().process_frame
  await RenderingServer.frame_post_draw
 var diagnostic = preload("res://scripts/debug/track_render_diagnostics.gd").new()
 var detail: Dictionary = diagnostic.sample(self,camera,renderer,true)
 var directory := ProjectSettings.globalize_path("res://../local/task0065")
 DirAccess.make_dir_recursive_absolute(directory)
 var diagnostic_file := FileAccess.open(directory.path_join("draw-"+RenderingServer.get_current_rendering_method()+"-"+mode+"-"+str(renderer==candidate)+"-"+str(camera.position)+"-"+str(-camera.basis.z)+".json"),FileAccess.WRITE)
 var targets: Dictionary=renderer.get_render_targets()
 var result := {"draws":Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME),"final":get_viewport().get_texture().get_image(),"summary":renderer.frame_summary()}
 for key in ["colour","native_depth"]:
  var texture: Texture2D=targets.get(key)
  if texture==null:return {"error":"Missing actual render target "+key}
  result[key]=texture.get_image()
 detail["image_sha256"]={}
 for key in ["final","colour","native_depth"]:
  var hash := HashingContext.new()
  hash.start(HashingContext.HASH_SHA256)
  hash.update(result[key].get_data())
  detail.image_sha256[key]=hash.finish().hex_encode()
 diagnostic_file.store_string(JSON.stringify(detail,"  "))
 return result
func _compare(label: String) -> bool:
 var a := await _capture(baseline)
 var b := await _capture(candidate)
 if a.has("error") or b.has("error"):
  _fail("Optimization capture failed: "+str(a)+str(b))
  return false
 print("DENSE_DRAW_COUNTS ",label," baseline=",a.get("draws")," candidate=",b.get("draws"))
 for key in ["packet_digest","commands","sections","zone","visible_instances"]:
  if a.summary.get(key)!=b.summary.get(key) or not a.summary.has(key):
   _fail("Optimization native frame differs: "+label+" "+key)
   return false
 if str(a.summary.get("packet_digest","")).is_empty() or int(a.summary.get("commands",0))==0:
  _fail("Empty native frame is not parity evidence")
  return false
 for key in ["final","colour","native_depth"]:
  var first: Image=a[key]
  var second: Image=b[key]
  if first.is_empty() or first.get_size()!=second.get_size() or first.get_data()!=second.get_data():
   var output := ProjectSettings.globalize_path("res://../local/task0055")
   DirAccess.make_dir_recursive_absolute(output)
   for saved in ["final","colour","native_depth"]:
    var baseline_image: Image=a[saved]
    var candidate_image: Image=b[saved]
    baseline_image.save_png(output.path_join(label+"-"+saved+"-baseline.png"))
    candidate_image.save_png(output.path_join(label+"-"+saved+"-"+mode+".png"))
   _fail("Optimization exact GPU mismatch: "+label+" "+key+" mode="+mode)
   return false
 print("TRACK_OPTIMIZATION_PARITY ",mode," ",label," ",JSON.stringify(candidate.get_optimization_state()))
 return true
func _clear_check() -> bool:
 for renderer in [baseline,candidate]:
  renderer.clear_track()
  var state: Dictionary=renderer.get_state()
  if state.get("ok",false) or state.get("loading",true) or int(state.get("render_nodes",-1))!=0:
   _fail("Optimization clear leaked track nodes")
   return false
 if candidate.get_optimization_state().effective!=selected:
  _fail("Optimization mode lost across clear")
  return false
 return true
func _run() -> void:
 if DisplayServer.get_name()=="headless":
  get_tree().quit(77)
  return
 var args := OS.get_cmdline_user_args()
 for argument in args:
  if argument.begins_with("--optimization-mode="):mode=argument.trim_prefix("--optimization-mode=")
 if mode not in ["reuse","multimesh","occlusion","opaque","world","world_reuse","default","optimized"]:
  _fail("Unknown optimization test mode")
  return
 var overlay := get_node_or_null("/root/DebugTools")
 if overlay:overlay.hide()
 get_window().size=Vector2i(1920,1080) if "--dense-pose" in args else Vector2i(960,540)
 DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
 camera=Camera3D.new()
 camera.keep_aspect=Camera3D.KEEP_WIDTH
 camera.fov=float(0x32dc)*360/65536
 add_child(camera)
 camera.current=true
 baseline=ClassDB.instantiate("Ps2TrackRenderer")
 candidate=ClassDB.instantiate("Ps2TrackRenderer")
 add_child(baseline)
 add_child(candidate)
 candidate.hide()
 var fixture: Dictionary=ClassDB.class_call_static("Ps2TrackRenderer","test_instance_pool")
 if not fixture.get("ok",false):
  _fail("MultiMesh resource fixture failed: "+str(fixture))
  return
 var off := {"resource_reuse":false,"particle_mesh_reuse":false,"instancing":false,"occlusion":false,"opaque_queue":false,"world_batches":false}
 baseline.configure_optimizations(off)
 selected=candidate.get_optimization_state().effective if mode=="default" else _options()
 if mode!="default" and not candidate.configure_optimizations(selected).get("ok",false):
  _fail("Valid optimization configuration rejected")
  return
 var rejected: Dictionary=candidate.configure_optimizations({"resource_reuse":not selected.resource_reuse,"unknown":true})
 if rejected.get("ok",true) or candidate.get_optimization_state().effective!=selected:
  _fail("Unknown option rejection was not atomic")
  return
 var catalog: Array=baseline.get_catalog()
 if catalog.is_empty():
  get_tree().quit(77)
  return
 var tracks: Array=[11]
 for argument in args:
  if argument.begins_with("--optimization-track="):tracks=[int(argument.trim_prefix("--optimization-track="))]
 if "--optimization-all-tracks" in args:
  tracks=[]
  for entry in catalog:tracks.append(int(entry.id))
 for number in tracks:
  if not await _load(number):
   _fail("Optimization pair load failed: "+str(number))
   return
  var locked: Dictionary=candidate.configure_optimizations(off)
  if locked.get("ok",true) or candidate.get_optimization_state().effective!=selected:
   _fail("Loaded reconfiguration was not atomically rejected")
   return
  baseline.restore_render_defaults()
  var initial: Dictionary=baseline.get_state()
  var start: Vector3=initial.start_position
  var forward: Vector3=initial.start_forward
  if "--dense-pose" in args:
   camera.position=Vector3(141.24,-17.98,697.04)
   camera.fov=75
   for heading in [Vector3.FORWARD,Vector3.RIGHT,Vector3.BACK,Vector3.LEFT,Vector3(-0.4,-0.1,-1).normalized(),Vector3(0.5,0.1,-1).normalized()]:
    camera.look_at(camera.position+heading)
    if not await _compare("dense-"+str(heading)):return
   camera.position+=Vector3(3,1,-5)
   camera.look_at(camera.position+Vector3.FORWARD)
   if not await _compare("dense-nearby"):return
   if not _clear_check():return
   print("TRACK_DENSE_PARITY_OK ",mode)
   get_tree().quit(0)
   return
  for offset in [0.0,50.0,-25.0]:
   var target: Vector3=start+forward*offset
   camera.position=target-forward*8+Vector3.UP*3
   camera.look_at(target)
   if not await _compare("track-"+str(number)+"-"+str(offset)):return
  if not _clear_check():return
 if "--geometry-only" in args:
  print("TRACK_GEOMETRY_PARITY_OK ",mode)
  get_tree().quit(0)
  return
 # Both new simulations start from identical renderer seed and get identical steps.
 if not await _load(11):
  _fail("Optimization reload failed")
  return
 camera.position=Vector3(170,-48,-640)
 camera.look_at(Vector3(190.068,-53.8522,-654.923))
 var particle_counts: Array=[]
 for renderer in [baseline,candidate]:
  if renderer==baseline:
   candidate.hide()
   baseline.show()
  else:
   baseline.hide()
   candidate.show()
  for index in 120:
   var state := _step(renderer,1.0/60)
   if not state.get("ok",false):
    _fail("Actual particle simulation failed: "+str(state))
    return
   await get_tree().process_frame
   await RenderingServer.frame_post_draw
  var prepared: Dictionary=renderer.get_state()
  particle_counts.append([prepared.get("particles_active",-1),prepared.get("particles_visible",-1)])
 if particle_counts[0]!=particle_counts[1] or int(particle_counts[0][0])<=0:
  _fail("Optimization simulations received unequal visible steps: "+str(particle_counts))
  return
 print("OPTIMIZATION_PARTICLE_COUNTS ",particle_counts)
 if not await _compare("forest-fire"):return
 if int(baseline.get_state().get("particles_visible",0))==0:
  _fail("Forest-fire parity had no real particles")
  return
 if not _clear_check():return
 for renderer in [baseline,candidate]:
  var failed: Dictionary=renderer.request_track(-999)
  if failed.get("loading",false) or failed.get("ok",false) or int(renderer.get_state().get("render_nodes",-1))!=0:
   _fail("Invalid track retained loaded resources")
   return
 print("TRACK_OPTIMIZATION_GPU_OK mode=",mode," tracks=",tracks.size())
 get_tree().quit(0)

