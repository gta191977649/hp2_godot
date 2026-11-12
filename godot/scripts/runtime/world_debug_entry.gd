extends RefCounted
# TASK-0107. Port-only launch/return routing. This owns no game simulation.
const SCENE := "res://scenes/race_scene.tscn"
const SELECTION := "port_world_debug_selection"
const PENDING := "port_world_debug_pending"
const RETURN_CONTEXT := "port_world_debug_return"
const RESTORE_CONTEXT := "port_world_debug_restore"

static func selection(tree: SceneTree) -> Dictionary:
	return tree.get_meta(SELECTION, {"track": 11, "car_id": -1, "skin_id": -1, "car_code": "ELISE", "reverse": false}).duplicate(true)

static func remember(tree: SceneTree, request: Dictionary) -> void:
	var chosen := selection(tree)
	for key in ["track", "car_id", "skin_id", "car_code", "reverse"]:
		if request.has(key):
			chosen[key] = request[key]
	tree.set_meta(SELECTION, chosen)

static func launch(origin: Node, request: Dictionary = {}) -> Error:
	var tree := origin.get_tree()
	remember(tree, request)
	var context: Dictionary = {}
	if origin.has_method("world_debug_return_context"):
		context = origin.world_debug_return_context()
	tree.set_meta(PENDING, selection(tree))
	tree.set_meta(RETURN_CONTEXT, context)
	return tree.change_scene_to_file(SCENE)

static func consume(tree: SceneTree) -> Dictionary:
	var request := selection(tree)
	if tree.has_meta(PENDING):
		request = tree.get_meta(PENDING).duplicate(true)
		tree.remove_meta(PENDING)
		return request
	# Direct scene launch is the same request path with explicit CLI selection.
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--track="):
			request.track = int(argument.trim_prefix("--track="))
		elif argument.begins_with("--car="):
			request.car_code = argument.trim_prefix("--car=")
			request.car_id = -1
			request.skin_id = -1
		elif argument.begins_with("--skin="):
			request.skin_id = int(argument.trim_prefix("--skin="))
		elif argument == "--reverse":
			request.reverse = true
	remember(tree, request)
	return request

static func return_to_origin(origin: Node) -> Error:
	var tree := origin.get_tree()
	var context: Dictionary = tree.get_meta(RETURN_CONTEXT, {})
	if tree.has_meta(RETURN_CONTEXT):
		tree.remove_meta(RETURN_CONTEXT)
	if not context.is_empty():
		tree.set_meta(RESTORE_CONTEXT, context)
		return tree.change_scene_to_file(str(context.scene))
	return tree.change_scene_to_file("res://scenes/frontend_native.tscn")

static func take_restore(tree: SceneTree, kind: String) -> Dictionary:
	var context: Dictionary = tree.get_meta(RESTORE_CONTEXT, {})
	if str(context.get("kind", "")) != kind:
		return {}
	tree.remove_meta(RESTORE_CONTEXT)
	return context
