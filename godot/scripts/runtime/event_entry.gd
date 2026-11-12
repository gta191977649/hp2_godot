extends RefCounted
# TASK-0110. Port-owned scene/lifetime transport. Original event selection,
# race setup, results and profile values remain native, separately versioned.
const REQUEST := "port_authored_event_request"
const FRONTEND := "port_authored_event_frontend"
const COMMITTED_RESULT := "port_authored_event_committed_result"
const SCENE := "res://scenes/race_scene.tscn"

static func launch(frontend: Node, request: Dictionary) -> Error:
	var tree := frontend.get_tree()
	if tree.has_meta(FRONTEND):
		return ERR_ALREADY_IN_USE
	var packed: PackedScene = load(SCENE)
	if packed == null:
		return ERR_CANT_OPEN
	tree.set_meta(REQUEST, request.duplicate(true))
	tree.set_meta(FRONTEND, frontend)
	frontend.suspend_for_event()
	var world := packed.instantiate()
	tree.root.add_child(world)
	tree.current_scene = world
	return OK

static func consume(tree: SceneTree) -> Dictionary:
	var request: Dictionary = tree.get_meta(REQUEST, {})
	if tree.has_meta(REQUEST):
		tree.remove_meta(REQUEST)
	return request.duplicate(true)

static func return_to_frontend(world: Node, result: Dictionary) -> Error:
	DebugTools.write_log("Authored event: returning to retained frontend")
	var tree := world.get_tree()
	var frontend: Node = tree.get_meta(FRONTEND, null)
	if not is_instance_valid(frontend):
		return ERR_DOES_NOT_EXIST
	if not result.is_empty():
		var saved := save_result(world, result)
		if saved != OK: return saved
	# An early return transports the live source RNG without inventing an
	# outcome or saving profile progress. A loading candidate has no live seed.
	var live_seed := -1
	if world.simulation != null and world.simulation.has_method("get_world_state"):
		var source: Dictionary = world.simulation.get_world_state()
		if source.has("random_state"):
			live_seed = int(source.random_state)
	var returned: Dictionary = frontend.layout.return_from_event(live_seed)
	if not returned.get("ok", false):
		world.diagnostic = str(returned.get("error", "Native event return failed"))
		return ERR_CANT_RESOLVE
	tree.remove_meta(FRONTEND)
	tree.current_scene = frontend
	frontend.resume_from_event(returned)
	DebugTools.write_log("Authored event: retained Event Tree restored")
	world.queue_free()
	return OK

# Port storage boundary shared by original autosave and navigation retry.
static func save_result(world: Node, result: Dictionary) -> Error:
	var frontend: Node = world.get_tree().get_meta(FRONTEND, null)
	if not is_instance_valid(frontend): return ERR_DOES_NOT_EXIST
	if not result.is_empty():
		# A failed navigation after successful storage must retry navigation only.
		# Keep the acknowledgement on this World, never on a future event scene.
		if world.has_meta(COMMITTED_RESULT):
			if world.get_meta(COMMITTED_RESULT) != result:
				world.diagnostic = "Committed event result changed before menu return"
				return ERR_INVALID_DATA
		else:
			var committed: Dictionary = frontend.layout.complete_event(result)
			if not committed.get("ok", false):
				world.diagnostic = str(committed.get("error", "Event profile commit failed"))
				return ERR_CANT_CREATE
			world.set_meta(COMMITTED_RESULT, result.duplicate(true))
	return OK
