extends SceneTree

var started := Time.get_ticks_msec()

func _initialize() -> void:
    call_deferred("change_scene_to_file", "res://native_game.tscn")

func _process(_delta: float) -> bool:
    if Time.get_ticks_msec() - started > 60000:
        quit(1)
    var race := root.get_node_or_null("NativeGame/RacePresentation")
    if race and race.race_scene() and race.telemetry().get("race_state") == 3:
        print("TRACK_AUDIT_READY ", race.telemetry().track)
        quit(0)
    return false
