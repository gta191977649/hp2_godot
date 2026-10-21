extends SceneTree
# Public host/session contract, not an original-game visual equivalence test.
func _initialize() -> void:
	_run.call_deferred()

func _check(value: bool, reason: String) -> void:
	if not value:
		push_error(reason)
		quit(1)
		assert(value, reason)

func _run() -> void:
	var first: Node3D = ClassDB.instantiate("Ps2TrackRenderer")
	var second: Node3D = ClassDB.instantiate("Ps2TrackRenderer")
	root.add_child(first)
	root.add_child(second)
	first.restore_render_defaults()
	_check(not first.get_render_controls().render_ready, "Unloaded controls were enabled")
	_check(first.get_render_controls().render_props, "Props must be visible by default")
	var initial: Array = second.get_effects()
	_check(initial.size() == 12, "Effect catalogue or LUT explanation missing")
	_check(not initial[1].enabled, "Costly dynamic reflection must default off")
	if first.get_render_controls().debug_controls:
		_check(preload("res://scripts/runtime/track_controls.gd").action(first, {"action": "track_props", "enabled": false}), "Props action was not handled")
		_check(not second.get_render_controls().render_props, "Props gate was not shared")
		first.set_effect_enabled("roadshine", false)
		first.set_effect_enabled("environment", true)
		_check(second.get_effects()[1].enabled, "Reflection opt-in was not shared")
		_check(not second.get_effects()[0].enabled, "Effect gate did not cross renderer instances")
		first.set_visibility_override(true, 1234.0)
		_check(second.get_render_controls().distance_override, "Visibility override was not session-wide")
		second.restore_render_defaults()
		_check(first.get_effects()[0].enabled and not first.get_render_controls().distance_override, "Original defaults did not restore all renderer instances")
		_check(not first.get_effects()[1].enabled, "Restore defaults re-enabled reflection capture")
	_check(first.get_render_controls().render_props, "Defaults did not restore props")
	var camera := Camera3D.new()
	root.add_child(camera)
	camera.current = true
	camera.keep_aspect = Camera3D.KEEP_WIDTH
	camera.fov = 50.0
	var state: Dictionary = first.request_track(11)
	while state.get("loading", false):
		state = first.pump_load()
		await process_frame
	_check(state.get("ok", false), "Original track load failed: " + str(state))
	camera.position = state.start_position - state.start_forward * 8.0 + Vector3.UP * 3.0
	camera.look_at(state.start_position)
	first.set_camera_mode(1)
	state = first.update_view(camera.position)
	_check(state.ok and is_equal_approx(camera.near, 0.5) and is_equal_approx(camera.far, 10000.0), "Original driving depth range was not applied")
	if first.get_render_controls().debug_controls:
		second.set_visibility_override(true, 1234.0)
		first.update_view(camera.position)
		_check(is_equal_approx(camera.far, 1234.0), "Shared override was not applied to the active camera")
		first.restore_render_defaults()
		first.update_view(camera.position)
		_check(is_equal_approx(camera.far, 10000.0), "Original automatic distance was not restored")
	first.clear_track()
	_check(not first.get_render_controls().render_ready, "Unloaded effect resources remained active")
	for effect in first.get_effects():
		_check(not effect.active, "Unloaded effect retained active state")
	first.queue_free()
	second.queue_free()
	camera.queue_free()
	await process_frame
	print("TRACK_CONTROLS_OK")
	quit(0)
