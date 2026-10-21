extends SceneTree
# TASK0059: Track11 reported movement/rotation failure, asset-dependent GPU check.
func _initialize() -> void:call_deferred("_run")
func _run() -> void:
 var viewer=load("res://scenes/debug/track_viewer.tscn").instantiate()
 root.add_child(viewer)
 current_scene=viewer
 var deadline=Time.get_ticks_msec()+120000
 while viewer.track_pending or not viewer.track_state.get("ok",false):
  if Time.get_ticks_msec()>deadline:push_error(str(viewer.track_state));quit(1);return
  await process_frame
 assert(int(viewer.track_state.track_id)==11)
 viewer.set_process(false)
 var start: Vector3=viewer.track_state.start_position
 var forward: Vector3=viewer.track_state.start_forward
 var gap_frames=0
 var supported_visible=false
 # Reproduction camera inputs, not replacement source geometry or game behavior.
 for center in [Vector2(-682.44364,290.50248),Vector2(-820.19283,130.55229)]:
  for i in 8:
   viewer.camera.position=Vector3(center.y,start.y+50,center.x)+Vector3(i*20,0,i*20)
   viewer.camera.rotation=Vector3(sin(i)*0.8,i*0.8,0)
   viewer._process(0.1)
   await process_frame
   await RenderingServer.frame_post_draw
   var state: Dictionary=viewer.track_model.get_state()
   assert(state.ok and state.loaded_instances==8407,str(state))
   assert(state.commands>0,"Track submissions vanished")
   if state.particles_visible>0:supported_visible=true
   if state.particles_unsupported_quads>0:
    gap_frames+=1
    assert(not state.particle_recovery_gaps.is_empty() and str(state.diagnostic).contains("NotImplemented"))
    if gap_frames==1:
     for argument in OS.get_cmdline_user_args():
      if argument.begins_with("--motion-capture="):root.get_texture().get_image().save_png(argument.trim_prefix("--motion-capture="))
   print("MOTION_FRAME ",i," commands=",state.commands," particles=",state.particles_visible," unsupported=",state.particles_unsupported_quads)
 assert(gap_frames>0,"Original reported alpha-bound failure was not exercised")
 assert(supported_visible,"Supported particles were disabled")
 root.get_node("DebugTools").request("reset")
 await process_frame
 await process_frame
 assert(viewer.camera.position.distance_to(start-forward*20+Vector3.UP*10)<0.001,"Reset Camera did not restore track start")
 viewer._process(0.016)
 await process_frame
 await RenderingServer.frame_post_draw
 assert(viewer.track_state.ok and viewer.track_state.triangles>0,"Reset did not restore drawing")
 print("TRACK_CAMERA_MOTION_OK gap_frames=",gap_frames)
 quit(0)

