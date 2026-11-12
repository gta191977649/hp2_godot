extends PanelContainer
# Port settings, explicitly separate from the original PS2 controller configuration.
signal closed
var bindings: RefCounted
var message: Label
var capture := ""
var capture_device := "keyboard"
var pending: InputEvent
var rows: Dictionary = {}
var conflict_dialog: ConfirmationDialog

func initialize(input: RefCounted) -> void:
	bindings = input
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	var background := StyleBoxFlat.new()
	background.bg_color = Color(0.015,0.02,0.03,0.98)
	add_theme_stylebox_override("panel",background)
	var margin := MarginContainer.new()
	margin.add_theme_constant_override("margin_left", 24)
	margin.add_theme_constant_override("margin_right", 24)
	margin.add_theme_constant_override("margin_top", 20)
	margin.add_theme_constant_override("margin_bottom", 20)
	add_child(margin)
	var column := VBoxContainer.new()
	margin.add_child(column)
	var title := Label.new()
	title.text = "PC key bindings"
	column.add_child(title)
	message = Label.new()
	message.text = bindings.warning if not bindings.warning.is_empty() else "Choose a binding. Escape cancels capture."
	message.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	column.add_child(message)
	var scroll := ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	column.add_child(scroll)
	var table := VBoxContainer.new()
	table.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll.add_child(table)
	for name in bindings.EVENTS:
		var row := HBoxContainer.new()
		table.add_child(row)
		var label := Label.new()
		label.text = str(name).capitalize()
		label.custom_minimum_size.x = 130
		row.add_child(label)
		var buttons: Array[Button] = []
		for device in ["keyboard", "gamepad"]:
			var button := Button.new()
			button.size_flags_horizontal = Control.SIZE_EXPAND_FILL
			button.clip_text = true
			button.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
			button.pressed.connect(_begin_capture.bind(str(name), device))
			row.add_child(button)
			buttons.append(button)
		var restore := Button.new()
		restore.text = "Reset"
		restore.pressed.connect(func():
			var result: Error = bindings.restore(str(name))
			message.text = "Defaults restored" if result == OK else "Could not save bindings"
			_refresh())
		row.add_child(restore)
		rows[name] = buttons
	var footer := HFlowContainer.new()
	column.add_child(footer)
	var restore_all := Button.new()
	restore_all.text = "Restore all defaults"
	restore_all.pressed.connect(func():
		capture = ""
		var result: Error = bindings.restore()
		message.text = "Defaults restored" if result == OK else "Could not save bindings"
		_refresh())
	footer.add_child(restore_all)
	var cancel := Button.new()
	cancel.text = "Cancel capture"
	cancel.pressed.connect(func(): capture = ""; message.text = "Capture cancelled")
	footer.add_child(cancel)
	var close := Button.new()
	close.text = "Back"
	close.pressed.connect(func(): capture = ""; closed.emit())
	footer.add_child(close)
	conflict_dialog = ConfirmationDialog.new()
	conflict_dialog.title = "Replace conflicting binding?"
	conflict_dialog.confirmed.connect(func(): _commit(true))
	add_child(conflict_dialog)
	_refresh()

func _refresh() -> void:
	var previous: String = bindings.last_device
	for name in rows:
		bindings.last_device = "keyboard"
		rows[name][0].text = bindings.hint(name,true)
		bindings.last_device = "gamepad"
		rows[name][1].text = bindings.hint(name,true)
		for button in rows[name]: button.tooltip_text = button.text
	bindings.last_device = previous

func _begin_capture(name: String, device: String) -> void:
	capture = name
	capture_device = device
	message.text = "Press %s input for %s; Escape cancels." % [device, name]

func handle(event: InputEvent) -> bool:
	if not visible: return false
	if event is InputEventKey and event.pressed and event.keycode == KEY_ESCAPE:
		if not capture.is_empty(): capture = ""; message.text = "Capture cancelled"
		elif not conflict_dialog.visible: closed.emit()
		return true
	if capture.is_empty() or conflict_dialog.visible: return event is InputEventKey or event is InputEventJoypadButton or event is InputEventJoypadMotion
	if event is InputEventKey:
		if not event.pressed or event.echo or capture_device != "keyboard": return true
	elif event is InputEventJoypadButton:
		if not event.pressed or capture_device != "gamepad": return true
	elif event is InputEventJoypadMotion:
		if absf(event.axis_value) < 0.8 or capture_device != "gamepad": return true
	else: return false
	pending = event.duplicate()
	_commit(false)
	return true

func _commit(replace: bool) -> void:
	var result: Dictionary = bindings.rebind(capture, pending, replace)
	if result.has("conflicts"):
		conflict_dialog.dialog_text = "Assigned to %s. Remove that assignment and use it for %s?" % [", ".join(result.conflicts), capture]
		conflict_dialog.popup_centered()
		return
	message.text = "Binding saved" if result.ok else str(result.error)
	capture = ""
	_refresh()
