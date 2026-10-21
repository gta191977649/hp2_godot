extends SceneTree
# TASK0087 bounded actual Track12 presentation, not a performance capture.
func _initialize() -> void:
 _run.call_deferred()
func require_ok(result: Dictionary) -> bool:
 if not result.get("ok",false):printerr("TRACK12_GAPS_FAIL: ",result);quit(1);return false
 return true
func _run() -> void:
 var holder := Node3D.new()
 root.add_child(holder)
 var camera := Camera3D.new()
 holder.add_child(camera)
 camera.current=true
 camera.near=.05
 var track: Node3D=ClassDB.instantiate("Ps2TrackRenderer")
 holder.add_child(track)
 var loaded: Dictionary=track.request_track(12)
 while loaded.get("loading",false):
  await process_frame
  loaded=track.pump_load()
 if not require_ok(loaded):return
 track.set_camera_mode(1)
 var driving: RefCounted=ClassDB.instantiate("Ps2DrivingRuntime")
 if not require_ok(driving.load_config("911TURBO")):return
 if not require_ok(driving.set_track(12)):return
 track.set_driving_effect_source(driving)
 var car: Node3D=ClassDB.instantiate("Ps2VehicleRenderer")
 holder.add_child(car)
 # Original traffic asset has no blur solid; positive original wheel alpha is valid.
 if not require_ok(car.load_car("CARS/CROWNVIC/GEOMETRY.BIN")):return
 driving.action("run")
 for frame in range(120):
  if not require_ok(driving.set_driving_input(.5,0.0,.1 if frame>60 else 0.0,0.0)):return
  var state: Dictionary=driving.tick(1.0/60.0)
  if not require_ok(state):return
  var body: Transform3D=state.body
  camera.position=body.origin+Vector3(5,4,-8)
  camera.look_at(body.origin)
  var display := state.duplicate(true)
  for wheel in display.wheels:wheel.blur_alpha=90.0
  if not require_ok(car.apply_driving_snapshot(display)):return
  if not require_ok(car.update_view(camera.position,-camera.global_basis.z,256.0,0)):return
  track.set_visual_timestep(1.0/60.0)
  if not require_ok(track.update_view(camera.position)):return
  await process_frame
  if DisplayServer.get_name()!="headless":await RenderingServer.frame_post_draw
 track.set_driving_effect_source(null)
 track.clear_track()
 car.clear_car()
 await process_frame
 print("TRACK12_GAPS_OK: original track, native driving, optional absent blur, particle backend=",OS.get_cmdline_user_args())
 quit(0)
