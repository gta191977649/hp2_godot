extends SceneTree
# Port-side startup errors must remain inside the GDExtension boundary.
func _initialize() -> void:
 _run.call_deferred()
func _run() -> void:
 var renderer=ClassDB.instantiate("Ps2TrackRenderer")
 if renderer==null:
  push_error("Worker configuration fixture requires native renderer")
  quit(1)
  return
 var state: Dictionary=renderer.get_state()
 var controls: Dictionary=renderer.get_render_controls()
 var expected := "--expect-startup-error" in OS.get_cmdline_user_args()
 var failed := not str(state.get("error","")).is_empty()
 renderer.free()
 if failed!=expected:
  push_error("Unexpected native worker startup result: "+str(state))
  quit(1)
  return
 print("TASK0072_WORKER_CONFIG_OK expected_error=",expected," controls=",controls)
 quit(0)
