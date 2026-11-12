extends Node
# TASK-0107. Host streaming of committed shared-World PCM; no voice synthesis.
var player: AudioStreamPlayer
var playback: AudioStreamGeneratorPlayback
var blocks: Array[PackedVector2Array] = []
var block_index := 0
var read_offset := 0
var queued_frames := 0
var submitted_frames := 0
var nonzero_frames := 0
var skipped_mixes := 0
var _last_skips := 0
var submit_usec := 0

func _ready() -> void:
	player = AudioStreamPlayer.new()
	var stream := AudioStreamGenerator.new()
	stream.mix_rate = 48000
	stream.buffer_length = 0.2
	player.stream = stream
	add_child(player)
	player.play()
	playback = player.get_stream_playback()

func publish(pcm: PackedVector2Array) -> void:
	if pcm.is_empty():
		return
	blocks.append(pcm)
	queued_frames += pcm.size()
	for sample in pcm:
		if sample != Vector2.ZERO:
			nonzero_frames += 1
	_drain()

func _process(_delta: float) -> void:
	_drain()

func _drain() -> void:
	if playback == null:
		return
	var skips := playback.get_skips()
	skipped_mixes += maxi(0, skips - _last_skips)
	_last_skips = skips
	if queued_frames == 0:
		return
	var started := Time.get_ticks_usec()
	_submit_available(playback.get_frames_available(), playback.push_buffer)
	submit_usec += Time.get_ticks_usec() - started

func _submit_available(available: int, push: Callable) -> void:
	while available > 0 and block_index < blocks.size():
		var block := blocks[block_index]
		var count := mini(available, block.size() - read_offset)
		var data := block if read_offset == 0 and count == block.size() else block.slice(read_offset, read_offset + count)
		if not push.call(data):
			break
		submitted_frames += count
		queued_frames -= count
		available -= count
		read_offset += count
		if read_offset == block.size():
			blocks[block_index] = PackedVector2Array()
			block_index += 1
			read_offset = 0
	if block_index == blocks.size():
		blocks.clear()
		block_index = 0
	elif block_index >= 32 and block_index >= blocks.size() / 2:
		blocks = blocks.slice(block_index)
		block_index = 0

func silence() -> void:
	blocks.clear()
	block_index = 0
	read_offset = 0
	queued_frames = 0
	_last_skips = 0
	if is_instance_valid(player):
		player.stop()
	playback = null

func resume_stream() -> void:
	if is_instance_valid(player) and not player.playing:
		player.play()
		playback = player.get_stream_playback()
