extends Node3D
# TASK0053: original Track11 Sun world point from chunk34202, not a synthetic light.
var renderer: Node3D
var camera: Camera3D
func _ready() -> void:
 _run.call_deferred()
func _fail(reason: String) -> void:
 push_error(reason)
 get_tree().quit(1)
func _frame() -> Dictionary:
 renderer.set_visual_timestep(0.0)
 return renderer.update_view(camera.position)
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
   _fail("Sun scene load timed out")
   return
  state=renderer.pump_load()
  await get_tree().process_frame
 if not state.get("ok",false):
  _fail("Sun scene load failed: "+str(state))
  return
 renderer.restore_render_defaults()
 camera.position=Vector3(-732,300,-98)
 camera.look_at(Vector3(650,1300,-3900))
 for index in 15:
  state=_frame()
  if not state.get("ok",false):
   _fail("Original Sun submission failed: "+str(state))
   return
  await get_tree().process_frame
  await RenderingServer.frame_post_draw
 print("SUN_FACING_STATE ",JSON.stringify(state))
 var visible_uncovered := int(state.get("sun_uncovered",0))
 var enabled: Image=get_viewport().get_texture().get_image()
 renderer.set_effect_enabled("sun",false)
 for index in 4:
  state=_frame()
  await get_tree().process_frame
  await RenderingServer.frame_post_draw
 var disabled: Image=get_viewport().get_texture().get_image()
 var output := ProjectSettings.globalize_path("res://../local/task0053")
 DirAccess.make_dir_recursive_absolute(output)
 enabled.save_png(output.path_join("sun-facing-enabled.png"))
 disabled.save_png(output.path_join("sun-facing-disabled.png"))
 if visible_uncovered<=0:
  _fail("Sun-facing clear-sky view retained zero original uncovered samples")
  return
 if enabled.get_data()==disabled.get_data():
  _fail("Sun-facing original packets produced no GPU pixel contribution")
  return
 renderer.set_effect_enabled("sun",true)
 # TASK0054: fixed original world anchor under independently changed camera poses.
 var sun_world := Vector3(650,1300,-3900)
 for pose in 3:
  camera.position=Vector3(-732,300,-98)+Vector3(35*pose,12*pose,-20*pose)
  camera.look_at(sun_world)
  camera.rotate_object_local(Vector3.UP,0.055 if pose!=1 else -0.07)
  camera.rotate_object_local(Vector3.RIGHT,0.07 if pose!=2 else -0.065)
  for index in 15:
   state=_frame()
   await get_tree().process_frame
   await RenderingServer.frame_post_draw
  var expected := camera.unproject_position(sun_world)
  var image: Image=get_viewport().get_texture().get_image()
  var weighted := Vector2.ZERO
  var count := 0.0
  # Restrict to the strong disc, excluding broad original flare halos.
  for y in image.get_height():
   for x in image.get_width():
    var c := image.get_pixel(x,y)
    if minf(c.r,minf(c.g,c.b))>0.90:
     weighted+=Vector2(x+0.5,y+0.5)
     count+=1.0
  if count<4:
   _fail("Moving-camera original Sun disc missing at pose "+str(pose))
   return
  var actual := weighted/count
  print("SUN_ANCHOR ",pose," expected=",expected," actual=",actual)
  var anchor_output := ProjectSettings.globalize_path("res://../local/task0054")
  DirAccess.make_dir_recursive_absolute(anchor_output)
  image.save_png(anchor_output.path_join("sun-anchor-"+str(pose)+".png"))
  if actual.distance_to(expected)>6.0:
   _fail("Sun GPU disc disagrees with fixed world anchor: "+str(actual)+" vs "+str(expected))
   return
 # The original start road is at native Z-89.604. This test camera is5m below
 # its actual opaque road polygons; no synthetic occluder or Sun is installed.
 camera.position=Vector3(-732.5417,-94.604,-98.08092)
 camera.look_at(Vector3(650,1300,-3900))
 for index in 15:
  state=_frame()
  await get_tree().process_frame
  await RenderingServer.frame_post_draw
 var blocked_uncovered := int(state.get("sun_uncovered",1024))
 get_viewport().get_texture().get_image().save_png(output.path_join("sun-facing-original-road-occluded.png"))
 print("SUN_ROAD_OCCLUDED_STATE ",JSON.stringify(state))
 if blocked_uncovered>=visible_uncovered:
  _fail("Original road did not reduce Sun depth-readback visibility")
  return
 print("TRACK_SUN_GPU_OK original world point, cached visibility, real-road occlusion and permission gate")
 get_tree().quit(0)
