extends SceneTree
# TASK-0046 port input semantics only: never asserts original game equivalence.
func key(code: Key, pressed: bool = true, echo: bool = false) -> InputEventKey:
	var event := InputEventKey.new()
	event.physical_keycode = code
	event.keycode = code
	event.pressed = pressed
	event.echo = echo
	return event

func _initialize() -> void:
	var adapter = load("res://scripts/debug/driving_input.gd").new()
	adapter.handle_input(key(KEY_W))
	assert(adapter.frame().throttle == 1)
	adapter.handle_input(key(KEY_W, false))
	assert(adapter.frame().throttle == 0)
	adapter.handle_input(key(KEY_A))
	assert(adapter.frame().steering == 1)
	adapter.handle_input(key(KEY_D))
	assert(adapter.frame().steering == 0)
	adapter.clear()
	adapter.handle_input(key(KEY_E))
	adapter.handle_input(key(KEY_E, true, true))
	assert(adapter.commands.size() == 1 and adapter.frame().shift == 1)
	adapter.handle_input(key(KEY_Q))
	assert(adapter.commands.size() == 2)
	adapter.committed()
	assert(adapter.frame().shift == -1)
	adapter.committed()
	assert(adapter.frame().shift == 0)
	adapter.handle_input(key(KEY_SPACE))
	assert(adapter.frame().handbrake == 1)
	adapter.handle_input(key(KEY_SPACE, false))
	assert(adapter.frame().handbrake == 0)
	# Releasing one device must not erase a held input from another device.
	adapter.handle_input(key(KEY_W))
	var trigger := InputEventJoypadMotion.new()
	trigger.axis = JOY_AXIS_TRIGGER_RIGHT
	trigger.axis_value = 0.75
	adapter.handle_input(trigger)
	adapter.handle_input(key(KEY_W, false))
	assert(adapter.frame().throttle > 0)
	trigger.axis_value = 0
	adapter.handle_input(trigger)
	assert(adapter.frame().throttle == 0)
	adapter.request({"transmission": 1})
	assert(adapter.frame().transmission == 1)
	adapter.committed()
	adapter.request({"neutral": true})
	assert(adapter.frame().neutral)
	adapter.handle_input(key(KEY_W), true)
	assert(adapter.commands.is_empty() and adapter.held.is_empty())
	adapter.request({"transmission": 1}, true)
	adapter.handle_input(key(KEY_Q))
	adapter.clear(true)
	assert(adapter.commands.size() == 1 and adapter.frame().transmission == 1)
	adapter.clear()
	assert(adapter.commands.is_empty())
	# Runtime remap proves the adapter contains no hidden physical W check.
	var original := InputMap.action_get_events("hp2_drive_throttle")
	InputMap.action_erase_events("hp2_drive_throttle")
	InputMap.action_add_event("hp2_drive_throttle", key(KEY_T))
	adapter.handle_input(key(KEY_W))
	assert(adapter.frame().throttle == 0)
	adapter.handle_input(key(KEY_T))
	assert(adapter.frame().throttle == 1)
	InputMap.action_erase_events("hp2_drive_throttle")
	for event in original:
		InputMap.action_add_event("hp2_drive_throttle", event)
	adapter.clear()
	var shift_binding := InputMap.action_get_events("hp2_drive_shift_up")
	InputMap.action_erase_events("hp2_drive_shift_up")
	InputMap.action_add_event("hp2_drive_shift_up", key(KEY_R))
	adapter.handle_input(key(KEY_E))
	assert(adapter.frame().shift == 0)
	adapter.handle_input(key(KEY_R))
	adapter.handle_input(key(KEY_R, true, true))
	assert(adapter.frame().shift == 1 and adapter.commands.size() == 1)
	assert(adapter.binding("shift_up") == "R")
	InputMap.action_erase_events("hp2_drive_shift_up")
	for event in shift_binding:
		InputMap.action_add_event("hp2_drive_shift_up", event)
	adapter.clear()
	print("DRIVING_INPUT_SMOKE_OK: InputMap/edges/remap/clear passed")
	quit()
