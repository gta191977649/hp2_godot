extends Node2D
# Explicit asset-free frontend stand-in used only by the viewer integration smoke.
func _ready() -> void:
	DebugTools.register_scene(self, "frontend")
func _input(event: InputEvent) -> void:
	DebugTools.handle_input(event)
func _exit_tree() -> void:
	DebugTools.unregister_scene(self)
