extends Node
# Port-side adapter regression: every original strip survives the host mesh capacity split.
func _ready() -> void:
    _run.call_deferred()
func _check(value: bool, message: String) -> void:
    if not value:
        push_error("CAR_SURFACE_BATCH_FAILED: " + message)
        get_tree().quit(1)
        assert(value, message)
func _meshes(node: Node) -> Array[MeshInstance3D]:
    var result: Array[MeshInstance3D] = []
    if node is MeshInstance3D and node.mesh is ArrayMesh:
        result.push_back(node)
    for child in node.get_children():
        result.append_array(_meshes(child))
    return result
func _verify(car: Node3D, state: Dictionary) -> int:
    var count := 0
    var triangles := 0
    var extras := 0
    var union_bounds := AABB()
    var first_bounds := true
    for node in _meshes(car):
        _check(node.mesh.get_surface_count() <= RenderingServer.MAX_MESH_SURFACES, "Per-mesh limit")
        if node.get_parent() != car:
            extras += 1
            _check(node.transform == Transform3D.IDENTITY, "Batch inherits original part transform")
        count += node.mesh.get_surface_count()
        if node.is_visible_in_tree():
            var local: Transform3D = car.global_transform.affine_inverse()*node.global_transform
            var bounds: AABB = local*node.get_aabb()
            union_bounds = bounds if first_bounds else union_bounds.merge(bounds)
            first_bounds = false
        for index in node.mesh.get_surface_count():
            _check(node.get_active_material(index) is ShaderMaterial, "Every original draw retains its shader")
            if node.is_visible_in_tree():
                triangles += node.mesh.surface_get_array_index_len(index) / 3
    for parent in car.get_children():
        if not parent.has_meta("hp2_part_name"):
            continue
        var emitted := 0
        for batch in _meshes(parent):
            emitted += batch.mesh.get_surface_count()
        var found := false
        for part in state.parts:
            if str(part.name) == str(parent.get_meta("hp2_part_name")):
                found = true
                _check(emitted==int(part.draw_surfaces), "Each native part retains every original surface")
                var batches := _meshes(parent)
                _check(batches.size()==maxi(1,ceili(float(part.draw_surfaces)/RenderingServer.MAX_MESH_SURFACES)),"Exact host batch partition")
                for index in batches.size()-1:
                    _check(batches[index].mesh.get_surface_count()==RenderingServer.MAX_MESH_SURFACES,"Every non-final batch is full")
        _check(found,"Batch parent keeps native part identity")
    _check(count == int(state.draw_surfaces), "Uploaded surfaces must equal original draw records")
    _check(union_bounds.position.is_equal_approx(state.bounds.position) and union_bounds.size.is_equal_approx(state.bounds.size), "Bounds include every mesh batch")
    _check(triangles == int(state.triangles), "No original visible triangles lost or duplicated")
    return extras
func _wait(car: Node3D) -> Dictionary:
    var deadline := Time.get_ticks_msec()+30000
    var state: Dictionary = car.get_state()
    while state.loading:
        _check(Time.get_ticks_msec() < deadline, "Async upload timeout")
        await get_tree().process_frame
        state = car.pump_load()
    _check(state.ok, str(state.error))
    return car.update_view(Vector3(5,4,9), Vector3.FORWARD, 1200.0, 0)
func _capture_binding_lifecycle() -> void:
    var car: Node3D = ClassDB.instantiate("Ps2VehicleRenderer")
    add_child(car)
    var image := Image.create(1,1,false,Image.FORMAT_RGBA8)
    image.fill(Color(0.25,0.5,0.75,128.0/255.0))
    var first_texture := ImageTexture.create_from_image(image)
    var second_texture := ImageTexture.create_from_image(image)
    var matrix := PackedFloat32Array([1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1])
    for attempt in 2:
        var loaded: Dictionary = car.load_car("CARS/ELISE/GEOMETRY.BIN")
        _check(loaded.ok,"Capture fixture original Elise load")
        var capture := {"enabled":true,"available":true,"texture":first_texture,"hack_matrix_native":matrix,"diagnostic":"binding fixture"}
        car.set_environment_capture(capture)
        car.update_view(Vector3(5,4,9),Vector3.FORWARD,1200.0,0)
        var reflection: ShaderMaterial
        var base: ShaderMaterial
        var blocked := 0
        for mesh in _meshes(car):
            for index in mesh.mesh.get_surface_count():
                var material: ShaderMaterial = mesh.get_active_material(index)
                if material.get_shader_parameter("flag_0x4"):
                    _check(material.next_pass==null,"Unsupported31C remains detached")
                    blocked += 1
                elif material.next_pass is ShaderMaterial and reflection==null:
                    base=material;reflection=material.next_pass
        _check(reflection!=null and blocked>0,"Both supported318 and guarded31C fixture routes")
        _check(reflection.get_shader_parameter("base_texture")==first_texture,"New/reloaded draws receive unchanged capture texture identity")
        var before: Vector3 = reflection.get_shader_parameter("environment_row0")
        var changed := matrix.duplicate()
        changed[0]=-1;changed[5]=-1
        capture.hack_matrix_native=changed
        for frame in 12:
            car.set_environment_capture(capture)
        car.update_view(Vector3(5,4,9),Vector3.FORWARD,1200.0,0)
        var after: Vector3 = reflection.get_shader_parameter("environment_row0")
        _check(not before.is_equal_approx(after),"HackMan matrix updates while binding identity stays cached")
        _check(str(car.get_state().environment_capture_diagnostic).contains("31C"),"Unsupported route diagnostic survives cache hits")
        _check(base.next_pass==reflection,"Repeated capture keeps same supported material")
        capture.enabled=false;car.set_environment_capture(capture)
        _check(base.next_pass==null,"Disabled capture detaches cached pass")
        capture.enabled=true;car.set_environment_capture(capture)
        _check(base.next_pass==reflection,"Reenabled capture restores same pass")
        capture.texture=second_texture;car.set_environment_capture(capture)
        _check(reflection.get_shader_parameter("base_texture")==second_texture,"Changed texture identity invalidates binding")
        capture.hack_matrix_native=PackedFloat32Array([1])
        car.set_environment_capture(capture)
        _check(not car.get_state().environment_capture_ready and base.next_pass==null,"Invalid capture clears ready binding")
        car.clear_car()
        _check(not car.get_state().environment_capture_ready,"Release resets capture readiness")
    car.queue_free()

func _run() -> void:
    var resolver: Object = ClassDB.instantiate("Ps2TextureLoader")
    if not resolver.resolve_root().ok:
        print("CAR_SURFACE_BATCH_SKIPPED: original assets not configured")
        get_tree().quit(77)
        return
    var car: Node3D = ClassDB.instantiate("Ps2VehicleRenderer")
    add_child(car)
    var selected: Dictionary = {}
    var loaded := 0
    var oversized := 0
    for entry in car.get_catalog():
        if not entry.available or entry.skins.is_empty():
            continue
        var state: Dictionary = car.load_car_id(int(entry.id), int(entry.skins[0].id))
        _check(state.ok, str(entry.code)+": "+str(state.error))
        state = car.update_view(Vector3(5,4,9), Vector3.FORWARD, 1200.0, 0)
        var extras := _verify(car, state)
        loaded += 1
        if extras:
            oversized += 1
            print("CAR_SURFACE_BATCH_MODEL ", entry.code, " extra_batches=", extras, " surfaces=", state.draw_surfaces)
            if selected.is_empty():
                selected = entry
    _check(oversized > 0, "Configured corpus must exercise a solid beyond the host capacity")
    # Reload one oversized part twice through time-sliced upload and the complete GPU batch cache.
    var expected_surfaces := 0
    var cached_batches: Dictionary = {}
    for attempt in 2:
        car.request_car("", int(selected.id), int(selected.skins[-1].id))
        var state := await _wait(car)
        _check(_verify(car,state)>0, "Async and warm loads retain extra batches")
        if attempt == 0:
            expected_surfaces = int(state.draw_surfaces)
        _check(int(state.draw_surfaces)==expected_surfaces, "Warm surface coverage")
        _check(int(state.gpu_hits)>0, "Warm batch cache used")
        for parent in car.get_children():
            if not parent.has_meta("hp2_part_name"):
                continue
            var ids: Array[int] = []
            for batch in _meshes(parent):
                ids.push_back(batch.mesh.get_instance_id())
            var key: String = parent.get_meta("hp2_part_name")
            if attempt == 0:
                cached_batches[key] = ids
            else:
                _check(cached_batches[key]==ids, "Complete mesh batch vectors reused from GPU cache")
    car.set_wireframe(true)
    for node in _meshes(car):
        _check(node.material_overlay != null, "Wireframe covers every batch")
    car.set_wireframe(false)
    var parent_name := ""
    for node in car.get_children():
        if node is MeshInstance3D and node.get_child_count()>0:
            parent_name = str(node.get_meta("hp2_part_name"))
            car.set_part_visible(parent_name,false)
            car.update_view(Vector3(5,4,9),Vector3.FORWARD,1200.0,0)
            await get_tree().process_frame
            for batch in _meshes(node):
                _check(not batch.is_visible_in_tree(),"Part visibility hides all batches")
            car.set_part_visible(parent_name,true)
            break
    _check(not parent_name.is_empty(), "Batched part visibility exercised")
    car.set_show_lods(true)
    await _wait(car)
    var total := 0
    for node in _meshes(car):
        total += node.mesh.get_surface_count()
        _check(node.mesh.get_surface_count()<=RenderingServer.MAX_MESH_SURFACES,"LOD batch capacity")
        for index in node.mesh.get_surface_count():
            _check(node.get_active_material(index) is ShaderMaterial,"LOD copy keeps batch materials")
    _check(total==int(car.get_state().draw_surfaces),"LOD copy draw coverage")
    car.clear_car()
    _check(car.get_child_count()==0,"All batch roots/children released")
    car.queue_free()
    await get_tree().process_frame
    _capture_binding_lifecycle()
    print("CAR_SURFACE_BATCH_OK: models=", loaded," oversized=",oversized," sync/async/cache/visibility/LOD/cleanup")
    get_tree().quit(0)
