extends Node
# Port-side voice transport only. Original samples, loops, gain and pitch are native.
var simulation: Object
var voices: Array[AudioStreamPlayer] = []
var requested: Array[bool] = []
var collision_streams: Array = []
var collision_players: Array[AudioStreamPlayer] = []
var collision_plays := 0
var splash_streams: Array = []
var splash_players: Array[AudioStreamPlayer] = []
var splash_plays := 0
var scrape_players: Dictionary = {}
var tire_streams: Array = []
var tire_players: Array[AudioStreamPlayer] = []
var tire_requested: Array[bool] = []
var diagnostic := ""
var updates := 0

func configure(native: Object) -> Dictionary:
	simulation = native
	var bank: Dictionary = simulation.get_driving_audio_bank()
	if not bank.get("ok", false):
		return bank
	collision_streams = bank.get("collision_streams", [])
	splash_streams = bank.get("splash_streams", [])
	if splash_streams.size() != 3:
		return {"ok": false, "error": "Original splash bank requires three samples"}
	tire_streams = bank.get("tire_streams", [])
	if tire_streams.size() != 4:
		return {"ok": false, "error": "Original tire bank requires four samples"}
	for index in 10:
		var player := AudioStreamPlayer.new()
		player.volume_linear = 0
		add_child(player)
		tire_players.append(player)
		tire_requested.append(false)
	var streams: Array = bank.engine_streams
	if streams.size() != 12:
		return {"ok": false, "error": "Original engine bank requires twelve voices"}
	streams = streams.duplicate()
	streams.append(bank.horn_stream)
	for stream in streams:
		var player := AudioStreamPlayer.new()
		player.stream = stream
		player.volume_linear = 0
		add_child(player)
		voices.append(player)
		requested.append(false)
	return {"ok": true}

func silence() -> void:
	for player in scrape_players.values():
		player.stop()
		player.stream = null
		player.queue_free()
	scrape_players.clear()
	for index in tire_players.size():
		tire_players[index].stop()
		tire_requested[index] = false
	for player in collision_players:
		player.stop()
		player.stream = null
		player.queue_free()
	collision_players.clear()
	for player in splash_players:
		player.stop()
		player.stream = null
		player.queue_free()
	splash_players.clear()
	if is_instance_valid(simulation):
		simulation.take_collision_audio_events()
		simulation.take_splash_audio_events()
	for i in voices.size():
		voices[i].stop()
		requested[i] = false

func _process(_delta: float) -> void:
	if not is_instance_valid(simulation) or voices.size() != 13:
		silence()
		return
	var state: Dictionary = simulation.get_driving_audio_state()
	if not state.get("ok", false):
		silence()
		var reason := str(state.get("error", "Vehicle audio state unavailable"))
		if reason != diagnostic:
			diagnostic = reason
			push_error(reason)
		return
	if not state.get("active", false):
		silence()
		return
	var values: Array = state.voices
	if values.size() != 12:
		silence()
		return
	for i in 13:
		var playing: bool = bool(values[i].playing) if i < 12 else bool(state.horn_playing)
		var gain: float = float(values[i].gain) if i < 12 else float(state.horn_gain)
		var pitch: float = float(values[i].pitch_scale) if i < 12 else float(state.horn_pitch_scale)
		# A natural end is not a new Play request. Original ADPCM flags own looping.
		voices[i].volume_linear = gain
		voices[i].pitch_scale = pitch
		if playing and not requested[i]:
			voices[i].play()
		elif not playing:
			voices[i].stop()
		requested[i] = playing
	var tire_values: Array = state.get("tire_voices", [])
	if tire_values.size() != 10:
		silence()
		return
	for index in 10:
		var voice: Dictionary = tire_values[index]
		var sample := int(voice.sample)
		var player := tire_players[index]
		var stream = tire_streams[sample]
		if player.stream != stream:
			player.stop()
			player.stream = stream
			tire_requested[index] = false
		player.volume_linear = float(voice.gain)
		player.pitch_scale = float(voice.pitch_scale)
		var playing := bool(voice.playing)
		if playing and not tire_requested[index]: player.play()
		elif not playing: player.stop()
		tire_requested[index] = playing
	var scrape_ids: Dictionary = {}
	for voice in state.get("collision_scrapes", []):
		var id := int(voice.id)
		scrape_ids[id] = true
		if not scrape_players.has(id):
			var player := AudioStreamPlayer.new()
			player.stream = collision_streams[int(voice.sample)]
			add_child(player)
			scrape_players[id] = player
			player.volume_linear = float(voice.gain)
			player.pitch_scale = float(voice.pitch_scale)
			player.play()
		else:
			scrape_players[id].volume_linear = float(voice.gain)
			scrape_players[id].pitch_scale = float(voice.pitch_scale)
	# Native volume includes the original per-update fade; absent IDs are released.
	for id in scrape_players.keys():
		if not scrape_ids.has(id):
			var player: AudioStreamPlayer = scrape_players[id]
			player.stop()
			player.stream = null
			player.queue_free()
			scrape_players.erase(id)
	# A native committed event is consumed once, independently of polling frequency.
	for index in range(collision_players.size() - 1, -1, -1):
		if not collision_players[index].playing:
			collision_players[index].stream = null
			collision_players[index].queue_free()
			collision_players.remove_at(index)
	for event in simulation.take_collision_audio_events():
		var sample := int(event.sample)
		if sample < 0 or sample >= collision_streams.size():
			push_error("Original collision sample index is unavailable")
			continue
		var player := AudioStreamPlayer.new()
		player.stream = collision_streams[sample]
		player.volume_linear = float(event.gain)
		player.pitch_scale = float(event.pitch_scale)
		add_child(player)
		player.play()
		collision_players.append(player)
		collision_plays += 1
	for index in range(splash_players.size() - 1, -1, -1):
		if not splash_players[index].playing:
			splash_players[index].stream = null
			splash_players[index].queue_free()
			splash_players.remove_at(index)
	for event in simulation.take_splash_audio_events():
		var sample := int(event.sample)
		if sample < 0 or sample >= splash_streams.size():
			push_error("Original splash sample index is unavailable")
			continue
		var player := AudioStreamPlayer.new()
		player.stream = splash_streams[sample]
		player.volume_linear = float(event.gain)
		player.pitch_scale = float(event.pitch_scale)
		add_child(player)
		player.play()
		splash_players.append(player)
		splash_plays += 1
	updates += 1

func _exit_tree() -> void:
	silence()
	for player in voices + tire_players:
		player.stream = null
	simulation = null
	collision_streams.clear()
	tire_streams.clear()
	splash_streams.clear()
