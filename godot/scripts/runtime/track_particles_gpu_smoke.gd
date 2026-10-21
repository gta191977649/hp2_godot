extends Node3D
# Real static emitter coordinates from original Track11 data; PC regression only.
var renderer: Node3D
var camera: Camera3D
func _ready() -> void:
 _run.call_deferred()
func _fail(reason: String) -> void:
 push_error(reason)
 get_tree().quit(1)
func _frame(dt: float) -> Dictionary:
 renderer.set_visual_timestep(dt)
 return renderer.update_view(camera.position)
# CPU mesh contains original GS XY fixed16 packet coordinates, before host shader conversion.
func _packet_boxes(node: Node, boxes: Array[Rect2]) -> void:
 if node is MeshInstance3D and node.is_visible_in_tree() and node.mesh:
  var material=node.material_override
  if material is ShaderMaterial and "particle_z" in material.shader.code:
   var vertices: PackedVector3Array=node.mesh.surface_get_arrays(0)[Mesh.ARRAY_VERTEX]
   var size := Vector2(get_viewport().get_visible_rect().size)
   for first in range(0,vertices.size(),4):
    var point := Vector2(vertices[first].x/16.0-1792.0,vertices[first].y/16.0-1824.0)*size/Vector2(512,448)
    var box := Rect2(point,Vector2.ZERO)
    for j in range(1,4):
     point=Vector2(vertices[first+j].x/16.0-1792.0,vertices[first+j].y/16.0-1824.0)*size/Vector2(512,448)
     box=box.expand(point)
    boxes.append(box.grow(2.0))
 for child in node.get_children():_packet_boxes(child,boxes)
func _particle_material(node: Node) -> ShaderMaterial:
 if node is MeshInstance3D and node.is_visible_in_tree():
  var material=node.material_override
  if material is ShaderMaterial and "particle_z" in material.shader.code:return material
 for child in node.get_children():
  var found := _particle_material(child)
  if found:return found
 return null
# Test-only asymmetric packet isolates the production shader's GS XY transport.
# It is not a reconstruction of an original emitter or game behavior.
func _transport_probe() -> bool:
 var original := _particle_material(renderer)
 if original==null:
  _fail("No actual particle material available for host transport probe")
  return false
 var viewport := SubViewport.new();viewport.use_hdr_2d=RenderingServer.get_current_rendering_method()!="gl_compatibility"
 viewport.size=Vector2i(256,256)
 viewport.own_world_3d=true
 viewport.transparent_bg=true
 viewport.render_target_update_mode=SubViewport.UPDATE_ALWAYS
 add_child(viewport)
 var probe_camera := Camera3D.new()
 viewport.add_child(probe_camera)
 probe_camera.current=true
 var instance := MeshInstance3D.new()
 var mesh := ArrayMesh.new()
 var arrays := []
 arrays.resize(Mesh.ARRAY_MAX)
 var vertices := PackedVector3Array()
 for uv in [Vector2(0.30,0.20),Vector2(0.40,0.20),Vector2(0.30,0.30),Vector2(0.40,0.30)]:
  vertices.append(Vector3((1792+uv.x*512)*16,(1824+uv.y*448)*16,0xffffff))
 arrays[Mesh.ARRAY_VERTEX]=vertices
 arrays[Mesh.ARRAY_TEX_UV]=PackedVector2Array([Vector2(0,0),Vector2(1,0),Vector2(0,1),Vector2(1,1)])
 arrays[Mesh.ARRAY_COLOR]=PackedColorArray([Color(1,1,1,0.5),Color(1,1,1,0.5),Color(1,1,1,0.5),Color(1,1,1,0.5)])
 arrays[Mesh.ARRAY_INDEX]=PackedInt32Array([0,1,2,1,3,2])
 mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES,arrays)
 instance.mesh=mesh
 instance.material_override=original.duplicate()
 viewport.add_child(instance)
 instance.custom_aabb=AABB(Vector3(-10000,-10000,-10000),Vector3(20000,20000,20000))
 for index in 4:
  await get_tree().process_frame
  await RenderingServer.frame_post_draw
 var image := viewport.get_texture().get_image()
 var count := 0
 var outside := 0
 var expected := Rect2(Vector2(0.30,0.20)*256,Vector2(0.10,0.10)*256).grow(2)
 for y in image.get_height():
  for x in image.get_width():
   var colour := image.get_pixel(x,y)
   if maxf(colour.r,maxf(colour.g,colour.b))<0.02:continue
   count+=1
   if not expected.has_point(Vector2(x+0.5,y+0.5)):outside+=1
 var output := ProjectSettings.globalize_path("res://../local/task0054")
 DirAccess.make_dir_recursive_absolute(output)
 image.save_png(output.path_join("particle-asymmetric-transport.png"))
 print("PARTICLE_TRANSPORT pixels=",count," outside=",outside)
 viewport.queue_free()
 if count<4 or outside>0:
  _fail("Production particle shader mirrors or loses asymmetric GS packet transport")
  return false
 return true
func _anchor_check(pose: int) -> bool:
 renderer.set_effect_enabled("particles",true)
 for index in 4:
  _frame(0)
  await get_tree().process_frame
  await RenderingServer.frame_post_draw
 var boxes: Array[Rect2]=[]
 _packet_boxes(renderer,boxes)
 var enabled: Image=get_viewport().get_texture().get_image()
 renderer.set_effect_enabled("particles",false)
 for index in 4:
  _frame(0)
  await get_tree().process_frame
  await RenderingServer.frame_post_draw
 var disabled: Image=get_viewport().get_texture().get_image()
 var output := ProjectSettings.globalize_path("res://../local/task0054")
 DirAccess.make_dir_recursive_absolute(output)
 enabled.save_png(output.path_join("particle-anchor-"+str(pose)+"-enabled.png"))
 disabled.save_png(output.path_join("particle-anchor-"+str(pose)+"-disabled.png"))
 var changed := 0
 var outside := 0
 for y in enabled.get_height():
  for x in enabled.get_width():
   var a := enabled.get_pixel(x,y)
   var b := disabled.get_pixel(x,y)
   if maxf(absf(a.r-b.r),maxf(absf(a.g-b.g),absf(a.b-b.b)))<0.025:continue
   changed+=1
   var contained := false
   for box in boxes:
    if box.has_point(Vector2(x+0.5,y+0.5)):
     contained=true
     break
   if not contained:outside+=1
 print("PARTICLE_ANCHOR ",pose," boxes=",boxes.size()," changed=",changed," outside=",outside)
 if boxes.is_empty() or changed<4 or outside>maxi(4,int(changed*0.01)):
  _fail("Particle GPU pixels disagree with original GS packet boxes at pose "+str(pose))
  return false
 return true
func _run() -> void:
 if DisplayServer.get_name()=="headless":
  get_tree().quit(77)
  return
 var overlay := get_node_or_null("/root/DebugTools")
 if overlay:overlay.hide()
 get_window().size=Vector2i(960,540)
 camera=Camera3D.new()
 camera.keep_aspect=Camera3D.KEEP_WIDTH
 camera.fov=float(0x32dc)*360.0/65536.0
 add_child(camera)
 camera.current=true
 renderer=ClassDB.instantiate("Ps2TrackRenderer")
 add_child(renderer)
 if renderer.get_catalog().is_empty():
  get_tree().quit(77)
  return
 var state: Dictionary=renderer.request_track(11)
 var deadline := Time.get_ticks_msec()+120000
 while state.get("loading",false):
  if Time.get_ticks_msec()>deadline:
   _fail("Particle scene load timed out")
   return
  state=renderer.pump_load()
  await get_tree().process_frame
 if not state.get("ok",false):
  _fail("Particle scene load failed: "+str(state))
  return
 renderer.restore_render_defaults()
 camera.position=Vector3(170,-48,-640)
 camera.look_at(Vector3(190.068,-53.8522,-654.923))
 for index in 120:
  state=_frame(1.0/60.0)
  if not state.get("ok",false):
   _fail("Original particle submission failed: "+str(state))
   return
  await get_tree().process_frame
  await RenderingServer.frame_post_draw
 if int(state.get("particles_active",0))==0 or int(state.get("particles_visible",0))==0:
  _fail("Actual forest-fire emitter did not produce visible native packets: "+str(state))
  return
 if not await _transport_probe():return
 for index in 3:
  _frame(0)
  await RenderingServer.frame_post_draw
 var enabled: Image=get_viewport().get_texture().get_image()
 renderer.set_effect_enabled("particles",false)
 for index in 3:
  state=_frame(0)
  await RenderingServer.frame_post_draw
 if int(state.get("particles_visible",-1))!=0:
  _fail("Particle permission did not suppress GPU submission")
  return
 var disabled: Image=get_viewport().get_texture().get_image()
 if enabled.get_data()==disabled.get_data():
  _fail("Visible native particle packets produced no GPU pixel contribution")
  return
 var output := ProjectSettings.globalize_path("res://../local/task0052")
 DirAccess.make_dir_recursive_absolute(output)
 enabled.save_png(output.path_join("world-particles-enabled.png"))
 disabled.save_png(output.path_join("world-particles-disabled.png"))
 # TASK0054: simulation frozen; only camera yaw, pitch and translation change.
 renderer.set_effect_enabled("sun",false)
 for pose in 3:
  camera.position=Vector3(170,-48,-640)+Vector3(pose*0.8,pose*0.3,-pose*0.6)
  if pose==2:camera.position=Vector3(169,-46,-639)
  camera.look_at(Vector3(190.068,-53.8522,-654.923))
  camera.rotate_object_local(Vector3.UP,0.04 if pose!=1 else -0.05)
  camera.rotate_object_local(Vector3.RIGHT,0.06 if pose!=2 else 0.03)
  if not await _anchor_check(pose):return
 renderer.restore_render_defaults()
 renderer.clear_track()
 print("TRACK_PARTICLES_GPU_OK original forest-fire packets, shared alpha ranks and permission gate")
 get_tree().quit(0)
