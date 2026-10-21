extends RefCounted
# Port diagnostic counters. Values describe the last rendered frame, not GPU timings.
var enabled := false
var latest: Dictionary = {}
var viewports: Array[WeakRef] = []
var generation := ""
var samples := 0

func sample(owner: Node, camera: Camera3D, renderer: Node3D, force := false) -> Dictionary:
    if not enabled and not force:
        return latest
    samples += 1
    if not force and samples % 6 != 1:
        return latest
    var state: Dictionary = renderer.get_state() if is_instance_valid(renderer) else {}
    var current := str(state.get("track_id", -1)) + str(state.get("loading", false))
    if current != generation or viewports.is_empty() or force:
        generation = current
        viewports.clear()
        viewports.append(weakref(owner.get_tree().root))
        for node in owner.get_tree().root.find_children("*", "SubViewport", true, false):
            viewports.append(weakref(node))
    var target_names: Dictionary = {}
    if is_instance_valid(renderer):
        var targets: Dictionary = renderer.get_render_targets()
        for key in targets:
            if targets[key] is Texture2D:
                target_names[targets[key].get_rid()] = key
    var optimization: Dictionary = renderer.get_optimization_state() if is_instance_valid(renderer) else {}
    for target in optimization.get("postprocess", {}).get("targets", []):
        target_names[str(target.rid)] = target.role
    var rows: Array = []
    for reference in viewports:
        var viewport: Viewport = reference.get_ref()
        if not is_instance_valid(viewport):
            generation = ""
            continue
        var row := {"path":str(viewport.get_path()), "size":[int(viewport.get_visible_rect().size.x), int(viewport.get_visible_rect().size.y)], "counter_available":null}
        row["role"] = target_names.get(str(viewport.get_viewport_rid()), target_names.get(viewport.get_texture().get_rid(), str(viewport.name)))
        row["update_mode"] = int(viewport.render_target_update_mode) if viewport is SubViewport else -1
        for entry in [["visible", Viewport.RENDER_INFO_TYPE_VISIBLE], ["shadow", Viewport.RENDER_INFO_TYPE_SHADOW], ["canvas", Viewport.RENDER_INFO_TYPE_CANVAS]]:
            row[entry[0]] = {"draws":viewport.get_render_info(entry[1], Viewport.RENDER_INFO_DRAW_CALLS_IN_FRAME), "primitives":viewport.get_render_info(entry[1], Viewport.RENDER_INFO_PRIMITIVES_IN_FRAME), "objects":viewport.get_render_info(entry[1], Viewport.RENDER_INFO_OBJECTS_IN_FRAME)}
        rows.append(row)
    var basis := camera.global_basis
    latest = {"schema":1, "backend":RenderingServer.get_current_rendering_method(), "render_frame":Engine.get_frames_drawn(), "total_draws":int(Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME)), "track":state.get("track_id", -1), "camera":{"position_godot":[camera.global_position.x,camera.global_position.y,camera.global_position.z], "position_hp2":[camera.global_position.z,camera.global_position.x,camera.global_position.y], "basis_columns":[[basis.x.x,basis.x.y,basis.x.z],[basis.y.x,basis.y.y,basis.y.z],[basis.z.x,basis.z.y,basis.z.z]], "fov":camera.fov, "keep_aspect":camera.keep_aspect, "near":camera.near, "far":camera.far}, "viewports":rows, "renderer":state, "optimization":optimization, "controls":renderer.get_render_controls() if is_instance_valid(renderer) else {}, "notes":"Engine last-frame counters; zero may mean unavailable. Visible primitives include triangles/lines/points. Particle draws share colour/depth viewports and cannot be isolated by viewport counters. Native batch counts are estimates, not GPU draws."}
    return latest

func lines() -> Array[String]:
    var result: Array[String] = []
    for row in latest.get("viewports", []):
        var label: String = str(row.role)
        result.append("%s [%dx%d]: 3D %d / canvas %d / shadow %d draws; %d visible primitives" % [label,row.size[0],row.size[1],row.visible.draws,row.canvas.draws,row.shadow.draws,row.visible.primitives])
    result.append("Particle draws share scene passes; primitives are not always triangles.")
    result.append("Zero counters may be unavailable; disabled targets may retain old values.")
    return result

func export_snapshot(owner: Node, camera: Camera3D, renderer: Node3D) -> String:
    sample(owner,camera,renderer,true)
    var directory := ProjectSettings.globalize_path("res://../local/task0065")
    DirAccess.make_dir_recursive_absolute(directory)
    var path := directory.path_join("track-draw-" + str(Time.get_ticks_usec()) + ".json")
    var file := FileAccess.open(path,FileAccess.WRITE)
    if file == null:
        return "Draw diagnostic export failed: " + str(FileAccess.get_open_error())
    file.store_string(JSON.stringify(latest,"  "))
    return path

func restore_snapshot(camera: Camera3D, renderer: Node3D, path: String) -> String:
    var parsed = JSON.parse_string(FileAccess.get_file_as_string(path))
    if not parsed is Dictionary or parsed.get("schema",0) != 1 or not parsed.has("camera"):
        return "Invalid draw diagnostic: " + path
    var state: Dictionary = renderer.get_state()
    if int(parsed.get("track",-1)) != int(state.get("track_id",-2)):
        return "Diagnostic track differs from loaded track"
    var pose: Dictionary = parsed.camera
    var p: Array = pose.get("position_godot",[])
    var columns: Array = pose.get("basis_columns",[])
    if p.size()!=3 or columns.size()!=3:
        return "Diagnostic camera pose is incomplete"
    for column in columns:
        if not column is Array or column.size()!=3:
            return "Diagnostic camera basis is incomplete"
    var transform := Transform3D(Basis(Vector3(columns[0][0],columns[0][1],columns[0][2]),Vector3(columns[1][0],columns[1][1],columns[1][2]),Vector3(columns[2][0],columns[2][1],columns[2][2])),Vector3(p[0],p[1],p[2]))
    var fov := float(pose.get("fov",0))
    if not transform.is_finite() or absf(transform.basis.determinant())<0.000001 or not is_finite(fov) or fov<=0 or fov>=180:
        return "Diagnostic camera values are invalid"
    camera.global_transform=transform
    camera.fov=fov
    camera.keep_aspect=int(pose.get("keep_aspect",camera.keep_aspect))
    var controls: Dictionary=parsed.get("controls",{})
    for effect in controls.get("effects",[]):
        renderer.set_effect_enabled(str(effect.id),bool(effect.enabled))
    renderer.set_render_props(bool(controls.get("render_props",true)))
    renderer.set_visibility_override(bool(controls.get("distance_override",false)),float(controls.get("distance",10000)))
    return "Restored diagnostic camera/effect controls; transient particle/history state is not a replay: " + path
