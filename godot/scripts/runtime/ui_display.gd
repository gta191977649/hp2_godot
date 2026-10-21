extends Node
# Port-only screen UI DPI. World cameras, 3D labels and original bitmap glyph
# coordinates remain in their own spaces. No viewport/render-resolution scaling.
var scale_factor := 1.0
var _host: RefCounted
var _elapsed := 0.0
var _controls: Dictionary = {}
func _ready() -> void:
 process_priority = -100
 if ClassDB.class_exists("Ps2RenderHost"):_host=ClassDB.instantiate("Ps2RenderHost")
 if _host != null and _host.has_method("get_ui_scale"):scale_factor=float(_host.get_ui_scale())
 get_tree().node_added.connect(_on_node_added)
 get_tree().node_removed.connect(_on_node_removed)
 _discover(get_tree().root)
func _discover(node: Node) -> void:
 _on_node_added(node)
 for child in node.get_children():_discover(child)
func _on_node_added(node: Node) -> void:
 if node is Control:_register.call_deferred(weakref(node))
func _on_node_removed(node: Node) -> void:
 if node is Control:_controls.erase(node.get_instance_id())
func _register(reference: WeakRef) -> void:
 var control=reference.get_ref() as Control
 if control==null or not control.is_inside_tree() or _controls.has(control.get_instance_id()):return
 if control.has_meta("_port_ui_dpi_baseline"):
  var original: Dictionary=control.get_meta("_port_ui_dpi_baseline")
  _controls[control.get_instance_id()]=original
  _apply_control(original)
  return
 var fonts: Dictionary={}
 var names=ThemeDB.get_default_theme().get_font_size_list(control.get_class())
 if control is Label or control is BaseButton or control is LineEdit or control is TextEdit:
  if not names.has("font_size"):names.append("font_size")
 for key in names:fonts[key]=control.get_theme_font_size(key)
 # Explicit overrides also cover custom theme names and RichTextLabel variants.
 for property in control.get_property_list():
  var key=String(property.name)
  if key.begins_with("theme_override_font_sizes/"):
   var font_name=key.trim_prefix("theme_override_font_sizes/")
   if control.has_theme_font_size_override(font_name):fonts[font_name]=control.get_theme_font_size(font_name)
 var settings: LabelSettings=null
 if control is Label and control.label_settings != null:
  settings=control.label_settings.duplicate()
  control.label_settings=settings
 var entry={"node":reference,"fonts":fonts,"settings":settings,"settings_size":settings.font_size if settings!=null else 0}
 control.set_meta("_port_ui_dpi_baseline",entry)
 _controls[control.get_instance_id()]=entry
 _apply_control(entry)
func _apply_control(entry: Dictionary) -> void:
 var control=entry.node.get_ref() as Control
 if control==null:return
 for key in entry.fonts:control.add_theme_font_size_override(key,maxi(1,roundi(float(entry.fonts[key])*scale_factor)))
 if entry.settings!=null:entry.settings.font_size=maxi(1,roundi(float(entry.settings_size)*scale_factor))
func _apply_scale(value: float) -> void:
 if not is_finite(value) or value<0.5 or value>8.0:return
 if is_equal_approx(scale_factor,value):return
 scale_factor=value
 for id in _controls.keys():
  if _controls[id].node.get_ref()==null:_controls.erase(id)
  else:_apply_control(_controls[id])
func _process(delta: float) -> void:
 _elapsed+=delta
 if _elapsed<0.2:return
 _elapsed=0.0
 if _host!=null and _host.has_method("get_ui_scale"):_apply_scale(float(_host.get_ui_scale()))
