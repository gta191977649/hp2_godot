extends SceneTree
# TASK0082 host lifecycle regression; original runtime equivalence not claimed.
func _initialize() -> void:
	_run.call_deferred()
func _check(value: bool, message: String) -> bool:
	if not value:
		printerr("DRIVING_AUDIO_HOST_FAIL: ", message)
		quit(1)
	return value
func _run() -> void:
	var simulation = ClassDB.instantiate("Ps2DrivingRuntime")
	var loaded: Dictionary = simulation.load_config("911TURBO")
	if not _check(loaded.get("ok", false), str(loaded)): return
	var transport = load("res://scripts/runtime/driving_audio.gd").new()
	root.add_child(transport)
	if not _check(transport.configure(simulation).get("ok", false), "configure"): return
	if not _check(transport.voices.size() == 13, "thirteen original streams"): return
	simulation.action("run")
	var audible := false
	var tires_audible := false
	for frame in 480:
		simulation.set_driving_input(1.0, 0.0, 0.7 if frame > 180 else 0.0, 1.0 if frame > 220 and frame < 300 else 0.0, 0, 0, false, frame < 30)
		var state: Dictionary = simulation.tick(1.0 / 60.0)
		if not _check(state.get("ok", false), str(state)): return
		transport._process(0.0)
		if frame == 10:
			if not _check(transport.requested[12] and state.lights.headlight_left, "horn press and headlight flash"): return
		if frame == 40:
			if not _check(not transport.requested[12] and not state.lights.headlight_left, "horn release"): return
		for voice in transport.voices:
			if voice.playing and voice.volume_linear > 0: audible = true
		for voice in transport.tire_players:
			if voice.playing and voice.volume_linear > 0: tires_audible = true
		if frame % 10 == 0: await process_frame
	if not _check(audible, "nonzero original voice playback"): return
	if not _check(tires_audible, "native drift and handbrake tire playback"): return
	simulation.action("pause")
	transport._process(0.0)
	for voice in transport.voices + transport.tire_players:
		if not _check(not voice.playing, "pause stops voices"): return
	simulation.action("restart")
	transport._process(0.0)
	if not _check(not simulation.get_driving_audio_state().active, "restart clears audio frame"): return
	transport.queue_free()
	simulation = null
	await create_timer(0.1).timeout
	print("DRIVING_AUDIO_HOST_OK")
	quit(0)
