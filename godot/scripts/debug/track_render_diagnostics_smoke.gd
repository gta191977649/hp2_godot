extends SceneTree
func _initialize() -> void:
    _run.call_deferred()
func _run() -> void:
    var renderer = ClassDB.instantiate("Ps2TrackRenderer")
    var camera := Camera3D.new()
    root.add_child(renderer)
    root.add_child(camera)
    camera.position=Vector3(141.24,-17.98,697.04)
    camera.rotation=Vector3(0.12,0.4,0)
    var expected := camera.transform
    var diagnostic = preload("res://scripts/debug/track_render_diagnostics.gd").new()
    var path: String=diagnostic.export_snapshot(root,camera,renderer)
    if not FileAccess.file_exists(path):
        push_error(path)
        quit(1)
        return
    camera.transform=Transform3D.IDENTITY
    var result: String=diagnostic.restore_snapshot(camera,renderer,path)
    if not result.begins_with("Restored") or not camera.transform.is_equal_approx(expected):
        push_error("Diagnostic pose restore failed: "+result)
        quit(1)
        return
    if diagnostic.latest.get("viewports",[]).is_empty() or not diagnostic.latest.has("controls") or not diagnostic.latest.has("backend"):
        push_error("Missing actual viewport/control diagnostic")
        quit(1)
        return
    renderer.queue_free()
    camera.queue_free()
    print("TRACK_RENDER_DIAGNOSTICS_OK pose/control export and restore; empty renderer is not asset/GPU coverage")
    quit(0)
