extends SceneTree

var deadline := 0
var schedule := ""
var jitter_index := 0
var stalled := false
var started := false
var display_configured := false
var frames := 0
var started_at := 0

func _initialize() -> void:
    deadline = Time.get_ticks_msec() + 90000
    schedule = OS.get_environment("HP2_MVP_PHYSICS_SCHEDULE")
    call_deferred("change_scene_to_file", "res://native_game.tscn")

func _process(_delta: float) -> bool:
    if Time.get_ticks_msec() > deadline:
        push_error("Race physics probe timed out")
        quit(1)
        return true
    var race := root.get_node_or_null("NativeGame/RacePresentation")
    if not is_instance_valid(race):
        return false
    if not display_configured:
        display_configured = true
        DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
        Engine.max_fps = int(schedule) if schedule.is_valid_int() else 60
    var state: Dictionary = race.telemetry()
    if state.get("physics_probe_complete", false):
        var duration := Time.get_ticks_usec() - started_at
        print("HP2_MVP_PHYSICS_COMPLETE frames=%d duration_us=%d stall=%s" % [frames, duration, stalled])
        quit(0)
        return true
    if state.get("race_state") != 3 or state.get("step", 0) <= 0:
        return false
    if not started:
        started = true
        started_at = Time.get_ticks_usec()
    frames += 1
    if schedule == "jitter":
        const DELAYS := [0, 11, 2, 23, 4, 0, 30]
        OS.delay_msec(DELAYS[jitter_index])
        jitter_index = (jitter_index + 1) % DELAYS.size()
    elif schedule == "stall" and not stalled and state.get("physics_probe_ticks", 0) >= 140:
        stalled = true
        OS.delay_msec(180)
    return false
