extends SceneTree
# Port-only all-level debug switch, both LOD backends.
func _initialize() -> void:call_deferred("_run")
func _run() -> void:
 var overlay=root.get_node_or_null("DebugTools")
 if overlay:overlay.hide()
 var world=Node3D.new()
 root.add_child(world)
 var camera=Camera3D.new()
 world.add_child(camera)
 camera.current=true
 for backend in ["original"]:
  var renderer=ClassDB.instantiate("Ps2TrackRenderer")
  world.add_child(renderer)
  assert(not renderer.configure_lod_backend("godot_hlod").ok)
  assert(renderer.configure_lod_backend(backend).ok)
  var state: Dictionary=renderer.request_track(11)
  while state.get("loading",false):
   state=renderer.pump_load()
   await process_frame
  assert(state.ok,str(state))
  var start: Vector3=state.start_position
  var forward: Vector3=state.start_forward
  camera.position=start-forward*8+Vector3.UP*3
  camera.look_at(start)
  for enabled in [false,true,false,true]:
   renderer.set_show_lod(enabled)
   state=renderer.update_view(camera.position)
   assert(state.ok and state.show_lod==enabled,str(state))
   await process_frame
   await RenderingServer.frame_post_draw
   var levels={}
   for node in renderer.get_children():
    if node is MeshInstance3D and node.visible and node.has_meta("hp2_hlod_slot"):
     if enabled:assert(node.visibility_range_begin==0 and node.visibility_range_end==0)
     levels[node.get_meta("hp2_hlod_slot")]=true
   if enabled:assert(levels.size()>1,"All-level debug did not submit multiple authored slots")
   print("SHOW_LOD_FRAME ",backend," enabled=",enabled," slots=",levels.keys())
  renderer.clear_track()
  assert(renderer.get_lod_state().resident_surface_nodes==0)
  renderer.queue_free()
  await process_frame
 print("SHOW_LOD_OK")
 quit()
