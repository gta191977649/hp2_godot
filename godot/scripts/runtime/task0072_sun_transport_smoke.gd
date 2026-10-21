extends Node3D
# TASK0072 port-side frame scheduling fixture. No original-runtime equivalence claim.
# Clear-sky and below-road anchors are from the existing Track11 Sun fixture.
const CLEAR := Vector3(-732,300,-98)
const BLOCKED := Vector3(-732.5417,-94.604,-98.08092)
const SUN := Vector3(650,1300,-3900)
var renderer: Node3D
var camera: Camera3D
var output := ""
var rows: Array = []
var errors: Array[String] = []
var finished := false
var expected_mode := "auto"

func _ready() -> void:
 _run.call_deferred()

func _write() -> void:
 var file := FileAccess.open(output.path_join("frames.json"),FileAccess.WRITE)
 if file==null:
  push_error("Sun fixture cannot write manifest: "+output)
  return
 file.store_string(JSON.stringify({"task":"TASK0072","fixture":"production Sun transport lifecycle","expected_mode":expected_mode,"engine":Engine.get_version_info(),"rendering_method":RenderingServer.get_current_rendering_method(),"rendering_driver":RenderingServer.get_current_rendering_driver_name(),"arguments":OS.get_cmdline_user_args(),"viewport":[get_viewport().get_visible_rect().size.x,get_viewport().get_visible_rect().size.y],"visual_timestep":0,"source_poses":{"clear_host":[CLEAR.x,CLEAR.y,CLEAR.z],"blocked_host":[BLOCKED.x,BLOCKED.y,BLOCKED.z],"sun_host":[SUN.x,SUN.y,SUN.z],"provenance":"Existing track_sun_gpu_smoke Track11 original chunk34202 world Sun and below original start-road anchors; camera route is synthetic port-side"},"image_policy":"SHA256 hashes raw Image bytes with format/dimensions; PNG files are viewable captures. Full target readouts are untimed fixture work, never performance evidence.","frames":rows,"errors":errors,"complete":finished and rows.size()==30 and errors.is_empty()},"  "))

func _fail(reason: String) -> void:
 errors.append(reason)
 if not output.is_empty(): _write()
 push_error(reason)
 get_tree().quit(1)

func _load_track() -> bool:
 var state: Dictionary=renderer.request_track(11)
 var deadline := Time.get_ticks_msec()+120000
 while state.get("loading",false):
  if Time.get_ticks_msec()>deadline:
   _fail("Sun transport Track11 load timed out")
   return false
  state=renderer.pump_load()
  await get_tree().process_frame
 if not state.get("ok",false):
  _fail("Sun transport Track11 load failed: "+str(state))
  return false
 renderer.restore_render_defaults()
 return true

func _pose(index: int) -> String:
 var label := "initial_clear"
 camera.position=CLEAR
 camera.look_at(SUN)
 if index>=6 and index<12:
  label="move_below_road"
  camera.position=CLEAR.lerp(BLOCKED,float(index-5)/6.0)
  camera.look_at(SUN)
 elif index>=12 and index<15:
  label="rotated_offscreen"
  camera.position=BLOCKED
  camera.look_at(SUN)
  camera.rotate_object_local(Vector3.UP,PI)
 elif index>=15 and index<18:
  label="disabled_clear"
 elif index>=18 and index<24:
  label="resumed_clear"
 elif index>=24:
  label="reloaded_initial_clear"
 return label

func _image(image: Image,path: String) -> Dictionary:
 if image==null or image.is_empty(): return {"error":"Missing capture image: "+path}
 var hash := HashingContext.new()
 hash.start(HashingContext.HASH_SHA256)
 hash.update(image.get_data())
 var result := {"sha256":hash.finish().hex_encode(),"width":image.get_width(),"height":image.get_height(),"format":image.get_format(),"raw_bytes":image.get_data().size(),"file":path}
 var saved := image.save_png(path)
 if saved!=OK: result["error"]="Image save failed: "+str(saved)+" "+path
 return result

func _run() -> void:
 if DisplayServer.get_name()=="headless":
  get_tree().quit(77)
  return
 output=ProjectSettings.globalize_path("res://../local/task0072/sun-transport-fixture")
 for argument in OS.get_cmdline_user_args():
  if argument.begins_with("--sun-transport-output="): output=argument.trim_prefix("--sun-transport-output=")
  elif argument.begins_with("--sun-transport-expect="): expected_mode=argument.trim_prefix("--sun-transport-expect=")
 output=output.replace("\\","/").simplify_path()
 if not output.is_absolute_path() or output.to_lower().find("/local/")<0 or expected_mode not in ["auto","cpu","gpu"]:
  push_error("Sun transport requires an absolute ignored local/ output and expect=auto/cpu/gpu")
  get_tree().quit(1)
  return
 if DirAccess.make_dir_recursive_absolute(output)!=OK:
  push_error("Cannot create Sun transport output directory")
  get_tree().quit(1)
  return
 for singleton in ["DebugTools","ProfileDebug"]:
  var overlay := get_node_or_null("/root/"+singleton)
  if overlay is CanvasLayer: overlay.hide()
  if overlay!=null: overlay.set_process(false)
 var requested_size := Vector2i(2880,1695) if "--sun-transport-native" in OS.get_cmdline_user_args() else Vector2i(960,540)
 get_window().size=requested_size
 get_window().content_scale_size=Vector2i.ZERO
 if Vector2i(get_viewport().get_visible_rect().size)!=requested_size:
  _fail("Sun transport requested viewport was constrained")
  return
 DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
 Engine.max_fps=0
 camera=Camera3D.new()
 camera.keep_aspect=Camera3D.KEEP_WIDTH
 camera.fov=float(0x32dc)*360.0/65536.0
 add_child(camera)
 camera.current=true
 renderer=ClassDB.instantiate("Ps2TrackRenderer")
 add_child(renderer)
 if renderer.get_catalog().is_empty():
  _fail("Sun transport requires configured original Track11 assets")
  return
 if not await _load_track(): return
 for index in 30:
  if index==24:
   renderer.clear_track()
   var cleared: Dictionary=renderer.get_state()
   if cleared.get("ok",false) or cleared.get("loading",true) or int(cleared.get("render_nodes",-1))!=0:
    _fail("Sun transport clear retained track resources: "+str(cleared))
    return
   if not await _load_track(): return
  var label := _pose(index)
  if "--sun-transport-spin" in OS.get_cmdline_user_args():
   # Port-only moving/rotating parity route; native culling stays authoritative.
   var phase:=float(index%24)/23.0
   camera.position=Vector3(-455.37,-34.04,-238.34)+Vector3(sin(phase*TAU)*6.0,0,(1.0-cos(phase*TAU))*6.0)
   camera.rotation=Vector3(0,phase*TAU*2.0,0)
   label="spin_"+label
  renderer.set_effect_enabled("sun",index<15 or index>=18)
  renderer.set_visual_timestep(0.0)
  var submitted: Dictionary=renderer.update_view(camera.position)
  if not submitted.get("ok",false):
   _fail("Sun transport original submission failed at frame%d: %s"%[index,str(submitted)])
   return
  var emitted: Dictionary=renderer.frame_summary()
  if str(emitted.get("packet_digest","")).is_empty():
   _fail("Sun transport native frame digest missing")
   return
  var pose: Dictionary={"position_host":[camera.global_position.x,camera.global_position.y,camera.global_position.z],"position_hp2":[camera.global_position.z,camera.global_position.x,camera.global_position.y],"basis":[[camera.global_basis.x.x,camera.global_basis.x.y,camera.global_basis.x.z],[camera.global_basis.y.x,camera.global_basis.y.y,camera.global_basis.y.z],[camera.global_basis.z.x,camera.global_basis.z.y,camera.global_basis.z.z]],"fov":camera.fov,"near":camera.near,"far":camera.far}
  var submission_process_frame := Engine.get_process_frames()
  await get_tree().process_frame
  await RenderingServer.frame_post_draw
  var state: Dictionary=renderer.get_state()
  var host: Dictionary=renderer.get_optimization_state()
  var sun: Dictionary=host.get("sun",{})
  var prefix := output.path_join("frame-%02d-%s"%[index,label])
  var captures: Dictionary={"final":_image(get_viewport().get_texture().get_image(),prefix+"-final.png")}
  var targets: Dictionary=renderer.get_render_targets()
  for key in ["colour","native_depth"]:
   var texture: Texture2D=targets.get(key)
   captures[key]=_image(texture.get_image() if texture!=null else null,prefix+"-"+key+".png")
  rows.append({"index":index,"phase":label,"sun_enabled":index<15 or index>=18,"submission_process_frame":submission_process_frame,"completed_draw_frame":Engine.get_frames_drawn(),"pose":pose,"submitted_native":emitted,"post_draw_sun":sun,"post_draw_costs":state.get("costs",{}),"captures":captures})
  _write()
  for key in captures:
   if captures[key].has("error"):
    _fail(str(captures[key].error))
    return
  if expected_mode=="gpu" and not bool(sun.get("gpu_resident",false)):
   _fail("Expected production GPU Sun transport, got: "+str(sun))
   return
  if expected_mode=="cpu" and bool(sun.get("gpu_resident",false)):
   _fail("Expected reference CPU Sun transport")
   return
  if bool(sun.get("gpu_resident",false)) and not bool(sun.get("gpu_compare",false)):
   if sun.get("uncovered")!=null or int(sun.get("readback_bytes",-1))!=0 or float(sun.get("readback_ms",-1))!=0:
    _fail("Production GPU Sun performed synchronous counter readback: "+str(sun))
    return
  if int(sun.get("count_mismatches",0))!=0:
   _fail("Sun transport counter mismatch: "+str(sun))
   return
  print("TASK0072_SUN_FRAME ",index," ",label," ",JSON.stringify(sun))
 finished=true
 _write()
 print("TASK0072_SUN_TRANSPORT_OK 30 production frames with per-frame colour/depth/final captures; untimed; ",output)
 get_tree().quit(0)
