extends Node3D
# TASK0052 actual-asset lifecycle/ownership regression; original assets external.
var camera: Camera3D
var first: Node3D
var second: Node3D
func _fail(reason: String) -> void:
 push_error(reason)
 get_tree().quit(1)
func _ready() -> void:
 _run.call_deferred()
func _run() -> void:
 if DisplayServer.get_name()=="headless":
  get_tree().quit(77)
  return
 var overlay := get_node_or_null("/root/DebugTools")
 if overlay:overlay.hide()
 get_window().size=Vector2i(640,360)
 camera=Camera3D.new()
 camera.near=.05
 camera.far=20000
 camera.keep_aspect=Camera3D.KEEP_WIDTH
 camera.fov=float(0x32DC)*360.0/65536.0
 camera.cull_mask=0xfff81
 add_child(camera)
 camera.current=true
 first=ClassDB.instantiate("Ps2TrackRenderer")
 second=ClassDB.instantiate("Ps2TrackRenderer")
 add_child(first)
 add_child(second)
 second.hide()
 if first.get_catalog().is_empty():
  if str(first.get_state().get("error","")).contains("not configured"):
   get_tree().quit(77)
  else:_fail("Original track catalogue unavailable")
  return
 var a: Dictionary=first.request_track(11)
 var b: Dictionary=second.request_track(11)
 var deadline := Time.get_ticks_msec()+120000
 while a.get("loading",false) or b.get("loading",false):
  if Time.get_ticks_msec()>deadline:
   _fail("Original track load timed out")
   return
  a=first.pump_load()
  b=second.pump_load()
  await get_tree().process_frame
 if not a.get("ok",false) or not b.get("ok",false):
  _fail("Original track pair failed: "+str(a)+str(b))
  return
 var position: Vector3=a.start_position
 var forward: Vector3=a.start_forward
 camera.position=position-forward*8+Vector3.UP*3
 camera.look_at(position,Vector3.UP)
 a=first.update_view(camera.position)
 if not a.get("ok",false) or camera.cull_mask!=0:
  _fail("Visible first renderer did not acquire postprocess camera: "+str(a))
  return
 var skip_source: bool=a.get("host_stats",{}).get("postprocess",{}).get("source_3d_skip_enabled",false)
 if skip_source and not get_viewport().disable_3d:
  _fail("Empty source3D viewport was not suppressed")
  return
 await get_tree().process_frame
 a=first.update_view(camera.position)
 if not a.get("ok",false):
  _fail("Source camera became unavailable after source3D suppression: "+str(a))
  return
 b=second.update_view(camera.position)
 if not b.get("ok",false) or camera.cull_mask!=0:
  _fail("Hidden renderer changed another compositor's camera")
  return
 second.show()
 b=second.update_view(camera.position)
 if b.get("ok",false) or not str(b.get("error","")).contains("already owned") or camera.cull_mask!=0:
  _fail("Second visible compositor did not reject shared camera safely: "+str(b))
  return
 first.hide()
 if camera.cull_mask!=0xfff81:
  _fail("Hidden owner did not restore exact camera mask")
  return
 if get_viewport().disable_3d:
  _fail("Hidden owner did not restore source3D rendering")
  return
 b=second.request_track(11)
 while b.get("loading",false):
  b=second.pump_load()
  await get_tree().process_frame
 if not b.get("ok",false):
  _fail("Intentional compositor conflict failed clean reload")
  return
 b=second.update_view(camera.position)
 if not b.get("ok",false) or camera.cull_mask!=0:
  _fail("Camera ownership did not transfer after release: "+str(b))
  return
 second.clear_track()
 if camera.cull_mask!=0xfff81:
  _fail("Track clear failed to restore camera mask")
  return
 if get_viewport().disable_3d:
  _fail("Track clear failed to restore source3D rendering")
  return
 first.clear_track()
 print("TRACK_POSTPROCESS_LIFECYCLE_GPU_OK hidden/reject/transfer/clear")
 get_tree().quit(0)
