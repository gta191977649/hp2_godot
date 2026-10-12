extends Control

# TASK-0013: renders the ORIGINAL HP2 front-end screens.
#
# Everything drawn here comes from the game's own authored data, read from the configured ZZDATA
# copy at runtime: screen/widget layout records (hp2::frontend::parse_screens), TexturePack
# textures decoded on demand, and the original bitmap fonts (hp2::frontend::parse_fonts) drawn
# glyph-by-glyph from their atlases. No hand-authored mock-up, no pre-extracted files.
#
# Known gaps, deliberately not faked:
#  * Kerning pair tables are parsed natively but not yet applied, so inter-letter spacing can be
#    a pixel or two wide versus the PS2.
#  * Widget animation/state (highlighting, scrolling, transitions) is not driven yet; screens are
#    drawn in their authored rest state with the authored placeholder strings.

const CANVAS := Vector2(640.0, 480.0)
static func _font_bundles() -> PackedStringArray:
	return PackedStringArray(["FRONTEND/FRONTB.BUN", "GLOBAL/GLOBALB.BUN", "GLOBAL/INGAMEB.BUN"])

var layout: Object
var loader: Object
var screen_index := 0
var screen_count := 0
var view_scale := 2
var info_label: Label
var show_info := true
var draw_list: Array = []
var font_cache: Dictionary = {}

func _ready() -> void:
	if not ClassDB.class_exists("Ps2FrontEndLayout") or not ClassDB.class_exists("Ps2TextureLoader"):
		push_error("GDExtension classes missing (Ps2FrontEndLayout / Ps2TextureLoader)")
		get_tree().quit(1)
		return
	layout = ClassDB.instantiate("Ps2FrontEndLayout")
	loader = ClassDB.instantiate("Ps2TextureLoader")

	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--screen="):
			screen_index = int(argument.trim_prefix("--screen="))
		elif argument.begins_with("--scale="):
			view_scale = int(argument.trim_prefix("--scale="))
		elif argument == "--no-info":
			show_info = false

	if DisplayServer.get_name() != "headless":
		get_window().size = Vector2i(int(CANVAS.x) * view_scale, int(CANVAS.y) * view_scale)

	var result: Dictionary = layout.load("FRONTEND/FRONTB.BUN")
	if not result.get("ok", false):
		_fatal(str(result.get("error", "layout load failed")))
		return
	screen_count = int(result.get("screen_count", 0))
	layout.load_fonts(_font_bundles())

	info_label = Label.new()
	info_label.position = Vector2(8, 4)
	info_label.add_theme_color_override("font_color", Color(1, 1, 0.55))
	info_label.add_theme_font_size_override("font_size", 13)
	info_label.visible = show_info
	add_child(info_label)

	_build_screen()

	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--capture="):
			await RenderingServer.frame_post_draw
			await RenderingServer.frame_post_draw
			var path: String = argument.trim_prefix("--capture=")
			if get_viewport().get_texture().get_image().save_png(path) != OK:
				_fatal("capture failed")
				return
			print("FRONTEND_CAPTURE_OK: ", path)
			get_tree().quit(0)
			return
		elif argument == "--frontend-smoke":
			_smoke()
			return

func _fatal(message: String) -> void:
	push_error(message)
	print("FRONTEND_ERROR: ", message)
	get_tree().quit(1)

func _font_for(hash_value: int) -> Dictionary:
	if font_cache.has(hash_value):
		return font_cache[hash_value]
	var font: Dictionary = layout.get_font(hash_value)
	if not font.is_empty():
		font["texture"] = loader.load_texture_by_hash(hash_value)
	font_cache[hash_value] = font
	return font

func _build_screen() -> void:
	draw_list.clear()
	var widgets: Array = layout.get_widgets(screen_index)
	var info: Dictionary = layout.get_screen_info(screen_index)

	# The authored layer byte counts down toward the viewer (grid/blueprint backdrops carry the
	# larger values, menu text carries 0), so paint descending, preserving authored order within
	# a layer.
	var ordered: Array = []
	for i in widgets.size():
		ordered.append({"w": widgets[i], "i": i})
	ordered.sort_custom(func(a, b):
		var la: int = int(a["w"]["layer"])
		var lb: int = int(b["w"]["layer"])
		if la != lb:
			return la > lb
		return int(a["i"]) < int(b["i"]))

	var quads := 0
	var glyph_texts := 0
	var missing := 0
	for entry in ordered:
		var w: Dictionary = entry["w"]
		var color := Color(w["color_r"], w["color_g"], w["color_b"], w["color_a"])
		var hash_value: int = int(w["texture_hash"])
		if bool(w["is_text"]):
			var text: String = str(w["text"])
			if text.is_empty():
				continue
			var font := _font_for(hash_value)
			if font.is_empty() or font.get("texture") == null:
				missing += 1
				continue
			draw_list.append({"kind": "text", "w": w, "color": color, "font": font})
			glyph_texts += 1
		else:
			if hash_value == 0:
				continue
			var texture: Texture2D = loader.load_texture_by_hash(hash_value)
			if texture == null:
				missing += 1
				continue
			draw_list.append({"kind": "quad", "w": w, "color": color, "texture": texture})
			quads += 1

	var names: PackedStringArray = info.get("names", PackedStringArray())
	var label := ""
	for n in names:
		if n != "NONAME":
			label = n
			break
	info_label.text = "screen %d/%d  widgets=%d  quads=%d text=%d missing=%d  [%s]   arrows change, Tab hides" % [screen_index, screen_count - 1, widgets.size(), quads, glyph_texts, missing, label]
	queue_redraw()

func _text_width(text: String, glyphs: Dictionary, scale_x: float) -> float:
	var total := 0.0
	for i in text.length():
		var metrics = glyphs.get(text.unicode_at(i))
		if metrics != null:
			total += float(metrics[4])
	return total * scale_x

func _draw() -> void:
	draw_set_transform(Vector2.ZERO, 0.0, Vector2(view_scale, view_scale))
	for item in draw_list:
		var w: Dictionary = item["w"]
		var color: Color = item["color"]
		var pos := Vector2(w["x"], w["y"])
		if item["kind"] == "quad":
			var texture: Texture2D = item["texture"]
			var size := Vector2(texture.get_width() * float(w["scale_x"]), texture.get_height() * float(w["scale_y"]))
			draw_texture_rect(texture, Rect2(pos, size), false, color)
		else:
			var font: Dictionary = item["font"]
			var glyphs: Dictionary = font["glyphs"]
			var atlas: Texture2D = font["texture"]
			var text: String = str(w["text"])
			var sx := float(w["scale_x"])
			var sy := float(w["scale_y"])
			var pen := pos
			# align 2 = left, 4 = centred on x (verified against PRESS_START_1 at x=320 and the
			# right-hand stat column at x=494).
			if int(w["text_align"]) == 4:
				pen.x -= _text_width(text, glyphs, sx) * 0.5
			for i in text.length():
				var metrics = glyphs.get(text.unicode_at(i))
				if metrics == null:
					continue
				var region := Rect2(float(metrics[0]), float(metrics[1]), float(metrics[2]), float(metrics[3]))
				var dest := Rect2(pen, Vector2(region.size.x * sx, region.size.y * sy))
				draw_texture_rect_region(atlas, dest, region, color)
				pen.x += float(metrics[4]) * sx

func _input(event: InputEvent) -> void:
	if not (event is InputEventKey) or not event.pressed:
		return
	match event.keycode:
		KEY_RIGHT:
			screen_index = (screen_index + 1) % screen_count
			_build_screen()
		KEY_LEFT:
			screen_index = (screen_index - 1 + screen_count) % screen_count
			_build_screen()
		KEY_TAB:
			show_info = not show_info
			info_label.visible = show_info
		KEY_ESCAPE:
			get_tree().quit(0)

func _smoke() -> void:
	var resolved := 0
	var missing := 0
	for i in screen_count:
		for w in layout.get_widgets(i):
			var h: int = int(w["texture_hash"])
			if h == 0:
				continue
			if bool(w["is_text"]):
				if not _font_for(h).is_empty():
					resolved += 1
				else:
					missing += 1
			elif loader.load_texture_by_hash(h) != null:
				resolved += 1
			else:
				missing += 1
	if screen_count < 40 or resolved < 500:
		_fatal("unexpected counts: screens=%d resolved=%d" % [screen_count, resolved])
		return
	print("FRONTEND_SMOKE_OK: screens=%d resolved=%d missing=%d" % [screen_count, resolved, missing])
	get_tree().quit(0)
