extends Node

var core
var position_value := 0
var accumulator := 0.0
var history: Array = []
var build_id := "unknown"
var feedback_key := KEY_F9
# Synthetic scheduling only; original timing remains unknown.
const STEP := 1.0 / 60.0

func _ready() -> void:
	if not ClassDB.class_exists("PortCore"):
		push_error("PortCore missing")
		get_tree().quit(1)
		return
	core = ClassDB.instantiate("PortCore")
	if FileAccess.file_exists("res://build_info.json"):
		var info = JSON.parse_string(FileAccess.get_file_as_string("res://build_info.json"))
		build_id = str(info.get("id", "unknown"))
	feedback_key = int(ProjectSettings.get_setting("port/feedback_key", KEY_F9))
	if core.scaffold_add(20, 22) != 42:
		get_tree().quit(1)
		return
	if "--smoke-test" in OS.get_cmdline_user_args():
		for axis in [-1, 0, 1, 1]:
			advance(axis)
		if position_value != 1 or core.frame() != 4:
			get_tree().quit(1)
			return
		print("PORT_SMOKE_OK: 42")
		get_tree().quit(0)
	elif "--feedback-test" in OS.get_cmdline_user_args():
		var right := InputEventKey.new()
		right.physical_keycode = KEY_RIGHT
		right.pressed = true
		Input.parse_input_event(right)
		await get_tree().create_timer(0.15).timeout
		right.pressed = false
		Input.parse_input_event(right)
		if position_value <= 0:
			push_error("Keyboard input did not advance native state")
			get_tree().quit(1)
			return
		var event := InputEventKey.new()
		event.keycode = feedback_key
		event.pressed = true
		Input.parse_input_event(event)
		await get_tree().process_frame
		await get_tree().process_frame
		get_tree().quit(0)

func advance(axis: int) -> void:
	position_value = core.tick(axis)
	history.append({"frame": core.frame(), "axis": axis})
	if history.size() > 300:
		history.pop_front()

func _process(delta: float) -> void:
	if core == null:
		return
	accumulator += delta
	var axis := int(Input.is_physical_key_pressed(KEY_RIGHT)) - int(Input.is_physical_key_pressed(KEY_LEFT))
	var pads := Input.get_connected_joypads()
	if not pads.is_empty():
		var stick := Input.get_joy_axis(pads[0], JOY_AXIS_LEFT_X)
		if abs(stick) > 0.25:
			axis = int(sign(stick))
	var budget := 8
	while accumulator >= STEP and budget > 0:
		advance(axis)
		accumulator -= STEP
		budget -= 1
	$Result.text = "Synthetic native demo | Build %s\nFrame %s | Position %s\nLeft/right arrows or gamepad; F9 feedback" % [build_id, core.frame(), position_value]

func _unhandled_key_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and not event.echo and event.keycode == feedback_key:
		capture_feedback()

func capture_feedback() -> void:
	var directory := "user://feedback"
	DirAccess.make_dir_recursive_absolute(directory)
	var stem := "%s/report-%s-%s" % [directory, Time.get_unix_time_from_system(), Time.get_ticks_usec()]
	var screenshot := "unavailable: headless renderer"
	if DisplayServer.get_name() != "headless":
		var image := get_viewport().get_texture().get_image()
		if image.save_png(stem + ".png") == OK:
			screenshot = ProjectSettings.globalize_path(stem + ".png")
	var report := {"schema_version": 1, "timestamp_utc": Time.get_datetime_string_from_system(true), "build_id": build_id, "provenance": "synthetic", "state": {"frame": core.frame(), "position": position_value}, "input_history": history.duplicate(true), "screenshot": screenshot, "logs": ["native extension loaded", "feedback callback invoked"], "engine": Engine.get_version_info()}
	var file := FileAccess.open(stem + ".json", FileAccess.WRITE)
	if file == null:
		push_error("Feedback write failed")
		get_tree().quit(1)
		return
	file.store_string(JSON.stringify(report, "\t"))
	file.close()
	print("FEEDBACK_OK: ", ProjectSettings.globalize_path(stem + ".json"))
