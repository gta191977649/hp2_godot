extends Node2D

# TASK-0018 / D008: the Godot host for the native front-end runtime.
#
# This script makes no drawing decisions and holds no game state. The host it replaced
# (scripts/frontend.gd, deleted in TASK-0018) pulled a Dictionary per widget every frame and did
# the draw ordering, glyph placement, text alignment and clip intersection itself in
# Control._draw.
#
# Here the division is:
#   native  (game_core + port_runtime) -- screens, menus, navigation, animation, measurement,
#                                         draw order, glyph layout, original clip rectangles,
#                                         and the resulting port::render::Frame
#   backend (Ps2RenderHost)            -- RenderingServer RIDs, canvas item, texture uploads
#   this script                        -- window, keys -> recovered JOY_EVENT_* ids, frame
#                                         delta, audio hardware, process lifetime
#
# Nothing below reads a widget, a position, a colour or a texture.

const CANVAS := Vector2(640.0, 480.0)

# Recovered joypad event ids (ELF table 0x002FA2D0 -> src/game_core/input/joyevents.hpp).
# Input translation is a host responsibility: the ids themselves are the original ones.
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

static func _key_map() -> Dictionary:
	return {
		KEY_UP: JOY_EVENT_SCROLL_UP,
		KEY_DOWN: JOY_EVENT_SCROLL_DOWN,
		KEY_LEFT: JOY_EVENT_SCROLL_LEFT,
		KEY_RIGHT: JOY_EVENT_SCROLL_RIGHT,
		KEY_W: JOY_EVENT_MENU_ANALOG_UP,
		KEY_S: JOY_EVENT_MENU_ANALOG_DOWN,
		KEY_A: JOY_EVENT_MENU_ANALOG_LEFT,
		KEY_D: JOY_EVENT_MENU_ANALOG_RIGHT,
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
		KEY_Q: JOY_EVENT_MENU_L1,
		KEY_E: JOY_EVENT_MENU_R1,
		KEY_1: JOY_EVENT_MENU_L2,
		KEY_3: JOY_EVENT_MENU_R2,
	}

static func _font_bundles() -> PackedStringArray:
	return PackedStringArray(["FRONTEND/FRONTB.BUN", "GLOBAL/GLOBALB.BUN", "GLOBAL/INGAMEB.BUN"])

var layout: Object
var render_host: Object
var intro_movie: Object
var background_movie: Object
var movie_audio: AudioStreamPlayer
var music_audio: AudioStreamPlayer
var view_scale := 2
var startup_active := false
var startup_movie_visible := false
var intro_letterbox := false
var screen_index := 0
var screen_count := 0
var last_stats: Dictionary = {}
var info_label: Label
var show_info := false
# Frame-rate readout. The window title carries it at all times so the two render paths can be
# compared without turning the overlay on; the on-screen line follows Tab like the rest.
var fps_label: Label
var frame_ms := 0.0
var title_cooldown := 0.0
const RENDER_PATH_NAME := "native frame + RenderingServer"

func _ready() -> void:
	for required in ["Ps2FrontEndLayout", "Ps2RenderHost", "Ps2MoviePlayer"]:
		if not ClassDB.class_exists(required):
			push_error("GDExtension class missing: %s" % required)
			get_tree().quit(1)
			return
	layout = ClassDB.instantiate("Ps2FrontEndLayout")
	render_host = ClassDB.instantiate("Ps2RenderHost")
	intro_movie = ClassDB.instantiate("Ps2MoviePlayer")
	background_movie = ClassDB.instantiate("Ps2MoviePlayer")
	movie_audio = AudioStreamPlayer.new()
	add_child(movie_audio)
	music_audio = AudioStreamPlayer.new()
	add_child(music_audio)
	music_audio.finished.connect(_next_music)

	var manual_screen := false
	var start_name := ""
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--screen="):
			screen_index = int(argument.trim_prefix("--screen="))
			manual_screen = true
		elif argument.begins_with("--screen-name="):
			start_name = argument.trim_prefix("--screen-name=")
			manual_screen = true
		elif argument.begins_with("--scale="):
			view_scale = int(argument.trim_prefix("--scale="))
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

	# The backend draws into this node's canvas item; window scaling is its concern, not a
	# transform applied to native geometry.
	render_host.attach(get_canvas_item())
	render_host.set_output_scale(float(view_scale))

	fps_label = _make_label(Vector2(8, 4), Color(0.6, 0.9, 1))
	info_label = _make_label(Vector2(8, 22), Color(1, 1, 0.55))

	if not manual_screen:
		startup_active = true
		screen_index = 22
	_open_screen()
	if startup_active:
		_apply_startup(intro_movie.begin_startup(false))

	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--menu-events="):
			for id_text in argument.trim_prefix("--menu-events=").split(",", false):
				_send_joy_event(int(id_text))

	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--capture="):
			await _capture(argument.trim_prefix("--capture="))
			return
		elif argument == "--native-render-smoke":
			_native_render_smoke()
			return
		elif argument == "--runtime-smoke":
			_runtime_smoke()
			return

func _make_label(position_px: Vector2, colour: Color) -> Label:
	var label := Label.new()
	label.position = position_px
	label.add_theme_color_override("font_color", colour)
	label.add_theme_font_size_override("font_size", 13)
	label.visible = show_info
	add_child(label)
	return label

# Engine.get_frames_per_second() is the engine's own once-a-second count; the millisecond figure
# is a smoothed frame time, which reacts faster and is the useful number when comparing paths.
func _update_fps(delta: float) -> void:
	frame_ms = delta * 1000.0 if frame_ms == 0.0 else frame_ms + (delta * 1000.0 - frame_ms) * 0.1
	var text := "%d FPS  %.2f ms  [%s]" % [Engine.get_frames_per_second(), frame_ms, RENDER_PATH_NAME]
	if fps_label != null:
		fps_label.text = text
	title_cooldown -= delta
	if title_cooldown <= 0.0 and DisplayServer.get_name() != "headless":
		title_cooldown = 0.25
		get_window().title = "HP2 PS2 Native Port - %s" % text

func _fatal(message: String) -> void:
	push_error(message)
	print("FRONTEND_ERROR: ", message)
	get_tree().quit(1)

func _open_screen(open_menu: bool = true) -> void:
	_configure_background()
	var opened: Dictionary = layout.menu_open(screen_index) if open_menu else layout.menu_state()
	if opened.has("not_implemented"):
		print("NOT_IMPLEMENTED: ", str(opened["not_implemented"]))

func _process(delta: float) -> void:
	_update_fps(delta)
	if layout == null:
		return
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
	var tick: Dictionary = layout.menu_tick(delta)
	if int(tick.get("screen", screen_index)) != screen_index:
		screen_index = int(tick["screen"])
		_open_screen(false)
	_play_menu_audio()
	_draw_frame()

# The whole render step: one native frame, one backend submission. No widget crosses this call.
func _draw_frame() -> void:
	last_stats = render_host.draw(layout, intro_movie, background_movie)
	if not bool(last_stats.get("ok", false)):
		push_error(str(last_stats.get("error", "render host draw failed")))
		return
	if show_info and info_label != null:
		info_label.text = "screen %d/%d  quads=%d glyphs=%d missing=%d clipped=%d textures=%d" % [
			screen_index, screen_count - 1, int(last_stats["quads"]), int(last_stats["glyphs"]),
			int(last_stats["missing"]), int(last_stats["clipped_out"]),
			int(last_stats["textures"])]

func _send_joy_event(joy_event: int) -> void:
	if startup_active:
		if joy_event in [JOY_EVENT_START, JOY_EVENT_MENU_SELECT]:
			_apply_startup(intro_movie.select_startup())
		return
	# The View Event Tree screen drives its own cursor (FUN_001FAF60 state 0); the runtime says
	# whether it consumed the event, so this script does not need to know which screen is open.
	if layout.event_tree_joy_event(joy_event):
		return
	var state: Dictionary = layout.menu_joy_event(0, joy_event, true)
	if not bool(state.get("ok", false)):
		push_error(str(state.get("error", "menu event failed")))
		return
	if state.has("not_implemented"):
		print("NOT_IMPLEMENTED: ", str(state["not_implemented"]))
	if int(state.get("screen", screen_index)) != screen_index:
		screen_index = int(state["screen"])
		_open_screen(false)
	_play_menu_audio()

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
			_open_screen()
		KEY_PAGEUP, KEY_BRACKETLEFT:
			screen_index = (screen_index - 1 + screen_count) % screen_count
			_open_screen()
		KEY_TAB:
			show_info = not show_info
			fps_label.visible = show_info
			info_label.visible = show_info
		KEY_ESCAPE:
			get_tree().quit(0)

# Host seam: the recovered startup state decides the action and the presentation; this forwards
# the presentation to the runtime so the frame builder, not this script, suppresses widgets.
func _apply_startup(result: Dictionary) -> void:
	match int(result.get("action", 0)):
		1:
			intro_letterbox = str(result["movie"]).contains("NFSINTRO")
			var opened: Dictionary = intro_movie.play(str(result["movie"]), false)
			if not bool(opened.get("ok", false)):
				_fatal(str(opened.get("not_implemented", "movie open failed")))
				return
			startup_movie_visible = true
			layout.set_startup_presentation(true, intro_letterbox, startup_active)
			movie_audio.stop()
			movie_audio.stream = intro_movie.get_audio_stream()
			movie_audio.pitch_scale = float(opened.get("audio_pitch_scale", 1.0))
			if movie_audio.stream != null and DisplayServer.get_name() != "headless":
				movie_audio.play()
		2:
			intro_movie.stop()
			movie_audio.stop()
			screen_index = 22
			startup_movie_visible = false
			layout.set_startup_presentation(false, false, startup_active)
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
			layout.set_startup_presentation(false, false, false)
			_open_screen()
	if startup_active:
		layout.title_visual_state(int(result.get("state", 1)) == 2, bool(result.get("can_start", false)))

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

func _play_menu_audio() -> void:
	for request in layout.drain_menu_audio():
		if request.has("not_implemented"):
			print("NOT_IMPLEMENTED: ", str(request["not_implemented"]))
			continue
		if DisplayServer.get_name() == "headless":
			continue
		var player := AudioStreamPlayer.new()
		player.stream = request["stream"]
		player.volume_linear = float(request.get("gain", 1.0))
		player.pitch_scale = float(request.get("pitch_scale", 1.0))
		add_child(player)
		player.finished.connect(player.queue_free)
		player.play()

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

func _exit_tree() -> void:
	for child in get_children():
		if child is AudioStreamPlayer:
			child.stop()
			child.stream = null
	if intro_movie != null:
		intro_movie.stop()
	if background_movie != null:
		background_movie.stop()
	if render_host != null:
		render_host.release()

func _capture(path: String) -> void:
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
	for capture_arg in OS.get_cmdline_user_args():
		if capture_arg.begins_with("--capture-after="):
			await get_tree().create_timer(float(capture_arg.trim_prefix("--capture-after="))).timeout
			await RenderingServer.frame_post_draw
	if get_viewport().get_texture().get_image().save_png(path) != OK:
		_fatal("capture failed")
		return
	print("FRONTEND_CAPTURE_OK: ", path)
	set_process(false)
	_exit_tree()
	await get_tree().process_frame
	await get_tree().process_frame
	get_tree().quit(0)

# Integration check for the native render path: every screen must build a frame through the
# backend, and the frame must come out of the runtime rather than from this script.
func _native_render_smoke() -> void:
	var total_quads := 0
	var total_glyphs := 0
	var empty_screens: Array = []
	# Screen 39 only has a tree once a mode is selected: FUN_001FB218 asks
	# FUN_00185568(0x2FC050), i.e. profile+0x1072C, which only a mode-select screen writes. So
	# walk the real route once first -- screen 1, down three to Tournament, select, then tick the
	# tExitScreenAnimation out -- instead of opening screen 39 cold and silently losing the
	# overlay from this smoke's coverage. JOY_EVENT_SCROLL_DOWN is 4, JOY_EVENT_MENU_SELECT 9.
	layout.menu_open(1)
	for _step in 3:
		layout.menu_joy_event(0, 4, true)
	layout.menu_joy_event(0, 9, true)
	layout.menu_tick(1.0)
	for index in screen_count:
		screen_index = index
		var opened: Dictionary = layout.menu_open(index)
		if not bool(opened.get("ok", false)) and not opened.has("not_implemented"):
			_fatal("screen %d failed to open: %s" % [index, str(opened.get("error", ""))])
			return
		var stats: Dictionary = render_host.draw(layout, intro_movie, background_movie)
		if not bool(stats.get("ok", false)):
			_fatal("screen %d render failed: %s" % [index, str(stats.get("error", ""))])
			return
		total_quads += int(stats["quads"])
		total_glyphs += int(stats["glyphs"])
		if int(stats["quads"]) + int(stats["glyphs"]) == 0:
			empty_screens.append(index)
	if total_quads == 0 or total_glyphs == 0:
		_fatal("native render produced no geometry (quads=%d glyphs=%d)" % [total_quads, total_glyphs])
		return
	if render_host.get_texture_count() == 0:
		_fatal("backend uploaded no textures")
		return
	print("NATIVE_RENDER_SMOKE_OK: screens=%d quads=%d glyphs=%d textures=%d empty=%s" % [
		screen_count, total_quads, total_glyphs, render_host.get_texture_count(), str(empty_screens)])
	get_tree().quit(0)

# TASK-0018: the host half of the former scripts/frontend.gd --runtime-smoke. Everything that is
# platform-independent -- localisation, menu samples, the jukebox, menus, clipping -- moved to
# src/tests/frontend_runtime_tests.cpp and runs under CTest. What is left here genuinely needs
# the host: MV30 decoding into a texture, PCM into an AudioStreamWAV, and the recovered title
# time gate driven by a real frame delta.
func _runtime_smoke() -> void:
	startup_active = true
	screen_index = 22
	_open_screen()
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
	# The authored PANELMOVIE slot must reach the renderer as a quad sampling the live movie.
	# The backend reports that directly, so this check reads no widget.
	var stats: Dictionary = render_host.draw(layout, intro_movie, background_movie)
	if not bool(stats.get("ok", false)) or int(stats.get("movie_quads", 0)) < 1:
		_fatal("authored PANELMOVIE widget did not reach the renderer: %s" % str(stats))
		return
	print("FRONTEND_RUNTIME_SMOKE_OK: startup movies/PCM, title gate, background movie, live movie quad")
	get_tree().quit(0)
