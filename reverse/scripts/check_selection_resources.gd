extends SceneTree
# TASK-0098: isolated port resource/upload fixture, not an original camera or UI oracle.
var failures := 0
func _initialize() -> void:
    run.call_deferred()
func check(value: bool, message: String) -> void:
    if not value:
        push_error("SELECTION_RESOURCES_FAILED: " + message)
        failures += 1
func finish(renderer: Node, world: Node) -> void:
    renderer.call("clear_car")
    world.queue_free()
    await process_frame
    await process_frame
    quit(0 if failures == 0 else 1)
func run() -> void:
    root.size = Vector2i(640, 480)
    var world := Node3D.new()
    root.add_child(world)
    var camera := Camera3D.new()
    world.add_child(camera)
    camera.position = Vector3(0, 5, 12)
    camera.look_at(Vector3.ZERO)
    camera.current = true
    var renderer: Node3D = ClassDB.instantiate("Ps2VehicleRenderer")
    world.add_child(renderer)
    for entry in [[0xd8173744, "PLATFORM"], [0xb077e934, "CARCOVER"], [0xd8173744, "PLATFORM"]]:
        renderer.call("request_frontend_solid", entry[0])
        var start := Time.get_ticks_msec()
        var state: Dictionary = renderer.call("get_state")
        while state.get("loading", false) and Time.get_ticks_msec() - start < 30000:
            await process_frame
            state = renderer.call("pump_load")
        check(not state.get("loading", true), "named-solid loading deadline")
        check(state.get("ok", false), str(entry[1]) + ": " + str(state.get("error", "missing state")))
        if not state.get("ok", false):
            await finish(renderer, world)
            return
        check(state.get("car_id", 0) == -1, "named solid became a car identity")
        var parts: Array = state.get("parts", [])
        check(parts.size() == 1 and parts[0].name == entry[1], "native original solid binding")
        var view: Dictionary = renderer.call("update_view", camera.global_position, -camera.global_basis.z, 300.0, 0)
        check(view.get("ok", false), str(view.get("error", "view update failed")))
        check(int(view.get("draw_surfaces", 0)) > 0, "original solid has no uploaded strips")
        print("SELECTION_RESOURCE_UPLOADED: ", entry[1], " ", view.get("summary", ""))
    # Supersede an in-flight request. Only the last generation may be published.
    renderer.call("request_frontend_solid", 0xb077e934)
    var replacement: Dictionary = renderer.call("request_frontend_solid", 0xd8173744)
    var generation: int = replacement.get("generation", -1)
    var start := Time.get_ticks_msec()
    while replacement.get("loading", false) and Time.get_ticks_msec() - start < 30000:
        await process_frame
        replacement = renderer.call("pump_load")
    check(replacement.get("ok", false) and replacement.get("completed", -2) == generation,
        "superseded request generation published stale display")
    var final_parts: Array = replacement.get("parts", [])
    check(final_parts.size() == 1 and final_parts[0].name == "PLATFORM", "stale cover replaced final platform")
    if failures == 0:
        print("SELECTION_RESOURCES_OK: original PLATFORM/CARCOVER strips, uploads and cancellation")
    await finish(renderer, world)
