extends SceneTree
# TASK0060 host lifecycle test. Same route and visual inputs before/after changes.
var renderer
var camera: Camera3D
var enforce=false
func _initialize() -> void:call_deferred("_run")
func checkpoint(label: String) -> void:
 var s: Dictionary=renderer.get_state()
 var out={"phase":label,"state":s.host_stats,"nodes":s.render_nodes,"commands":s.get("commands",0),"engine_static":OS.get_static_memory_usage(),"engine_nodes":get_node_count(),"engine_resources":Performance.get_monitor(Performance.OBJECT_RESOURCE_COUNT)}
 if renderer.has_method("get_memory_state"):out.memory=renderer.get_memory_state()
 print("MEMORY_CHECKPOINT ",JSON.stringify(out))
func load_track() -> Dictionary:
 var s: Dictionary=renderer.request_track(11)
 var deadline=Time.get_ticks_msec()+120000
 while s.get("loading",false):
  assert(Time.get_ticks_msec()<deadline,"Track load timed out")
  s=renderer.pump_load()
  await process_frame
 assert(s.ok,str(s))
 return s
func tick() -> void:
 renderer.set_visual_timestep(0.0)
 var s: Dictionary=renderer.update_view(camera.position)
 assert(s.ok,str(s))
 await process_frame
 await RenderingServer.frame_post_draw
func _run() -> void:
 enforce=OS.get_cmdline_user_args().has("--memory-enforce")
 var overlay=root.get_node_or_null("DebugTools")
 if overlay:overlay.hide()
 root.size=Vector2i(1920,1080)
 var world=Node3D.new()
 root.add_child(world)
 camera=Camera3D.new()
 camera.keep_aspect=Camera3D.KEEP_WIDTH
 camera.fov=float(0x32dc)*360/65536
 world.add_child(camera)
 camera.current=true
 renderer=ClassDB.instantiate("Ps2TrackRenderer")
 world.add_child(renderer)
 checkpoint("empty")
 var s=await load_track()
 checkpoint("loaded")
 var start: Vector3=s.start_position
 var forward: Vector3=s.start_forward
 camera.position=start-forward*8+Vector3.UP*3
 camera.look_at(start)
 for i in 12:await tick()
 checkpoint("stationary")
 var hashes=[]
 var residency=[]
 for lap in 3:
  var lap_hashes=[]
  for center in [Vector2(-682.44364,290.50248),Vector2(-820.19283,130.55229)]:
   for i in 12:
    camera.position=Vector3(center.y,start.y+50,center.x)+Vector3(i*20,0,i*20)
    camera.rotation=Vector3(sin(i)*0.8,i*0.8,0)
    await tick()
    lap_hashes.append(renderer.frame_summary().get("packet_digest",""))
  hashes.append(lap_hashes)
  if enforce:
   var memory: Dictionary=renderer.get_memory_state()
   assert(memory.inactive_draws<=memory.inactive_draw_limit and memory.inactive_mesh_payload_bytes<=memory.inactive_mesh_limit,str(memory))
   residency.append([memory.draws,memory.source_meshes,memory.depth_proxies,memory.depth_materials,memory.uniform_cache_materials,memory.batch_entries,memory.batch_payload_bytes])
  checkpoint("lap"+str(lap))
 assert(hashes[1]==hashes[2],"Repeated native route packets changed")
 if enforce:assert(residency[1]==residency[2],"Host residency grew on repeated warmed route: "+str(residency))
 print("MEMORY_ROUTE_HASHES ",JSON.stringify(hashes[2]))
 if enforce:
  var m: Dictionary=renderer.get_memory_state()
  assert(m.inactive_draws<=m.inactive_draw_limit,str(m))
  assert(m.debug_geometry_bytes==0,"Disabled debug geometry allocated")
 renderer.clear_track()
 for i in 8:await process_frame
 checkpoint("cleared")
 if enforce:
  var m: Dictionary=renderer.get_memory_state()
  assert(m.draws==0 and m.source_meshes==0 and m.batch_entries==0 and m.depth_proxies==0 and m.materials==0 and m.source_bytes==0,str(m))
 s=await load_track()
 camera.position=start-forward*8+Vector3.UP*3
 camera.look_at(start)
 for i in 12:await tick()
 checkpoint("reloaded")
 var capture=""
 for arg in OS.get_cmdline_user_args():
  if arg.begins_with("--memory-capture="):capture=arg.trim_prefix("--memory-capture=")
 if not capture.is_empty():root.get_texture().get_image().save_png(capture)
 if enforce:
  for switches in [[true,false],[true,true],[false,true],[false,false]]:
   renderer.set_render_collision(switches[0])
   renderer.set_render_material(switches[1])
   await tick()
   var debug: Dictionary=renderer.get_state()
   assert(debug.debug_fill_visible==(switches[0] or switches[1]) and debug.debug_edges_visible==switches[0],"Lazy overlay visibility mismatch")
  assert(renderer.get_memory_state().debug_geometry_bytes==0,"Disabled overlay retained geometry")
 renderer.clear_track()
 for i in 8:await process_frame
 checkpoint("final_clear")
 # Worker cancellation must consume/release the owned result, not retain a future.
 renderer.request_track(11)
 renderer.clear_track()
 for i in 8:await process_frame
 var failed: Dictionary=renderer.request_track(-1)
 assert(not failed.ok and not failed.loading and not str(failed.error).is_empty(),"Invalid track did not report failure")
 checkpoint("cancelled_and_failed")
 if enforce:
  var memory: Dictionary=renderer.get_memory_state()
  assert(memory.source_bytes==0 and memory.draws==0 and memory.source_meshes==0 and memory.depth_proxies==0,str(memory))
 print("TRACK_MEMORY_OK")
 quit(0)
