extends "res://scripts/runtime/frontend_controls_acceptance.gd"
# TASK-0104 port layout checks. Uses production inputs and actual window rendering.
const SIZES := [Vector2i(640,480),Vector2i(1280,720),Vector2i(1920,1080),Vector2i(2560,1080),Vector2i(3440,1440),Vector2i(1024,768),Vector2i(800,1000)]

func _loaded() -> bool:
	var deadline := Time.get_ticks_msec()+20000
	while Time.get_ticks_msec()<deadline:
		if frontend.runtime_error_count>0: return false
		var state: Dictionary=frontend.layout.menu_state().selection
		if state.active and state.loaded and state.overlay==0 and frontend.last_stats.get("shared_frontend_target",false): return true
		await process_frame
	return false

func _resize(dimensions: Vector2i) -> void:
	root.size=dimensions
	for frame in 5: await process_frame
	await RenderingServer.frame_post_draw

func _regions(label: String) -> void:
	var ui: Control=frontend.controls
	_check(ui.shell is VBoxContainer and ui.body_scroll is ScrollContainer,label+" uses Godot layout containers")
	_check(is_equal_approx(ui.content.size.y+ui.footer.size.y,root.size.y),label+" toolbar and content partition the window")
	if ui.snapshot.page=="View Event Tree":
		var event_area: Rect2=ui.event_view_rect()
		_check(event_area.size.x>=ui.content_size().x-20,label+" event viewport uses full safe width")
		if event_area.size.x/event_area.size.y>2.3:
			_check(is_equal_approx(ui.event_scale(),event_area.size.y/ui.snapshot.event_clip.size.y),label+" wide event scale follows authored viewport height, not480")
	var items: Array[Control]=[]
	for control in ui.nodes.values():
		var widget: Dictionary=control.get_meta("frontend_authored")
		if widget.name.begins_with("MENUITEM_") and widget.interactive: items.append(control)
		if widget.name=="SCROLLING_TEXT":
			var clip: Control=control.get_meta("frontend_clip_root")
			_check(Rect2(Vector2.ZERO,ui.content_size()).grow(1).encloses(clip.get_global_rect()),label+" description clip remains in content")
		if control.has_node("OriginalNumber"):
			var number: Control=control.get_node("OriginalNumber")
			_check(number.get_child_count()==widget.glyphs.size(),label+" native glyph commands represented by Labels")
			for index in widget.glyphs.size():
				var glyph: Label=number.get_child(index)
				var expected: Rect2=ui._map_rect(widget.glyphs[index].bounds,"event")
				_check(glyph.global_position.distance_to(expected.position+ui.content.global_position+ui.snapshot.display_offset*ui.event_scale())<0.1,label+" glyph origin matches native draw command")
			var clip: Control=control.get_meta("frontend_clip_root")
			_check(clip.get_global_rect().size.is_equal_approx(ui.event_view_rect().size),label+" event uses full Container view clip")
		if widget.name=="TRACK_LINE_1" and widget.wrap:
			_check(widget.bounds.position.y>widget.clip.position.y+30,label+" paragraph preserves text origin below track labels")
	items.sort_custom(func(a: Control,b: Control): return a.get_meta("frontend_authored").name<b.get_meta("frontend_authored").name)
	if ui.snapshot.page=="New Main Menu" and items.size()==3:
		var gap1: float=items[1].get_global_rect().position.y-items[0].get_global_rect().position.y
		var gap2: float=items[2].get_global_rect().position.y-items[1].get_global_rect().position.y
		_check(absf(gap1-gap2)<1,label+" menu rows retain equal spacing")
		_check(gap1<items[0].get_global_rect().size.y*1.5,label+" portrait resize does not split menu rows")
	for viewport in ui.layer_viewports.values():
		_check(viewport.size==Vector2i(ui.content_size()),label+" Control viewport equals compositor target")

func _ink(image: Image) -> Rect2:
	var bounds := Rect2()
	var found := false
	for y in image.get_height():
		for x in image.get_width():
			if image.get_pixel(x,y).a<=0.15: continue
			if not found: bounds=Rect2(x,y,1,1); found=true
			else: bounds=bounds.merge(Rect2(x,y,1,1))
	return bounds

func _glyph_rasters() -> void:
	# Freeze the owner while reading rendered glyphs so native fade/zoom cannot
	# destroy sampled controls between GPU frames. This is a port test fixture.
	frontend.set_process(false)
	var covered: Dictionary={}
	var cases: Array=frontend.controls.nodes.values().duplicate()
	var fixture: Control
	for source in cases:
		var widget: Dictionary=source.get_meta("frontend_authored")
		if not str(widget.id).begins_with("event:"): continue
		var probe: Dictionary=widget.duplicate(true)
		probe.id="event:reference32"
		probe.text="32"
		probe.interactive=false
		probe.glyphs=frontend.layout.ui_number_reference(32)
		fixture=frontend.controls._create(probe)
		fixture.set_meta("frontend_layout_group","event")
		frontend.controls.content.add_child(fixture)
		frontend.controls._update(fixture,probe)
		frontend.controls._layout_widget(fixture)
		cases.append(fixture)
		break
	for control in cases:
		var widget: Dictionary=control.get_meta("frontend_authored")
		if not str(widget.id).begins_with("event:") or not str(widget.text).strip_edges() in ["1","9","10","32"]: continue
		covered[str(widget.text).strip_edges()]=true
		var atlas: Image=frontend.layout.ui_texture(widget.texture).get_image()
		for index in widget.glyphs.size():
			var command: Dictionary=widget.glyphs[index]
			if command.code==32: continue
			var viewport := SubViewport.new()
			viewport.size=Vector2i(512,256)
			viewport.disable_3d=true
			viewport.transparent_bg=true
			viewport.render_target_update_mode=SubViewport.UPDATE_ALWAYS
			root.add_child(viewport)
			var glyph: Label=control.get_node("OriginalNumber").get_child(index).duplicate()
			glyph.position=Vector2(16,16)
			glyph.add_theme_color_override("font_color",Color.WHITE)
			viewport.size=Vector2i(maxi(512,ceili(command.source.size.x*glyph.scale.x+32)),maxi(256,ceili(command.source.size.y*glyph.scale.y+32)))
			viewport.add_child(glyph)
			for frame in 3: await process_frame
			await RenderingServer.frame_post_draw
			var actual := _ink(viewport.get_texture().get_image())
			var source: Rect2=command.source
			var reference := _ink(atlas.get_region(Rect2i(source.position-Vector2(0.5,0.5),source.size)))
			var expected := Rect2(Vector2(16,16)+reference.position*glyph.scale,reference.size*glyph.scale)
			_check(actual.has_area() and actual.position.distance_to(expected.position)<=3 and actual.end.distance_to(expected.end)<=3,"rendered digit "+str(widget.text)+" ink matches original atlas/native command extent")
			viewport.queue_free()
	_check(covered.size()==4,"native number raster cases 1 9 10 and explicit 32 fixture covered")
	if fixture!=null: fixture.queue_free()
	frontend.set_process(true)

func _run() -> void:
	phase="layout"
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--controls-output="): output=argument.trim_prefix("--controls-output=")
	if output.is_empty() or DisplayServer.get_name()=="headless": quit(2); return
	frontend=load("res://scenes/frontend_native.tscn").instantiate()
	root.add_child(frontend)
	root.get_node("DebugTools").set_info(false)
	_check(await _ready_class("New Main Menu"),"layout production main menu ready")
	for frame in 5: await process_frame
	for dimensions in SIZES:
		var generation: int=frontend.controls.generation
		await _resize(dimensions)
		_regions("main "+str(dimensions))
		_check(frontend.controls.generation==generation,"main resize retains generation")
		await _capture("layout-main-%dx%d"%[dimensions.x,dimensions.y])
	var settled_revision: int=frontend.controls.layout_revision
	for frame in 6: await process_frame
	_check(frontend.controls.layout_revision==settled_revision,"native animation updates do not reflow the entire layout")
	# The global DPI service scales PC controls once, while bitmap font metrics
	# remain in the native authored space. Exercise both directions and restore.
	var dpi: Node=root.get_node("UiDisplay")
	var baseline: float=dpi.scale_factor
	var original: Control=_menu("MENUITEM_1")
	var font_size: int=original.get_theme_font_size("font_size")
	for factor in [1.0,1.5,2.0]:
		dpi._apply_scale(factor)
		_check(original.get_theme_font_size("font_size")==font_size,"DPI does not rescale original glyphs "+str(factor))
		var pc_button: Button=frontend.controls.tools.get_node("KeyBinding")
		_check(pc_button.get_theme_font_size("font_size")==roundi(14*factor),"DPI scales toolbar once "+str(factor))
	dpi._apply_scale(baseline)
	frontend.controls.show_bindings()
	for dimensions in [Vector2i(800,1000),Vector2i(1280,720)]:
		await _resize(dimensions)
		_check(frontend.controls.bindings_panel.visible,"resize preserves PC modal")
		_check(frontend.controls.bindings_panel.size==Vector2(dimensions),"modal fills resized window")
		for buttons in frontend.controls.bindings_panel.rows.values():
			for button in buttons:
				_check(button.get_global_rect().position.x>=0 and button.get_global_rect().end.x<=dimensions.x,"modal binding columns fit without horizontal clipping")
		await _capture("layout-bindings-%dx%d"%[dimensions.x,dimensions.y])
	frontend.controls.bindings_panel.hide()
	await _click(_menu("MENUITEM_3"))
	_check(await _ready_class("Options"),"layout Options entry")
	await _click(_menu("MENUITEM_1"))
	_check(await _ready_class("Audio Options"),"layout Audio entry")
	var slider: HSlider
	for control in frontend.controls.nodes.values():
		if control is HSlider: slider=control; break
	if slider!=null:
		var slider_generation: int=frontend.controls.generation
		var point: Vector2=slider.get_global_rect().position+slider.get_global_rect().size*Vector2(0.2,0.5)
		await _mouse(point,true)
		await _resize(Vector2i(800,1000))
		var motion := InputEventMouseMotion.new()
		motion.position=slider.get_global_rect().position+slider.get_global_rect().size*Vector2(0.8,0.5)
		motion.global_position=motion.position
		motion.button_mask=MOUSE_BUTTON_MASK_LEFT
		motion.relative=Vector2(20,0)
		Input.parse_input_event(motion)
		await process_frame
		await _mouse(motion.position,false)
		_check(frontend.controls.generation==slider_generation and slider.value>0.6,"slider drag survives resize without reopening native page")
		for control in frontend.controls.nodes.values():
			var widget: Dictionary=control.get_meta("frontend_authored")
			if control.get_meta("frontend_layout_group")=="body" and widget.bounds.position.y>=480 and control.has_meta("frontend_clip_root"):
				_check(not control.get_global_rect().intersection(control.get_meta("frontend_clip_root").get_global_rect()).has_area(),"portrait audio does not reveal off-scanout templates")
		await _capture("layout-audio-drag-portrait")
	await _press(KEY_ESCAPE)
	_check(await _ready_class("Options"),"layout Audio return")
	await _press(KEY_ESCAPE)
	_check(await _ready_class("New Main Menu"),"layout Options return")
	await _resize(Vector2i(1280,720))
	# Actual mouse entry to mode selection, then original keyboard route to event1.
	await _click(_menu("MENUITEM_1"))
	if not await _ready_class("Cops On Mode Select"): _check(false,"mouse mode entry after resize"); await _finish(); return
	for step in 3: await _press(KEY_DOWN)
	await _press(KEY_ENTER)
	if not await _ready_class("View Event Tree"): _check(false,"production event tree entry"); await _finish(); return
	await create_timer(1.0).timeout
	await _glyph_rasters()
	for dimensions in SIZES:
		await _resize(dimensions)
		_regions("event "+str(dimensions))
		for control in frontend.controls.nodes.values():
			if control.has_node("OriginalNumber"):
				_check(control.get_node("OriginalNumber").mouse_filter==Control.MOUSE_FILTER_IGNORE,"number visual does not intercept native event button")
		await _capture("layout-event-%dx%d"%[dimensions.x,dimensions.y])
	# The Help hint is present, but this native owner has no MENU_HELP branch.
	# Do not pretend F1 opened a paragraph or Back would close a nonexistent popup.
	checks.append("event Help hint inspected; native Help entry remains unbound")
	await _resize(Vector2i(3440,1440))
	var first_event: Control
	for control in frontend.controls.nodes.values():
		var widget: Dictionary=control.get_meta("frontend_authored")
		if str(widget.id).begins_with("event:") and str(widget.text).strip_edges()=="1": first_event=control; break
	_check(first_event is Button,"wide event1 has a real clickable native target")
	if first_event==null: await _finish(); return
	await _click(first_event)
	_regions("event transition")
	await _glyph_rasters()
	await create_timer(1.2).timeout
	_regions("event details")
	await _capture("layout-event-details")
	await _press(KEY_ENTER)
	if not await _ready_class("Select Car") or not await _loaded(): _check(false,"production selector loaded"); await _finish(); return
	var original_selection: Dictionary=frontend.layout.menu_state().selection.duplicate()
	var generation: int=frontend.controls.generation
	for dimensions in SIZES:
		await _resize(dimensions)
		_check(await _loaded(),"shared selection target ready after resize "+str(dimensions))
		_regions("selection "+str(dimensions))
		var state: Dictionary=frontend.layout.menu_state().selection
		_check(frontend.controls.generation==generation and state.car==original_selection.car and state.skin==original_selection.skin,"selection survives resize "+str(dimensions))
		await _capture("layout-selection-%dx%d"%[dimensions.x,dimensions.y])
	for step in 12:
		root.size=Vector2i(900+step*20,700+step*7)
		await process_frame
	await _resize(Vector2i(1280,720))
	_check(await _loaded() and frontend.controls.generation==generation,"continuous window resize preserves selection and rebuilds matching targets")
	# Route a real GUI click through the transparent layer viewports after resize.
	for control in frontend.controls.nodes.values():
		var id: String=control.get_meta("frontend_widget_id")
		if id.begins_with("selection:skin:") and id!="selection:skin:"+str(original_selection.skin):
			await _click(control)
			_check(frontend.layout.menu_state().selection.skin!=original_selection.skin,"resized SubViewport mouse hit chooses native skin")
			break
	await _press(KEY_ESCAPE)
	_check(await _ready_class("View Event Tree"),"resized selection returns to event details")
	await create_timer(1.2).timeout
	await _press(KEY_ENTER)
	_check(await _ready_class("Select Car") and await _loaded(),"resized event reenters production selection")
	await _finish()
