extends "res://scripts/runtime/track_optimization_smoke.gd"
# TASK0055 actual ELISE metadata supplies capture origin; fixed display pose, no physics claim.
var car: Node3D
func _capture(renderer: Node3D) -> Dictionary:
 if renderer==baseline:
  candidate.hide()
  baseline.show()
 else:
  baseline.hide()
  candidate.show()
 var state: Dictionary
 for index in 18:
  var projection_scale := 512.0/(2.0*tan(deg_to_rad(camera.fov)*0.5))
  car.update_view(camera.position,-camera.global_basis.z,projection_scale,-1)
  renderer.set_visual_state(car.get_visual_state())
  state=_step(renderer)
  car.set_environment_capture(renderer.get_environment_capture())
  car.update_view(camera.position,-camera.global_basis.z,float(state.get("original_focal_h",projection_scale)),-1)
  if not state.get("ok",false):return {"error":str(state)}
  await get_tree().process_frame
  await RenderingServer.frame_post_draw
 var capture: Dictionary=renderer.get_environment_capture()
 if not capture.get("available",false):return {"error":"Original car capture did not become ready: "+str(capture)}
 var texture: Texture2D=capture.get("texture")
 if texture==null:return {"error":"Ready capture has no actual atlas"}
 var result := {"final":get_viewport().get_texture().get_image(),"summary":renderer.frame_summary(),"atlas":texture.get_image(),"matrix":capture.get("hack_matrix_native"),"views":state.get("view_submissions",[])}
 var targets: Dictionary=renderer.get_render_targets()
 for key in ["colour","native_depth"]:
  var target: Texture2D=targets.get(key)
  if target==null:return {"error":"Missing actual target "+key}
  result[key]=target.get_image()
 return result
func _reflection_compare(label: String) -> bool:
 var a := await _capture(baseline)
 var b := await _capture(candidate)
 if a.has("error") or b.has("error"):
  _fail("Reflection optimization capture failed: "+str(a)+str(b))
  return false
 for key in ["packet_digest","commands","sections","zone","visible_instances"]:
  if not a.summary.has(key) or a.summary[key]!=b.summary.get(key):
   _fail("Reflection main native packet mismatch: "+key)
   return false
 if a.matrix!=b.matrix or a.views.size()<2 or a.views.size()!=b.views.size():
  _fail("Reflection matrix or active capture-view submission closure differs")
  return false
 var capture_reductions := 0
 for index in a.views.size():
  for key in ["view_id","commands"]:
   if a.views[index].get(key)!=b.views[index].get(key):
    _fail("Reflection native per-view counts differ")
    return false
  if int(a.views[index].get("view_id",1))!=1:
   capture_reductions+=maxi(0,int(a.views[index].get("batches",0))-int(b.views[index].get("batches",0)))
 print("REFLECTION_CAPTURE_BATCH_REDUCTIONS ",label," ",capture_reductions)
 for key in ["final","colour","native_depth","atlas"]:
  var first: Image=a[key]
  var second: Image=b[key]
  if first.is_empty() or first.get_size()!=second.get_size() or first.get_data()!=second.get_data():
   var output := ProjectSettings.globalize_path("res://../local/task0055")
   DirAccess.make_dir_recursive_absolute(output)
   for saved in ["final","colour","native_depth","atlas"]:
    a[saved].save_png(output.path_join(label+"-"+saved+"-baseline.png"))
    b[saved].save_png(output.path_join(label+"-"+saved+"-"+mode+".png"))
   _fail("Reflection exact GPU mismatch: "+label+" "+key)
   return false
 print("TRACK_OPTIMIZATION_REFLECTION_PARITY ",label," views=",JSON.stringify(b.views)," host=",JSON.stringify(candidate.get_optimization_state()))
 return true
func _run() -> void:
 if DisplayServer.get_name()=="headless":
  get_tree().quit(77)
  return
 for argument in OS.get_cmdline_user_args():
  if argument.begins_with("--optimization-mode="):mode=argument.trim_prefix("--optimization-mode=")
 var overlay := get_node_or_null("/root/DebugTools")
 if overlay:overlay.hide()
 get_window().size=Vector2i(1920,1080)
 camera=Camera3D.new()
 camera.keep_aspect=Camera3D.KEEP_WIDTH
 camera.fov=float(0x32dc)*360/65536
 add_child(camera)
 camera.current=true
 baseline=ClassDB.instantiate("Ps2TrackRenderer")
 candidate=ClassDB.instantiate("Ps2TrackRenderer")
 car=ClassDB.instantiate("Ps2VehicleRenderer")
 add_child(baseline)
 add_child(candidate)
 add_child(car)
 candidate.hide()
 baseline.configure_optimizations({"resource_reuse":false,"particle_mesh_reuse":false,"instancing":false,"occlusion":false,"opaque_queue":false,"world_batches":false})
 selected=_options()
 candidate.configure_optimizations(selected)
 var chosen := {}
 for entry in car.get_catalog():
  if str(entry.code)=="ELISE":chosen=entry
 if chosen.is_empty() or not chosen.get("available",false) or chosen.get("skins",[]).is_empty():
  _fail("Actual ELISE catalogue fixture unavailable")
  return
 var car_state: Dictionary=car.request_car(str(chosen.path),int(chosen.id),int(chosen.skins[0].id))
 var deadline := Time.get_ticks_msec()+120000
 while car_state.get("loading",false):
  if Time.get_ticks_msec()>deadline:
   _fail("ELISE load timed out")
   return
  car_state=car.pump_load()
  await get_tree().process_frame
 if not car_state.get("ok",false) or not await _load(11):
  _fail("Reflection pair/car load failed: "+str(car_state))
  return
 baseline.restore_render_defaults()
 # TASK0076: reflection-specific parity deliberately opts in to costly captures.
 baseline.set_effect_enabled("environment",true)
 var initial: Dictionary=baseline.get_state()
 var start: Vector3=initial.start_position
 var forward: Vector3=initial.start_forward
 car.position=start+Vector3.UP*0.5
 car.look_at(car.position-forward)
 var visual: Dictionary=car.get_visual_state()
 if not visual.get("valid",visual.get("ok",false)):
  # The real API carries its own source/pose fields; absence is explicit failure.
  if not visual.has("environment_position_native"):
   _fail("Real car visual-state metadata missing: "+str(visual))
   return
 # Exercise viewer batching before attaching the real car reflection consumer.
 if "--viewer-transition" in OS.get_cmdline_user_args():
  camera.position=Vector3(141.24,-17.98,697.04)
  camera.look_at(camera.position+Vector3.FORWARD)
  for renderer in [baseline,candidate]:
   baseline.visible=renderer==baseline
   candidate.visible=renderer==candidate
   for index in 6:
    renderer.set_visual_state({})
    _step(renderer)
    await get_tree().process_frame
    await RenderingServer.frame_post_draw
 # Exact native first driving snapshot, matching race_scene initial presentation.
 var simulation: RefCounted=ClassDB.instantiate("Ps2DrivingRuntime")
 simulation.load_config("ELISE")
 simulation.set_track(11)
 simulation.action("run")
 var driving: Dictionary=simulation.tick(0.0)
 simulation.action("pause")
 if not driving.get("ok",false) or not driving.get("body_known",false):
  _fail("Actual initial native driving snapshot unavailable: "+str(driving))
  return
 car.apply_driving_snapshot(driving)
 var distance: float=maxf(4.0,car_state.bounds.size.z*1.5)
 var chase: Vector3=car.transform.basis.z
 chase.y=0
 var target: Vector3=car.transform*car_state.bounds.get_center()
 camera.position=target-chase.normalized()*distance*cos(deg_to_rad(18.0))+Vector3.UP*distance*sin(deg_to_rad(18.0))
 camera.look_at(target)
 if not await _reflection_compare("reflection-native-drive-1080p"):return
 for offset in [0.0,12.0,-4.0]:
  car.rotate_y(0.15) # Exercise declared camera AND basis-dependent light writers.
  camera.position=start-forward*(8.0+offset)+Vector3.UP*3
  camera.look_at(start)
  if not await _reflection_compare("reflection-"+str(offset)):return
 if not _clear_check():return
 car.clear_car()
 print("TRACK_OPTIMIZATION_REFLECTION_GPU_OK actual ELISE pose, active native capture views and exact atlas/colour/depth")
 get_tree().quit(0)

