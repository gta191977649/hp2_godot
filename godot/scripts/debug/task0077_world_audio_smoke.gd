extends SceneTree
## Deterministic host transport test with a sink that applies backpressure.

func _initialize() -> void:
	var host = preload("res://scripts/runtime/world_audio.gd").new()
	var first := PackedVector2Array([Vector2(1, -1), Vector2.ZERO, Vector2(2, -2)])
	var second := PackedVector2Array([Vector2(3, -3), Vector2(4, -4)])
	var received := PackedVector2Array()
	var accept := func(block: PackedVector2Array) -> bool:
		received.append_array(block)
		return true
	host.publish(first)
	host.publish(second)
	host._submit_available(2, accept)
	var partial: bool = host.read_offset == 2 and host.queued_frames == 3
	host._submit_available(3, func(_block: PackedVector2Array) -> bool: return false)
	var retained: bool = host.read_offset == 2 and host.queued_frames == 3
	host._submit_available(3, accept)
	var expected := first.duplicate()
	expected.append_array(second)
	var drained: bool = received == expected and host.submitted_frames == 5 and host.nonzero_frames == 4
	host.publish(first)
	host._submit_available(1, accept)
	host.silence()
	var cleared: bool = host.blocks.is_empty() and host.queued_frames == 0 and host.read_offset == 0
	host.free()
	if not (partial and retained and drained and cleared):
		push_error("World PCM block transport lost ordering, offset or lifecycle state")
		quit(1)
		return
	print("TASK0077_WORLD_AUDIO_OK partial/rejected/order/silence")
	quit()
