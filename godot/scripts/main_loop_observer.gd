extends Control

var observer: Object
var running := false
var state: Dictionary = {}
var scenario: OptionButton
var strict: CheckBox
var alternate: CheckBox
var auto_boundary: CheckBox
var show_skipped: CheckBox
var status_text: Label
var timing_text: Label
var identity_text: Label
var frame_tree: Tree
var startup_tree: Tree
var world_tree: Tree
var tabs: TabContainer

func _ready() -> void:
	if not ClassDB.class_exists("MainLoopObserver"):
		push_error("MainLoopObserver GDExtension class missing")
		get_tree().quit(1)
		return
	observer = ClassDB.instantiate("MainLoopObserver")
	if DisplayServer.get_name() != "headless":
		get_window().size = Vector2i(1380, 900)
	_build_ui()
	_reset()
	if "--observer-smoke" in OS.get_cmdline_user_args():
		_smoke()
		return
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--observer-capture="):
			_advance(0.016, true)
			await RenderingServer.frame_post_draw
			await RenderingServer.frame_post_draw
			var capture_path: String = argument.trim_prefix("--observer-capture=")
			var result := get_viewport().get_texture().get_image().save_png(capture_path)
			if result != OK:
				push_error("Observer capture failed")
				get_tree().quit(1)
				return
			print("OBSERVER_CAPTURE_OK: ", capture_path)
			get_tree().quit(0)
			return

func _button(parent: Container, text: String, action: Callable) -> void:
	var button := Button.new()
	button.text = text
	button.pressed.connect(action)
	parent.add_child(button)

func _tree(parent: Node, title: String, headings: Array[String], widths: Array[int]) -> Tree:
	var tree := Tree.new()
	tree.name = title
	tree.columns = headings.size()
	tree.column_titles_visible = true
	tree.hide_root = true
	tree.size_flags_vertical = Control.SIZE_EXPAND_FILL
	tree.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	for index in headings.size():
		tree.set_column_title(index, headings[index])
		tree.set_column_custom_minimum_width(index, widths[index])
		tree.set_column_expand(index, index == headings.size() - 1)
	parent.add_child(tree)
	return tree

func _build_ui() -> void:
	var background := ColorRect.new()
	background.color = Color(0.035, 0.052, 0.08)
	background.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	background.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(background)
	var margin := MarginContainer.new()
	margin.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	for side in ["left", "top", "right", "bottom"]:
		margin.add_theme_constant_override("margin_" + side, 22)
	add_child(margin)
	var layout := VBoxContainer.new()
	layout.add_theme_constant_override("separation", 12)
	margin.add_child(layout)
	var title := Label.new()
	title.text = "HP2 Main Loop Observer"
	title.add_theme_font_size_override("font_size", 28)
	layout.add_child(title)
	var subtitle := Label.new()
	subtitle.text = "TASK-0010  |  61 call sites / 39 targets  |  Static structure + synthetic fixtures  |  Original behavior unverified"
	subtitle.modulate = Color(0.64, 0.75, 0.91)
	layout.add_child(subtitle)
	var options := HBoxContainer.new()
	layout.add_child(options)
	scenario = OptionButton.new()
	for label in ["World update", "No world object", "World gate active", "Record/replay field branch", "State predicate override", "Compare + optional callback suppressed"]:
		scenario.add_item(label)
	scenario.item_selected.connect(func(_index: int) -> void: _reset())
	options.add_child(scenario)
	strict = CheckBox.new()
	strict.text = "Strict (no handlers supplied)"
	strict.toggled.connect(func(_enabled: bool) -> void: _reset())
	options.add_child(strict)
	alternate = CheckBox.new()
	alternate.text = "Alternate bootstrap"
	alternate.toggled.connect(func(_enabled: bool) -> void: _reset())
	options.add_child(alternate)
	var actions := HBoxContainer.new()
	layout.add_child(actions)
	_button(actions, "Start", _start)
	_button(actions, "Pause", func() -> void: running = false; _refresh())
	_button(actions, "Single frame / resume", _single_step)
	_button(actions, "Reset", _reset)
	_button(actions, "Send boundary", _send_boundary)
	_button(actions, "Stop", func() -> void: running = false; state = observer.shutdown(); _refresh())
	auto_boundary = CheckBox.new()
	auto_boundary.text = "Synthetic boundary per host step"
	auto_boundary.button_pressed = true
	actions.add_child(auto_boundary)
	show_skipped = CheckBox.new()
	show_skipped.text = "Show skipped slots"
	show_skipped.toggled.connect(func(_enabled: bool) -> void: _refresh())
	actions.add_child(show_skipped)
	status_text = Label.new()
	status_text.add_theme_font_size_override("font_size", 18)
	layout.add_child(status_text)
	timing_text = Label.new()
	layout.add_child(timing_text)
	tabs = TabContainer.new()
	tabs.size_flags_vertical = Control.SIZE_EXPAND_FILL
	layout.add_child(tabs)
	var headings: Array[String] = ["Frame", "Phase", "Module", "Call PC", "Target", "Event", "Evidence / note"]
	var widths: Array[int] = [54, 140, 158, 108, 108, 180, 350]
	frame_tree = _tree(tabs, "Frame trace", headings, widths)
	startup_tree = _tree(tabs, "Startup", headings, widths)
	world_tree = _tree(tabs, "World connections", ["Slot", "Call PC", "Target", "Confidence", "Observed connection"], [185, 110, 110, 90, 650])
	identity_text = Label.new()
	identity_text.modulate = Color(0.64, 0.75, 0.91)
	identity_text.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	layout.add_child(identity_text)
	var footer := Label.new()
	footer.text = "Yellow rows are synthetic placeholders. No vehicle, physics or renderer body runs here. Host delta never replaces the game-step fixture."
	footer.modulate = Color(0.93, 0.77, 0.49)
	footer.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	layout.add_child(footer)

func _reset() -> void:
	running = false
	state = observer.initialize(scenario.selected, strict.button_pressed, alternate.button_pressed)
	_refresh()

func _start() -> void:
	if state.get("status", "Stopped") in ["Stopped", "Error", "Unimplemented"]:
		_reset()
	running = state.get("status", "") in ["Ready", "Completed", "WaitingForFrameBoundary"]
	_refresh()

func _single_step() -> void:
	running = false
	if state.get("status", "") == "Stopped":
		_reset()
	_advance(0.0, auto_boundary.button_pressed)

func _send_boundary() -> void:
	if state.get("status", "") == "WaitingForFrameBoundary":
		_advance(0.0, true)

func _process(delta: float) -> void:
	if running and observer != null:
		_advance(delta, auto_boundary.button_pressed)

func _advance(delta: float, boundary: bool) -> void:
	state = observer.step_frame(delta, boundary)
	if state.get("status", "") in ["Error", "Unimplemented", "Stopped"]:
		running = false
	_refresh()

func _fill_trace(tree: Tree, trace: Array) -> void:
	tree.clear()
	var root := tree.create_item()
	for event: Dictionary in trace:
		if event["event"] == "Skipped" and not show_skipped.button_pressed:
			continue
		var row := tree.create_item(root)
		var target_label: String = "-" if event["target"] == "0x00000000" else str(event["target"])
		var fields: Array = [event["frame"], event["phase"], event["module"], event["pc"], target_label, event["event"], event["evidence"] + " | " + event["note"]]
		var color := Color(0.83, 0.88, 0.96)
		if event["event"] == "SyntheticPlaceholder":
			color = Color(0.96, 0.81, 0.52)
		elif event["event"] in ["Unimplemented", "Error"]:
			color = Color(1.0, 0.47, 0.46)
		elif event["event"] in ["Skipped", "Branch"]:
			color = Color(0.58, 0.67, 0.80)
		elif event["event"] in ["Waiting", "Boundary"]:
			color = Color(0.50, 0.87, 0.86)
		for index in fields.size():
			row.set_text(index, str(fields[index]))
			row.set_custom_color(index, color)
			row.set_tooltip_text(index, str(fields[index]))

func _number(value: Variant) -> String:
	if value == null:
		return "unknown"
	return "%.6f" % float(value)

func _refresh() -> void:
	if observer == null:
		return
	status_text.text = "%s  |  %s  |  Completed frames: %s  |  %s @ %s  |  Unresolved slots: %s" % ["RUNNING" if running else "PAUSED", state.get("status"), state.get("completed_frames"), state.get("phase"), state.get("pc"), state.get("unresolved_count")]
	if not str(state.get("error", "")).is_empty():
		status_text.text += "\n" + str(state["error"])
	timing_text.text = "Host delta: %s s  |  Base-step fixture: %s  |  Effective step: %s  |  Scaled elapsed: %s  |  Boundary: %s / snapshot %s" % [_number(state.get("host_delta_seconds")), _number(state.get("base_step")), _number(state.get("effective_step")), _number(state.get("scaled_elapsed")), state.get("boundary_counter"), state.get("boundary_snapshot")]
	_fill_trace(frame_tree, observer.get_trace(false))
	_fill_trace(startup_tree, observer.get_trace(true))
	world_tree.clear()
	var root := world_tree.create_item()
	for connection: Dictionary in observer.get_world_connections():
		var row := world_tree.create_item(root)
		var values: Array = [connection["slot"], connection["pc"], connection["target"], connection["confidence"], connection["contract"]]
		for index in values.size():
			row.set_text(index, str(values[index]))
			row.set_tooltip_text(index, str(values[index]))
	identity_text.text = "ELF SHA-256: %s  |  Revision %s\nWorld connections are nested metadata; virtual targets and vehicle partitions remain unresolved. Runtime and physical behavior are not verified." % [state.get("elf_sha256", "unknown"), state.get("executable_revision", "unknown")]

func _call_pcs() -> Array[String]:
	var pcs: Array[String] = []
	for event: Dictionary in observer.get_trace(false):
		if event["event"] in ["Executed", "SyntheticPlaceholder"]:
			pcs.append(event["pc"])
	return pcs

func _expect(condition: bool, message: String) -> bool:
	if condition:
		return true
	push_error("Observer smoke: " + message)
	get_tree().quit(1)
	return false

func _smoke() -> void:
	for index in 6:
		scenario.select(index)
		_reset()
		if not _expect(state["status"] == "Ready", "initialize"):
			return
		_advance(0.375, true)
		if not _expect(state["status"] == "Completed" and state["completed_frames"] == 1, "frame completion"):
			return
		var pcs := _call_pcs()
		if not _expect(("0x0012F538" in pcs) == (index in [0, 5]), "world branch"):
			return
		if not _expect(pcs.count("0x0012F3C0") == 1 and pcs.count("0x0012F3F4") == 1, "repeated state target"):
			return
		if not _expect(is_equal_approx(float(state["base_step"]), 0.02), "host delta altered base step"):
			return
	scenario.select(0)
	_reset()
	auto_boundary.button_pressed = false
	_single_step()
	if not _expect(state["status"] == "WaitingForFrameBoundary", "wait"):
		return
	var before: Array = observer.get_trace(false)
	_single_step()
	if not _expect(observer.get_trace(false) == before, "waiting repeated work"):
		return
	_send_boundary()
	if not _expect(state["completed_frames"] == 1 and _call_pcs().back() == "0x0012F674", "resume tail"):
		return
	alternate.button_pressed = true
	_reset()
	var bootstrap_pcs: Array[String] = []
	for event: Dictionary in observer.get_trace(true):
		if event["event"] in ["Executed", "SyntheticPlaceholder"]:
			bootstrap_pcs.append(event["pc"])
	if not _expect("0x0012F034" in bootstrap_pcs and not "0x0012F048" in bootstrap_pcs, "alternate startup"):
		return
	strict.button_pressed = true
	_reset()
	if not _expect(state["status"] == "Unimplemented" and state["pc"] == "0x0012F014", "strict exact missing call"):
		return
	if not _expect(observer.get_world_connections().size() == 19, "connection table"):
		return
	strict.button_pressed = false
	alternate.button_pressed = false
	auto_boundary.button_pressed = true
	_reset()
	_start()
	if not _expect(running, "start"):
		return
	running = false
	var count: int = state["completed_frames"]
	_process(0.1)
	if not _expect(state["completed_frames"] == count, "pause"):
		return
	print("MAIN_LOOP_OBSERVER_SMOKE_OK: six scenarios, wait/resume, startup, strict, pause, nested slots")
	get_tree().quit(0)
