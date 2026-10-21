extends Node
# Port-side Forward+ display diagnostic; no material or recovered-state changes.
var enabled := false
var original_root_mode := Viewport.DEBUG_DRAW_DISABLED
var root_viewport: WeakRef
var colour_rid := RID()

func _ready() -> void:
	process_priority = 90
	set_process(false)

func _colour_viewport() -> RID:
	var state: Dictionary = get_parent().get_optimization_state()
	for target in state.get("postprocess", {}).get("targets", []):
		if str(target.get("role", "")) == "colour":
			return target.get("rid", RID())
	return RID()

func set_enabled(value: bool) -> void:
	if value == enabled:
		return
	if value and RenderingServer.get_current_rendering_method() != "forward_plus":
		push_error("Track wireframe debug requires Forward+")
		return
	if value:
		root_viewport = weakref(get_viewport())
		original_root_mode = get_viewport().debug_draw
		enabled = true
		set_process(true)
		_apply()
	else:
		var current := _colour_viewport()
		if current.is_valid():
			RenderingServer.viewport_set_debug_draw(current, RenderingServer.VIEWPORT_DEBUG_DRAW_DISABLED)
		_restore_root()
		enabled = false
		colour_rid = RID()
		set_process(false)

func _apply() -> void:
	get_viewport().debug_draw = Viewport.DEBUG_DRAW_WIREFRAME
	var current := _colour_viewport()
	if current.is_valid() and current != colour_rid:
		RenderingServer.viewport_set_debug_draw(current, RenderingServer.VIEWPORT_DEBUG_DRAW_WIREFRAME)
	colour_rid = current

func _process(_delta: float) -> void:
	if not bool(get_parent().get_render_controls().get("render_ready", false)):
		set_enabled(false)
		return
	_apply()

func _restore_root() -> void:
	if root_viewport != null:
		var viewport := root_viewport.get_ref() as Viewport
		if viewport != null:
			viewport.debug_draw = original_root_mode
	root_viewport = null

func _exit_tree() -> void:
	if enabled:
		_restore_root()
