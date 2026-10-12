extends Control

# TASK-0013 / TASK-0014 / TASK-0015: renders and now DRIVES the original HP2 front-end screens.
#
# Everything drawn comes from the game's own authored data, read from the configured ZZDATA copy
# at runtime: screen/widget layout records (hp2::frontend::parse_screens), TexturePack textures
# decoded on demand, and the original bitmap fonts drawn glyph-by-glyph from their atlases.
#
# Interaction is the recovered menu runtime, not a Godot UI: keys are translated into the original
# JOY_EVENT_* ids and handed to hp2::frontend::MenuGroup_0016AA00, which builds the item list the
# way FUN_0016A858 does (the screen's own "MENUITEM_%d" widgets), moves the selection with
# FUN_0016A5E0 / FUN_0016A6C8 and recolours the highlighted item through FUN_00169BD0. The orange
# highlight and grey disabled colours are the values stored in the ELF, not a theme.
#
# TASK-0016 reads FE localisation, MV30 movies and original PS2 ADPCM at runtime.
# Remaining gaps: saved preferences/full boot flow, additional screen handlers,
# font kerning, clipping/gradient details, and original-runtime comparison.

const CANVAS := Vector2(640.0, 480.0)

# Recovered joypad event ids (ELF table 0x002FA2D0 -> src/game_core/input/joyevents.hpp).
const JOY_EVENT_SCROLL_UP := 3
const JOY_EVENT_SCROLL_DOWN := 4
const JOY_EVENT_SCROLL_LEFT := 5
const JOY_EVENT_SCROLL_RIGHT := 6
const JOY_EVENT_START := 7
const JOY_EVENT_SELECT := 8
const JOY_EVENT_MENU_SELECT := 9
const JOY_EVENT_MENU_CANCEL := 10
const JOY_EVENT_MENU_BACK := 11
const JOY_EVENT_MENU_HELP := 12
const JOY_EVENT_MENU_L1 := 13
const JOY_EVENT_MENU_L2 := 14
const JOY_EVENT_MENU_R1 := 15
const JOY_EVENT_MENU_R2 := 16
const JOY_EVENT_MENU_SQUARE := 17
const JOY_EVENT_MENU_ANALOG_LEFT := 18
const JOY_EVENT_MENU_ANALOG_RIGHT := 19
const JOY_EVENT_MENU_ANALOG_UP := 20
const JOY_EVENT_MENU_ANALOG_DOWN := 21

# Keyboard mapping for PC debugging. The right column is the PS2 control the event name implies,
# which is why the D-pad/analog split is kept: the original treats SCROLL_* and MENU_ANALOG_* as
# separate ids that happen to be handled together, and both paths stay reachable from the keyboard.
static func _key_map() -> Dictionary:
	return {
		# D-pad
		KEY_UP: JOY_EVENT_SCROLL_UP,
		KEY_DOWN: JOY_EVENT_SCROLL_DOWN,
		KEY_LEFT: JOY_EVENT_SCROLL_LEFT,
		KEY_RIGHT: JOY_EVENT_SCROLL_RIGHT,
		# left analog stick
		KEY_W: JOY_EVENT_MENU_ANALOG_UP,
		KEY_S: JOY_EVENT_MENU_ANALOG_DOWN,
		KEY_A: JOY_EVENT_MENU_ANALOG_LEFT,
		KEY_D: JOY_EVENT_MENU_ANALOG_RIGHT,
		# face buttons
		KEY_ENTER: JOY_EVENT_START,
		KEY_KP_ENTER: JOY_EVENT_START,
		KEY_SPACE: JOY_EVENT_MENU_SELECT,
		KEY_Z: JOY_EVENT_MENU_SELECT,
		KEY_X: JOY_EVENT_MENU_CANCEL,
		KEY_BACKSPACE: JOY_EVENT_MENU_BACK,
		KEY_C: JOY_EVENT_MENU_BACK,
		KEY_V: JOY_EVENT_MENU_SQUARE,
		KEY_H: JOY_EVENT_MENU_HELP,
		KEY_F1: JOY_EVENT_MENU_HELP,
		KEY_BACKSLASH: JOY_EVENT_SELECT,
		# shoulders
		KEY_Q: JOY_EVENT_MENU_L1,
		KEY_E: JOY_EVENT_MENU_R1,
		KEY_1: JOY_EVENT_MENU_L2,
		KEY_3: JOY_EVENT_MENU_R2,
	}

static func _font_bundles() -> PackedStringArray:
	return PackedStringArray(["FRONTEND/FRONTB.BUN", "GLOBAL/GLOBALB.BUN", "GLOBAL/INGAMEB.BUN"])

var layout: Object
var loader: Object
var intro_movie: Object
var background_movie: Object
var movie_audio: AudioStreamPlayer
var music_audio: AudioStreamPlayer
var startup_active := false
var startup_movie_visible := false
var intro_letterbox := false
var screen_index := 0
var screen_count := 0
var view_scale := 2
var info_label: Label
var menu_label: Label
var notice_label: Label
var show_info := false
var draw_list: Array = []
var font_cache: Dictionary = {}
var menu_state: Dictionary = {}
var last_event_text := ""
var pad_index := 0

func _ready() -> void:
	if not ClassDB.class_exists("Ps2FrontEndLayout") or not ClassDB.class_exists("Ps2TextureLoader"):
		push_error("GDExtension classes missing (Ps2FrontEndLayout / Ps2TextureLoader)")
		get_tree().quit(1)
		return
	layout = ClassDB.instantiate("Ps2FrontEndLayout")
	loader = ClassDB.instantiate("Ps2TextureLoader")
	intro_movie = ClassDB.instantiate("Ps2MoviePlayer")
	background_movie = ClassDB.instantiate("Ps2MoviePlayer")
	movie_audio = AudioStreamPlayer.new()
	add_child(movie_audio)
	music_audio = AudioStreamPlayer.new()
	add_child(music_audio)
	music_audio.finished.connect(_next_music)

	var start_name := ""
	var manual_screen := false
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--screen="):
			screen_index = int(argument.trim_prefix("--screen="))
			manual_screen = true
		elif argument.begins_with("--screen-name="):
			start_name = argument.trim_prefix("--screen-name=")
			manual_screen = true
		elif argument.begins_with("--scale="):
			view_scale = int(argument.trim_prefix("--scale="))
		elif argument == "--no-info":
			show_info = false
		elif argument == "--info":
			show_info = true
		elif argument.ends_with("-smoke"):
			manual_screen = true

	if DisplayServer.get_name() != "headless":
		get_window().size = Vector2i(int(CANVAS.x) * view_scale, int(CANVAS.y) * view_scale)

	var result: Dictionary = layout.load("FRONTEND/FRONTB.BUN")
	if not result.get("ok", false):
		_fatal(str(result.get("error", "layout load failed")))
		return
	screen_count = int(result.get("screen_count", 0))
	layout.load_fonts(_font_bundles())

	if start_name != "":
		var resolved: int = layout.find_screen_by_name(start_name)
		if resolved < 0:
			_fatal("unknown screen name: %s" % start_name)
			return
		screen_index = resolved

	info_label = _make_label(Vector2(8, 4), Color(1, 1, 0.55))
	menu_label = _make_label(Vector2(8, 22), Color(0.6, 1, 0.75))
	notice_label = _make_label(Vector2(8, 40), Color(1, 0.55, 0.45))
	notice_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	notice_label.size = Vector2(int(CANVAS.x) * view_scale - 16, 60)

	if not manual_screen:
		startup_active = true
		screen_index = 22
	_build_screen()
	if startup_active:
		_apply_startup(intro_movie.begin_startup(false))

	# Replay a comma-separated list of recovered JOY_EVENT_* ids before capturing, so a screenshot
	# can show the selection somewhere other than its initial position.
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--menu-events="):
			for id_text in argument.trim_prefix("--menu-events=").split(",", false):
				_send_joy_event(int(id_text))

	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--capture="):
			await RenderingServer.frame_post_draw
			await RenderingServer.frame_post_draw
			for capture_arg in OS.get_cmdline_user_args():
				if capture_arg.begins_with("--capture-after="):
					await get_tree().create_timer(float(capture_arg.trim_prefix("--capture-after="))).timeout
					await RenderingServer.frame_post_draw
			var path: String = argument.trim_prefix("--capture=")
			if get_viewport().get_texture().get_image().save_png(path) != OK:
				_fatal("capture failed")
				return
			print("FRONTEND_CAPTURE_OK: ", path)
			set_process(false)
			_exit_tree()
			await get_tree().process_frame
			await get_tree().process_frame
			get_tree().quit(0)
			return
		elif argument == "--frontend-smoke":
			_smoke()
			return
		elif argument == "--runtime-smoke":
			_runtime_smoke()
			return
		elif argument == "--activation-smoke":
			_activation_smoke()
			return
		elif argument == "--submenu-smoke":
			_submenu_smoke()
			return
		elif argument == "--animation-smoke":
			_animation_smoke()
			return
		elif argument == "--menu-smoke":
			_menu_smoke()
			return

func _make_label(position_px: Vector2, colour: Color) -> Label:
	var label := Label.new()
	label.position = position_px
	label.add_theme_color_override("font_color", colour)
	label.add_theme_font_size_override("font_size", 13)
	label.visible = show_info
	add_child(label)
	return label

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

func _build_screen(open_menu: bool = true) -> void:
	_configure_background()
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

	var dimensions := PackedVector2Array()
	dimensions.resize(widgets.size())
	var quads := 0
	var glyph_texts := 0
	var missing := 0
	for entry in ordered:
		var w: Dictionary = entry["w"]
		var hash_value: int = int(w["texture_hash"])
		if bool(w["is_text"]):
			var text: String = str(w["text"])
			if text.is_empty() and str(w["name"]) not in ["ANIMATING_MENUITEM", "SCROLLING_TEXT"]:
				continue
			var font := _font_for(hash_value)
			if font.is_empty() or font.get("texture") == null:
				missing += 1
				continue
			dimensions[int(entry["i"])] = Vector2(_text_width(text, font["glyphs"], float(w["scale_x"])), float(font["line_height"]) * float(w["scale_y"]))
			draw_list.append({"kind": "text", "w": w, "slot": int(entry["i"]), "font": font})
			glyph_texts += 1
		else:
			if hash_value == 0:
				continue
			if hash_value == 0xc3f0caef:
				draw_list.append({"kind": "quad", "w": w, "slot": int(entry["i"]), "texture": null})
				quads += 1
				continue
			var texture: Texture2D = loader.load_texture_by_hash(hash_value)
			if texture == null:
				missing += 1
				continue
			dimensions[int(entry["i"])] = texture.get_size() * Vector2(float(w["scale_x"]), float(w["scale_y"]))
			draw_list.append({"kind": "quad", "w": w, "slot": int(entry["i"]), "texture": texture})
			quads += 1

	# Build the menu for this screen through the recovered runtime (FUN_0016A858).
	layout.set_widget_measurements(screen_index, dimensions)
	var opened: Dictionary = layout.menu_open(screen_index) if open_menu else layout.menu_state()
	menu_state = opened
	if opened.has("not_implemented"):
		notice_label.text = str(opened["not_implemented"])
	else:
		notice_label.text = ""

	var screen_name: String = str(info.get("screen_name", ""))
	if screen_name.is_empty():
		screen_name = "name hash 0x%08x (name not recovered)" % int(info.get("name_hash", 0))
	info_label.text = "screen %d/%d  %s  widgets=%d quads=%d text=%d missing=%d" % [
		screen_index, screen_count - 1, screen_name, widgets.size(), quads, glyph_texts, missing]
	_refresh_menu_label()
	queue_redraw()

func _refresh_menu_label() -> void:
	var items: int = int(menu_state.get("item_count", 0))
	var current: int = int(menu_state.get("current", -1))
	var sound: String = str(menu_state.get("sound_name", ""))
	var messages: PackedStringArray = menu_state.get("messages", PackedStringArray())
	var last_message := "" if messages.is_empty() else messages[messages.size() - 1]
	menu_label.text = "MENUITEM count=%d current=%d (%s)   %s   sound=%s   msg=%s   [PgUp/PgDn screen, Tab info, F8/F9 gaps]" % [
		items, current, str(menu_state.get("current_name", "-")), last_event_text, sound, last_message]

func _text_width(text: String, glyphs: Dictionary, scale_x: float) -> float:
	var total := 0.0
	for i in text.length():
		var metrics = glyphs.get(text.unicode_at(i))
		if metrics != null:
			total += float(metrics[4])
	return total * scale_x

func _process(delta: float) -> void:
	if startup_active:
		var movie_tick: Dictionary = intro_movie.tick(delta)
		if movie_tick.has("not_implemented"):
			_fatal(str(movie_tick["not_implemented"]))
			return
		_apply_startup(intro_movie.tick_startup(bool(movie_tick.get("finished", false)), delta))
	if background_movie != null:
		var bg_tick: Dictionary = background_movie.tick(delta)
		if bg_tick.has("not_implemented"):
			_fatal(str(bg_tick["not_implemented"]))
			return
	if layout == null or draw_list.is_empty():
		return
	var tick: Dictionary = layout.menu_tick(delta)
	if not bool(tick.get("ok", false)):
		notice_label.text = str(tick.get("error", "menu tick failed"))
	if tick.has("not_implemented"):
		notice_label.text = str(tick["not_implemented"])
	if int(tick.get("screen", screen_index)) != screen_index:
		screen_index = int(tick["screen"])
		_build_screen(false)
	_play_menu_audio()
	queue_redraw()

func _play_menu_audio() -> void:
	for request in layout.drain_menu_audio():
		if request.has("not_implemented"):
			notice_label.text = str(request["not_implemented"])
			continue
		if DisplayServer.get_name() == "headless":
			continue # Validate decoded resources in smoke; no hardware audio endpoint.
		var player := AudioStreamPlayer.new()
		player.stream = request["stream"]
		player.volume_linear = float(request.get("gain", 1.0))
		player.pitch_scale = float(request.get("pitch_scale", 1.0))
		add_child(player)
		player.finished.connect(player.queue_free)
		player.play()

# Port rendering seam for the original per-widget clip rectangles (TASK-0017).
# Intersect destination geometry and shift/scale UVs together, preserving partial glyphs.
func _draw_clipped_texture(texture: Texture2D, dest: Rect2, region: Rect2, color: Color, clip: Rect2) -> void:
	if not dest.has_area():
		return
	if not clip.has_area():
		draw_texture_rect_region(texture, dest, region, color)
		return
	var visible := dest.intersection(clip)
	if not visible.has_area():
		return
	var uv_scale := region.size / dest.size
	var cropped := Rect2(region.position + (visible.position - dest.position) * uv_scale, visible.size * uv_scale)
	draw_texture_rect_region(texture, visible, cropped, color)

func _draw() -> void:
	# Read live fields every frame: FUN_001410A0 copies target text/font/transform into the ghost,
	# and the recovered scale, alpha and selection-bar functions mutate the authored records.
	if layout == null:
		return
	var dynamic: Array = layout.get_widget_dynamic(screen_index)
	var ordered := draw_list.duplicate()
	ordered.sort_custom(func(a, b):
		var wa: Dictionary = dynamic[int(a["slot"])]
		var wb: Dictionary = dynamic[int(b["slot"])]
		if int(wa["layer"]) != int(wb["layer"]):
			return int(wa["layer"]) > int(wb["layer"])
		return int(a["slot"]) < int(b["slot"]))
	draw_set_transform(Vector2.ZERO, 0.0, Vector2(view_scale, view_scale))
	if startup_movie_visible:
		var name: String = "TITLE_LETTERBOX" if intro_letterbox else "TITLE_FULLSCREEN"
		for widget in dynamic:
			if str(widget["name"]) == name:
				var texture: Texture2D = intro_movie.get_texture()
				if texture != null:
					var rect := Rect2(Vector2(widget["x"], widget["y"]), Vector2(512.0 * float(widget["scale_x"]), 256.0 * float(widget["scale_y"])))
					draw_texture_rect(texture, rect, false)
		return
	for item in ordered:
		var slot: int = int(item["slot"])
		var w: Dictionary = dynamic[slot]
		if int(w["visible"]) == 0:
			continue
		if startup_active and str(w["name"]) == "TITLE_LETTERBOX":
			continue
		var color := Color(float(w["color_r"]), float(w["color_g"]), float(w["color_b"]), float(w["color_a"]))
		var pos := Vector2(w["x"], w["y"])
		var clip: Rect2 = w.get("clip_rect", Rect2())
		if item["kind"] == "quad":
			var texture: Texture2D = background_movie.get_texture() if int(w["texture_hash"]) == 0xc3f0caef else loader.load_texture_by_hash(int(w["texture_hash"]))
			if texture == null:
				continue
			var size := Vector2(texture.get_width() * float(w["scale_x"]), texture.get_height() * float(w["scale_y"]))
			_draw_clipped_texture(texture, Rect2(pos, size), Rect2(Vector2.ZERO, texture.get_size()), color, clip)
		else:
			var font: Dictionary = _font_for(int(w["texture_hash"]))
			if font.is_empty() or font.get("texture") == null:
				continue
			var glyphs: Dictionary = font["glyphs"]
			var atlas: Texture2D = font["texture"]
			var text: String = str(w["text"])
			var sx := float(w["scale_x"])
			var sy := float(w["scale_y"])
			var pen := pos
			# align 2 = left, 4 = centred on x (verified against PRESS_START_1 at x=320 and the
			# right-hand stat column at x=494).
			if (int(w["text_align"]) & 4) != 0:
				pen.x -= _text_width(text, glyphs, sx) * 0.5
			for i in text.length():
				var metrics = glyphs.get(text.unicode_at(i))
				if metrics == null:
					continue
				var region := Rect2(float(metrics[0]), float(metrics[1]), float(metrics[2]), float(metrics[3]))
				var dest := Rect2(pen, Vector2(region.size.x * sx, region.size.y * sy))
				_draw_clipped_texture(atlas, dest, region, color, clip)
				pen.x += float(metrics[4]) * sx

func _send_joy_event(joy_event: int) -> void:
	if startup_active:
		if joy_event in [JOY_EVENT_START, JOY_EVENT_MENU_SELECT]:
			_apply_startup(intro_movie.select_startup())
		return
	var state: Dictionary = layout.menu_joy_event(pad_index, joy_event, true)
	if not bool(state.get("ok", false)):
		last_event_text = "%s -> %s" % [layout.joy_event_name(joy_event), str(state.get("error", "?"))]
		_refresh_menu_label()
		return
	menu_state = state
	if int(state.get("screen", screen_index)) != screen_index:
		screen_index = int(state["screen"])
		_build_screen(false)
	last_event_text = str(layout.joy_event_name(joy_event))
	notice_label.text = str(state.get("not_implemented", ""))
	notice_label.visible = show_info or not notice_label.text.is_empty()
	_play_menu_audio()
	_refresh_menu_label()
	queue_redraw()

func _probe(which: String) -> void:
	var result: Dictionary = layout.probe_not_implemented(which)
	notice_label.text = str(result.get("not_implemented", result.get("error", "")))

func _input(event: InputEvent) -> void:
	if not (event is InputEventKey) or not event.pressed or event.echo:
		return
	var map := _key_map()
	if map.has(event.keycode):
		_send_joy_event(int(map[event.keycode]))
		return
	match event.keycode:
		KEY_PAGEDOWN, KEY_BRACKETRIGHT:
			screen_index = (screen_index + 1) % screen_count
			_build_screen()
		KEY_PAGEUP, KEY_BRACKETLEFT:
			screen_index = (screen_index - 1 + screen_count) % screen_count
			_build_screen()
		KEY_F5:
			_build_screen()
		KEY_F8:
			_probe("highlight_animation")
		KEY_F9:
			_probe("localised_text")
		KEY_F10:
			_probe("popup")
		KEY_TAB:
			show_info = not show_info
			info_label.visible = show_info
			menu_label.visible = show_info
			notice_label.visible = show_info or not notice_label.text.is_empty()
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

# Headless check of the recovered interaction: NEW_MAIN_MENU must resolve by name, build a menu
# from its own MENUITEM widgets, move the selection on the recovered joypad events, report the
# original sound event names, and surface hp2::NotImplemented for the paths that are not done.
func _menu_smoke() -> void:
	var main_menu: int = layout.find_screen_by_name("NEW_MAIN_MENU")
	if main_menu != 0:
		_fatal("NEW_MAIN_MENU must resolve to screen 0, got %d" % main_menu)
		return
	screen_index = main_menu
	_build_screen()

	var items: int = int(menu_state.get("item_count", 0))
	if items < 2:
		_fatal("NEW_MAIN_MENU must expose at least two MENUITEM widgets, got %d" % items)
		return

	var start: int = int(menu_state.get("current", -1))
	_send_joy_event(JOY_EVENT_SCROLL_DOWN)
	var after_down: int = int(menu_state.get("current", -1))
	if after_down == start:
		_fatal("JOY_EVENT_SCROLL_DOWN did not move the selection")
		return
	if str(menu_state.get("sound_name", "")) != "MenuUpDown":
		_fatal("SCROLL_DOWN must request the MenuUpDown sound, got '%s'" % str(menu_state.get("sound_name", "")))
		return

	_send_joy_event(JOY_EVENT_MENU_ANALOG_UP)
	if int(menu_state.get("current", -1)) != start:
		_fatal("JOY_EVENT_MENU_ANALOG_UP did not return to the starting item")
		return

	_send_joy_event(JOY_EVENT_MENU_SELECT)
	if int(menu_state.get("activated_name_hash", 0)) == 0:
		_fatal("JOY_EVENT_MENU_SELECT did not activate an item")
		return
	if str(menu_state.get("sound_name", "")) != "MenuSelect":
		_fatal("MENU_SELECT must request the MenuSelect sound")
		return

	# The disabled/highlight colours must be the ones stored in the ELF, applied by the recovered
	# colour path to the real widget records.
	var colours: PackedColorArray = layout.get_widget_colours(screen_index)
	var found_highlight := false
	for c in colours:
		if absf(c.r - 1.0) < 0.01 and absf(c.g - 0.5) < 0.01 and absf(c.b) < 0.01:
			found_highlight = true
			break
	if not found_highlight:
		_fatal("no widget carries the recovered highlight colour (0x80,0x40,0x00)")
		return

	for which in ["highlight_animation", "localised_text", "popup"]:
		var probe: Dictionary = layout.probe_not_implemented(which)
		if not probe.has("not_implemented"):
			_fatal("probe '%s' must raise hp2::NotImplemented" % which)
			return

	print("FRONTEND_MENU_SMOKE_OK: screen=%d items=%d start=%d after_down=%d" % [
		screen_index, items, start, after_down])
	get_tree().quit(0)

# TASK-0015: host integration check on real authored widgets; does not establish PS2 equivalence.
func _animation_smoke() -> void:
	screen_index = 0
	_build_screen()
	var widgets: Array = layout.get_widget_dynamic(0)
	var ghost_slot := -1
	var target_slot := -1
	for i in widgets.size():
		if str(widgets[i]["name"]) == "ANIMATING_MENUITEM":
			ghost_slot = i
		if int(widgets[i]["name_hash"]) == int(menu_state["current_name_hash"]):
			target_slot = i
	var ghost_drawn := false
	for entry in draw_list:
		if int(entry["slot"]) == ghost_slot:
			ghost_drawn = true
	if ghost_slot < 0 or target_slot < 0 or not ghost_drawn:
		_fatal("animation template/target missing from renderer")
		return
	layout.menu_tick(0.0)
	var initial: Array = layout.get_widget_dynamic(0)
	layout.menu_tick(0.25)
	var halfway: Array = layout.get_widget_dynamic(0)
	var target_scale: float = float(initial[target_slot]["scale_x"])
	if absf(float(initial[ghost_slot]["scale_x"]) - target_scale) > 0.0001 or absf(float(initial[ghost_slot]["color_a"]) - 1.0) > 0.0001:
		_fatal("ghost did not start at target scale and alpha 128")
		return
	if float(halfway[ghost_slot]["scale_x"]) <= target_scale or float(halfway[ghost_slot]["color_a"]) >= 1.0:
		_fatal("ghost did not grow/fade at quarter second")
		return
	if str(halfway[ghost_slot]["text"]) != str(halfway[target_slot]["text"]):
		_fatal("ghost did not copy target text")
		return
	layout.menu_tick(1.76)
	layout.menu_tick(0.0)
	var repeated: Array = layout.get_widget_dynamic(0)
	if absf(float(repeated[ghost_slot]["scale_x"]) - target_scale) > 0.0001:
		_fatal("ghost did not restart after two second period")
		return
	_send_joy_event(JOY_EVENT_SCROLL_DOWN)
	layout.menu_tick(0.0)
	var retargeted: Array = layout.get_widget_dynamic(0)
	var new_text := ""
	for w in retargeted:
		if int(w["name_hash"]) == int(menu_state["current_name_hash"]):
			new_text = str(w["text"])
	if str(retargeted[ghost_slot]["text"]) != new_text:
		_fatal("ghost did not retarget after navigation")
		return
	if bool(layout.menu_tick(-1.0).get("ok", true)):
		_fatal("negative delta accepted")
		return
	print("FRONTEND_ANIMATION_SMOKE_OK: pulse, repeat, retarget, renderer template")
	get_tree().quit(0)

# TASK-0015: real data end-to-end activation, deferred exit and explicit missing handlers.
func _activation_smoke() -> void:
	for choice in 3:
		screen_index = 0
		_build_screen()
		for step in choice:
			_send_joy_event(JOY_EVENT_SCROLL_DOWN)
		_send_joy_event(JOY_EVENT_MENU_SELECT)
		var target: int = [1, 2, 24][choice]
		if screen_index != target:
			_fatal("main-menu activation expected %d got %d" % [target, screen_index])
			return
		if choice < 2 and int(menu_state.get("dat_002FC070", -1)) != 1 - choice:
			_fatal("main-menu flag mismatch")
			return
		if choice < 2:
			var missing: Dictionary = layout.menu_joy_event(0, JOY_EVENT_MENU_SELECT, true)
			if not missing.has("not_implemented"):
				_fatal("untranslated mode handler must raise")
				return
	_send_joy_event(JOY_EVENT_MENU_BACK)
	if screen_index != 0:
		_fatal("Options Back did not return to main")
		return
	_send_joy_event(JOY_EVENT_MENU_BACK)
	if screen_index != 0:
		_fatal("main menu must reject Back")
		return
	for choice in 7:
		screen_index = 24
		_build_screen()
		for step in choice:
			_send_joy_event(JOY_EVENT_SCROLL_DOWN)
		_send_joy_event(JOY_EVENT_MENU_SELECT)
		if choice == 2:
			if not notice_label.text.contains("001EE6C0") or screen_index != 24:
				_fatal("Display preparation must report missing state")
				return
			continue
		var target: int = [28, 31, 26, 37, 29, 16, 30][choice]
		if choice in [0, 1, 4]:
			if screen_index != 24 or not bool(menu_state.get("exiting", false)):
				_fatal("Options must animate before switching")
				return
			layout.menu_tick(0.5)
			menu_state = layout.menu_state()
			screen_index = int(menu_state["screen"])
			_build_screen(false)
		if screen_index != target:
			_fatal("Options choice %d expected %d got %d" % [choice, target, screen_index])
			return
		if choice == 4 and str(menu_state.get("active_class", "")) != "NameEntry":
			_fatal("shared document must preserve NameEntry class")
			return
	print("FRONTEND_ACTIVATION_SMOKE_OK: main routes, mode flag, Back, Options exit, explicit gaps")
	get_tree().quit(0)

# TASK-0016 host seams: actions/filenames/timing are produced by native recovered title state.
func _apply_startup(result: Dictionary) -> void:
	if startup_active:
		layout.title_visual_state(int(result.get("state", 1)) == 2, bool(result.get("can_start", false)))
	match int(result.get("action", 0)):
		1:
			startup_movie_visible = true
			intro_letterbox = str(result["movie"]).contains("NFSINTRO")
			var opened: Dictionary = intro_movie.play(str(result["movie"]), false)
			if not bool(opened.get("ok", false)):
				_fatal(str(opened.get("not_implemented", "movie open failed")))
				return
			movie_audio.stop()
			movie_audio.stream = intro_movie.get_audio_stream()
			movie_audio.pitch_scale = float(opened.get("audio_pitch_scale", 1.0))
			if movie_audio.stream != null and DisplayServer.get_name() != "headless":
				movie_audio.play()
		2:
			startup_movie_visible = false
			intro_movie.stop()
			movie_audio.stop()
			screen_index = 22
			if bool(result.get("start_music", false)):
				_start_music()
			else:
				layout.title_pulse_restart()
		3:
			startup_active = false
			startup_movie_visible = false
			intro_movie.stop()
			movie_audio.stop()
			screen_index = 0
			_build_screen()
	queue_redraw()

func _configure_background() -> void:
	if screen_index == 0 and not startup_active:
		_start_music()
	if background_movie == null:
		return
	var filename: String = {
		0: "MOVIES/mainmenu_movie.AVI",
		1: "MOVIES/hotpursuit_movie.AVI",
		2: "MOVIES/worldracing_ movie.AVI",
		24: "MOVIES/options_movie.AVI",
		26: "MOVIES/options_movie_new.AVI",
		27: "MOVIES/options_movie_new.AVI",
		28: "MOVIES/options_movie_new.AVI",
		31: "MOVIES/options_movie_new.AVI",
	}.get(screen_index, "")
	if filename.is_empty():
		background_movie.stop()
		return
	var result: Dictionary = background_movie.play(filename, true)
	if not bool(result.get("ok", false)):
		_fatal(str(result.get("not_implemented", "background movie open failed")))

func _exit_tree() -> void:
	for child in get_children():
		if child is AudioStreamPlayer:
			child.stop()
			child.stream = null
	if intro_movie != null:
		intro_movie.stop()
	if background_movie != null:
		background_movie.stop()

func _start_music() -> void:
	if DisplayServer.get_name() == "headless":
		return
	_play_music_request(layout.frontend_music(false))

func _next_music() -> void:
	_play_music_request(layout.frontend_music(true))

func _play_music_request(request: Dictionary) -> void:
	if not bool(request.get("ok", false)):
		_fatal(str(request.get("not_implemented", "jukebox scheduling failed")))
		return
	if request.has("stream"):
		music_audio.stream = request["stream"]
		music_audio.volume_linear = float(request["gain"])
		music_audio.pitch_scale = float(request.get("pitch_scale", 1.0))
		music_audio.play()

# TASK-0016 integration checks use actual configured assets; no original equivalence claim.
func _runtime_smoke() -> void:
	startup_active = true
	screen_index = 22
	_build_screen()
	_apply_startup(intro_movie.begin_startup(false))
	if not startup_movie_visible or intro_letterbox or intro_movie.get_texture().get_width() != 512:
		_fatal("startup EA logo binding failed")
		return
	var audio: AudioStreamWAV = intro_movie.get_audio_stream()
	if audio == null or audio.get_data().is_empty() or audio.format != AudioStreamWAV.FORMAT_16_BITS or not audio.stereo or audio.mix_rate != 48000:
		_fatal("original intro PCM missing")
		return
	var tick: Dictionary = intro_movie.tick(0.15)
	if int(tick.get("frame", -1)) != 0:
		_fatal("original audio/video lead missing")
		return
	tick = intro_movie.tick(0.25)
	if int(tick.get("frame", 0)) < 6 or tick.has("not_implemented"):
		_fatal("MV30 consecutive frame update failed")
		return
	_apply_startup(intro_movie.tick_startup(true, 0.2))
	if not intro_letterbox:
		_fatal("second intro must use letterbox geometry")
		return
	_apply_startup(intro_movie.select_startup())
	_apply_startup(intro_movie.tick_startup(false, 0.0))
	_apply_startup(intro_movie.tick_startup(false, 4.0))
	_apply_startup(intro_movie.select_startup())
	if not startup_active:
		_fatal("title must reject activation at exactly four seconds")
		return
	_apply_startup(intro_movie.tick_startup(false, 0.001))
	_apply_startup(intro_movie.select_startup())
	if startup_active or screen_index != 0:
		_fatal("title did not request main menu after time gate")
		return
	if background_movie.get_texture() == null or background_movie.get_texture().get_height() != 384:
		_fatal("original background movie unavailable")
		return
	var bg_tick: Dictionary = background_movie.tick(0.2)
	if int(bg_tick.get("frame", 0)) < 6:
		_fatal("background movie did not advance")
		return
	var panel_found := false
	for item in draw_list:
		if int(item["w"]["texture_hash"]) == 0xc3f0caef:
			panel_found = true
	if not panel_found:
		_fatal("authored PANELMOVIE widget not retained by renderer")
		return
	layout.menu_tick(0.0)
	layout.menu_tick(0.25)
	var hint := ""
	for widget in layout.get_widget_dynamic(0):
		if str(widget["name"]) == "SCROLLING_TEXT":
			hint = str(widget["text"])
			if float(widget["x"]) >= 640.0:
				_fatal("hint did not enter at authored X")
				return
	if hint.is_empty() or hint.contains("This text will scroll"):
		_fatal("hint still shows author placeholder")
		return
	layout.menu_tick(0.05)
	layout.menu_joy_event(0, JOY_EVENT_SCROLL_DOWN, true)
	var sounds: Array = layout.drain_menu_audio()
	if sounds.is_empty():
		_fatal("native bSound request was not delivered")
		return
	var request: Dictionary = sounds[sounds.size() - 1]
	var sound: AudioStreamWAV = request.get("stream")
	if sound == null or not sound.stereo or sound.mix_rate != 22050 or sound.get_data().size() != 23408:
		_fatal("original menu stereo PCM mismatch")
		return
	if absf(float(request["gain"]) - 0.1023) > 0.001:
		_fatal("original menu category/descriptor gain mismatch")
		return
	var music: Dictionary = layout.frontend_music(false)
	var music_stream: AudioStreamWAV = music.get("stream")
	if music_stream == null or int(music.get("index", -1)) != 5 or music_stream.mix_rate != 32000:
		_fatal("original first frontend music selection/PCM failed")
		return
	if absf(float(music.get("gain", 0.0)) - 0.45) > 0.0001:
		_fatal("original music category gain mismatch")
		return
	if absf(float(music.get("pitch_scale", 0.0)) - 1.0103759765625) > 0.000001:
		_fatal("original music hardware pitch mismatch")
		return
	var next_music: Dictionary = layout.frontend_music(true)
	if int(next_music.get("index", -1)) != 7:
		_fatal("original music completion selection failed")
		return
	for hint_screen in [0, 1, 2, 24, 28, 31]:
		var opened_hint: Dictionary = layout.menu_open(hint_screen)
		if opened_hint.has("not_implemented"):
			_fatal("hint screen open failed: " + str(hint_screen))
			return
		layout.menu_tick(0.0)
		layout.menu_tick(0.25)
		var found_hint := false
		for widget in layout.get_widget_dynamic(hint_screen):
			if str(widget["name"]) == "SCROLLING_TEXT":
				found_hint = true
				if str(widget["text"]).is_empty() or str(widget["text"]).contains("This text will scroll"):
					_fatal("authored placeholder survived in screen " + str(hint_screen))
					return
		if not found_hint:
			_fatal("recovered hint attachment missing")
			return
	print("FRONTEND_RUNTIME_SMOKE_OK: startup movies/PCM, title gate, background, localisation, stereo SFX/gains, music")
	get_tree().quit(0)

# TASK-0017: actual authored submenu history and Audio viewport integration.
func _enter_option_for_test(option_index: int) -> void:
	startup_active = false
	screen_index = 0
	_build_screen()
	_send_joy_event(JOY_EVENT_SCROLL_DOWN)
	_send_joy_event(JOY_EVENT_SCROLL_DOWN)
	_send_joy_event(JOY_EVENT_MENU_SELECT)
	for i in option_index:
		_send_joy_event(JOY_EVENT_SCROLL_DOWN)
	_send_joy_event(JOY_EVENT_MENU_SELECT)
	_process(1.0) # Finish the original Options exit path, if this route uses one.

func _backspace_for_test(pressed: bool = true, repeated: bool = false) -> void:
	var event := InputEventKey.new()
	event.keycode = KEY_BACKSPACE
	event.pressed = pressed
	event.echo = repeated
	_input(event)

func _submenu_smoke() -> void:
	for route in [{"item":0,"screen":28},{"item":1,"screen":31},{"item":3,"screen":37},
		{"item":4,"screen":29},{"item":5,"screen":16},{"item":6,"screen":30}]:
		_enter_option_for_test(int(route["item"]))
		if screen_index != int(route["screen"]):
			_fatal("submenu route failed: " + str(route))
			return
		_backspace_for_test(false)
		_backspace_for_test(true,true)
		if screen_index != int(route["screen"]):
			_fatal("Backspace release/repeat must not pop submenu history")
			return
		_backspace_for_test()
		if screen_index != 24 or not notice_label.text.is_empty():
			_fatal("submenu Back failed: " + str(route) + " " + notice_label.text)
			return
		_backspace_for_test()
		if screen_index != 0 or int(menu_state.get("history_depth",-1)) != 0:
			_fatal("submenu class history did not return to main")
			return
	for main_item in [0,1]:
		screen_index = 0
		_build_screen()
		for i in main_item:
			_send_joy_event(JOY_EVENT_SCROLL_DOWN)
		_send_joy_event(JOY_EVENT_MENU_SELECT)
		_backspace_for_test()
		if screen_index != 0:
			_fatal("mode selector Back failed")
			return
	_enter_option_for_test(0)
	_send_joy_event(JOY_EVENT_SCROLL_UP)
	if int(menu_state.get("current",-1)) != 0:
		_fatal("Audio must block upward wrap at the first item")
		return
	var initial: Array = layout.get_widget_dynamic(28)
	var footer_y := 0.0
	var original_y := 0.0
	var clip_bottom := 0.0
	for widget in initial:
		if str(widget["name"]) == "SCROLLING_TEXT":
			footer_y = float(widget["y"])
		if int(widget["name_hash"]) in [0xd1cda594,0x9345e4a7] and int(widget["visible"]) != 0:
			_fatal("Audio selector arrows visible on a volume row")
			return
		if str(widget["name"]) == "MENUITEM_9":
			original_y = float(widget["y"])
			var clip: Rect2 = widget.get("clip_rect",Rect2())
			clip_bottom = clip.end.y
	if clip_bottom <= 0.0 or original_y <= clip_bottom:
		_fatal("Audio overflow must start clipped below the authored mask")
		return
	for i in 8:
		_send_joy_event(JOY_EVENT_SCROLL_DOWN)
	_process(0.5)
	var scrolled := false
	for widget in layout.get_widget_dynamic(28):
		if str(widget["name"]) == "MENUITEM_9":
			scrolled = float(widget["y"]) < original_y and float(widget["y"]) < clip_bottom
		if str(widget["name"]) == "SCROLLING_TEXT" and float(widget["y"]) != footer_y:
			_fatal("Audio footer moved with scrolling menu")
			return
	if not scrolled:
		_fatal("Audio selected overflow item did not scroll into viewport")
		return
	_send_joy_event(JOY_EVENT_SCROLL_UP) # Audio Mode, exact selector attachment.
	_process(0.5)
	var target_rect := Rect2()
	var arrows: Array = []
	for widget in layout.get_widget_dynamic(28):
		if int(widget["name_hash"]) == 0x2932dfca:
			target_rect = Rect2(Vector2(widget["x"],widget["y"]),Vector2(0,20))
		if int(widget["name_hash"]) in [0xd1cda594,0x9345e4a7]:
			arrows.append(widget)
	for arrow in arrows:
		if int(arrow["visible"]) == 0 or float(arrow["y"]) >= footer_y:
			_fatal("Audio selector did not follow its scrolling target")
			return
	for i in 8:
		_send_joy_event(JOY_EVENT_SCROLL_DOWN)
	if int(menu_state.get("current",-1)) != 10:
		_fatal("Audio must block downward wrap at the last item")
		return
	_backspace_for_test()
	if screen_index != 24:
		_fatal("Audio Back failed after scrolling")
		return
	_backspace_for_test()
	var before_language := ""
	for widget in layout.get_widgets(0):
		if str(widget["name"]) == "MENUITEM_1":
			before_language = str(widget["text"])
	_enter_option_for_test(6)
	_send_joy_event(JOY_EVENT_SCROLL_DOWN) # French, original enum1.
	_backspace_for_test()
	var after_language := ""
	for widget in layout.get_widgets(0):
		if str(widget["name"]) == "MENUITEM_1":
			after_language = str(widget["text"])
	if screen_index != 24 or before_language == after_language:
		_fatal("Language Back did not apply its selected locale before direct pop")
		return
	screen_index = 26 # Debug entry; Options snapshot dependency is tested as a gap separately.
	_build_screen()
	var display_original_y := 0.0
	var display_clip := Rect2()
	for widget in layout.get_widget_dynamic(26):
		if str(widget["name"]) == "MENUITEM_9":
			display_original_y = float(widget["y"])
			display_clip = widget.get("clip_rect",Rect2())
	if not display_clip.has_area() or display_original_y <= display_clip.end.y:
		_fatal("Display overflow clip missing")
		return
	for i in 12:
		_send_joy_event(JOY_EVENT_SCROLL_DOWN)
	_process(0.5)
	if int(menu_state.get("current",-1)) != 8:
		_fatal("Display must block last-row wrap")
		return
	for widget in layout.get_widget_dynamic(26):
		if str(widget["name"]) == "MENUITEM_9" and float(widget["y"]) >= display_clip.end.y:
			_fatal("Display overflow did not scroll into view")
			return
	print("FRONTEND_SUBMENU_SMOKE_OK: 8 Back routes, locale direct-pop, Audio/Display clipping/scroll, selectors, nonwrapends")
	get_tree().quit(0)
