extends SceneTree
# Port-only capture/export boundary, including long runs, retry and manual UI.
var failed := false
func check(value: bool, message: String) -> void:
 if not value:
  failed=true
  push_error(message)
func _initialize() -> void:_run.call_deferred()
func _run() -> void:
 var profiler=load("res://scripts/debug/profile_debug.gd").new()
 root.add_child(profiler)
 profiler.set_process(false)
 profiler.enabled=true
 profiler.output=ProjectSettings.globalize_path("user://task0077-boundary.json")
 profiler.begin_capture(0,3,true)
 for frame in 8:profiler._process(1.0/60.0)
 check(profiler.samples.size()==3,"Held capture exceeded limit during long measurement")
 check(not profiler.capture_active and profiler.export_held,"Capture did not stop while retaining export hold")
 check(profiler.export_status=="ready" and not profiler.exported,"Held capture exported inside measurement")
 check(profiler.capture_completed_process_frame>=0,"Missing capture completion frame")
 check(profiler.export_profile().is_empty() and not profiler.exported,"Manual export bypassed hold")
 profiler.capture_next_frames()
 check(profiler.samples.size()==3 and profiler.export_held,"UI replaced benchmark-owned capture")
 var ended := Time.get_ticks_usec()
 profiler.export_held=false
 var dispatched := Time.get_ticks_usec()
 profiler.export_profile()
 check(dispatched>=ended and profiler.exported,"Untimed synchronous export failed")
 var document=JSON.parse_string(FileAccess.get_file_as_string(profiler.output))
 check(document is Dictionary and document.samples.size()==3,"Export lost bounded samples")
 profiler.begin_capture(0,2)
 check(not profiler.export_held and profiler.capture_completed_process_frame==-1,"New ordinary capture retained prior boundary")
 profiler._process(1.0/60.0)
 profiler._process(1.0/60.0)
 check(profiler.exported and profiler.samples.size()==2,"Ordinary automatic export regressed")
 profiler.enabled=false
 profiler.free()
 print("TASK0077_CAPTURE_BOUNDARY_", "FAILED" if failed else "OK")
 quit(1 if failed else 0)
