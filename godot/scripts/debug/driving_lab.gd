extends RefCounted
# TASK-0107. Compatibility launcher only; simulation belongs to race_scene.
var viewer: Node3D
var lab: Object
var error := ""
var native_tick_calls := 0
var state: Dictionary = {}

func _init(owner: Node3D) -> void:
	viewer = owner

func engaged() -> bool:
	return false

func captures_pointer() -> bool:
	return false

func start() -> bool:
	error = ""
	if viewer.pending_load or viewer.track_pending:
		error = "Wait for the selected original resources to finish loading"
		return false
	var result: Error = preload("res://scripts/runtime/world_debug_entry.gd").launch(viewer, viewer.world_debug_launch_request())
	if result != OK:
		error = "World scene transition failed: %d" % result
	return result == OK

func stop() -> void:
	pass

func process(_delta: float) -> void:
	pass

func sync_lifecycle() -> void:
	pass

func handle_input(_event: InputEvent, _consumed: bool = false) -> void:
	pass

func action(_request: Dictionary) -> void:
	error = "Embedded driving was replaced by the unified World scene"

func snapshot() -> Dictionary:
	return {"kind": "world_debug_launcher", "diagnostic": error}
