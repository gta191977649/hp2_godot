extends RefCounted
# One port UI-action adapter for every consumer of the shared track renderer.
static func snapshot(renderer: Node) -> Dictionary:
	if renderer == null:
		return {}
	var state: Dictionary = renderer.get_render_controls()
	var debug_node := renderer.get_node_or_null("PortTrackWireframeDebug")
	var effects: Array = state.get("effects", [])
	var supported := RenderingServer.get_current_rendering_method() == "forward_plus"
	var enabled := debug_node != null and bool(debug_node.enabled)
	effects.append({"id":"port_wireframe", "name":"Wireframe debug", "category":"Debug (Godot)", "enabled":enabled, "active":enabled, "available":supported, "reason":"Port-side geometry wireframe; Forward+ only. Depth/history and native selection stay unchanged."})
	state["effects"] = effects
	return state
static func action(renderer: Node, request: Dictionary) -> bool:
	if renderer == null:
		return false
	match str(request.get("action", "")):
		"track_effect":
			if str(request.get("name", "")) == "port_wireframe":
				var debug_node := renderer.get_node_or_null("PortTrackWireframeDebug")
				if debug_node == null:
					debug_node = preload("res://scripts/runtime/track_wireframe_debug.gd").new()
					debug_node.name = "PortTrackWireframeDebug"
					renderer.add_child(debug_node)
				debug_node.set_enabled(bool(request.get("enabled", false)))
				return true
			renderer.set_effect_enabled(str(request.get("name", "")), bool(request.get("enabled", false)))
		"track_render_defaults":
			var debug_node := renderer.get_node_or_null("PortTrackWireframeDebug")
			if debug_node != null:
				debug_node.set_enabled(false)
			renderer.restore_render_defaults()
		"track_props":
			renderer.set_render_props(bool(request.get("enabled", true)))
		"track_distance":
			renderer.set_visibility_override(bool(request.get("enabled", false)), float(request.get("value", 10000.0)))
		_:
			return false
	return true
