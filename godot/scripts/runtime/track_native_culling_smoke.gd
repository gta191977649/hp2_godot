extends SceneTree
# TASK0058: one original track only; actual renderer/camera/lifecycle.
func _initialize() -> void:call_deferred("_run")
func _run() -> void:
 var overlay=root.get_node_or_null("DebugTools")
 if overlay:overlay.hide()
 root.size=Vector2i(1280,720)
 var capture_dir=ProjectSettings.globalize_path("user://debug/task0058")
 for argument in OS.get_cmdline_user_args():
  if argument.begins_with("--native-capture-dir="):capture_dir=argument.trim_prefix("--native-capture-dir=")
 DirAccess.make_dir_recursive_absolute(capture_dir)
 var world=Node3D.new()
 root.add_child(world)
 var camera=Camera3D.new()
 camera.keep_aspect=Camera3D.KEEP_WIDTH
 camera.fov=float(0x32dc)*360/65536
 world.add_child(camera)
 camera.current=true
 var renderer=ClassDB.instantiate("Ps2TrackRenderer")
 world.add_child(renderer)
 assert(not renderer.configure_lod_backend("godot_hlod").ok,"Retired host backend accepted")
 assert(renderer.configure_lod_backend("original").ok)
 var state: Dictionary=renderer.request_track(11)
 var deadline=Time.get_ticks_msec()+120000
 while state.get("loading",false):
  assert(Time.get_ticks_msec()<deadline,"Load timeout")
  state=renderer.pump_load()
  await process_frame
 assert(state.ok,str(state))
 assert(renderer.get_lod_state().resource_levels==0,"Normal load created all-level descriptor")
 var start: Vector3=state.start_position
 var forward: Vector3=state.start_forward
 var counters=[]
 for offset in [0.0,50.0,100.0]:
  var target=start+forward*offset
  camera.position=target-forward*8+Vector3.UP*3
  camera.look_at(target)
  state=renderer.update_view(camera.position)
  assert(state.ok,str(state))
  await process_frame
  await RenderingServer.frame_post_draw
  var controls: Dictionary=renderer.get_render_controls()
  var lod: Dictionary=renderer.get_lod_state()
  assert(lod.backend=="original" and not lod.host_lod_selection)
  assert(lod.native_culling_views[0].native_selection)
  assert(controls.visible_instances>0 and controls.visible_instances<controls.loaded_instances)
  assert(controls.considered_instances<controls.loaded_instances)
  for node in renderer.get_children():
   if node is GeometryInstance3D:assert(node.visibility_range_begin==0 and node.visibility_range_end==0,"Engine LOD distance still active")
  if offset==0.0 and DisplayServer.get_name()!="headless":root.get_texture().get_image().save_png(capture_dir.path_join("native-track11-start.png"))
  counters.append(controls.visible_instances)
  print("NATIVE_CULL_FRAME ",lod.native_culling_views[0]," loaded=",controls.loaded_instances," sections_loaded=",controls.loaded_sections)
 assert(counters[0]!=counters[1] or counters[1]!=counters[2],"Camera motion did not change selection")
 if "--particle-gap-smoke" in OS.get_cmdline_user_args():
  var gap_frames=0
  var supported_visible=false
  for center in [Vector2(-682.44364,290.50248),Vector2(-820.19283,130.55229)]:
   for i in 8:
    camera.position=Vector3(center.y,start.y+50,center.x)+Vector3(i*20,0,i*20)
    camera.rotation=Vector3(sin(i)*0.8,i*0.8,0)
    renderer.set_visual_timestep(0.1)
    state=renderer.update_view(camera.position)
    assert(state.ok and state.loaded_instances==8407 and state.commands>0,str(state))
    if state.particles_unsupported_quads>0:
     gap_frames+=1
     assert(not state.particle_recovery_gaps.is_empty())
    if state.particles_visible>0:supported_visible=true
    await process_frame
    await RenderingServer.frame_post_draw
  assert(gap_frames>0 and supported_visible,"Particle gap not exercised or all supported particles disabled")
  camera.position=start-forward*8+Vector3.UP*3
  camera.look_at(start)
  state=renderer.update_view(camera.position)
  assert(state.ok and state.triangles>0)
  print("PARTICLE_GAP_BOUNDARY_OK frames=",gap_frames)
 for enabled in [true,false]:
  renderer.set_show_lod(enabled)
  state=renderer.update_view(camera.position)
  assert(state.ok and renderer.get_lod_state().native_culling_views[0].native_selection!=enabled)
  await process_frame
  await RenderingServer.frame_post_draw
 if DisplayServer.get_name()!="headless":root.get_texture().get_image().save_png(capture_dir.path_join("native-track11.png"))
 renderer.clear_track()
 assert(renderer.get_lod_state().native_culling_views.is_empty())
 assert(renderer.get_lod_state().resident_surface_nodes==0)
 assert(not renderer.request_track(-1).ok)
 state=renderer.request_track(11)
 while state.get("loading",false):
  state=renderer.pump_load()
  await process_frame
 assert(state.ok,str(state))
 renderer.clear_track()
 print("NATIVE_TRACK_CULLING_OK")
 quit(0)
