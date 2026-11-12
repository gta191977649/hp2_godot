extends RefCounted
# TASK-0104: PC device policy. Native event IDs/handlers remain authoritative.
signal changed
const PREFIX := "hp2_frontend_"
const EVENTS := {"up":3,"down":4,"left":5,"right":6,"start":7,"select":8,
	"confirm":9,"cancel":10,"back":11,"help":12,"l1":13,"l2":14,"r1":15,
	"r2":16,"square":17,"analog_left":18,"analog_right":19,"analog_up":20,"analog_down":21}
var path := "user://frontend_bindings.cfg"
var warning := ""
var held: Dictionary = {}
var defaults: Dictionary = {}
var last_device := "keyboard"
# Device inventory is injectable only for isolated input-contract tests.
var device_inventory: Callable = func(): return Input.get_connected_joypads()

func initialize() -> void:
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--frontend-bindings="):
			path = argument.trim_prefix("--frontend-bindings=")
	for name in EVENTS:
		var action := PREFIX + str(name)
		var project_defaults: Dictionary = ProjectSettings.get_setting("input/"+action, {})
		assert(not project_defaults.is_empty(), "Missing frontend InputMap action: " + action)
		if not InputMap.has_action(action): InputMap.add_action(action,project_defaults.deadzone)
		InputMap.action_erase_events(action)
		for event in project_defaults.events: InputMap.action_add_event(action,event)
		defaults[name] = InputMap.action_get_events(action).duplicate(true)
	_load()

func _describe(event: InputEvent) -> Dictionary:
	if event is InputEventKey:
		return {"kind":"key", "code":event.physical_keycode if event.physical_keycode != 0 else event.keycode}
	if event is InputEventJoypadButton:
		return {"kind":"button", "code":event.button_index}
	if event is InputEventJoypadMotion:
		return {"kind":"axis", "code":event.axis, "sign":signf(event.axis_value)}
	return {}

func _event(data: Dictionary) -> InputEvent:
	var code: int = data.get("code", -1)
	if code < 0: return null
	match str(data.get("kind", "")):
		"key":
			if code == 0: return null
			var event := InputEventKey.new()
			event.physical_keycode = code
			return event
		"button":
			if code >= JOY_BUTTON_MAX: return null
			var event := InputEventJoypadButton.new()
			event.button_index = code
			return event
		"axis":
			if code >= JOY_AXIS_MAX or absf(float(data.get("sign", 0))) != 1: return null
			var event := InputEventJoypadMotion.new()
			event.axis = code
			event.axis_value = data.sign
			return event
	return null

func _load() -> void:
	if not FileAccess.file_exists(path): return
	var config := ConfigFile.new()
	if config.load(path) != OK or config.get_value("frontend", "version", 0) != 1:
		warning = "Key bindings could not be loaded; defaults restored."
		return
	var pending: Dictionary = {}
	var seen: Dictionary = {}
	for name in EVENTS:
		var values = config.get_value("bindings", name, null)
		if not values is Array:
			warning = "Invalid key bindings; defaults restored."
			return
		var parsed: Array[InputEvent] = []
		for value in values:
			if not value is Dictionary or _event(value) == null:
				warning = "Invalid key bindings; defaults restored."
				return
			var identity := JSON.stringify(_describe(_event(value)))
			if seen.has(identity):
				warning = "Conflicting saved bindings; defaults restored."
				return
			seen[identity] = true
			parsed.append(_event(value))
		pending[name] = parsed
	for name in pending:
		InputMap.action_erase_events(PREFIX + name)
		for event in pending[name]: InputMap.action_add_event(PREFIX + name, event)

func save() -> Error:
	var config := ConfigFile.new()
	config.set_value("frontend", "version", 1)
	for name in EVENTS:
		var values: Array[Dictionary] = []
		for event in InputMap.action_get_events(PREFIX + name): values.append(_describe(event))
		config.set_value("bindings", name, values)
	var result := config.save(path + ".tmp")
	if result != OK: return result
	return DirAccess.rename_absolute(ProjectSettings.globalize_path(path + ".tmp"), ProjectSettings.globalize_path(path))

func rebind(name: String, event: InputEvent, replace_conflicts: bool = false) -> Dictionary:
	if not EVENTS.has(name) or _event(_describe(event)) == null: return {"ok":false,"error":"Unsupported binding"}
	var conflicts: Array[String] = []
	for other in EVENTS:
		if other == name: continue
		for old in InputMap.action_get_events(PREFIX + other):
			if _describe(old) == _describe(event): conflicts.append(other)
	if not conflicts.is_empty() and not replace_conflicts: return {"ok":false,"conflicts":conflicts}
	for other in conflicts:
		for old in InputMap.action_get_events(PREFIX + other):
			if _describe(old) == _describe(event): InputMap.action_erase_event(PREFIX + other, old)
	# Replace the selected device class, retaining the other device bindings.
	var kind: String = _describe(event).kind
	for old in InputMap.action_get_events(PREFIX + name):
		if (_describe(old).kind == "key") == (kind == "key"): InputMap.action_erase_event(PREFIX + name, old)
	InputMap.action_add_event(PREFIX + name, _event(_describe(event)))
	changed.emit()
	var result := save()
	return {"ok":result == OK,"error":"" if result == OK else "Cannot save key bindings"}

func restore(name: String = "") -> Error:
	for key in EVENTS:
		if name != "" and name != key: continue
		InputMap.action_erase_events(PREFIX + key)
		for event in defaults[key]: InputMap.action_add_event(PREFIX + key, event)
	changed.emit()
	return save()

func hint(name: String, all_bindings: bool = false) -> String:
	var text: PackedStringArray = []
	for event in InputMap.action_get_events(PREFIX + name):
		if (event is InputEventKey) == (last_device == "keyboard"): text.append(event.as_text().replace(" (Physical)", "").replace(" - Physical", ""))
	return (" / ".join(text) if all_bindings else text[0]) if not text.is_empty() else "Unbound"

func handle(event: InputEvent) -> Array[Dictionary]:
	var out: Array[Dictionary] = []
	if not (event is InputEventKey or event is InputEventJoypadButton or event is InputEventJoypadMotion): return out
	if event is InputEventKey and event.echo: return out
	if event is InputEventKey and event.physical_keycode == 0:
		event = event.duplicate()
		event.physical_keycode = event.keycode
	var port := 0
	if not event is InputEventKey:
		port = device_inventory.call().find(event.device)
		if port < 0 or port > 1: return out
		last_device = "gamepad"
	else: last_device = "keyboard"
	for name in EVENTS:
		var action := PREFIX + str(name)
		var matches := event.is_action(action)
		var axis_pressed := false
		if event is InputEventJoypadMotion:
			matches = false
			for mapping in InputMap.action_get_events(action):
				if mapping is InputEventJoypadMotion and mapping.axis == event.axis:
					matches = true
					axis_pressed = axis_pressed or event.axis_value*signf(mapping.axis_value)>InputMap.action_get_deadzone(action)
		if not matches: continue
		var identity := "%s:%d:%s" % [name, port, JSON.stringify(_describe(event))]
		if event is InputEventJoypadMotion:
			identity = "%s:%d:axis%d" % [name, port, event.axis]
		var pressed := axis_pressed if event is InputEventJoypadMotion else event.is_action_pressed(action, false)
		if pressed and not held.has(identity):
			var already_held := false
			for source in held.values():
				if source.name==name and source.port==port: already_held=true
			held[identity] = {"name":name,"port":port,"time":0.0,"next":0.35}
			if not already_held: out.append({"id":EVENTS[name],"port":port,"pressed":true})
		elif not pressed and held.has(identity):
			held.erase(identity)
			var still_held := false
			for source in held.values():
				if source.name==name and source.port==port: still_held=true
			if not still_held: out.append({"id":EVENTS[name],"port":port,"pressed":false})
	return out

func tick(delta: float) -> Array[Dictionary]:
	var out: Array[Dictionary] = []
	var repeated: Dictionary = {}
	for entry in held.values():
		if not entry.name in ["up","down","left","right","analog_up","analog_down","analog_left","analog_right"]: continue
		entry.time += delta
		if entry.time >= entry.next:
			entry.next = entry.time + 0.09
			var key := "%s:%d" % [entry.name,entry.port]
			if repeated.has(key): continue
			repeated[key] = true
			out.append({"id":EVENTS[entry.name],"port":entry.port,"pressed":true})
	return out

func release_all() -> Array[Dictionary]:
	var out: Array[Dictionary] = []
	for entry in held.values(): out.append({"id":EVENTS[entry.name],"port":entry.port,"pressed":false})
	held.clear()
	return out
