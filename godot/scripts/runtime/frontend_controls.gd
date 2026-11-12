extends Control
# TASK-0104: actual Control presentation of native semantic widgets.
signal action_requested(action: Dictionary)
const Profiles = preload("res://scripts/runtime/frontend_layout_profiles.gd")
const BindingsPanel = preload("res://scripts/runtime/frontend_bindings_panel.gd")
var layout: Object
var frontend: Node
var bindings: RefCounted
var generation := -1
var nodes: Dictionary = {}
var layers: Dictionary = {}
var fonts: Dictionary = {}
var font_sizes: Dictionary = {}
var snapshot: Dictionary = {}
var bindings_panel: PanelContainer
var tools: HBoxContainer
var fps_slot: Control
var footer: PanelContainer
var shell: VBoxContainer
var content: Control
var scaffold: VBoxContainer
var body_scroll: ScrollContainer
var body_document: Control
var slots: Dictionary = {}
var layout_dirty := true
var layout_revision := 0
var _last_startup := false
var _layout_factor := 1.0
var selection := false
var layer_viewports: Dictionary = {}
var mouse_viewport: SubViewport
var shader: Shader
var pointer_motion_frame := -100
var layout_focus_after_frame := -100

func initialize(owner_node: Node, native: Object, input: RefCounted) -> void:
	frontend = owner_node
	layout = native
	bindings = input
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	shader = Shader.new()
	shader.code = """shader_type canvas_item;
uniform vec4 corner0 : source_color = vec4(1.0);
uniform vec4 corner1 : source_color = vec4(1.0);
uniform vec4 corner2 : source_color = vec4(1.0);
uniform vec4 corner3 : source_color = vec4(1.0);
uniform vec2 uv0 = vec2(0.0); uniform vec2 uv1 = vec2(1.0,0.0);
uniform vec2 uv2 = vec2(1.0); uniform vec2 uv3 = vec2(0.0,1.0);
void fragment(){vec2 t=UV;vec2 sample_uv=mix(mix(uv0,uv1,t.x),mix(uv3,uv2,t.x),t.y);
COLOR=texture(TEXTURE,sample_uv)*mix(mix(corner0,corner1,t.x),mix(corner3,corner2,t.x),t.y);}
"""
	_build_fonts()
	shell = VBoxContainer.new()
	shell.name = "FrontendWindowLayout"
	shell.add_theme_constant_override("separation",0)
	add_child(shell)
	shell.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	content = Control.new()
	content.name = "FrontendContent"
	content.size_flags_vertical = Control.SIZE_EXPAND_FILL
	content.mouse_filter = Control.MOUSE_FILTER_IGNORE
	content.clip_contents = true
	shell.add_child(content)
	content.resized.connect(_invalidate_layout)
	var safe := MarginContainer.new()
	safe.name = "FrontendSafeArea"
	safe.mouse_filter = Control.MOUSE_FILTER_IGNORE
	for side in ["left","right","top","bottom"]: safe.add_theme_constant_override("margin_"+side,8)
	content.add_child(safe)
	safe.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	scaffold = VBoxContainer.new()
	scaffold.mouse_filter = Control.MOUSE_FILTER_IGNORE
	scaffold.add_theme_constant_override("separation",0)
	safe.add_child(scaffold)
	for group in ["header","body","description","footer"]:
		var slot: Control
		if group == "body":
			body_scroll = ScrollContainer.new()
			body_scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
			body_scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
			body_scroll.mouse_filter = Control.MOUSE_FILTER_PASS
			body_document = Control.new()
			body_document.mouse_filter = Control.MOUSE_FILTER_IGNORE
			body_document.size_flags_horizontal = Control.SIZE_EXPAND_FILL
			body_scroll.add_child(body_document)
			body_scroll.get_v_scroll_bar().value_changed.connect(func(_value: float): _invalidate_layout())
			slot = body_scroll
		else:
			slot = Control.new()
			slot.mouse_filter = Control.MOUSE_FILTER_IGNORE
		slot.name = "FrontendRegion_"+group
		scaffold.add_child(slot)
		slot.resized.connect(_invalidate_layout)
		slots[group] = slot
	footer = PanelContainer.new()
	footer.name = "PcFrontendToolbar"
	footer.z_index = 99
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.015,0.015,0.02,1)
	footer.add_theme_stylebox_override("panel",style)
	shell.add_child(footer)
	var toolbar_margin := MarginContainer.new()
	for side in ["left","right"]: toolbar_margin.add_theme_constant_override("margin_"+side,12)
	for side in ["top","bottom"]: toolbar_margin.add_theme_constant_override("margin_"+side,4)
	footer.add_child(toolbar_margin)
	var toolbar := HBoxContainer.new()
	toolbar_margin.add_child(toolbar)
	fps_slot = Control.new()
	fps_slot.name = "FrontendImGuiFpsRegion"
	fps_slot.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	fps_slot.mouse_filter = Control.MOUSE_FILTER_IGNORE
	toolbar.add_child(fps_slot)
	tools = HBoxContainer.new()
	tools.name = "PcFrontendTools"
	toolbar.add_child(tools)
	var key_button := Button.new()
	key_button.name = "KeyBinding"
	key_button.text = "KeyBinding"
	key_button.add_theme_font_size_override("font_size",14)
	key_button.pressed.connect(show_bindings)
	tools.add_child(key_button)
	bindings_panel = BindingsPanel.new()
	bindings_panel.name = "PcFrontendBindings"
	bindings_panel.z_index = 110
	add_child(bindings_panel)
	bindings_panel.initialize(bindings)
	bindings_panel.closed.connect(func(): bindings_panel.hide(); release_devices())
	bindings_panel.hide()
	get_viewport().size_changed.connect(_invalidate_layout)

func _build_fonts() -> void:
	var folder := "user://frontend_font_cache_v1"
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(folder))
	for entry in layout.ui_fonts():
		var texture: Texture2D = layout.ui_texture(entry.texture)
		if texture == null or int(entry.height) == 0: continue
		var stem := str(entry.texture)
		var image_name := stem + ".png"
		var font_path := folder.path_join(stem + ".fnt")
		if texture.get_image().save_png(folder.path_join(image_name)) != OK:
			frontend._report("Cannot create private frontend font cache", true)
			continue
		var content := "info face=\"HP2 %s\" size=%d\ncommon lineHeight=%d base=%d scaleW=%d scaleH=%d pages=1 packed=0\npage id=0 file=\"%s\"\nchars count=%d\n" % [stem,entry.height,entry.height,entry.height,texture.get_width(),texture.get_height(),image_name,entry.glyphs.size()]
		for glyph in entry.glyphs:
			var uv: Rect2 = glyph.uv
			content += "char id=%d x=%d y=%d width=%d height=%d xoffset=0 yoffset=0 xadvance=%d page=0 chnl=15\n" % [glyph.code,roundi(uv.position.x),roundi(uv.position.y),roundi(uv.size.x),roundi(uv.size.y),roundi(glyph.advance)]
		content += "kernings count=%d\n" % entry.kerning.size()
		for kern in entry.kerning:
			content += "kerning first=%d second=%d amount=%d\n" % [kern.first,kern.second,roundi(kern.amount)]
		var file := FileAccess.open(font_path, FileAccess.WRITE)
		if file == null:
			frontend._report("Cannot write private frontend font table", true)
			continue
		file.store_string(content)
		file.close()
		var font := FontFile.new()
		if font.load_bitmap_font(font_path) != OK:
			frontend._report("Cannot bind original frontend bitmap font", true)
			continue
		# Preserve fractional native advances after BMFont's integer import.
		for glyph in entry.glyphs:
			var index := font.get_glyph_index(entry.height, glyph.code, 0)
			font.set_glyph_advance(0, entry.height, index, Vector2(glyph.advance, 0))
		font_sizes[entry.texture] = entry.height
		fonts[entry.texture] = font

func _layer_order(layer: int) -> int:
	if layer >= 32: return (layer/32 as int)*32 + _layer_order(layer%32)
	if layer >= 20 and layer <= 24: return 24 - layer
	if layer <= 15: return 5 + 15 - layer
	if layer <= 19: return 21 + 19 - layer
	return 25 + 28 - layer

func _layer(layer: int) -> Control:
	if layers.has(layer): return layers[layer]
	var root := Control.new()
	root.name = "OriginalLayer_%d" % layer
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.set_meta("frontend_layer", layer)
	if selection:
		var viewport := SubViewport.new()
		viewport.name = "ControlLayerViewport_%d" % layer
		viewport.transparent_bg = true
		viewport.disable_3d = true
		viewport.gui_disable_input = false
		viewport.render_target_update_mode = SubViewport.UPDATE_ONCE
		viewport.size = Vector2i(content_size())
		add_child(viewport)
		viewport.add_child(root)
		layer_viewports[layer] = viewport
	else:
		root.z_index = _layer_order(layer)
		content.add_child(root)
	root.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	layers[layer] = root
	return root

func sync() -> Dictionary:
	var current: Dictionary = layout.ui_snapshot()
	if not current.get("ok", false): return current
	if generation != int(current.generation):
		for node in layers.values():
			if is_instance_valid(node): node.hide(); node.queue_free()
		for viewport in layer_viewports.values(): viewport.queue_free()
		layers.clear()
		layer_viewports.clear()
		nodes.clear()
		mouse_viewport = null
		generation = current.generation
		selection = current.selection
		layout_dirty = true

	if _last_startup != frontend.startup_active or snapshot.get("display_offset",Vector2.ZERO) != current.display_offset:
		layout_dirty = true
	_last_startup = frontend.startup_active
	var changed_controls: Array[Control] = []
	var dirty_layers: Dictionary = {}
	snapshot = current
	var seen: Dictionary = {}
	var widgets: Array = current.widgets.duplicate()
	widgets.sort_custom(func(a: Dictionary, b: Dictionary):
		return _layer_order(a.layer+a.stack*32) < _layer_order(b.layer+b.stack*32) if a.layer+a.stack*32 != b.layer+b.stack*32 else false)
	for widget in widgets:
		var movie := _movie_texture(widget)
		if movie != null:
			# Movie planes are runtime resources, absent from the static
			# TexturePack bounds. Preserve the authored widget transform.
			widget.bounds=Rect2(widget.bounds.position,movie.get_size()*widget.scale)
		seen[widget.id] = true
		var control: Control = nodes.get(widget.id)
		var button_needed: bool = widget.kind == "widget" and widget.interactive
		if control != null and ((button_needed != (control is Button)) or (widget.kind=="editor" and not control is LineEdit)):
			if control.has_meta("frontend_clip_root"): control.get_meta("frontend_clip_root").queue_free()
			else: control.queue_free()
			control = null
		if control == null:
			layout_dirty = true
			control = _create(widget)
			nodes[widget.id] = control
			if widget.clip.has_area() or selection or frontend.startup_active or snapshot.page=="View Event Tree" or Profiles.group_for(widget, selection) in ["body","description"]:
				var clip := Control.new()
				clip.name = "Clip_" + control.name
				clip.clip_contents = true
				clip.mouse_filter = Control.MOUSE_FILTER_IGNORE
				_layer(widget.layer+widget.stack*32).add_child(clip)
				clip.add_child(control)
				control.set_meta("frontend_clip_root", clip)
			else: _layer(widget.layer+widget.stack*32).add_child(control)
		var previous: Dictionary = control.get_meta("frontend_authored")
		var movie_widget: bool = widget.texture == 0xc3f0caef or (widget.texture == 0 and frontend.startup_movie_visible)
		if previous != widget or movie_widget or control.is_queued_for_deletion():
			dirty_layers[widget.layer+widget.stack*32] = true
		if previous.bounds != widget.bounds or previous.clip != widget.clip or previous.scale != widget.scale or previous.text_bounds != widget.text_bounds:
			changed_controls.append(control)
		if not selection or previous != widget or movie_widget or layout_dirty: _update(control, widget)
	for id in nodes.keys():
		if not seen.has(id):
			var old: Control = nodes[id]
			var authored: Dictionary = old.get_meta("frontend_authored")
			dirty_layers[authored.layer+authored.stack*32] = true
			if old.has_meta("frontend_clip_root"): old.get_meta("frontend_clip_root").queue_free()
			else: old.queue_free()
			nodes.erase(id)
			layout_dirty = true
	footer.visible = not frontend.startup_active
	if layout_dirty:
		_layout_all()
		for layer in layer_viewports: dirty_layers[layer] = true
	else:
		for control in changed_controls: _layout_widget(control)
	# Engine hover/focus/pressed states can redraw a Button without a native
	# snapshot change. Keep interactive layers live; cache only static art/text.
	for control in nodes.values():
		if control is BaseButton or control is LineEdit or control is Range:
			var authored: Dictionary = control.get_meta("frontend_authored")
			dirty_layers[authored.layer+authored.stack*32] = true
	for layer in dirty_layers:
		if layer_viewports.has(layer): layer_viewports[layer].render_target_update_mode = SubViewport.UPDATE_ONCE
	return {"ok":true,"controls":nodes.size(),"layers":layers.size(),"selection":selection,"layout_revision":layout_revision,"content_size":content_size()}

func _create(widget: Dictionary) -> Control:
	var control: Control
	if widget.kind == "editor":
		var editor := LineEdit.new()
		editor.max_length = 7
		editor.name = "NativeNameEdit"
		editor.text_submitted.connect(func(_text: String): _action(widget.id,"activate"))
		editor.text_changed.connect(func(value: String):
			var allowed := "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!.# "
			var filtered := ""
			for index in value.length():
				if allowed.contains(value[index]): filtered += value[index]
			if filtered != value:
				var caret: int = editor.caret_column
				editor.text = filtered
				editor.caret_column = mini(caret,filtered.length())
			action_requested.emit({"generation":generation,"id":widget.id,"kind":"edit_text","text":filtered,"port":widget.port}))
		editor.add_theme_stylebox_override("normal",StyleBoxEmpty.new())
		editor.add_theme_stylebox_override("focus",StyleBoxEmpty.new())
		editor.grab_focus.call_deferred()
		control = editor
	elif widget.kind == "slider":
		var slider := HSlider.new()
		slider.min_value = 0
		slider.max_value = 1
		slider.step = 0.001
		slider.focus_mode = Control.FOCUS_NONE
		slider.mouse_entered.connect(_action.bind(widget.id,"focus"))
		slider.value_changed.connect(func(value: float):
			if not bindings_panel.visible: action_requested.emit({"generation":generation,"id":widget.id,"kind":"set_value","value":value}))
		slider.add_theme_stylebox_override("slider",StyleBoxEmpty.new())
		slider.add_theme_stylebox_override("grabber_area",StyleBoxEmpty.new())
		slider.add_theme_stylebox_override("grabber_area_highlight",StyleBoxEmpty.new())
		# The original native animated marker stays visible as a TextureRect.
		# HSlider supplies hit testing/dragging, with no second host marker.
		var transparent := Image.create(1,1,false,Image.FORMAT_RGBA8)
		transparent.fill(Color.TRANSPARENT)
		var handle := ImageTexture.create_from_image(transparent)
		for icon in ["grabber","grabber_highlight","grabber_disabled"]: slider.add_theme_icon_override(icon,handle)
		control = slider
	elif widget.interactive:
		var button := Button.new()
		button.flat = true
		for style in ["normal","hover","pressed","focus","disabled"]: button.add_theme_stylebox_override(style,StyleBoxEmpty.new())
		# Native bounds already apply original right/centre alignment (E-0039-F03).
		button.alignment = HORIZONTAL_ALIGNMENT_LEFT
		button.clip_text = false
		button.focus_mode = Control.FOCUS_NONE
		button.pressed.connect(_action.bind(widget.id, "activate",0,generation,widget.port))
		button.mouse_entered.connect(_action.bind(widget.id, "focus",0,generation,widget.port))
		button.gui_input.connect(func(event: InputEvent):
			if event is InputEventMouseButton and event.pressed and event.button_index in [MOUSE_BUTTON_WHEEL_UP,MOUSE_BUTTON_WHEEL_DOWN]:
				_action(widget.id,"adjust",-1 if event.button_index==MOUSE_BUTTON_WHEEL_UP else 1)
				button.accept_event())
		if int(widget.type) == 2:
			var visual := TextureRect.new()
			visual.name = "OriginalArt"
			visual.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
			visual.mouse_filter = Control.MOUSE_FILTER_IGNORE
			var material := ShaderMaterial.new()
			material.shader = shader
			visual.material = material
			button.add_child(visual)
			visual.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
		if str(widget.id).begins_with("event:"):
			var number := Control.new()
			number.name = "OriginalNumber"
			number.set_meta("frontend_bitmap_font",true)
			number.mouse_filter = Control.MOUSE_FILTER_IGNORE
			button.add_child(number)
		control = button
	elif str(widget.id).begins_with("event:"):
		control = Control.new()
		control.mouse_filter = Control.MOUSE_FILTER_IGNORE
		var number := Control.new()
		number.name = "OriginalNumber"
		number.mouse_filter = Control.MOUSE_FILTER_IGNORE
		control.add_child(number)
	elif int(widget.type) == 1:
		control = Label.new()
		control.mouse_filter = Control.MOUSE_FILTER_IGNORE
	elif int(widget.texture) == 0 and not frontend.startup_movie_visible:
		var solid := ColorRect.new()
		solid.mouse_filter = Control.MOUSE_FILTER_IGNORE
		control = solid
	else:
		var texture := TextureRect.new()
		texture.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		texture.stretch_mode = TextureRect.STRETCH_SCALE
		texture.mouse_filter = Control.MOUSE_FILTER_IGNORE
		var material := ShaderMaterial.new()
		material.shader = shader
		texture.material = material
		control = texture
	control.name = "Native_%s" % str(widget.id).replace(":", "_")
	control.set_meta("frontend_widget_id", widget.id)
	control.set_meta("frontend_generation", generation)
	control.set_meta("frontend_authored", widget)
	return control

func _update(control: Control, widget: Dictionary) -> void:
	control.set_meta("frontend_authored", widget)
	if control is BaseButton:
		control.disabled = not widget.enabled or not widget.interactive
		control.mouse_filter = Control.MOUSE_FILTER_STOP if widget.interactive else Control.MOUSE_FILTER_IGNORE
	if str(widget.id).begins_with("event:"):
		_update_number(control,widget)
		return
	if control is LineEdit:
		if control.text != widget.text:
			var caret: int = control.caret_column
			control.text = widget.text
			control.caret_column = mini(caret,control.text.length())
		if fonts.has(widget.texture):
			control.add_theme_font_override("font",fonts[widget.texture])
			control.add_theme_font_size_override("font_size",font_sizes[widget.texture])
		control.add_theme_color_override("font_color",widget.color)
	elif control is HSlider:
		control.editable = widget.enabled
		control.set_value_no_signal(widget.value)
	elif (control is Button or control is Label) and int(widget.type) == 1:
		if control is Label:
			control.clip_text = false
			control.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART if widget.wrap else TextServer.AUTOWRAP_OFF
		if control.has_node("OriginalNumber"):
			control.text = ""
			control = control.get_node("OriginalNumber")
		control.text = str(widget.text).replace("@", "\n")
		if fonts.has(widget.texture):
			control.add_theme_font_override("font", fonts[widget.texture])
			control.add_theme_font_size_override("font_size", font_sizes[widget.texture])
		for color_name in ["font_color","font_hover_color","font_pressed_color","font_focus_color","font_disabled_color"]:
			control.add_theme_color_override(color_name, widget.color)
		if control is Button: control.disabled = not widget.enabled
	elif control is ColorRect:
		control.color = widget.color
	elif control is TextureRect or (control is Button and int(widget.type) == 2):
		if control is Button:
			control.disabled = not widget.enabled
			control = control.get_node("OriginalArt")
		var movie := _movie_texture(widget)
		control.texture = movie if movie != null or widget.texture == 0xc3f0caef else layout.ui_texture(widget.texture)
		var material: ShaderMaterial = control.material
		for i in 4:
			material.set_shader_parameter("corner%d" % i, widget.colors[i])
			var uv: Vector2 = widget.uv[i]
			if movie != null and widget.texture == 0xc3f0caef:
				# Match the authored quad's inward half-texel offsets.
				uv += Vector2(0.5 if i in [0,3] else -0.5,0.5 if i<2 else -0.5)/movie.get_size()
			material.set_shader_parameter("uv%d" % i, uv)

func _movie_texture(widget: Dictionary) -> Texture2D:
	if widget.texture == 0xc3f0caef:
		return frontend.background_movie.get_texture()
	if widget.texture == 0 and frontend.startup_movie_visible:
		return frontend.intro_movie.get_texture()
	return null

func fps_region() -> Rect2:
	return fps_slot.get_global_rect()

func footer_height() -> float:
	return footer.size.y if footer.visible else 0.0

func content_size() -> Vector2:
	return Vector2(maxf(1,content.size.x),maxf(1,content.size.y))

func _invalidate_layout() -> void:
	layout_dirty = true
	pointer_motion_frame = -100
	layout_focus_after_frame = Engine.get_process_frames()+2

func _region(group: String) -> Rect2:
	if group in ["event","event_detail"]: return event_view_rect()
	if group in ["scene","background"]: return Rect2(Vector2.ZERO,content_size())
	var slot: Control = slots["footer" if group == "logo" else group]
	return Rect2(slot.global_position-content.global_position,slot.size)

func event_view_rect() -> Rect2:
	var body: Control=slots.body
	var region := Rect2(body.global_position-content.global_position,body.size)
	return region

func event_scale() -> float:
	var area := event_view_rect()
	return minf(area.size.x/640.0,area.size.y/maxf(1,snapshot.event_clip.size.y))

func _detail_reference() -> Rect2:
	for widget in snapshot.widgets:
		if int(widget.stack)>0 and widget.wrap and widget.clip.has_area(): return widget.clip
	return snapshot.event_clip

func _detail_scale() -> float:
	var reference := _detail_reference()
	return minf(event_view_rect().size.x/maxf(1,reference.size.x),event_view_rect().size.y/maxf(1,reference.size.y))

func _map_rect(rect: Rect2, group: String, span := false) -> Rect2:
	var area := _region(group)
	var factor := _layout_factor
	if group == "background":
		# Cover the window with a uniformly scaled authored backdrop. Animation
		# translation stays relative to the same centred 640x480 reference.
		factor = maxf(content_size().x/640.0,content_size().y/480.0)
		return Rect2((content_size()-Vector2(640,480)*factor)/2+rect.position*factor,rect.size*factor)
	if group == "event_detail":
		factor=_detail_scale()
		return Rect2(event_view_rect().get_center()+(rect.position-_detail_reference().get_center())*factor,rect.size*factor)
	if group == "event":
		factor=event_scale()
		return Rect2(event_view_rect().get_center()+(rect.position-Vector2(320,240))*factor,rect.size*factor)
	if group == "scene":
		factor = minf(content_size().x/640.0,content_size().y/480.0)
		return Rect2((content_size()-Vector2(640,480)*factor)/2+rect.position*factor,rect.size*factor)
	var origin := Vector2.ZERO
	match group:
		"body": origin.y = 70
		"description": origin.y = 341
		"footer", "logo": origin.y = 379
	var result := Rect2(area.position+(rect.position-origin)*factor,rect.size*factor)
	if group == "body": result.position.y -= body_scroll.scroll_vertical
	if group == "logo": result.position.x += area.size.x-640*factor
	if span: result.size.x = maxf(0,area.end.x-result.position.x)
	return result

func _layout_all() -> void:
	if not is_inside_tree() or content == null: return
	layout_dirty = false
	layout_revision += 1
	var dimensions := content_size()
	_layout_factor = maxf(0.75,minf((dimensions.x-16)/640.0,(dimensions.y-16)/480.0))
	var has_description := false
	var has_footer := false
	var body_height := 0.0
	for control in nodes.values():
		var widget: Dictionary = control.get_meta("frontend_authored")
		var group := Profiles.group_for(widget, selection or frontend.startup_active)
		# Tree panels, connectors, labels and native camera transitions share one
		# space; moving its static body separately breaks their relationships.
		if snapshot.page=="View Event Tree" and (group=="body" or (group=="scene" and (str(widget.id).begins_with("event") or int(widget.stack)>0))): group="event"
		if snapshot.page=="View Event Tree" and int(widget.stack)>0 and not str(widget.id).begins_with("event"): group="event_detail"
		if snapshot.page=="View Event Tree" and widget.name=="EVENT_TITLE_BAR": group="description"
		control.set_meta("frontend_layout_group",group)
		has_description = has_description or group=="description"
		has_footer = has_footer or group in ["footer","logo"]
		if group=="body" and widget.interactive:
			body_height = maxf(body_height,minf(480,widget.bounds.end.y)-70)
	slots.header.custom_minimum_size.y = 70*_layout_factor
	slots.description.custom_minimum_size.y = 38*_layout_factor if has_description else 0
	slots.footer.custom_minimum_size.y = 101*_layout_factor if has_footer else 0
	body_document.custom_minimum_size.y = maxf(0,body_height)*_layout_factor
	for viewport in layer_viewports.values():
		if viewport.size != Vector2i(dimensions): viewport.size = Vector2i(dimensions)
	for control in nodes.values(): _layout_widget(control)

func _layout_widget(control: Control) -> void:
	var widget: Dictionary = control.get_meta("frontend_authored")
	var group: String = control.get_meta("frontend_layout_group")
	var is_text := control is Label or control is LineEdit or (control is Button and int(widget.type)==1 and not control.has_node("OriginalNumber"))
	var span: bool = not is_text and group in ["header","description","body"] and (widget.name.begins_with("MI_") or widget.name in ["SCROLL_TEXT_CLIP","WHITEOUT_BAR","BOTTOM_MASK","GRID_SHADOW"] or widget.bounds.size.x>=640)
	var visual_bounds: Rect2 = widget.text_bounds if str(widget.id).begins_with("event:") and control is Label else widget.bounds
	var rect := _map_rect(visual_bounds,group,span)
	var factor := _layout_factor
	if group=="background": factor=maxf(content_size().x/640.0,content_size().y/480.0)
	if group=="scene": factor=minf(content_size().x/640.0,content_size().y/480.0)
	if group=="event": factor=event_scale()
	if group=="event_detail": factor=_detail_scale()
	var offset: Vector2 = snapshot.get("display_offset",Vector2.ZERO)*factor
	rect.position += offset
	control.position = rect.position
	if control.has_meta("frontend_clip_root"):
		var clip: Control = control.get_meta("frontend_clip_root")
		var clip_rect := _map_rect(widget.clip,group,group=="description") if widget.clip.has_area() else _region(group)
		clip_rect.position += offset
		if group in ["event","event_detail"]: clip_rect=clip_rect.intersection(event_view_rect())
		if group=="body":
			clip_rect=clip_rect.intersection(_region(group))
			var authored_scanout := _map_rect(Rect2(0,0,640,480),"body")
			authored_scanout.position += offset
			# Retain original vertical hiding while allowing wide menu art to span
			# the Container's full width.
			authored_scanout.position.x = _region(group).position.x
			authored_scanout.size.x = _region(group).size.x
			clip_rect=clip_rect.intersection(authored_scanout)
		if group=="scene" and not str(widget.id).begins_with("event"):
			# Off-canvas authored thumbnail/editor templates are deliberately outside
			# the original scanout. Extra portrait space must not reveal them.
			var scanout := _map_rect(Rect2(0,0,640,480),"scene")
			scanout.position += offset
			clip_rect=clip_rect.intersection(scanout)
		clip.position = clip_rect.position
		clip.size = clip_rect.size.max(Vector2.ZERO)
		control.position -= clip.position
	if is_text:
		var native_scale: Vector2 = widget.scale
		control.scale = native_scale*factor
		control.size = Vector2(maxf(1,visual_bounds.size.x/maxf(absf(native_scale.x),0.001)),maxf(1,visual_bounds.size.y/maxf(absf(native_scale.y),0.001)))
		# Let Godot wrap help/details in the resized group; normal text preserves
		# native measured origins and original scrolling remains clipped.
		if widget.wrap:
			control.size.x = rect.size.x/maxf(absf(control.scale.x),0.001)
			if fonts.has(widget.texture): control.add_theme_constant_override("line_spacing",roundi(font_sizes[widget.texture]*(1.0/maxf(absf(native_scale.y),0.001)-1.0)))
	else:
		control.scale = Vector2.ONE
		control.size = rect.size.max(Vector2.ZERO)
	if control.has_node("OriginalNumber"):
		var number: Control = control.get_node("OriginalNumber")
		number.position = Vector2.ZERO
		number.size = rect.size
		for index in widget.glyphs.size():
			var glyph: Dictionary = widget.glyphs[index]
			var label: Label = number.get_child(index)
			var destination := _map_rect(glyph.bounds,group)
			label.position = destination.position+offset-rect.position
			label.size = glyph.source.size
			label.scale = glyph.bounds.size/glyph.source.size*factor

func _update_number(control: Control, widget: Dictionary) -> void:
	var number: Control=control.get_node("OriginalNumber")
	while number.get_child_count()>widget.glyphs.size():
		var last: Node=number.get_child(number.get_child_count()-1)
		number.remove_child(last)
		last.queue_free()
	while number.get_child_count()<widget.glyphs.size():
		var glyph := Label.new()
		glyph.set_meta("frontend_bitmap_font",true)
		glyph.mouse_filter=Control.MOUSE_FILTER_IGNORE
		glyph.text_direction=Control.TEXT_DIRECTION_LTR
		number.add_child(glyph)
	for index in widget.glyphs.size():
		var label: Label=number.get_child(index)
		label.text=String.chr(widget.glyphs[index].code)
		label.add_theme_font_override("font",fonts[widget.texture])
		label.add_theme_font_size_override("font_size",font_sizes[widget.texture])
		label.add_theme_color_override("font_color",widget.color)

func _action(id: String, kind: String, direction: int = 0, source_generation: int = -1, port: int = 0) -> void:
	if bindings_panel.visible or (source_generation >= 0 and source_generation != generation): return
	if kind == "focus" and (Engine.get_process_frames() <= layout_focus_after_frame or Engine.get_process_frames()-pointer_motion_frame > 1): return
	action_requested.emit({"generation":generation,"id":id,"kind":kind,"direction":direction,"port":port})

func show_bindings() -> void:
	release_devices()
	bindings_panel.show()
	bindings_panel.move_to_front()

func release_devices() -> void:
	for command in bindings.release_all(): frontend._send_joy_event(command.id,command.port,command.pressed)

func handle_modal(event: InputEvent) -> bool:
	return bindings_panel.handle(event) if bindings_panel.visible else false

func forward_mouse(event: InputEvent) -> bool:
	if not selection or bindings_panel.visible or not (event is InputEventMouseButton or event is InputEventMouseMotion): return false
	if mouse_viewport != null and is_instance_valid(mouse_viewport):
		mouse_viewport.push_input(_content_event(event),true)
		if event is InputEventMouseButton and not event.pressed: mouse_viewport = null
		return true
	if not Rect2(content.global_position,content_size()).has_point(event.position): return false
	var order: Array = layer_viewports.keys()
	order.sort_custom(func(a: int,b: int): return _layer_order(a)>_layer_order(b))
	for key in order:
		var viewport: SubViewport = layer_viewports[key]
		viewport.push_input(_content_event(event),true)
		if viewport.is_input_handled():
			if event is InputEventMouseButton and event.pressed: mouse_viewport = viewport
			return true
	return false

func _content_event(event: InputEvent) -> InputEvent:
	var local := event.duplicate()
	local.position = event.position-content.global_position
	local.global_position = local.position
	return local

func composition_layers() -> Array[Dictionary]:
	var out: Array[Dictionary] = []
	var order: Array = layer_viewports.keys()
	order.sort_custom(func(a: int,b: int): return _layer_order(a)<_layer_order(b))
	for layer in order: out.append({"layer":layer%32,"texture":layer_viewports[layer].get_texture()})
	return out

func text_focus() -> bool:
	var focus := get_viewport().gui_get_focus_owner()
	return focus is LineEdit and focus.has_meta("frontend_widget_id")
