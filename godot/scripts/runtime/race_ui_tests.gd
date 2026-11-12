extends SceneTree
# TASK-0101: synthetic port-presentation tests, never gameplay acceptance.
# Run: Godot --headless --path godot --script res://scripts/runtime/race_ui_tests.gd
const UI := preload("res://scripts/runtime/race_ui.gd")
var failures := 0
var pauses := 0
var resumes := 0
var returns := 0

func _initialize() -> void:
	call_deferred("_run")

func _check(condition: bool, message: String) -> void:
	if not condition:
		failures += 1
		push_error("race_ui fixture: " + message)

func _run() -> void:
	var ui := UI.new()
	root.add_child(ui)
	ui.pause_requested.connect(func(): pauses += 1)
	ui.resume_requested.connect(func(): resumes += 1)
	ui.return_requested.connect(func(): returns += 1)
	await process_frame
	for event_type in range(7):
		var rules := {"event_type": event_type, "event_name": "Fixture event", "objective_text": "Fixture goal", "hud": {"clock_text": "00:12.50", "rank": 2, "participant_count": 5, "display_lap": 2, "lap_limit": 3, "score": 2000, "countdown_number": 4, "point_to_point": event_type == 5, "progress_percent": 41.25}}
		if event_type == 6:
			rules["police"] = {"being_pursued": true, "hud_level": 3, "tickets": 1, "ticket_limit": 3, "be_a_cop_seconds": 12.5, "arrests": 1, "arrest_quota": 4}
		ui.present({"status": "Running", "rule_view": rules})
		_check(ui._top.visible, "HUD visible for event %d" % event_type)
		_check(ui._metrics.text.contains("00:12.50"), "native clock preserved")
		_check(ui._metrics.text.contains("Position") == (event_type != 6), "rank applies to racing, not police quota")
		_check(ui._metrics.text.contains("Progress") == (event_type == 5), "delivery progress visibility")
		_check(ui._metrics.text.contains("Lap") == (event_type != 5), "circuit lap visibility")
		_check(ui._police.visible == (event_type == 6), "optional police presence")
		_check(ui._countdown.text == "GO", "source countdown 4 means GO")
		_check(not ui._modal.visible, "no running modal")
		ui._pause_button.pressed.emit()
	_check(pauses == 7, "pause button requests only")
	ui.present({"status": "Running", "rule_view": {"event_type": 5, "hud": {"rank": 1, "participant_count": 1, "point_to_point": true, "progress_percent": 20.0}}})
	_check(not ui._metrics.text.contains("Position"), "single-car delivery hides meaningless rank")
	ui.present({"status": "Running", "speed_kmh": 123.4, "player_finished": true, "source_ui_screen_count": 1, "controls_text": "Fixture controls", "rule_view": {"hud": {"countdown_number": 4}}})
	_check(ui._metrics.text.contains("123 km/h"), "optional source speed")
	_check(ui._footer.text.contains("Waiting for results") and ui._footer.text.contains("Fixture controls"), "explicit finish state and host controls")
	_check(not ui._countdown.visible, "original source presentation suppresses duplicate countdown")
	ui.present({"status": "Paused"})
	_check(ui._resume.has_focus(), "pause keyboard focus")
	var enter := InputEventAction.new()
	enter.action = "ui_accept"
	enter.pressed = true
	root.push_input(enter)
	await process_frame
	enter = InputEventAction.new()
	enter.action = "ui_accept"
	enter.pressed = false
	root.push_input(enter)
	await process_frame
	_check(resumes == 1, "keyboard resume emits request")
	# Long localized source-style objective, including explicit @ markers.
	# Synthetic prose avoids redistributing extracted campaign strings.
	var long_objective := "Deliver the car to the finish as quickly as possible.@@Avoid traffic and police, keep control through the corners, and reach the destination before the target time.@@This full objective remains available while paused."
	root.size = Vector2i(640, 480)
	await process_frame
	ui.present({"status": "Running", "controls_text": "W/S throttle/brake · A/D steer · P pause · Esc return", "rule_view": {"event_name": "A very long source event title for a narrow viewport", "objective_text": long_objective, "hud": {"clock_text": "00:12.50", "rank": 2, "participant_count": 5, "point_to_point": true, "progress_percent": 41.25, "score": 2000}}})
	await process_frame
	await process_frame
	_check(ui._objective.max_lines_visible == 2 and ui._objective.clip_text, "objective bounded to two HUD lines")
	_check(not ui._objective.text.contains("@") and ui._objective_raw == long_objective, "presentation markers converted without mutating source text")
	_check(ui._objective.get_theme_font_size("font_size") <= 20, "responsive font actually inherited (actual%d root%s)" % [ui._objective.get_theme_font_size("font_size"), ui._root.size])
	_check(ui._top.get_rect().end.y < 480 * 0.45, "long objective HUD leaves the world visible at640x480")
	_check(ui._objective.size.y >= 24 and ui._metrics.size.y >= 24, "objective and metrics have visible text height (%s / %s)" % [ui._objective.size, ui._metrics.size])
	_check(ui._top.get_global_rect().encloses(ui._metrics.get_global_rect()), "metrics remain inside HUD panel")
	_check(ui._footer.get_rect().end.y <= 480 and ui._footer.get_rect().position.y > ui._top.get_rect().end.y, "footer remains visible and clear of HUD")
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--ui-screenshot="):
			await create_timer(0.15).timeout
			await RenderingServer.frame_post_draw
			var path := argument.trim_prefix("--ui-screenshot=")
			var image := root.get_texture().get_image()
			_check(not image.is_empty() and image.save_png(path) == OK, "optional640x480 screenshot saved")
	ui.present({"status": "Paused"})
	_check(ui._detail.text.contains(ui._presentation_text(long_objective)), "full source objective available in pause scroll")
	var rows: Array = []
	for index in range(32):
		rows.append({"rank": index + 1, "driver_number": index, "time": "00:12.50", "reason": 1, "score": 2000, "points": 5})
	ui.present({"status": "Running"})
	ui.present({"status": "Results", "event_result": {"ready": true, "event_name": "Fixture result", "event_status": 4, "rows": rows}})
	_check(ui._return.has_focus(), "results keyboard focus")
	_check(ui._rows.get_child_count() == 33, "all settlement rows retained")
	_check(ui._detail.text.contains("Passed"), "source status shown")
	for extent in [Vector2i(640, 480), Vector2i(1024, 768), Vector2i(1280, 720), Vector2i(1680, 720)]:
		root.size = extent
		await process_frame
		await process_frame
		var bounds := Rect2(Vector2.ZERO, ui._root.size)
		_check(bounds.encloses(ui._modal.get_rect()), "modal bounds %s" % extent)
		var scroll := ui._rows.get_parent() as ScrollContainer
		_check(scroll.get_v_scroll_bar().max_value > scroll.size.y, "results scroll %s" % extent)
	# Mouse activation through the viewport, using the visible action's center.
	var center := ui._return.get_global_rect().get_center()
	var click := InputEventMouseButton.new()
	click.button_index = MOUSE_BUTTON_LEFT
	click.position = center
	click.pressed = true
	root.push_input(click)
	click = InputEventMouseButton.new()
	click.button_index = MOUSE_BUTTON_LEFT
	click.position = center
	click.pressed = false
	root.push_input(click)
	await process_frame
	_check(returns == 1, "mouse return emits request")
	ui.present({"status": "Blocked", "diagnostic": "Fixture source guard 0x00123456"})
	_check(ui._detail.text.contains("0x00123456"), "source diagnostic preserved")
	_check(not ui._resume.visible, "blocked state cannot resume")
	ui.present({"status": "Running", "rule_view": {"hud": {}}})
	_check(not ui._police.visible and ui._metrics.text.is_empty(), "missing metrics remain absent")
	ui.present({"status": "LoadingActors", "event_name": "Fixture loading"})
	_check(ui._top.visible and ui._title.text == "Fixture loading" and ui._footer.text.contains("LoadingActors"), "loading title and status remain visible")
	print("race_ui port presentation fixture: %d failure(s); no original gameplay equivalence claim" % failures)
	quit(0 if failures == 0 else 1)
