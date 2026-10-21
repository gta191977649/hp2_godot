extends SceneTree
# Host text is DPI-aware in Release without ImGui or viewer dependencies.
func _initialize() -> void:call_deferred("_run")
func _run() -> void:
 var display=root.get_node("UiDisplay")
 var original_host=display._host
 assert(original_host!=null and original_host.get_ui_scale()>=0.5)
 display._host=null
 var label=Label.new()
 label.text="Host DPI text"
 label.add_theme_font_size_override("font_size",15)
 root.add_child(label)
 var styled=Label.new()
 styled.label_settings=LabelSettings.new()
 styled.label_settings.font_size=17
 var shared=styled.label_settings
 root.add_child(styled)
 await process_frame
 for scale in [1.0,1.5,2.0,1.0]:
  display._apply_scale(scale)
  assert(label.get_theme_font_size("font_size")==roundi(15*scale))
  assert(styled.label_settings.font_size==roundi(17*scale))
  assert(shared.font_size==17,"DPI mutated shared label settings")
 root.remove_child(label)
 assert(not display._controls.has(label.get_instance_id()))
 root.add_child(label)
 await process_frame
 assert(label.get_theme_font_size("font_size")==15)
 display._host=original_host
 print("UI_TEXT_DPI_OK Release-safe fonts, shared settings and lifecycle")
 quit(0)
