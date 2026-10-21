# Port-side synthetic diagnostic, TASK-0080. No original assets or game behavior.
# Run with the pinned engine, --headless --path godot --script <absolute path>.
extends SceneTree

func _initialize() -> void:
	for device_id in [0, 1, 3]:
		var adapter = load("res://scripts/debug/driving_input.gd").new()
		for axis_value in [-1.0, -0.5, 0.0, 0.5, 1.0, 0.0]:
			var event = InputEventJoypadMotion.new()
			event.device = device_id
			event.axis = JOY_AXIS_LEFT_X
			event.axis_value = axis_value
			adapter.handle_input(event)
			print("TASK0080_AXIS device=", device_id, " raw=", axis_value,
				" steering=", adapter.frame().steering)
	quit()
