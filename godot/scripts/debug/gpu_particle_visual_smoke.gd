extends SceneTree
# TASK0085 real production wheel assets; image readback is test-only.
func _initialize() -> void:
 _run.call_deferred()

func _run() -> void:
 var owner := Node3D.new()
 root.add_child(owner)
 var camera := Camera3D.new()
 owner.add_child(camera)
 camera.current=true
 camera.near=0.05
 var simulation: RefCounted=ClassDB.instantiate("Ps2DrivingRuntime")
 var result: Dictionary=simulation.load_config("911TURBO")
 if not result.get("ok",false):
  printerr(result)
  quit(1)
  return
 simulation.action("run")
 var maximum_pixels := 0
 for frame in range(300):
  simulation.set_driving_input(1.0,0.6 if frame>=120 and "--gpu-particle-drift" in OS.get_cmdline_user_args() else 0.0,0.0 if frame<120 else 1.0,1.0 if frame>=180 else 0.0)
  var state: Dictionary=simulation.tick(1.0/60.0)
  if not state.get("ok",false):
   printerr(state)
   quit(1)
   return
  var body: Transform3D=state.body
  camera.position=body.origin+Vector3(5,4,-8)
  camera.look_at(body.origin)
  result=simulation.present_particles(owner,camera)
  if not result.get("ok",false):
   printerr(result)
   quit(1)
   return
  await process_frame
  await RenderingServer.frame_post_draw
  if frame in [120,180,240,299]:
   var image := root.get_texture().get_image()
   var pixels := 0
   # Exclude the diagnostic HUD in the upper-right corner.
   for y in range(image.get_height()/5,image.get_height()):
    for x in range(image.get_width()):
     var c := image.get_pixel(x,y)
     if maxf(c.r,maxf(c.g,c.b))>0.025:pixels+=1
   maximum_pixels=maxi(maximum_pixels,pixels)
   print("GPU_PARTICLE_IMAGE frame=",frame," pixels=",pixels," stats=",result.get("stats",{}))
   for argument in OS.get_cmdline_user_args():
    if argument.begins_with("--gpu-particle-images="):
     image.save_png(argument.trim_prefix("--gpu-particle-images=")+"-%d.png"%frame)
 if maximum_pixels<100:
  printerr("GPU_PARTICLE_VISUAL_FAIL: no substantive production particle pixels")
  quit(1)
  return
 simulation.clear_particles()
 print("GPU_PARTICLE_VISUAL_OK: pixels=",maximum_pixels)
 quit(0)
