extends SceneTree
# TASK0083 port transport fixture; native wall/selector tests cover the producer.
class QueueFixture extends RefCounted:
	var bank: Dictionary
	var state: Dictionary
	var events: Array = []
	func get_driving_audio_bank() -> Dictionary: return bank
	func get_driving_audio_state() -> Dictionary: return state
	func take_splash_audio_events() -> Array: return []
	func take_collision_audio_events() -> Array:
		var pending := events
		events = []
		return pending
func _initialize() -> void: _run.call_deferred()
func _check(value: bool, text: String) -> bool:
	if not value:
		printerr("COLLISION_AUDIO_HOST_FAIL: ", text)
		quit(1)
	return value
func _run() -> void:
	var native = ClassDB.instantiate("Ps2DrivingRuntime")
	var loaded: Dictionary = native.load_config("911TURBO")
	if not _check(loaded.get("ok", false), str(loaded)): return
	native.action("run")
	native.tick(1.0 / 60.0)
	var fixture := QueueFixture.new()
	fixture.bank = native.get_driving_audio_bank()
	fixture.state = native.get_driving_audio_state()
	for voice in fixture.state.voices: voice.playing = false
	for voice in fixture.state.tire_voices: voice.playing = false
	if not _check(fixture.bank.collision_streams.size() == 18, "original eighteen patches"): return
	var display = load("res://scripts/runtime/driving_audio.gd").new()
	root.add_child(display)
	if not _check(display.configure(fixture).get("ok", false), "configure"): return
	fixture.state.collision_scrapes = [{"id": 42, "sample": 15, "gain": 0.6, "pitch_scale": 1.0}]
	fixture.events = [{"sample": 1, "gain": 0.7, "pitch_scale": 0.9}]
	display._process(0.0)
	if not _check(display.collision_plays == 1 and display.collision_players[0].playing, "one-shot starts"): return
	display._process(0.0)
	if not _check(display.collision_plays == 1, "polling does not repeat consumed event"): return
	await create_timer(0.3).timeout
	if not _check(display.scrape_players.has(42) and display.scrape_players[42].playing, "persistent scrape starts"): return
	fixture.state.collision_scrapes[0].gain = 0.1
	display._process(0.0)
	if not _check(is_equal_approx(display.scrape_players[42].volume_linear, 0.1), "native fade gain is applied"): return
	fixture.state.collision_scrapes = []
	display._process(0.0)
	if not _check(display.scrape_players.is_empty(), "finished scrape is released"): return
	fixture.state.active = false
	display._process(0.0)
	if not _check(display.collision_players.is_empty(), "pause releases impact voices"): return
	display.queue_free()
	fixture = null
	native = null
	await create_timer(0.3).timeout
	print("COLLISION_AUDIO_HOST_OK")
	quit(0)
