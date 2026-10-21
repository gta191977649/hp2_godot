extends SceneTree
# TASK0086 production geometry/textures/state; no original-runtime equivalence.
var car: Node3D
var camera: Camera3D
var driving: RefCounted
var track: Node3D
func _initialize() -> void:
 _run.call_deferred()
func check(okay: bool, message: String) -> bool:
 if not okay:printerr("VEHICLE_FLARES_FAIL: ",message);quit(1)
 return okay
func draw() -> void:
 await process_frame
 await RenderingServer.frame_post_draw
func present(snapshot: Dictionary, offset: Vector3) -> Dictionary:
 var body: Transform3D=snapshot.body
 camera.position=body.origin+offset
 camera.look_at(body.origin+Vector3(0,.3,0))
 var result: Dictionary=car.apply_driving_snapshot(snapshot)
 if not result.get("ok",false):return result
 result=car.update_view(camera.position,-camera.global_basis.z,256.0,0)
 if not result.get("ok",false):return result
 if track!=null:
  track.set_visual_timestep(0.0)
  var track_state: Dictionary=track.update_view(camera.position)
  if not track_state.get("ok",false):return track_state
 return car.get_visual_state()
func pixels(image: Image) -> int:
 var count := 0
 for y in range(image.get_height()/5,image.get_height()):
  for x in range(image.get_width()):
   var c := image.get_pixel(x,y)
   if maxf(c.r,maxf(c.g,c.b))>.025:count+=1
 return count
func _run() -> void:
 var world := Node3D.new()
 root.add_child(world)
 camera=Camera3D.new()
 world.add_child(camera)
 camera.current=true
 camera.near=.05
 car=ClassDB.instantiate("Ps2VehicleRenderer")
 world.add_child(car)
 var loaded: Dictionary=car.load_car("CARS/911TURBO/GEOMETRY.BIN")
 if not check(loaded.get("ok",false),str(loaded)):return
 driving=ClassDB.instantiate("Ps2DrivingRuntime")
 loaded=driving.load_config("911TURBO")
 if not check(loaded.get("ok",false),str(loaded)):return
 if "--flare-track" in OS.get_cmdline_user_args():
  track=ClassDB.instantiate("Ps2TrackRenderer")
  world.add_child(track)
  var loaded_track: Dictionary=track.request_track(11)
  while loaded_track.get("loading",false):
   await process_frame
   loaded_track=track.pump_load()
  if not check(loaded_track.get("ok",false),str(loaded_track)):return
  if not check(driving.set_track(11).get("ok",false),"Original Track11 driving state"):return
  track.set_camera_mode(1)
 driving.action("run")
 driving.set_driving_input(0.0,0.0,0.0,0.0,-1,0,false,true)
 var state: Dictionary=driving.tick(1.0/60.0)
 if not check(state.get("ok",false) and state.has("flare_intensities"),str(state)):return
 var result := present(state,Vector3(0,1,8))
 if not check(result.get("flare_diagnostic","").is_empty(),str(result)):return
 print("VEHICLE_FLARES_FRONT ",result.get("light_flares",[]))
 if not check(result.get("light_flares",[]).size()>0,"Horn produced no original front flares"):return
 await draw()
 for argument in OS.get_cmdline_user_args():
  if argument.begins_with("--flare-images="):root.get_texture().get_image().save_png(argument.trim_prefix("--flare-images=")+"-front.png")
 if "--flare-exit-live" in OS.get_cmdline_user_args():
  print("VEHICLE_FLARES_OK: quit with live attached nodes")
  quit(0)
  return
 # Isolate attached sprites; a real opaque host occluder must hide them.
 for child in car.get_children():
  if child is MeshInstance3D:
   var material: Material=child.material_override
   if not material is ShaderMaterial or not "flare_colour" in material.shader.code:child.hide()
 await draw()
 var unobstructed := pixels(root.get_texture().get_image())
 if not check(unobstructed>100,"No actual flare pixels"):return
 var box := MeshInstance3D.new()
 var mesh := BoxMesh.new()
 mesh.size=Vector3(10,10,.5)
 box.mesh=mesh
 var black := StandardMaterial3D.new()
 black.shading_mode=BaseMaterial3D.SHADING_MODE_UNSHADED
 black.albedo_color=Color.BLACK
 box.material_override=black
 world.add_child(box)
 box.global_transform=Transform3D(camera.global_basis,(camera.position+state.body.origin)*.5)
 await draw()
 var blocked := pixels(root.get_texture().get_image())
 if track==null and not check(blocked<unobstructed*.05,"Occluder leaked flare pixels: %d/%d"%[blocked,unobstructed]):return
 print("VEHICLE_FLARES_OCCLUSION ",unobstructed," -> ",blocked)
 box.queue_free()
 await draw()
 var near := result.light_flares[0].half_size as float
 result=present(state,Vector3(0,1,24))
 if not check(result.get("flare_diagnostic","").is_empty() and result.light_flares.size()>0,"Far view lost original flares"):return
 if not check(is_finite(result.light_flares[0].half_size) and result.light_flares[0].half_size>0,"Original far-view flare scale is invalid"):return
 result=present(state,Vector3(8,1,0))
 if not check(result.get("flare_diagnostic","").is_empty(),"Side view failed"):return
 print("VEHICLE_FLARES_SIDE ",result.get("light_flares",[]).size())
 # Native state selector is separately tested; these channels test presentation.
 for fixture in ["night","brake","reverse"]:
  var sample := state.duplicate(true)
  var values := PackedFloat32Array()
  values.resize(12)
  if fixture=="night":values[0]=.75;values[1]=.75
  elif fixture=="brake":values[2]=1;values[3]=1;values[4]=1
  else:values[5]=1;values[6]=1
  sample.flare_intensities=values
  result=present(sample,Vector3(0,1,8 if fixture=="night" else -8))
  if not check(result.get("flare_diagnostic","").is_empty() and result.get("light_flares",[]).size()>0,fixture+": "+str(result)):return
  await draw()
  for argument in OS.get_cmdline_user_args():
   if argument.begins_with("--flare-images="):root.get_texture().get_image().save_png(argument.trim_prefix("--flare-images=")+"-"+fixture+".png")
 # Original cop markers and parameter colours; selector phase is a native test.
 loaded=car.load_car("CARS/BMWM5COP/GEOMETRY.BIN")
 if not check(loaded.get("ok",false),str(loaded)):return
 loaded=driving.load_config("BMWM5COP")
 if not check(loaded.get("ok",false),str(loaded)):return
 if track!=null and not check(driving.set_track(11).get("ok",false),"Cop Track11 rebind"):return
 driving.action("run")
 var phases: Array=[]
 for frame in range(10):
  driving.set_driving_input(0.0,0.0,0.0,0.0,-1,0,false,frame==0)
  var cop_state: Dictionary=driving.tick(1.0/60.0)
  if not check(cop_state.get("ok",false),str(cop_state)):return
  var active: PackedFloat32Array=cop_state.flare_intensities
  if not check(active[7]+active[8]>0,"Original cop horn callback did not activate lights"):return
  var phase := [active[9],active[10],active[11]]
  if phase not in phases:phases.append(phase)
  result=present(cop_state,Vector3(6,2,4))
  if not check(result.get("flare_diagnostic","").is_empty() and result.get("light_flares",[]).size()>0,"Actual original cop phase produced no flares"):return
  await draw()
  state=cop_state
 if not check(phases.size()>1,"Original cop phase did not change"):return
 print("VEHICLE_FLARES_NATIVE_COP_PHASES ",phases)
 for channel in [9,10,11]:
  var cop := state.duplicate(true)
  var values := PackedFloat32Array()
  values.resize(12)
  values[channel]=1
  cop.flare_intensities=values
  result=present(cop,Vector3(6,2,4))
  if not check(result.get("flare_diagnostic","").is_empty() and result.get("light_flares",[]).size()>0,"Cop phase channel%d: %s"%[channel,result]):return
  await draw()
  for argument in OS.get_cmdline_user_args():
   if argument.begins_with("--flare-images="):root.get_texture().get_image().save_png(argument.trim_prefix("--flare-images=")+"-cop-%d.png"%channel)
 # A reset snapshot must clear previously active attached sprites.
 driving.action("restart")
 driving.action("run")
 state=driving.tick(1.0/60.0)
 result=present(state,Vector3(0,1,8))
 if not check(result.get("flare_diagnostic","").is_empty() and result.get("light_flares",[]).is_empty(),"Reset retained previous flare state"):return
 car.clear_car()
 await draw()
 if not check(car.get_child_count()==0,"Unload retained attached flare nodes"):return
 if track!=null:
  track.clear_track()
  track.queue_free()
 print("VEHICLE_FLARES_OK")
 quit(0)
