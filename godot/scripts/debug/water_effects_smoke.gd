extends SceneTree
# TASK0089 host transport fixture. Native tests own water scheduling/particle rules.
class AudioFixture extends RefCounted:
	var bank: Dictionary
	var state: Dictionary
	var pending: Array = []
	func get_driving_audio_bank() -> Dictionary: return bank
	func get_driving_audio_state() -> Dictionary: return state
	func take_collision_audio_events() -> Array: return []
	func take_splash_audio_events() -> Array:
		var events := pending
		pending = []
		return events
func _initialize() -> void: _run.call_deferred()
func check(value: bool, text: String) -> bool:
	if not value:
		printerr("WATER_EFFECTS_HOST_FAIL: ", text)
		quit(1)
	return value
func ok(value: Dictionary) -> bool: return check(value.get("ok", false), str(value))
func _run() -> void:
	var owner := Node3D.new()
	root.add_child(owner)
	var camera := Camera3D.new()
	owner.add_child(camera)
	camera.current = true
	var native = ClassDB.instantiate("Ps2DrivingRuntime")
	if not ok(native.load_config("911TURBO")): return
	if not ok(native.set_track(12)): return
	if not ok(native.action("run")): return
	var state: Dictionary = native.tick(1.0 / 60.0)
	if not ok(state) or not check(state.has("wipers"), "native publishes original raw wiper state"): return
	if not check(native.take_splash_audio_events().is_empty() and native.take_splash_audio_events().is_empty(), "road start does not fabricate splash"): return
	var car = ClassDB.instantiate("Ps2VehicleRenderer")
	owner.add_child(car)
	car.set_show_lods(false)
	if not ok(car.load_car("CARS/911TURBO/GEOMETRY.BIN")): return
	if not ok(car.apply_driving_snapshot(state)): return
	var body: Transform3D = state.body
	camera.position = body.origin + Vector3(4, 3, -6)
	camera.look_at(body.origin)
	if not ok(car.update_view(camera.position, -camera.basis.z, 1000.0, 0)): return
	var wipers: Array[Node] = car.find_children("*WIPER*", "MeshInstance3D", true, false)
	if not check(wipers.size() == 2, "original 911TURBO wiper solids loaded"): return
	for node in wipers:
		if not check(not node.visible, "inactive wiper solid is not drawn"): return
	# Explicit test-only snapshot injection tests the host consumer; production
	# scheduling is covered with actual native water contacts in native tests.
	var active: Dictionary = state.duplicate(true)
	active.wipers = {"state": 0, "on_time": 10, "off_time": 174, "world_frame": 10}
	if not ok(car.apply_driving_snapshot(active)): return
	if not ok(car.update_view(camera.position, -camera.basis.z, 1000.0, 0)): return
	for node in wipers:
		if not check(node.visible, "active near wipers render"): return
	var first: Transform3D = wipers[0].transform
	active.wipers.world_frame = 11
	if not ok(car.apply_driving_snapshot(active)): return
	if not check(wipers[0].transform.is_equal_approx(first), "original frame scale holds two frames"): return
	active.wipers.world_frame = 30
	if not ok(car.apply_driving_snapshot(active)): return
	if not check(not wipers[0].transform.is_equal_approx(first), "authored rigid animation reaches another matrix"): return
	if DisplayServer.get_name() != "headless":
		await process_frame
		await RenderingServer.frame_post_draw
		for argument in OS.get_cmdline_user_args():
			if argument.begins_with("--water-screenshot="):
				if not check(root.get_texture().get_image().save_png(argument.trim_prefix("--water-screenshot=")) == OK, "save ignored visual evidence"): return
	var completed: Transform3D = wipers[0].transform
	var malformed: Dictionary = active.duplicate(true)
	malformed.wipers.world_frame = -1
	if not check(not car.apply_driving_snapshot(malformed).get("ok", true), "invalid original frame width rejected"): return
	if not check(wipers[0].transform.is_equal_approx(completed), "failed publication preserves previous wiper matrix"): return
	if not ok(car.apply_driving_snapshot(state)): return
	for node in wipers:
		if not check(not node.visible, "native inactive snapshot clears test animation"): return
	var fixture := AudioFixture.new()
	fixture.bank = native.get_driving_audio_bank()
	fixture.state = native.get_driving_audio_state()
	if not ok(fixture.bank) or not ok(fixture.state): return
	if not check(fixture.bank.splash_streams.size() == 3, "three original splash patches"): return
	for stream in fixture.bank.splash_streams:
		if not check(stream.mix_rate == 22050 and not stream.data.is_empty(), "original splash sample decoded"): return
	for voice in fixture.state.voices: voice.playing = false
	for voice in fixture.state.tire_voices: voice.playing = false
	var audio = load("res://scripts/runtime/driving_audio.gd").new()
	owner.add_child(audio)
	if not ok(audio.configure(fixture)): return
	fixture.pending = [{"sample": 0, "gain": 0.6, "pitch_scale": 1.0}]
	audio._process(0)
	if not check(audio.splash_plays == 1 and audio.splash_players.size() == 1, "committed splash starts once"): return
	audio._process(0)
	if not check(audio.splash_plays == 1, "repeated polling does not replay event"): return
	fixture.pending = [{"sample": 2, "gain": 0.4, "pitch_scale": 1.0}]
	fixture.state.active = false
	audio._process(0)
	if not check(audio.splash_players.is_empty() and fixture.pending.is_empty(), "pause silences and drains pending splash"): return
	if not ok(native.action("restart")): return
	if not check(native.take_splash_audio_events().is_empty(), "restart clears native event queue"): return
	await process_frame
	if DisplayServer.get_name() != "headless": await RenderingServer.frame_post_draw
	owner.queue_free()
	native = null
	await process_frame
	print("WATER_EFFECTS_HOST_OK")
	quit(0)
