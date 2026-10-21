extends Node
func _ready() -> void:
	var renderer = ClassDB.instantiate("Ps2TrackRenderer")
	if renderer == null or not renderer.has_method("test_mesh_batch_cache"):
		push_error("Track batch cache engine fixture is not registered")
		get_tree().quit(1)
		return
	var result: Dictionary = renderer.call("test_mesh_batch_cache")
	renderer.free()
	if not result.get("ok", false):
		push_error("Track batch cache: " + str(result.get("error", "unknown failure")))
		get_tree().quit(1)
		return
	print("TRACK_MESH_BATCH_CACHE_OK ", result)
	get_tree().quit(0)
