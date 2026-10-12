extends Control

# TASK-0011: operable front-end menu closed loop.
# Confirmed edge: TitleScreen --(message 1, FUN_0016eef8)--> New Main Menu.
# "MumboJumboScreen" is a confirmed registered sibling class (FUN_0016f810) whose edge
# to TitleScreen/New Main Menu was not traced this session; it is offered here as a
# clearly-labelled structural placeholder, not an invented game-accurate transition.
# See docs/FRONTEND_MENU.md for full evidence and open items.

var observer: Object
var texture_loader: Object
var state: Dictionary = {}
var status_text: Label
var current_text: Label
var history_list: ItemList
var registry_list: ItemList
var evidence_text: Label
var asset_status_text: Label

func _ready() -> void:
	if not ClassDB.class_exists("FrontEndMenuObserver"):
		push_error("FrontEndMenuObserver GDExtension class missing")
		get_tree().quit(1)
		return
	observer = ClassDB.instantiate("FrontEndMenuObserver")
	if ClassDB.class_exists("Ps2TextureLoader"):
		texture_loader = ClassDB.instantiate("Ps2TextureLoader")
	if DisplayServer.get_name() != "headless":
		get_window().size = Vector2i(1380, 760)
	_build_ui()
	_reset()
	if "--frontend-menu-smoke" in OS.get_cmdline_user_args():
		_smoke()
		return
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--frontend-menu-actions="):
			for action in argument.trim_prefix("--frontend-menu-actions=").split(","):
				match action:
					"press_start": _advance_confirmed()
					"construct_mumbo": _advance_placeholder()
					"back": _go_back()
					"reset": _reset()
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--frontend-menu-capture="):
			await RenderingServer.frame_post_draw
			await RenderingServer.frame_post_draw
			var capture_path: String = argument.trim_prefix("--frontend-menu-capture=")
			var result := get_viewport().get_texture().get_image().save_png(capture_path)
			if result != OK:
				push_error("Frontend menu capture failed")
				get_tree().quit(1)
				return
			print("FRONTEND_MENU_CAPTURE_OK: ", capture_path)
			get_tree().quit(0)
			return

func _load_ps2_texture(relative_path: String, texture_name: String) -> Texture2D:
	if not texture_loader:
		return null
	var texture: Texture2D = texture_loader.load_texture(relative_path, texture_name)
	if not texture:
		push_warning("Ps2TextureLoader: %s" % texture_loader.get_last_error())
	return texture

func _button(parent: Container, text: String, action: Callable) -> void:
	var button := Button.new()
	button.text = text
	button.pressed.connect(action)
	parent.add_child(button)

func _build_ui() -> void:
	var background := ColorRect.new()
	background.color = Color(0.035, 0.052, 0.08)
	background.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	background.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(background)
	# Real PS2 frontend textures, decoded directly from the user's configured ZZDATA copy at
	# runtime by the native Ps2TextureLoader (src/platform/godot_bridge/ps2_texture_loader.cpp,
	# backed by hp2::assets::Hp2TpkArchive -- TASK-0012). No pre-extracted image files are
	# shipped; if no ZZDATA root is configured this panel is skipped and the status label below
	# explains why. This panel is decorative: the original widget/layout record format was not
	# reverse engineered, so these are NOT placed at their original positions. Never present this
	# as the reconstructed original menu layout.
	var metal_texture: Texture2D = _load_ps2_texture("FRONTEND/FRONTB.BUN", "BRUSHEDMETAL")
	if metal_texture:
		var metal := TextureRect.new()
		metal.texture = metal_texture
		metal.stretch_mode = TextureRect.STRETCH_TILE
		metal.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
		metal.mouse_filter = Control.MOUSE_FILTER_IGNORE
		metal.modulate = Color(1, 1, 1, 0.5)
		add_child(metal)
		var dim := ColorRect.new()
		dim.color = Color(0.02, 0.03, 0.05, 0.72)
		dim.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
		dim.mouse_filter = Control.MOUSE_FILTER_IGNORE
		add_child(dim)
	var margin := MarginContainer.new()
	margin.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	for side in ["left", "top", "right", "bottom"]:
		margin.add_theme_constant_override("margin_" + side, 22)
	add_child(margin)
	var layout := VBoxContainer.new()
	layout.add_theme_constant_override("separation", 12)
	margin.add_child(layout)
	var header := HBoxContainer.new()
	header.add_theme_constant_override("separation", 14)
	layout.add_child(header)
	var logo_texture: Texture2D = _load_ps2_texture("FRONTEND/FRONTB.BUN", "LOGO_NFS")
	if logo_texture:
		var logo := TextureRect.new()
		logo.texture = logo_texture
		logo.custom_minimum_size = Vector2(128, 64)
		logo.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
		logo.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST
		header.add_child(logo)
	var title := Label.new()
	title.text = "HP2 Front-End Menu — Minimal Closed Loop"
	title.add_theme_font_size_override("font_size", 26)
	header.add_child(title)
	var subtitle := Label.new()
	subtitle.text = "TASK-0011/0012 | ScreenManager structural projection | decorative real PS2 textures (LOGO_NFS, BRUSHEDMETAL); original widget layout NOT reverse engineered"
	subtitle.modulate = Color(0.64, 0.75, 0.91)
	subtitle.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	layout.add_child(subtitle)
	asset_status_text = Label.new()
	asset_status_text.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	if texture_loader:
		var root_info: Dictionary = texture_loader.resolve_root()
		if root_info.get("ok", false):
			asset_status_text.text = "PS2 asset root: %s" % root_info.get("root", "")
			asset_status_text.modulate = Color(0.55, 0.85, 0.6)
		else:
			asset_status_text.text = "PS2 asset root not configured: %s" % root_info.get("error", "")
			asset_status_text.modulate = Color(0.93, 0.6, 0.5)
	else:
		asset_status_text.text = "Ps2TextureLoader GDExtension class missing; real textures disabled."
		asset_status_text.modulate = Color(0.93, 0.6, 0.5)
	layout.add_child(asset_status_text)
	current_text = Label.new()
	current_text.add_theme_font_size_override("font_size", 20)
	layout.add_child(current_text)
	status_text = Label.new()
	layout.add_child(status_text)
	var actions := HBoxContainer.new()
	actions.add_theme_constant_override("separation", 10)
	layout.add_child(actions)
	_button(actions, "Press Start  (confirmed: TitleScreen -> New Main Menu)", func() -> void: _advance_confirmed())
	_button(actions, "Construct MumboJumboScreen  (registered sibling; edge unconfirmed)", func() -> void: _advance_placeholder())
	_button(actions, "Back", func() -> void: _go_back())
	_button(actions, "Reset", func() -> void: _reset())
	var lists := HBoxContainer.new()
	lists.add_theme_constant_override("separation", 18)
	lists.size_flags_vertical = Control.SIZE_EXPAND_FILL
	layout.add_child(lists)
	var history_box := VBoxContainer.new()
	history_box.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	lists.add_child(history_box)
	var history_label := Label.new()
	history_label.text = "History stack (back target on top; manager+0x180, capacity 12)"
	history_box.add_child(history_label)
	history_list = ItemList.new()
	history_list.size_flags_vertical = Control.SIZE_EXPAND_FILL
	history_box.add_child(history_list)
	var registry_box := VBoxContainer.new()
	registry_box.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	lists.add_child(registry_box)
	var registry_label := Label.new()
	registry_label.text = "Registered screens (name-keyed lookup; FUN_0016c8c8)"
	registry_box.add_child(registry_label)
	registry_list = ItemList.new()
	registry_list.size_flags_vertical = Control.SIZE_EXPAND_FILL
	registry_box.add_child(registry_list)
	evidence_text = Label.new()
	evidence_text.modulate = Color(0.93, 0.77, 0.49)
	evidence_text.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	layout.add_child(evidence_text)
	evidence_text.text = "Evidence: RequestScreen = FUN_0014cc78 -> FUN_0016c4c0 | FindByName = FUN_0016c8c8 (strcmp FUN_002535f8) | PushHistory = FUN_0016b418 | PopHistory = FUN_0016b438 | \"New Main Menu\" string @0x002b2f00 | TitleScreen class @FUN_0016f440 (ctor FUN_002577c0) | instance handler FUN_0016eef8 (message 1). No PCSX2/original comparison performed."

func _advance_confirmed() -> void:
	if state.get("current_screen", "") == "TitleScreen":
		state = observer.request_screen("New Main Menu", true)
	elif state.get("current_screen", "") == "New Main Menu":
		state = observer.request_screen("TitleScreen", true)
	_refresh()

func _advance_placeholder() -> void:
	state = observer.request_screen("MumboJumboScreen", true)
	_refresh()

func _go_back() -> void:
	state = observer.go_back()
	_refresh()

func _reset() -> void:
	state = observer.initialize()
	_refresh()

func _refresh() -> void:
	current_text.text = "Current screen: %s" % (state.get("current_screen", "") if state.get("has_current", false) else "(none)")
	status_text.text = "History depth: %s / %s  |  %s" % [state.get("history_depth", 0), state.get("history_capacity", 0), state.get("verification", "")]
	history_list.clear()
	for entry in observer.get_history():
		history_list.add_item(str(entry))
	registry_list.clear()
	for entry in observer.get_registered_screens():
		var label: String = str(entry)
		if label == state.get("current_screen", ""):
			label += "  <- current"
		registry_list.add_item(label)

func _expect(condition: bool, message: String) -> bool:
	if condition:
		return true
	push_error("Frontend menu smoke: " + message)
	get_tree().quit(1)
	return false

func _smoke() -> void:
	_reset()
	if not _expect(state["current_screen"] == "TitleScreen", "initial screen"):
		return
	_advance_confirmed()
	if not _expect(state["current_screen"] == "New Main Menu" and state["history_depth"] == 1, "confirmed TitleScreen -> New Main Menu edge"):
		return
	_go_back()
	if not _expect(state["current_screen"] == "TitleScreen" and state["history_depth"] == 0, "back navigation restores TitleScreen"):
		return
	_advance_placeholder()
	if not _expect(state["current_screen"] == "MumboJumboScreen" and state["history_depth"] == 1, "placeholder sibling switch"):
		return
	_reset()
	if not _expect(state["current_screen"] == "TitleScreen" and state["history_depth"] == 0, "reset returns to initial screen"):
		return
	print("FRONTEND_MENU_OBSERVER_SMOKE_OK: confirmed edge, back navigation, placeholder sibling, reset")
	get_tree().quit(0)
