extends SceneTree
# Port-side draw-budget diagnosis. Isolation gates are not performance acceptance.
var scene: Node
var viewports: Array[Viewport]=[]
func _initialize() -> void:
 _run.call_deferred()
func _find(node: Node) -> void:
 if node is Viewport:viewports.append(node)
 for child in node.get_children():_find(child)
func _frame() -> Dictionary:
 scene.track_renderer.set_visual_state(scene.vehicle_renderer.get_visual_state())
 scene.track_renderer.set_visual_timestep(0)
 var state: Dictionary=scene.track_renderer.update_view(scene.camera.position)
 scene.vehicle_renderer.set_environment_capture(scene.track_renderer.get_environment_capture())
 scene.vehicle_renderer.update_view(scene.camera.position,-scene.camera.basis.z,float(state.get("original_focal_h",1)),-1)
 return state
func _run() -> void:
 if DisplayServer.get_name()=="headless":
  quit(77)
  return
 root.size=Vector2i(1920,1080)
 DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
 var debug := root.get_node_or_null("DebugTools")
 if debug:
  debug.hide()
  debug.set_process(false)
 scene=load("res://scenes/race_scene.tscn").instantiate()
 root.add_child(scene)
 current_scene=scene
 var deadline := Time.get_ticks_msec()+120000
 while scene.phase=="Loading" and Time.get_ticks_msec()<deadline:await process_frame
 if scene.phase!="Running":
  push_error("Draw-budget scene failed: "+str(scene.diagnostic))
  quit(1)
  return
 scene.set_process(false)
 scene.simulation.action("pause")
 for child in scene.get_children():
  if child is CanvasLayer:child.hide()
 var results: Array=[]
 for mode in ["original","without_sky","without_car","without_car_or_sky"]:
  scene.track_renderer.set_effect_enabled("sky",mode in ["original","without_car"])
  scene.vehicle_renderer.visible=mode in ["original","without_sky"]
  var state: Dictionary={}
  for frame in 24:
   state=_frame()
   if not state.get("ok",false):
    push_error(str(state))
    quit(1)
    return
   await process_frame
   await RenderingServer.frame_post_draw
  viewports.clear()
  _find(root)
  var frames: Array=[]
  for frame in 8:
   state=_frame()
   var scheduled: Dictionary={}
   for viewport in viewports:scheduled[viewport.get_instance_id()]=not viewport is SubViewport or viewport.render_target_update_mode!=SubViewport.UPDATE_DISABLED
   await process_frame
   await RenderingServer.frame_post_draw
   var rows: Array=[]
   for viewport in viewports:
    var row := {"path":str(viewport.get_path()),"size":str(viewport.get_visible_rect().size),"scheduled":scheduled[viewport.get_instance_id()]}
    for kind in [Viewport.RENDER_INFO_TYPE_VISIBLE,Viewport.RENDER_INFO_TYPE_SHADOW,Viewport.RENDER_INFO_TYPE_CANVAS]:
     row[str(kind)]={"draws":viewport.get_render_info(kind,Viewport.RENDER_INFO_DRAW_CALLS_IN_FRAME),"objects":viewport.get_render_info(kind,Viewport.RENDER_INFO_OBJECTS_IN_FRAME),"primitives":viewport.get_render_info(kind,Viewport.RENDER_INFO_PRIMITIVES_IN_FRAME)}
    rows.append(row)
   frames.append({"total_draws":Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME),"viewports":rows})
  results.append({"mode":mode,"visibility":{"loaded_instances":state.loaded_instances,"considered_instances":state.considered_instances,"visible_instances":state.visible_instances,"loaded_sections":state.loaded_sections,"sections":state.sections,"far":state.effective_far},"sky_draws":state.sky_draws,"submissions":state.view_submissions,"car":scene.vehicle_renderer.get_state(),"frames":frames})
  print("TRACK_DRAW_BREAKDOWN ",mode," ",frames[0].total_draws," sky_nodes=",state.sky_draws)
 scene.track_renderer.restore_render_defaults()
 scene._release()
 var file := FileAccess.open("D:/dev/hp2_godot/local/task0053/draw-breakdown-after.json",FileAccess.WRITE)
 file.store_string(JSON.stringify(results,"  "))
 print("TRACK_DRAW_BREAKDOWN_OK")
 quit(0)
