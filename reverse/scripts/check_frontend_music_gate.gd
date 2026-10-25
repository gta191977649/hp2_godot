extends SceneTree
# TASK-0092 E-0092-J08: port binding regression using configured original assets.
func _initialize() -> void:
    call_deferred("_check")
func _check() -> void:
    if "--host-audio" in OS.get_cmdline_user_args():
        await _check_host()
        return
    var layout = ClassDB.instantiate("Ps2FrontEndLayout")
    assert(layout.load("FRONTEND/FRONTB.BUN").get("ok", false))
    layout.menu_open(22)
    layout.set_startup_presentation(true, false, true)
    for i in range(4):
        assert(not layout.frontend_music(false).has("stream"), "intro poll started BGM")
    layout.set_startup_presentation(false, false, true)
    assert(not layout.frontend_music(false).has("stream"), "skipped intro Title started BGM")
    assert(not layout.frontend_music(true).has("stream"), "idle completion started BGM")
    var start: Dictionary = layout.start_frontend_music()
    assert(start.get("ok", false) and start.has("stream") and start.get("index", -1) == 5)
    assert(not layout.frontend_music(false).has("stream"), "poll restarted an active song")
    var next: Dictionary = layout.frontend_music(true)
    assert(next.get("ok", false) and next.has("stream") and next.get("index", -1) == 7)
    print("FRONTEND_MUSIC_GATE_OK: idle/intro/skipped-Title silent; explicit start5; completion7")
    quit(0)

func _check_host() -> void:
    # Exercise actual host polling and AudioStreamPlayer in a window, without UI automation.
    var scene = load("res://scenes/frontend_native.tscn").instantiate()
    root.add_child(scene)
    await create_timer(0.3).timeout
    var player: AudioStreamPlayer = scene.get("music_audio")
    assert(not player.playing, "host polling started BGM on direct Title")
    scene.call("_start_music")
    await create_timer(0.1).timeout
    assert(player.playing, "explicit original start did not reach host audio device")
    print("FRONTEND_HOST_MUSIC_GATE_OK: Title silent; explicit start plays")
    scene.queue_free()
    await process_frame
    await create_timer(0.1).timeout
    quit(0)
