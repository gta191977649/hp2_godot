extends SceneTree

var started := Time.get_ticks_msec()

func _initialize() -> void:
    call_deferred("change_scene_to_file", "res://native_game.tscn")

func _process(_delta: float) -> bool:
    if Time.get_ticks_msec() - started > 60000:
        quit(1)
    var race := root.get_node_or_null("NativeGame/RacePresentation")
    if race and race.race_scene() and race.telemetry().get("race_state") == 3:
        var submitted := {}
        for instance in race.race_scene().get_children():
            if not instance is MeshInstance3D or not instance.mesh or not instance.mesh.has_meta("hp2_road_water"):
                continue
            for i in range(instance.mesh.get_surface_count()):
                var material := instance.mesh.surface_get_material(i) as ShaderMaterial
                if not material or not material.shader.resource_path.begins_with("res://shader/road_water_"):
                    continue
                var surface := [instance.mesh.get_instance_id(), i]
                if submitted.has(surface):
                    continue
                submitted[surface] = true
                print("TRACK_PUDDLE %08x %08x %d %s %d" % [
                    material.get_meta("hp2_texture_key", 0), material.get_meta("hp2_puddle_base_key", 0),
                    int(material.get_shader_parameter("gs_has_wet_mask") == true),
                    material.shader.resource_path.get_file(), instance.mesh.surface_get_array_index_len(i) / 3])
        print("TRACK_AUDIT_READY ", race.telemetry().track)
        quit(0)
    return false
