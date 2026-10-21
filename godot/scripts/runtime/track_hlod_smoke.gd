extends SceneTree
# TASK0056: real engine HLOD decisions, resource policy, lifecycle and capture probes.
var world: Node3D
var camera: Camera3D
var renderer: Node3D
var cancel_tested := false
func _initialize() -> void:call_deferred("_run")
func _check(value: bool, message: String) -> bool:
 if value:return true
 push_error(message)
 quit(1)
 return false
func _frames(count: int) -> void:
 for i in count:
  await process_frame
  await RenderingServer.frame_post_draw
func _fixture_mesh(color: Color, first: float, last: float) -> MeshInstance3D:
 var mesh := MeshInstance3D.new()
 var box := BoxMesh.new()
 box.size=Vector3(2,2,2)
 mesh.mesh=box
 var shader := Shader.new()
 shader.code="shader_type spatial; render_mode unshaded; void fragment(){ALBEDO=vec3(%f,%f,%f);}" % [color.r,color.g,color.b]
 var material := ShaderMaterial.new()
 material.shader=shader
 mesh.material_override=material
 mesh.visibility_range_begin=first
 mesh.visibility_range_end=last
 world.add_child(mesh)
 return mesh
func _load(number: int) -> bool:
 var frame_before: int=renderer.get_lod_state().visual_frame_counter
 var state: Dictionary=renderer.request_track(number)
 var deadline := Time.get_ticks_msec()+240000
 while state.get("loading",false):
  if Time.get_ticks_msec()>deadline:return _check(false,"HLOD load timeout")
  state=renderer.pump_load()
  if not cancel_tested and state.get("phase","")=="Warming Godot HLOD resources" and state.get("loading",false):
   cancel_tested=true
   renderer.clear_track()
   if not _check(int(renderer.get_lod_state().visual_frame_counter)==frame_before,"Cancelled prewarm changed visual scheduling"):return false
   state=renderer.request_track(number)
  await process_frame
 return _check(state.get("ok",false),"HLOD load failed: "+str(state.get("error"))) and _check(int(renderer.get_lod_state().visual_frame_counter)==frame_before,"Resource warm-up advanced original visual scheduling")
func _run() -> void:
 var overlay := root.get_node_or_null("DebugTools")
 if overlay:overlay.hide()
 root.size=Vector2i(640,360)
 world=Node3D.new()
 root.add_child(world)
 camera=Camera3D.new()
 camera.keep_aspect=Camera3D.KEEP_WIDTH
 camera.fov=float(0x32dc)*360/65536
 world.add_child(camera)
 camera.current=true
 var near_mesh := _fixture_mesh(Color.RED,0,10)
 var far_mesh := _fixture_mesh(Color.GREEN,10.000001,100)
 for distance in [5.0,20.0,5.0]:
  camera.position=Vector3(0,0,distance)
  camera.look_at(Vector3.ZERO)
  await _frames(4)
  if DisplayServer.get_name()!="headless":
   var image := root.get_texture().get_image()
   var pixel := image.get_pixel(image.get_width()/2,image.get_height()/2)
   if not _check(pixel.r>0.8 if distance<10 else pixel.g>0.8,"Engine did not switch HLOD fixture: "+str(pixel)):return
 near_mesh.queue_free()
 far_mesh.queue_free()
 await process_frame
 renderer=ClassDB.instantiate("Ps2TrackRenderer")
 world.add_child(renderer)
 if not _check(renderer.configure_lod_backend("godot_hlod").ok,"HLOD configuration failed"):return
 if not _check(not renderer.configure_lod_backend("invalid").ok,"Invalid backend accepted"):return
 if not _check(not renderer.configure_optimizations({"instancing":true}).ok,"Conflicting MultiMesh accepted"):return
 if not await _load(11):return
 if not _check(not renderer.configure_lod_backend("original").ok,"Loaded backend changed"):return
 var state: Dictionary=renderer.get_state()
 var start: Vector3=state.start_position
 var forward: Vector3=state.start_forward
 for fov in [float(0x32dc)*360/65536,75.0]:
  camera.fov=fov
  for offset in [0.0,50.0,-25.0]:
   var target: Vector3=start+forward*offset
   camera.position=target-forward*8+Vector3.UP*3
   camera.look_at(target)
   state=renderer.update_view(camera.position)
   if not _check(state.ok,"HLOD frame failed: "+str(state.get("error"))):return
   await _frames(3)
   var anchors := {}
   var count := 0
   for node in renderer.get_children():
    if node is MeshInstance3D and node.has_meta("hp2_hlod_instance"):
     var instance: int=node.get_meta("hp2_hlod_instance")
     var anchor: Vector3=node.global_transform*node.custom_aabb.get_center()
     var configured: Dictionary=renderer.get_lod_ranges(instance,0)
     if not _check(configured.ok and anchor.distance_to(configured.anchor)<0.005,"Surface range center differs from source anchor"):return
     if anchors.has(instance):
      if not _check(anchor.distance_to(anchors[instance])<0.005,"LOD levels have different range centers"):return
     else:anchors[instance]=anchor
     if not _check(node.visibility_range_end>node.visibility_range_begin and node.visibility_range_begin_margin==0 and node.visibility_range_end_margin==0,"Invalid engine range"):return
     count+=1
   if not _check(count>0,"No persistent HLOD instances"):return
   var proxies := world.find_children("*","MeshInstance3D",true,false)
   var proxy_count := 0
   for proxy in proxies:
    if proxy.has_meta("hp2_hlod_depth_proxy"):
     var configured: Dictionary=renderer.get_lod_ranges(proxy.get_meta("hp2_hlod_instance"),0)
     var found := false
     for level in configured.levels:
      if level.slot==proxy.get_meta("hp2_hlod_slot"):
       found=proxy.visibility_range_begin==level.begin and proxy.visibility_range_end==level.end
     if not _check(found,"Depth proxy uses a different HLOD interval"):return
     proxy_count+=1
   if DisplayServer.get_name()!="headless" and not _check(proxy_count>0,"No HLOD depth proxies inspected"):return
   print("TRACK_HLOD_FRAME ",renderer.get_lod_state()," engine_draw_calls=",Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME))
 if DisplayServer.get_name()!="headless":
  var output := ProjectSettings.globalize_path("res://../local/task0056")
  DirAccess.make_dir_recursive_absolute(output)
  root.get_texture().get_image().save_png(output.path_join("track-hlod.png"))
 renderer.clear_track()
 if not _check(renderer.get_lod_state().resident_surface_nodes==0,"HLOD resources retained after clear"):return
 if not _check(not renderer.request_track(-1).ok,"Invalid catalogue request accepted"):return
 if not await _load(21):return
 renderer.clear_track()
 if not _check(renderer.configure_lod_backend("original").ok,"Backend reset after clear failed"):return
 print("TRACK_HLOD_GPU_OK")
 quit(0)

