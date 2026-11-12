extends SceneTree
# Isolated port input contracts; fake pad inventory is not production-device acceptance.
const Adapter = preload("res://scripts/runtime/frontend_input.gd")
var errors: Array[String] = []
var output := ""
func _initialize() -> void: call_deferred("_run")
func _check(ok: bool, name: String) -> void:
	if not ok: errors.append(name)
func _key(code: Key, down: bool) -> InputEventKey:
	var event := InputEventKey.new()
	event.physical_keycode=code
	event.keycode=code
	event.pressed=down
	return event
func _run() -> void:
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--controls-output="): output=argument.trim_prefix("--controls-output=")
	if output.is_empty(): quit(2); return
	var input := Adapter.new()
	input.path=output.path_join("input-unit.cfg")
	input.initialize()
	_check(input.handle(_key(KEY_ENTER,true)).size()==1,"confirm press once")
	_check(input.handle(_key(KEY_ENTER,true)).is_empty(),"duplicate press ignored")
	_check(input.handle(_key(KEY_SPACE,true)).is_empty(),"alias does not duplicate held confirm")
	_check(input.handle(_key(KEY_ENTER,false)).is_empty(),"alias keeps confirm held")
	_check(input.handle(_key(KEY_SPACE,false)).size()==1,"last alias releases confirm")
	var echo := _key(KEY_ENTER,true)
	echo.echo=true
	_check(input.handle(echo).is_empty(),"OS repeat ignored")
	_check(input.handle(_key(KEY_DOWN,true)).size()==1,"navigation press")
	_check(input.tick(0.2).is_empty(),"repeat initial delay")
	_check(input.tick(0.2).size()==1,"repeat after delay")
	_check(input.release_all().size()==1 and input.held.is_empty(),"focus loss releases held input")
	input.device_inventory=func(): return PackedInt32Array([10,20])
	var axis := InputEventJoypadMotion.new()
	axis.device=20
	axis.axis=JOY_AXIS_LEFT_X
	axis.axis_value=-1
	var left: Array=input.handle(axis)
	_check(left.size()==1 and left[0].id==18 and left[0].port==1,"left axis maps second port")
	axis.axis_value=1
	var right: Array=input.handle(axis)
	_check(right.size()==2 and right[0].id==18 and not right[0].pressed and right[1].id==19 and right[1].pressed,"axis sign reversal releases previous direction")
	axis.axis_value=0.1
	var neutral: Array=input.handle(axis)
	_check(neutral.size()==1 and neutral[0].id==19 and not neutral[0].pressed,"axis deadzone releases")
	axis.device=99
	_check(input.handle(axis).is_empty(),"unknown device ignored")
	var rebound: Dictionary=input.rebind("confirm",_key(KEY_F6,true))
	_check(rebound.ok,"save custom binding")
	var loaded := Adapter.new()
	loaded.path=input.path
	loaded.initialize()
	_check(loaded.hint("confirm").contains("F6"),"reload saved binding")
	var conflict: Dictionary=loaded.rebind("confirm",_key(KEY_ESCAPE,true))
	_check(not conflict.ok and conflict.has("conflicts"),"conflict rejected without replacement")
	_check(loaded.rebind("confirm",_key(KEY_ESCAPE,true),true).ok,"explicit conflict replacement")
	_check(not loaded.hint("back",true).contains("Escape"),"conflict removed from previous action")
	_check(loaded.restore()==OK and loaded.hint("confirm")=="Enter","restore all defaults")
	# Valid ConfigFile syntax with invalid schema must leave all defaults intact.
	var config := ConfigFile.new()
	config.set_value("frontend","version",99)
	config.save(input.path)
	var corrupt := Adapter.new()
	corrupt.path=input.path
	corrupt.initialize()
	_check(not corrupt.warning.is_empty() and corrupt.hint("confirm")=="Enter","invalid configuration falls back with warning")
	var file := FileAccess.open(output.path_join("input-contract.json"),FileAccess.WRITE)
	file.store_string(JSON.stringify({"ok":errors.is_empty(),"errors":errors,"scope":"port unit contracts; injected pad inventory"},"\t"))
	file.close()
	print("FRONTEND_INPUT_CONTRACT_", "OK" if errors.is_empty() else "FAIL", " ",errors)
	quit(0 if errors.is_empty() else 2)
