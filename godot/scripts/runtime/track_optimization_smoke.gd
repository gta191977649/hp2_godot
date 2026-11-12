extends Node3D
# TASK0055 host optimization parity. No recovered behavior or native frame mutation.
var baseline: Node3D
var candidate: Node3D
var camera: Camera3D
var mode := "optimized"
var selected := {}
var pose_path := ""
var probe_scene_depth := false
var explicit_source_samples := 0
var quality_changed_pixels := false
func _ready() -> void:_run.call_deferred()
func _fail(reason: String) -> void:
 push_error(reason)
 get_tree().quit(1)
func _options() -> Dictionary:
 var result := {"resource_reuse":false,"particle_mesh_reuse":false,"instancing":false,"occlusion":false,"opaque_queue":false,"world_batches":false,"depth_batches":false,"race_world_batches":false,"shadow_stream":false,"static_depth_sources":false}
 if mode=="static_depth_sources":result.static_depth_sources=true
 if mode=="depth_batches":
  result.resource_reuse=true
  result.depth_batches=true
 if mode=="race_world_batches":
  result.resource_reuse=true
  result.race_world_batches=true
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
 var stats: Dictionary=renderer.get_optimization_state()
 var result := {"draws":Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME),"final":get_viewport().get_texture().get_image(),"summary":renderer.frame_summary(),"depth_batches":stats.get("postprocess",{}).get("encoded_depth_batch_draws",0),"static_depth_sources":stats.get("postprocess",{}).get("static_depth_sources",0)}
 if mode=="godot_quality":
  result["fxaa"]=stats.get("postprocess",{}).get("colour_fxaa",false)
  result["sampling_materials"]=stats.get("godot_sampling_materials",0)
  result["mip_textures"]=stats.get("godot_mip_textures",0)
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
func _compare_scene_depth(label: String, reference: Image) -> bool:
 var colour_texture: Texture2D=candidate.get_render_targets().get("colour")
 var colour_camera: Camera3D
 for child in candidate.get_children():
  if child is SubViewport and child.get_texture()==colour_texture:
   colour_camera=child.get_camera_3d()
   break
 if colour_camera==null:
  _fail("Colour camera is unavailable for compositor depth probe")
  return false
 var depth: Dictionary=candidate.get_state().visibility_audit
 var near_far:=Vector2(depth.effective_near,depth.effective_far)
 var script=preload("res://scripts/debug/task0077_scene_depth_probe.gd")
 var stages: Array[CompositorEffect]=[
  script.new(CompositorEffect.EFFECT_CALLBACK_TYPE_PRE_TRANSPARENT,near_far),
  script.new(CompositorEffect.EFFECT_CALLBACK_TYPE_POST_TRANSPARENT,near_far)]
 var compositor:=Compositor.new()
 compositor.compositor_effects=stages
 colour_camera.compositor=compositor
 for index in 3:
  await RenderingServer.frame_post_draw
 var inputs: Array[Image]=[]
 for stage in stages:
  stage.request_image()
  inputs.append(await stage.captured)
 colour_camera.compositor=null
 for stage in stages:
  stage.close()
  await stage.released
 var encoded:=reference.duplicate() as Image
 encoded.convert(Image.FORMAT_RGBAF)
 var expected:=encoded.get_data().to_float32_array()
 for index in inputs.size():
  var result:=inputs[index]
  if result.is_empty() or result.get_size()!=encoded.get_size():
   _fail("Scene depth probe did not produce a matching raster")
   return false
  var actual:=result.get_data()
  var foreground:=0
  var missing:=0
  var unequal:=0
  for offset in range(0,expected.size(),4):
   var expected_z:=roundi(expected[offset]*255.0) | (roundi(expected[offset+1]*255.0)<<8) | (roundi(expected[offset+2]*255.0)<<16)
   var actual_z:=int(actual[offset]) | (int(actual[offset+1])<<8) | (int(actual[offset+2])<<16)
   foreground+=int(expected_z!=0)
   missing+=int(expected_z!=0 and actual_z==0)
   unequal+=int(expected_z!=actual_z)
  print("TASK0077_SCENE_DEPTH ",label," ",["pre_transparent","post_transparent"][index],
   " foreground=",foreground," missing=",missing," unequal=",unequal," pixels=",encoded.get_width()*encoded.get_height())
 return true

func _compare(label: String) -> bool:
 var a := await _capture(baseline)
 var b := await _capture(candidate)
 if a.has("error") or b.has("error"):
  _fail("Optimization capture failed: "+str(a)+str(b))
  return false
 print("DENSE_DRAW_COUNTS ",label," baseline=",a.get("draws")," candidate=",b.get("draws"))
 print("ENCODED_DEPTH_BATCHES ",label," ",b.get("depth_batches",0))
 if mode=="static_depth_sources":
  print("STATIC_DEPTH_SOURCES ",label," ",b.static_depth_sources)
  explicit_source_samples+=int(b.static_depth_sources)
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
  if mode=="godot_quality" and key!="native_depth" and not first.is_empty() and first.get_size()==second.get_size():
   quality_changed_pixels=quality_changed_pixels or first.get_data()!=second.get_data()
   continue
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
 if mode=="godot_quality":
  if a.fxaa or a.sampling_materials!=0 or not b.fxaa or b.sampling_materials<=0:
   _fail("Godot quality fixture did not exercise the expected viewport and samplers")
   return false
  print("TASK0077_GODOT_QUALITY ",label," materials=",b.sampling_materials," mip_textures=",b.mip_textures," encoded_depth=exact")
 if probe_scene_depth and not await _compare_scene_depth(label,b.native_depth):return false
 print("TRACK_OPTIMIZATION_PARITY ",mode," ",label," ",JSON.stringify(candidate.get_optimization_state()))
 return true
func _clear_check() -> bool:
 if mode=="static_depth_sources" and explicit_source_samples==0:
  _fail("Fixed camera comparison did not exercise explicit static depth sources")
  return false
 if mode=="godot_quality" and not quality_changed_pixels:
  _fail("Godot quality fixture did not change any colour pixels")
  return false
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
 probe_scene_depth="--probe-scene-depth" in args
 for argument in args:
  if argument.begins_with("--optimization-mode="):mode=argument.trim_prefix("--optimization-mode=")
  if argument.begins_with("--optimization-poses="):pose_path=argument.trim_prefix("--optimization-poses=")
 if mode not in ["reuse","multimesh","occlusion","opaque","world","world_reuse","default","optimized","depth_batches","race_world_batches","static_depth_sources","godot_quality"]:
  _fail("Unknown optimization test mode")
  return
 var overlay := get_node_or_null("/root/DebugTools")
 if overlay:overlay.hide()
 get_window().size=Vector2i(1920,1080) if "--dense-pose" in args or not pose_path.is_empty() else Vector2i(960,540)
 DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
 camera=Camera3D.new()
 camera.keep_aspect=Camera3D.KEEP_WIDTH
 camera.fov=float(0x32dc)*360/65536
 add_child(camera)
 camera.current=true
 baseline=ClassDB.instantiate("Ps2TrackRenderer")
 candidate=ClassDB.instantiate("Ps2TrackRenderer")
 if mode=="godot_quality":candidate.set_race_godot_quality(true,true)
 add_child(baseline)
 add_child(candidate)
 candidate.hide()
 if ClassDB.class_has_method("Ps2TrackRenderer","test_instance_pool"):
  var fixture: Dictionary=ClassDB.class_call_static("Ps2TrackRenderer","test_instance_pool")
  if not fixture.get("ok",false):
   _fail("MultiMesh resource fixture failed: "+str(fixture))
   return
 var off := {"resource_reuse":false,"particle_mesh_reuse":false,"instancing":false,"occlusion":false,"opaque_queue":false,"world_batches":false}
 if mode in ["depth_batches","race_world_batches"]:
  off.resource_reuse=true
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
  if mode=="race_world_batches":
   for renderer in [baseline,candidate]:
    var applied: Dictionary=renderer.apply_world_objects([],[])
    if not applied.get("ok",false):
     _fail("World scenery fixture failed: "+str(applied))
     return
    renderer.set_world_vehicle_visuals([])
  if not pose_path.is_empty():
   var poses: Variant=JSON.parse_string(FileAccess.get_file_as_string(pose_path))
   if not poses is Array or poses.size()!=3:
    _fail("Expected three recorded camera poses")
    return
   for pose in poses:
    var columns: Array=pose.basis
    var position: Array=pose.position
    camera.transform=Transform3D(Basis(Vector3(columns[0][0],columns[0][1],columns[0][2]),
     Vector3(columns[1][0],columns[1][1],columns[1][2]),Vector3(columns[2][0],columns[2][1],columns[2][2])),
     Vector3(position[0],position[1],position[2]))
    camera.fov=float(pose.fov)
    if not await _compare(str(pose.label)):return
   if not _clear_check():return
   print("TRACK_GEOMETRY_PARITY_OK ",mode)
   get_tree().quit(0)
   return
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

