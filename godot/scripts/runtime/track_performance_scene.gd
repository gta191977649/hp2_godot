extends "res://scripts/runtime/race_scene.gd"
# Delay inherited _ready's load until benchmark options are applied.
var benchmark_start_allowed := false
func begin_session() -> void:
 if benchmark_start_allowed:
  super.begin_session()

# Loading may finish before the awaiting benchmark resumes. Never allow an extra
# real-time Running frame/physics tick between that transition and the fixed loop.
func _process(_delta: float) -> void:
 if phase=="Loading":
  super._process(0.0)
