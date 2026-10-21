extends Node
# Actual ImGui clicks through the existing UI action chain; no original-game claim.
var scene: Node
func _ready() -> void:
 _run.call_deferred()
func _frames(count: int=3) -> void:
 for index in count:await get_tree().process_frame
func _fail(reason: String) -> void:
 push_error(reason)
 get_tree().quit(1)
func _send(event: InputEvent) -> void:
 Input.parse_input_event(event)
 await _frames(2)
func _click(point: Vector2) -> void:
 var motion := InputEventMouseMotion.new()
 motion.position=point
 await _send(motion)
 var button := InputEventMouseButton.new()
 button.position=point
 button.button_index=MOUSE_BUTTON_LEFT
 button.pressed=true
 await _send(button)
 button=button.duplicate()
 button.pressed=false
 await _send(button)
 await _frames()
func _target(name: String) -> bool:
 await _frames()
 var targets: Dictionary=DebugTools.host.viewer_ui_targets()
 if not targets.has(name):
  _fail("Missing ImGui target "+name)
  return false
 await _click(targets[name])
 return true
func _allowed(id: String) -> bool:
 for effect in scene.track_renderer.get_effects():
  if effect.id==id:return bool(effect.enabled)
 return false
func _run() -> void:
 if DisplayServer.get_name()=="headless":
  get_tree().quit(77)
  return
 get_window().size=Vector2i(1280,800)
 scene=load("res://scenes/race_scene.tscn").instantiate()
 add_child(scene)
 var deadline := Time.get_ticks_msec()+120000
 while scene.phase=="Loading":
  if Time.get_ticks_msec()>deadline:
   _fail("Effects UI scene load timed out")
   return
  await get_tree().process_frame
 if scene.phase!="Running":
  _fail("Effects UI scene failed: "+str(scene.diagnostic))
  return
 scene.simulation.action("pause")
 scene.track_renderer.restore_render_defaults()
 # Use a fixture-owned layout; a saved user scroll position can put recorded
 # target coordinates outside the visible panel and click underlying controls.
 var ini := ProjectSettings.globalize_path("user://debug/task0052-effects-ui.ini")
 var reset := FileAccess.open(ini,FileAccess.WRITE)
 reset.close()
 DebugTools.host.console_init(ini,false)
 DebugTools.show()
 Input.mouse_mode=Input.MOUSE_MODE_VISIBLE
 await _frames(5)
 if not DebugTools.host.viewer_ui_targets().has("effect_roadshine"):
  if not await _target("track_effects_group"):return
 if not _allowed("roadshine"):
  _fail("Original Roadshine permission default must be enabled")
  return
 if not await _target("effect_roadshine"):return
 if _allowed("roadshine"):
  _fail("Actual ImGui checkbox did not disable shared Roadshine")
  return
 if not await _target("effect_roadshine"):return
 if not _allowed("roadshine"):
  _fail("Actual ImGui checkbox did not restore shared Roadshine")
  return
 await RenderingServer.frame_post_draw
 var path := ProjectSettings.globalize_path("res://../local/task0052")
 DirAccess.make_dir_recursive_absolute(path)
 get_viewport().get_texture().get_image().save_png(path.path_join("effects-imgui.png"))
 if not await _target("track_effects_group"):return
 if not DebugTools.host.viewer_ui_targets().has("track_distance_override"):
  if not await _target("track_visibility_group"):return
 var original_commands: int=scene.track_renderer.get_state().view_submissions[0].commands
 if not scene.track_renderer.get_render_controls().render_props:
  _fail("Props default must be enabled")
  return
 if not await _target("track_props"):return
 scene.track_renderer.update_view(scene.track_renderer.get_viewport().get_camera_3d().global_position)
 var hidden_commands: int=scene.track_renderer.get_state().view_submissions[0].commands
 if scene.track_renderer.get_render_controls().render_props or hidden_commands>=original_commands or hidden_commands<=0:
  _fail("Props checkbox did not hide placed scenery while retaining world geometry: enabled="+str(scene.track_renderer.get_render_controls().render_props)+" commands="+str(original_commands)+" hidden="+str(hidden_commands))
  return
 if not await _target("track_props"):return
 scene.track_renderer.update_view(scene.track_renderer.get_viewport().get_camera_3d().global_position)
 if not scene.track_renderer.get_render_controls().render_props or scene.track_renderer.get_state().view_submissions[0].commands!=original_commands:
  _fail("Props checkbox did not restore original draws")
  return
 print("TRACK_PROPS_UI_OK commands=",original_commands," hidden=",hidden_commands)
 if not await _target("track_distance_override"):return
 if not scene.track_renderer.get_render_controls().distance_override:
  _fail("Actual ImGui visibility override did not reach renderer")
  return
 if not await _target("track_distance_reset"):return
 if scene.track_renderer.get_render_controls().distance_override:
  _fail("Actual ImGui visibility restore did not reach renderer")
  return
 scene.track_renderer.restore_render_defaults()
 scene._release()
 print("TRACK_EFFECTS_UI_OK ImGui effects and visibility actions")
 get_tree().quit(0)
