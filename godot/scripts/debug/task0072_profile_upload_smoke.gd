extends Node
# TASK0072 port-side exact UI upload contract. Synthetic static packets only.
var output := ""
var results: Array = []
var host: RefCounted
var surface: Node2D

func _ready() -> void:
 _run.call_deferred()

func _fail(reason: String) -> void:
 push_error(reason)
 get_tree().quit(1)

func _packet(frame: int, compact: bool, bulk: bool) -> Dictionary:
 return {"enabled":true,"engine_frame":frame,"compact_history":compact,"ui_bulk_upload":bulk,"metrics":{"frame_wall_ms":14.0+float(frame%17)*0.1,"process_cpu_ms":8.0+float(frame%11)*0.1,"host_ms":3.0+float(frame%7)*0.1,"sun_readback_ms":0.0,"draw_calls":850,"primitives":56411,"collector_ms":0.25},"viewports":[{"role":"colour","gpu_ms":null,"source_frame":null,"timing_status":"Synthetic unavailable sample"}],"functions":[{"name":"Synthetic upload fixture","ms":3.0,"calls":1,"max_ms":3.0,"source":"Port fixture packet"}],"recording":false,"output":"synthetic-fixture.json","verify_history":true}

func _status(packet: Dictionary) -> Dictionary:
 return {"screen":"frontend","fps":60.0,"ms":16.0,"ui_scale":1.0,"profile_debug":packet}

func _capture(packet: Dictionary, label: String) -> Dictionary:
 var state: Dictionary={}
 # Same frozen engine_frame makes history selection/content identical across modes.
 for frame in 3:
  state=host.console_frame(1280,800,1.0/60.0,_status(packet))
  if not state.get("ok",false): return {"error":"Console draw failed: "+str(state)}
  await get_tree().process_frame
  await RenderingServer.frame_post_draw
 var image: Image=get_viewport().get_texture().get_image()
 if image==null or image.is_empty(): return {"error":"Empty final UI image"}
 var varied := false
 var first := image.get_pixel(0,0)
 for y in range(0,image.get_height(),8):
  for x in range(0,image.get_width(),8):
   if image.get_pixel(x,y)!=first:
    varied=true
    break
  if varied: break
 if not varied or int(state.get("batches",0))<=0: return {"error":"UI fixture produced no visible pixels/batches"}
 var profile: Dictionary=state.get("profile_debug",{})
 if not bool(profile.get("visible",false)) or int(profile.get("history",0))!=240:
  return {"error":"Expected visible Profile graph with 240 history entries: "+str(profile)}
 if bool(state.get("ui_bulk_upload",not bool(packet.ui_bulk_upload)))!=bool(packet.ui_bulk_upload):
  return {"error":"Native upload switch was not acknowledged"}
 var path := output.path_join(label+".png")
 if image.save_png(path)!=OK: return {"error":"Failed to save UI image: "+path}
 var hash := HashingContext.new()
 hash.start(HashingContext.HASH_SHA256)
 hash.update(image.get_data())
 return {"image":image,"sha256":hash.finish().hex_encode(),"width":image.get_width(),"height":image.get_height(),"format":image.get_format(),"file":path,"state":state}

func _pair(compact: bool) -> bool:
 surface=Node2D.new()
 add_child(surface)
 host=ClassDB.instantiate("Ps2RenderHost")
 host.attach(surface.get_canvas_item())
 # Private fixture settings; never reads or writes the user's normal UI ini.
 var settings := output.path_join("layout-compact-"+str(compact)+".ini")
 host.console_init(settings,false)
 host.viewer_ui_snapshot({"kind":"frontend"})
 for frame in 240:
  var row: Dictionary=host.console_frame(1280,800,1.0/60.0,_status(_packet(frame,compact,false)))
  if not row.get("ok",false):
   _fail("History fill failed: "+str(row))
   return false
 var packet := _packet(239,compact,false)
 var baseline := await _capture(packet,"compact-"+str(compact)+"-append")
 packet.ui_bulk_upload=true
 var candidate := await _capture(packet,"compact-"+str(compact)+"-bulk")
 if baseline.has("error") or candidate.has("error"):
  _fail("UI upload capture failed: "+str(baseline.get("error",""))+" "+str(candidate.get("error","")))
  return false
 var a: Image=baseline.image
 var b: Image=candidate.image
 if a.get_size()!=b.get_size() or a.get_format()!=b.get_format() or a.get_data()!=b.get_data():
  _fail("Bulk UI upload differs from append at compact="+str(compact))
  return false
 baseline.erase("image")
 candidate.erase("image")
 results.append({"compact_history":compact,"baseline":baseline,"candidate":candidate,"exact_pixels":true,"frozen_engine_frame":239})
 host=null
 surface.queue_free()
 await get_tree().process_frame
 await RenderingServer.frame_post_draw
 return true

func _run() -> void:
 if DisplayServer.get_name()=="headless":
  get_tree().quit(77)
  return
 if not ClassDB.class_exists("Ps2RenderHost"):
  _fail("Profile upload fixture requires the native Debug extension")
  return
 output=ProjectSettings.globalize_path("res://../local/task0072/profile-upload")
 for argument in OS.get_cmdline_user_args():
  if argument.begins_with("--profile-upload-output="): output=argument.trim_prefix("--profile-upload-output=")
 output=output.replace("\\","/").simplify_path()
 if not output.is_absolute_path() or output.to_lower().find("/local/")<0:
  _fail("Profile upload captures require an absolute ignored local/ output")
  return
 if DirAccess.make_dir_recursive_absolute(output)!=OK:
  _fail("Cannot create profile upload output")
  return
 for singleton in ["DebugTools","ProfileDebug"]:
  var overlay := get_node_or_null("/root/"+singleton)
  if overlay is CanvasLayer: overlay.hide()
  if overlay!=null:
   overlay.set_process(false)
   overlay.set_process_input(false)
   overlay.set_process_unhandled_input(false)
 get_window().size=Vector2i(1280,800)
 for compact in [false,true]:
  if not await _pair(compact): return
 var file := FileAccess.open(output.path_join("result.json"),FileAccess.WRITE)
 if file==null:
  _fail("Cannot write profile upload result")
  return
 file.store_string(JSON.stringify({"task":"TASK0072","fixture":"Synthetic frozen Profile Debug upload pixels","engine":Engine.get_version_info(),"rendering_method":RenderingServer.get_current_rendering_method(),"rendering_driver":RenderingServer.get_current_rendering_driver_name(),"pairs":results,"complete":true,"limits":"Exact host UI upload parity only; fixture readbacks are untimed and do not establish performance or PS2 equivalence."},"  "))
 file.close()
 print("TASK0072_PROFILE_UPLOAD_OK frozen 240-entry graph history; exact append/bulk bytes with compact false and true")
 get_tree().quit(0)
