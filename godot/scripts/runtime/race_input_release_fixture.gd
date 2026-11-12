# TASK-0112 test-only lifecycle override; actual inherited input code unchanged.
extends "res://scripts/runtime/race_scene.gd"
var unhandled_releases := 0
func _ready() -> void:
	set_process(false)
	phase = "Running"
func _unhandled_input(event: InputEvent) -> void:
	if event.is_released(): unhandled_releases += 1
	super._unhandled_input(event)
func _exit_tree() -> void:
	pass
