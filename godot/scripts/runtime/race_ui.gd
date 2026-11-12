extends CanvasLayer
# TASK-0101 E-0101-FR01. Port-owned presentation of immutable source snapshots.
# No timers, result calculation, simulation controls or coordinator transitions.
signal pause_requested
signal resume_requested
signal return_requested

var _root: Control
var _top: PanelContainer
var _title: Label
var _objective: Label
var _metrics: Label
var _police: Label
var _pause_button: Button
var _countdown: Label
var _modal: PanelContainer
var _modal_title: Label
var _detail: Label
var _rows: VBoxContainer
var _resume: Button
var _return: Button
var _footer: Label
var _last_result_key := ""
var _objective_raw := ""

func _ready() -> void:
	layer = 20
	_root = Control.new()
	_root.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	_root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(_root)
	_top = _panel()
	_root.add_child(_top)
	var top_box := VBoxContainer.new()
	_top.add_child(top_box)
	var heading := HBoxContainer.new()
	top_box.add_child(heading)
	_title = _label("")
	_title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_title.clip_text = true
	_title.text_overrun_behavior = TextServer.OVERRUN_TRIM_ELLIPSIS
	heading.add_child(_title)
	_pause_button = _button("Pause", func(): pause_requested.emit())
	heading.add_child(_pause_button)
	_objective = _label("")
	_objective.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_objective.max_lines_visible = 2
	_objective.clip_text = true
	top_box.add_child(_objective)
	_metrics = _label("")
	_metrics.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_metrics.max_lines_visible = 2
	_metrics.clip_text = true
	top_box.add_child(_metrics)
	_police = _label("")
	_police.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_police.max_lines_visible = 2
	_police.clip_text = true
	top_box.add_child(_police)
	_countdown = _label("")
	_countdown.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_countdown.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	_countdown.set_anchors_and_offsets_preset(Control.PRESET_CENTER)
	_countdown.offset_left = -120
	_countdown.offset_right = 120
	_countdown.offset_top = -80
	_countdown.offset_bottom = 80
	_countdown.add_theme_font_size_override("font_size", 64)
	_root.add_child(_countdown)
	_modal = _panel()
	_root.add_child(_modal)
	var modal_box := VBoxContainer.new()
	_modal.add_child(modal_box)
	_modal_title = _label("")
	_modal_title.add_theme_font_size_override("font_size", 28)
	modal_box.add_child(_modal_title)
	var scroll := ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	scroll.focus_mode = Control.FOCUS_ALL
	modal_box.add_child(scroll)
	_rows = VBoxContainer.new()
	_rows.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	scroll.add_child(_rows)
	_detail = _label("")
	_detail.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_rows.add_child(_detail)
	var actions := HBoxContainer.new()
	actions.alignment = BoxContainer.ALIGNMENT_END
	modal_box.add_child(actions)
	_resume = _button("Resume", func(): resume_requested.emit())
	actions.add_child(_resume)
	_return = _button("Return to Event Tree", func(): return_requested.emit())
	actions.add_child(_return)
	_footer = _label("")
	_footer.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_footer.max_lines_visible = 3
	_footer.clip_text = true
	_footer.set_anchors_and_offsets_preset(Control.PRESET_BOTTOM_WIDE)
	_footer.offset_left = 16
	_footer.offset_right = -16
	_footer.offset_top = -64
	_footer.offset_bottom = -12
	_root.add_child(_footer)
	_root.resized.connect(_layout)
	_layout()
	_modal.hide()
	_top.hide()

# Call only after adding this node to the tree. Input is a committed snapshot;
# missing optional fields remain absent. Host owns keyboard shortcuts/signals.
func present(snapshot: Dictionary) -> void:
	var phase := str(snapshot.get("status", snapshot.get("phase", "")))
	var source_save_failed := bool(snapshot.get("source_save_failed", false))
	_root.visible = not bool(snapshot.get("source_ui_enabled", false)) or phase in ["Blocked", "Failed"] or source_save_failed
	if not _root.visible: return
	if source_save_failed:
		_top.hide()
		_countdown.hide()
		_modal.show()
		_modal_title.text = "Save failed"
		_detail.text = str(snapshot.get("diagnostic", "Private profile save failed."))
		_resume.text = "Retry save"
		_resume.show()
		_return.hide()
		_footer.text = "Enter: retry private profile save"
		return
	_resume.text = "Resume"
	_return.show()
	var rules: Dictionary = snapshot.get("rule_view", {})
	var hud: Dictionary = rules.get("hud", {})
	var result: Dictionary = snapshot.get("event_result", {})
	var is_results := phase == "Results" and bool(result.get("ready", false))
	var paused := phase in ["Paused", "ReadyPaused"]
	var stopped := phase in ["Blocked", "Failed"]
	_top.visible = not is_results
	_title.text = str(rules.get("event_name", snapshot.get("event_name", "Race")))
	var event_types := {0: "Race", 1: "Time trial", 2: "Lap knockout", 3: "Race knockout", 4: "Tournament", 5: "Delivery", 6: "Be a cop"}
	if rules.has("event_type"):
		_title.text += " · " + str(event_types.get(int(rules.event_type), "Event %s" % rules.event_type))
	if rules.has("objective_text"):
		_objective_raw = str(rules.objective_text)
	_objective.text = _presentation_text(_objective_raw)
	_objective.visible = not _objective.text.is_empty()
	var metrics: PackedStringArray = []
	if snapshot.has("speed_kmh"):
		metrics.append("Speed  %.0f km/h" % float(snapshot.speed_kmh))
	if hud.has("clock_text"):
		metrics.append("Time  " + str(hud.clock_text))
	if hud.has("rank") and int(hud.get("participant_count", 0)) > 1 and int(rules.get("event_type", -1)) != 6:
		metrics.append("Position  %s / %s" % [hud.rank, hud.participant_count])
	if bool(hud.get("point_to_point", false)) and hud.has("progress_percent"):
		metrics.append("Progress  %.1f%%" % float(hud.progress_percent))
	elif hud.has("display_lap") and hud.has("lap_limit"):
		metrics.append("Lap  %s / %s" % [hud.display_lap, hud.lap_limit])
	if hud.has("score"):
		metrics.append("Score  %s" % hud.score)
	if rules.has("stage_index") and rules.has("stage_count"):
		metrics.append("Stages completed  %s / %s" % [rules.stage_index, rules.stage_count])
	_metrics.text = "    ·    ".join(metrics)
	# Container height must shrink again after a longer previous snapshot.
	_top.set_deferred("size", Vector2(_top.size.x, 0))
	_metrics.modulate = Color(1.0, 0.65, 0.55) if bool(hud.get("clock_overrun", false)) else Color.WHITE
	_police.text = _police_text(rules.get("police", {}))
	_police.visible = not _police.text.is_empty()
	_pause_button.disabled = phase != "Running"
	var number := int(hud.get("countdown_number", 0))
	_countdown.visible = phase == "Running" and number in [1, 2, 3, 4] and int(snapshot.get("source_ui_screen_count", 0)) == 0
	_countdown.text = "GO" if number == 4 else str(number)
	var was_visible := _modal.visible
	_modal.visible = paused or stopped or is_results
	_resume.visible = paused
	_modal_title.text = "Results" if is_results else ("Paused" if paused else "Simulation stopped")
	var footer: PackedStringArray = []
	if not phase.is_empty():
		footer.append("Finished · Waiting for results" if phase == "Running" and bool(snapshot.get("player_finished", false)) else phase)
	var controls := str(snapshot.get("controls_text", "Tab focus · Enter / Space activate"))
	if not controls.is_empty():
		footer.append(controls)
	if not stopped and not str(snapshot.get("diagnostic", "")).is_empty():
		footer.append(str(snapshot.diagnostic))
	_footer.text = "\n".join(footer)
	if is_results:
		var result_key := JSON.stringify(result)
		if result_key != _last_result_key:
			_last_result_key = result_key
			_show_results(result, rules)
	else:
		_last_result_key = ""
		_clear_result_rows()
		_detail.text = str(snapshot.get("diagnostic", "")) if stopped else "Resume to continue the current race."
		if paused and not _objective_raw.is_empty():
			_detail.text += "\n\n" + _presentation_text(_objective_raw)
	if _modal.visible and not was_visible:
		if paused:
			_resume.grab_focus()
		else:
			_return.grab_focus()

func _show_results(result: Dictionary, rules: Dictionary) -> void:
	_clear_result_rows()
	var statuses := {0: "Locked", 1: "Untried", 2: "In progress", 3: "Failed", 4: "Passed"}
	var lines: PackedStringArray = [str(result.get("event_name", rules.get("event_name", "")))]
	var aborted := bool(result.get("aborted", false))
	if aborted:
		lines.append("Aborted")
	# E-0101-RA03: PostResults leaves the old state intact when aborted.
	# A retained PASSED state must not describe the interrupted attempt.
	if not aborted and result.has("event_status"):
		lines.append(str(statuses.get(int(result.event_status), "Status %s" % result.event_status)))
	if result.has("player_points"):
		lines.append("Player points  %s" % result.player_points)
	_detail.text = "\n".join(lines)
	var reasons := {0: "Not finished", 1: "Finished", 2: "Arrested", 3: "Knocked out", 4: "Police won", 5: "Police lost"}
	for value in result.get("rows", []):
		if not value is Dictionary:
			continue
		var row: Dictionary = value
		var fields: PackedStringArray = []
		if row.has("rank"):
			fields.append("Position %s" % row.rank)
		if row.has("driver_number"):
			fields.append("Driver %s" % row.driver_number)
		if row.has("time"):
			fields.append(str(row.time))
		if row.has("reason"):
			fields.append(str(reasons.get(int(row.reason), "Reason %s" % row.reason)))
		for key in ["score", "points", "bonus", "tickets"]:
			if row.has(key):
				fields.append("%s %s" % [key.capitalize(), row[key]])
		var line := _label("    ·    ".join(fields))
		line.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		_rows.add_child(line)
	for value in rules.get("standings", []):
		if not value is Dictionary:
			continue
		var standing: Dictionary = value
		var fields: PackedStringArray = []
		for key in ["driver_number", "standing_rank", "points", "recorded_rank"]:
			if standing.has(key):
				fields.append("%s %s" % [key.replace("_", " ").capitalize(), standing[key]])
		var line := _label("    ·    ".join(fields))
		line.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		_rows.add_child(line)

func _police_text(police: Dictionary) -> String:
	var fields: PackedStringArray = []
	if bool(police.get("being_pursued", false)) and police.has("hud_level"):
		fields.append("Pursuit level  %s" % police.hud_level)
	if police.has("tickets") and police.has("ticket_limit"):
		fields.append("Tickets  %s / %s" % [police.tickets, police.ticket_limit])
	if police.has("be_a_cop_seconds"):
		fields.append("Police time  %.1fs" % float(police.be_a_cop_seconds))
	if police.has("arrests") and police.has("arrest_quota"):
		fields.append("Arrests  %s / %s" % [police.arrests, police.arrest_quota])
	return "    ·    ".join(fields)

func _clear_result_rows() -> void:
	for child in _rows.get_children():
		if child != _detail:
			_rows.remove_child(child)
			child.queue_free()

func _layout() -> void:
	var extent := _root.size
	# A Theme propagates through children; a Control font override does not.
	if _root.theme == null:
		_root.theme = Theme.new()
	var font_size := int(clampf(extent.y / 42.0, 12.0, 20.0))
	_root.theme.default_font_size = font_size
	# Explicit type entries also replace an inherited project theme's type size.
	_root.theme.set_font_size("font_size", "Label", font_size)
	_root.theme.set_font_size("font_size", "Button", font_size)
	# Clipped Labels otherwise report a one-pixel minimum to VBoxContainer.
	for label in [_objective, _metrics, _police]:
		label.custom_minimum_size.y = ceilf(font_size * 2.6)
	_countdown.add_theme_font_size_override("font_size", int(clampf(extent.y * 0.08, 28.0, 64.0)))
	_modal_title.add_theme_font_size_override("font_size", int(clampf(extent.y / 32.0, 18.0, 28.0)))
	var gutter := clampf(extent.x * 0.025, 12.0, 32.0)
	_top.position = Vector2(gutter, gutter)
	_top.size = Vector2(maxf(240.0, extent.x - 2.0 * gutter), 0)
	var modal_size := Vector2(minf(900.0, extent.x - 2.0 * gutter), minf(600.0, extent.y - 2.0 * gutter))
	_modal.position = (extent - modal_size) * 0.5
	_modal.size = modal_size

# Port-only readability convention, not an implementation of the original
# localization/control-code parser. The committed snapshot stays unmodified.
func _presentation_text(raw: String) -> String:
	return raw.replace("@", "\n")

func _panel() -> PanelContainer:
	var panel := PanelContainer.new()
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.025, 0.04, 0.065, 0.90)
	style.set_corner_radius_all(10)
	style.set_content_margin_all(16)
	panel.add_theme_stylebox_override("panel", style)
	return panel

func _label(text: String) -> Label:
	var label := Label.new()
	label.set_meta("port_viewport_font_size", true)
	label.text = text
	label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	return label

func _button(text: String, action: Callable) -> Button:
	var button := Button.new()
	button.set_meta("port_viewport_font_size", true)
	button.text = text
	button.custom_minimum_size = Vector2(112, 44)
	button.focus_mode = Control.FOCUS_ALL
	button.pressed.connect(action)
	return button
