extends SceneTree
# Port-only async export ownership, completion/error and shutdown contract.
var failed := false
var directory := ""
func check(condition: bool, message: String) -> void:
    if not condition:
        failed=true
        push_error(message)
func _initialize() -> void:_run.call_deferred()
func _observer() -> Node:
    var observer=load("res://scripts/debug/profile_debug.gd").new()
    root.add_child(observer)
    observer.set_process(false)
    observer.capture_active=false
    observer.async_export=true
    observer.export_identity_cache=observer._source_identity()
    return observer
func _fill(observer: Node, marker: int, count: int) -> void:
    observer.begin_capture(0,600)
    for index in count:
        observer.samples.append({"engine_frame":index,"metrics":{"marker":marker,"frame_wall_ms":16.0},"context":{"camera":{"position":[1.25,2.5,3.75],"basis":[[1.0,0.0,0.0],[0.0,1.0,0.0],[0.0,0.0,1.0]]}},"viewports":[{"role":"colour","gpu_ms":null}],"functions":[{"name":"Injected export contract","ms":1.0,"calls":1}]})
func _finish(observer: Node) -> bool:
    var deadline := Time.get_ticks_msec()+15000
    while observer.export_thread!=null or not observer.export_queue.is_empty():
        observer._poll_exports()
        if Time.get_ticks_msec()>deadline:
            check(false,"Async export did not finish before bounded timeout")
            return false
        await process_frame
    return true
func _read(path: String) -> Dictionary:
    check(FileAccess.file_exists(path),"Async export final file missing: "+path)
    var value=JSON.parse_string(FileAccess.get_file_as_string(path))
    check(value is Dictionary,"Async export JSON is invalid: "+path)
    return value if value is Dictionary else {}
func _run() -> void:
    directory=ProjectSettings.globalize_path("res://../local/task0072/profile-export-contract-"+str(Time.get_ticks_usec()))
    DirAccess.make_dir_recursive_absolute(directory)
    var observer := _observer()
    _fill(observer,7,600)
    var first := directory.path_join("first.json")
    observer.output=first
    check(observer.export_profile()==first,"Async export must immediately return its requested path")
    check(not observer.exported and not observer.capture_active,"Queued capture must stop collecting without claiming completed export")
    var request: int=observer.export_request_sequence
    check(observer.export_profile()==first and observer.export_request_sequence==request,"Pending export must not clone/enqueue the capture again")
    observer.samples[0].metrics.marker=999
    observer.samples[0].context.camera.position[0]=999.0
    _fill(observer,8,40)
    var second := directory.path_join("second.json")
    observer.output=second
    check(observer.export_profile()==second,"A new capture must be independently queued")
    if not await _finish(observer):
        observer.free()
        quit(1)
        return
    var old := _read(first)
    var latest := _read(second)
    check(old.samples.size()==600 and old.samples[0].metrics.marker==7 and old.samples[0].context.camera.position[0]==1.25,"Worker snapshot aliases cleared or mutated live history")
    check(latest.samples.size()==40 and latest.samples[0].metrics.marker==8,"Queued capture payload was replaced by another capture")
    check(observer.exported and observer.export_status=="complete" and observer.export_error.is_empty(),"Successful rename did not publish completion")
    check(old.export.async and old.export.snapshot_clone_ms>=0 and old.source_identity.hash_capture_phase=="profiler_startup","Export clone timing/startup hash provenance missing")
    # Existing destination is replaced by the next complete file, never a partial.
    _fill(observer,9,3)
    observer.output=second
    observer.export_profile()
    await _finish(observer)
    check(_read(second).samples[0].metrics.marker==9,"Atomic replacement did not replace the previous completed export")
    # Failure is explicit and retryable, without claiming exported=true.
    var barrier := directory.path_join("regular-file")
    var file := FileAccess.open(barrier,FileAccess.WRITE)
    file.store_string("blocked directory")
    file.close()
    _fill(observer,10,1)
    observer.output=barrier.path_join("cannot-write.json")
    observer.export_profile()
    await _finish(observer)
    check(not observer.exported and observer.export_status=="failed" and not observer.export_error.is_empty(),"Write failure incorrectly completed or hid its error")
    observer.output=directory.path_join("retry.json")
    observer.export_profile()
    await _finish(observer)
    check(observer.exported and _read(str(observer.output)).samples[0].metrics.marker==10,"Failed async export could not be retried")
    # Unsupported live Objects are rejected by type, never stringified on worker.
    _fill(observer,11,1)
    observer.samples[0]["object"]=root
    observer.output=directory.path_join("object-rejected.json")
    observer.export_profile()
    await _finish(observer)
    check(not observer.exported and observer.export_error.contains("Object"),"Worker serialized an active Object from a sample")
    observer.samples.clear()
    observer.free()
    # Node teardown must drain its owned worker without deferred callbacks to freed Nodes.
    var ending := _observer()
    _fill(ending,12,600)
    var shutdown := directory.path_join("shutdown.json")
    ending.output=shutdown
    ending.export_profile()
    ending.free()
    check(_read(shutdown).samples.size()==600,"Owned export worker was abandoned during node teardown")
    var files := DirAccess.get_files_at(directory)
    for name in files:check(not ".tmp-" in name,"Async export left a temporary file: "+name)
    print("TASK0072_PROFILE_EXPORT_", "FAILED" if failed else "OK", " ",directory)
    quit(1 if failed else 0)
