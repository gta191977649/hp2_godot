extends SceneTree
# TASK0061: actual Track11 UI, forced DPI scales without changing OS settings.
func _initialize() -> void:call_deferred("_run")
func _run() -> void:
 var display=root.get_node("UiDisplay")
 print("UI_DPI_PLATFORM scale=",display.scale_factor)
 var platform_query=display._host
 display._host=null
 root.size=Vector2i(1920,1080)
 var viewer=load("res://scenes/debug/track_viewer.tscn").instantiate()
 root.add_child(viewer)
 current_scene=viewer
 var deadline=Time.get_ticks_msec()+120000
 while viewer.track_pending or not viewer.track_state.get("ok",false):
  assert(Time.get_ticks_msec()<deadline,str(viewer.track_state))
  await process_frame
 viewer.set_process(false)
 var debug=root.get_node("DebugTools")
 var label=Label.new()
 label.text="DPI text15px"
 label.position=Vector2(1000,80)
 label.add_theme_font_size_override("font_size",15)
 root.add_child(label)
 await process_frame
 var capture_dir="user://debug/task0061"
 for arg in OS.get_cmdline_user_args():
  if arg.begins_with("--dpi-capture-dir="):capture_dir=arg.trim_prefix("--dpi-capture-dir=")
 DirAccess.make_dir_recursive_absolute(capture_dir)
 var start: Vector3=viewer.camera.global_position
 var sizes=[]
 for scale in [1.0,1.5,2.0,1.0]:
  display._apply_scale(scale)
  viewer.camera.global_position=start+Vector3(scale*10,scale*3,-scale*5)
  viewer._process(0.0)
  for i in 3:await process_frame
  await RenderingServer.frame_post_draw
  var frame: Dictionary=debug.last_frame
  assert(frame.ok and is_equal_approx(float(frame.ui_scale),scale),str(frame))
  assert(frame.coordinates_visible)
  var camera: Vector3=viewer.camera.global_position
  assert(frame.camera_position_godot.distance_to(camera)<0.001)
  assert(frame.camera_position_hp2.distance_to(Vector3(camera.z,camera.x,camera.y))<0.001)
  assert(label.get_theme_font_size("font_size")==roundi(15*scale),"Godot text was not DPI scaled")
  var revision=int(frame.font_revision)
  await process_frame
  assert(int(debug.last_frame.font_revision)==revision,"Font rebuilt without scale change")
  sizes.append([scale,frame.font_width,frame.font_height])
  var target: Vector2=debug.host.viewer_ui_targets().render_collision
  var mouse=InputEventMouseMotion.new()
  mouse.position=target
  assert(debug.host.console_input(mouse),"DPI-scaled UI target missed input capture")
  root.get_texture().get_image().save_png(capture_dir.path_join("dpi-%d.png" % roundi(scale*100)))
 assert(sizes[1][1]*sizes[1][2]>sizes[0][1]*sizes[0][2],"Font atlas resolution did not increase")
 assert(sizes[3]==sizes[0],"Scale return compounded font dimensions")
 display._apply_scale(2.0)
 root.remove_child(label)
 assert(not display._controls.has(label.get_instance_id()),"DPI service retained removed control")
 root.add_child(label)
 await process_frame
 assert(label.get_theme_font_size("font_size")==30,"Re-entering UI compounded font scale")
 display._apply_scale(1.0)
 display._host=platform_query
 viewer.track_model.clear_track()
 print("UI_DPI_OK ",sizes)
 quit(0)
