extends RefCounted
# TASK-0046: port-side semantic adapter. Bindings live only in project InputMap.
const CONTINUOUS := ["throttle", "brake", "left", "right", "handbrake"]
var held: Dictionary = {}
var sources: Dictionary = {}
var commands: Array[Dictionary] = []
var command_requests := 0
var command_commits := 0
var last_command: Dictionary = {}

func clear(preserve_gui: bool = false) -> void:
	held.clear()
	sources.clear()
	if preserve_gui:
		commands.assign(commands.filter(func(command: Dictionary) -> bool: return bool(command.get("_gui", false))))
	else:
		commands.clear()

func handle_input(event: InputEvent, blocked: bool = false) -> void:
	if blocked:
		clear()
		return
	for name in CONTINUOUS:
		var action := "hp2_drive_" + str(name)
		if event.is_action(action):
			var strength := event.get_action_strength(action)
			var source := "%s:%d" % [event.get_class(), event.device]
			if event is InputEventKey:
				source += ":%d" % (event.physical_keycode if event.physical_keycode != 0 else event.keycode)
			elif event is InputEventJoypadMotion:
				source += ":%d" % event.axis
			elif event is InputEventJoypadButton:
				source += ":%d" % event.button_index
			var values: Dictionary = sources.get(name, {})
			if strength > 0:
				values[source] = strength
			else:
				values.erase(source)
			sources[name] = values
			var combined := 0.0
			for value in values.values():
				combined = maxf(combined, float(value))
			if combined > 0:
				held[name] = combined
			else:
				held.erase(name)
	if event.is_action_pressed("hp2_drive_shift_down", false):
		request({"shift": -1})
	if event.is_action_pressed("hp2_drive_shift_up", false):
		request({"shift": 1})

func request(command: Dictionary, from_gui: bool = false) -> void:
	var queued := command.duplicate()
	queued["_gui"] = from_gui
	commands.append(queued)
	command_requests += 1
	last_command = queued.duplicate()

func frame() -> Dictionary:
	var result := {"throttle": float(held.get("throttle", 0)), "brake": float(held.get("brake", 0)),
		"steering": float(held.get("left", 0)) - float(held.get("right", 0)),
		"handbrake": float(held.get("handbrake", 0)), "transmission": -1, "shift": 0, "neutral": false}
	if not commands.is_empty():
		result.merge(commands.front(), true)
	return result

func committed() -> void:
	if not commands.is_empty():
		commands.pop_front()
		command_commits += 1

func binding(name: String) -> String:
	for event in InputMap.action_get_events("hp2_drive_" + name):
		if event is InputEventKey:
			return event.as_text().replace(" (Physical)", "")
	return name

func hints() -> Array[String]:
	return ["%s/%s: throttle / brake-reverse | %s/%s: steer" % [binding("throttle"), binding("brake"), binding("left"), binding("right")],
		"%s/%s: shift down/up | %s: handbrake" % [binding("shift_down"), binding("shift_up"), binding("handbrake")],
		"Mouse: look | %s: release cursor | Click world: capture" % binding("release_cursor")]
