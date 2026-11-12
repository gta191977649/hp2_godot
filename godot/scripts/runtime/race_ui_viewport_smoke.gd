extends "res://scripts/runtime/race_scene.gd"
# TASK-0112 GPU COMPONENT: authored offscreen columns, actual RaceScene UI layout.
# No World is constructed and no outcome/position/clock/lap data is written.
func _ready() -> void:
	_run.call_deferred()
func _process(_delta: float) -> void:
	pass
func _input(_event: InputEvent) -> void:
	pass
func _unhandled_input(_event: InputEvent) -> void:
	pass
func _run() -> void:
	source_ui_canvas = Control.new()
	add_child(source_ui_canvas)
	source_ui_canvas.clip_contents = true
	var viewport_size := get_viewport().get_visible_rect().size
	_layout_source_ui_canvas(viewport_size, Vector4(0, 0, 644, 480))
	source_ui_resources = ClassDB.instantiate("Ps2FrontEndLayout")
	var loaded: Dictionary = source_ui_resources.load("GLOBAL/INGAMEB.BUN")
	if not loaded.get("ok", false):
		printerr("UI_VIEWPORT_COMPONENT_FAILED: ", loaded)
		get_tree().quit(1)
		return
	source_ui_resources.load_fonts(PackedStringArray(["GLOBAL/GLOBALB.BUN", "GLOBAL/INGAMEB.BUN", "FRONTEND/FRONTB.BUN"]))
	source_ui_host = ClassDB.instantiate("Ps2RenderHost")
	source_ui_host.attach(source_ui_canvas.get_canvas_item())
	var drawn: Dictionary = source_ui_host.debug_draw_authored_screen(source_ui_resources, 0xab049b2b, viewport_size.x, viewport_size.y)
	if not drawn.get("ok", false):
		printerr("UI_VIEWPORT_COMPONENT_FAILED: ", drawn)
		get_tree().quit(1)
		return
	await get_tree().process_frame
	RenderingServer.force_draw(false)
	var clipped := get_viewport().get_texture().get_image()
	var output := OS.get_environment("HP2_UI_VIEWPORT_COMPONENT")
	clipped.save_png(output + "-clipped.png")
	var clip_end := int(ceilf(source_ui_canvas.position.x + source_ui_canvas.size.x))
	var clipped_pixels := _gutter_pixels(clipped, clip_end)
	# Negative control changes only the real Control clipping property.
	source_ui_canvas.clip_contents = false
	await get_tree().process_frame
	RenderingServer.force_draw(false)
	var leaking := get_viewport().get_texture().get_image()
	leaking.save_png(output + "-negative.png")
	var leaked_pixels := _gutter_pixels(leaking, clip_end)
	if clipped_pixels != 0 or leaked_pixels < 20:
		printerr("UI_VIEWPORT_COMPONENT_FAILED: positive=", clipped_pixels, " negative=", leaked_pixels)
		get_tree().quit(1)
		return
	print("UI_VIEWPORT_COMPONENT_OK: actual authored x791 columns, source644 guard band; positive=0 gutter pixels, negative=", leaked_pixels, "; no live race fixture")
	get_tree().quit(0)
func _gutter_pixels(image: Image, begin: int) -> int:
	var count := 0
	# Ignore only the global developer FPS overlay's bottom strip.
	for y in range(0, image.get_height() - 24):
		for x in range(begin, image.get_width()):
			var pixel := image.get_pixel(x, y)
			if maxf(pixel.r, maxf(pixel.g, pixel.b)) > 0.01: count += 1
	return count
