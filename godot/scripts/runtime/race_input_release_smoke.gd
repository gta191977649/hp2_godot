extends Node
# TASK-0112 port input-routing regression. Actual RaceScene input methods are
# inherited unchanged. Only native bootstrap/frame/teardown are suppressed;
# this fixture never creates a World or writes simulation/finish state.

class ReleaseControl extends Control:
	var releases := 0
	func _gui_input(event: InputEvent) -> void:
		if event.is_released():
			releases += 1
			accept_event()

class EarlierInputConsumer extends Node:
	var releases := 0
	func _input(event: InputEvent) -> void:
		if event.is_released():
			releases += 1
			get_viewport().set_input_as_handled()

var report := {"task": "TASK-0112", "scope": "actual RaceScene inherited input methods with native lifecycle disabled", "native_world_created": false, "original_equivalence": false, "cases": []}
var root: Window
var scene: Node3D
var gui: ReleaseControl

func _ready() -> void:
	root = get_tree().root
	_run.call_deferred()

func _key(code: Key, pressed: bool) -> InputEventKey:
	var event := InputEventKey.new()
	event.physical_keycode = code
	event.keycode = code
	event.pressed = pressed
	return event

func _push(event: InputEvent) -> void:
	root.push_input(event)

func _check(condition: bool, reason: String) -> bool:
	if condition: return true
	report["ok"] = false
	report["error"] = reason
	_write()
	push_error("RACE_INPUT_RELEASE_FAILED: " + reason)
	get_tree().quit(1)
	return false

func _write() -> void:
	var output := "res://../local/task0112/race-input-release-smoke.json"
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--report="): output = argument.trim_prefix("--report=")
	var path := ProjectSettings.globalize_path(output)
	DirAccess.make_dir_recursive_absolute(path.get_base_dir())
	var file := FileAccess.open(path, FileAccess.WRITE)
	if file != null: file.store_string(JSON.stringify(report, "  "))

func _run() -> void:
	scene = load("res://scripts/runtime/race_input_release_fixture.gd").new()
	root.add_child(scene)
	gui = ReleaseControl.new()
	gui.focus_mode = Control.FOCUS_ALL
	gui.size = Vector2(300, 200)
	scene.add_child(gui)
	await get_tree().process_frame
	gui.grab_focus()
	await get_tree().process_frame
	if not _check(gui.has_focus(), "Release-consuming GUI fixture did not own keyboard focus"): return
	for binding in [[KEY_W, "throttle"], [KEY_S, "brake"], [KEY_A, "left"], [KEY_D, "right"], [KEY_SPACE, "handbrake"], [KEY_H, "horn"]]:
		scene.input_adapter.clear()
		var keycode: Key = binding[0]
		var action: String = binding[1]
		var gui_before := gui.releases
		var unhandled_before: int = scene.unhandled_releases
		_push(_key(keycode, true))
		if not _check(scene.input_adapter.held.get(action, 0.0) == 1.0, "Actual RaceScene did not accept driving press " + action): return
		_push(_key(keycode, false))
		if not _check(not scene.input_adapter.held.has(action), "GUI-consumed release left held action " + action): return
		if not _check(gui.releases == gui_before + 1 and scene.unhandled_releases == unhandled_before, "Release did not pass through _input then get consumed before _unhandled_input"): return
		report.cases.append({"action": action, "press_accepted": true, "release_consumed_by_gui": true, "held_cleared": true})
	# The early keyboard release must preserve an independently held trigger.
	scene.input_adapter.clear()
	_push(_key(KEY_W, true))
	var trigger := InputEventJoypadMotion.new()
	trigger.device = 0
	trigger.axis = JOY_AXIS_TRIGGER_RIGHT
	trigger.axis_value = 0.75
	gui.release_focus()
	_push(trigger)
	if not _check(scene.input_adapter.sources.get("throttle", {}).size() == 2, "Independent trigger press did not reach actual driving adapter"): return
	gui.grab_focus()
	_push(_key(KEY_W, false))
	if not _check(scene.input_adapter.frame().throttle > 0.0, "Early keyboard release erased an independently held trigger"): return
	trigger = trigger.duplicate()
	trigger.axis_value = 0.0
	_push(trigger)
	if not _check(scene.input_adapter.frame().throttle == 0.0, "Neutral trigger left throttle held after GUI keyboard release"): return
	report.cases.append({"action": "throttle", "multiple_devices": true, "keyboard_release_preserves_trigger": true, "neutral_trigger_clears": true})
	# Real source-menu route consumes every event while save failure is visible.
	# A plain RefCounted satisfies only the presence guard; no method is invoked.
	gui.release_focus()
	scene.input_adapter.clear()
	_push(_key(KEY_S, true))
	if not _check(scene.input_adapter.held.get("brake", 0.0) == 1.0, "Menu route brake press setup failed"): return
	scene.event_request = {"port_input_fixture": true}
	scene.simulation = RefCounted.new()
	scene.phase = "Paused"
	scene.source_save_failed = true
	var before_menu: int = scene.unhandled_releases
	_push(_key(KEY_S, false))
	if not _check(not scene.input_adapter.held.has("brake") and scene.unhandled_releases == before_menu, "Original source-menu release consumption left brake held"): return
	report.cases.append({"action": "brake", "release_consumed_by_actual_source_menu": true, "held_cleared": true})
	scene.simulation = null
	scene.event_request = {}
	scene.source_save_failed = false
	scene.phase = "Running"
	# An earlier _input consumer is a documented limit, not an invented guarantee:
	# Godot skips later _input nodes once input has already been marked handled.
	scene.input_adapter.clear()
	_push(_key(KEY_A, true))
	var earlier := EarlierInputConsumer.new()
	scene.add_child(earlier)
	_push(_key(KEY_A, false))
	if not _check(earlier.releases == 1 and scene.input_adapter.held.has("left"), "Earlier _input consumer ordering probe differed from expected engine routing"): return
	report["earlier_input_consumer_limit_confirmed"] = true
	scene.input_adapter.clear()
	scene.queue_free()
	await get_tree().process_frame
	report["ok"] = true
	report["gui_release_cases"] = 6
	_write()
	print("RACE_INPUT_RELEASE_OK: actual RaceScene release-before-GUI/source-menu routing; earlier _input consumer limit recorded; no native World")
	get_tree().quit()
