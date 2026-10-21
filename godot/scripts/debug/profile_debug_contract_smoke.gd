extends SceneTree
# Port-side injected timestamp contract, not a GPU performance/parity test.
var failure := false
func check(condition: bool, message: String) -> void:
    if not condition:
        failure=true
        push_error(message)
func _initialize() -> void:
    _run.call_deferred()
func _counter_payload(count: int, frame: int) -> PackedByteArray:
    var packed := PackedByteArray()
    packed.resize(16)
    for pixel in 4:
        packed[pixel*4]=count&255
        packed[pixel*4+1]=(count>>8)&255
        var shift := (pixel%2)*16
        packed[pixel*4+2]=(frame>>shift)&255
        packed[pixel*4+3]=(frame>>(shift+8))&255
    return packed
func _run() -> void:
    var observer=load("res://scripts/debug/profile_debug.gd").new()
    root.add_child(observer)
    observer.set_process(false)
    observer._accept_timestamps(7,{"42":{"begin":[1000,1000000],"end":[2500,2500000]}})
    check(observer.timestamp_rows["42"].source_frame==7,"Completed RD frame identity lost")
    check(observer.timestamp_rows["42"].cpu_ms==1.5 and observer.timestamp_rows["42"].gpu_ms==1.5,"CPU microseconds/GPU nanoseconds units differ")
    observer._accept_timestamps(7,{"42":{"begin":[1000,1000000],"end":[2500,2500000]}})
    check(observer.timestamp_rows.is_empty(),"Repeated completion must not replay timing")
    observer._accept_timestamps(6,{})
    check(observer.timestamp_frame==7 and observer.timestamp_rows.is_empty(),"Older completion must not rewind timing")
    observer._accept_timestamps(8,{"42":{"begin":[1000,1000000]},"43":{"begin":[2000,3000],"end":[1000,2000]}})
    check(not observer.timestamp_rows.has("42"),"Unpaired viewport timestamp must be unavailable")
    check(observer.timestamp_rows["43"].cpu_ms==null and observer.timestamp_rows["43"].gpu_ms==null,"Negative interval must be unavailable")
    observer._accept_timestamps(-1,{})
    check(observer.timestamp_rows.is_empty() and observer.timestamp_status=="rendering_device_unavailable","Missing RD must not expose cached timing")
    var scene=Node3D.new()
    root.add_child(scene)
    current_scene=scene
    var camera=Camera3D.new()
    scene.add_child(camera)
    camera.current=true
    camera.position=Vector3(12.345678,23.456789,34.567891)
    camera.rotation=Vector3(0.123456,0.234567,0.345678)
    var context: Dictionary=observer._capture_context(null,{"effective_near":0.5,"effective_far":10000,"lod":{"visual_frame_counter":72}})
    check(context.camera.position[0]==camera.position.x,"Camera coordinates must retain numerical precision")
    check(context.camera.basis[2][1]==camera.basis.z.y,"Camera full orientation must be retained")
    check(context.renderer_frame==72 and context.effective_far==10000,"Context source generation/distance lost")
    var roundtrip: Dictionary=JSON.parse_string(JSON.stringify(context))
    check(roundtrip.camera.basis is Array and roundtrip.camera.position is Array,"Camera must export as JSON numeric arrays")
    observer.sun_telemetry_owner=123
    observer.sun_telemetry_generation=4
    observer.sun_telemetry_request=9
    observer.sun_telemetry_available=true
    observer.sun_telemetry_pending=true
    observer.sun_telemetry_textures[0]=true
    var packed := _counter_payload(1023,0)
    observer._accept_sun_telemetry(packed,RID(),0,4,123,9)
    check(observer.sun_telemetry_latest.uncovered==1023 and not observer.sun_telemetry_pending,"Readonly async counter decode failed")
    check(observer.sun_telemetry_latest.age_frames==Engine.get_process_frames(),"Async counter age must use engine process-frame domain")
    for count in 1025:
        packed=_counter_payload(count,0)
        observer._accept_sun_telemetry(packed,RID(),0,4,123,9)
        check(observer.sun_telemetry_latest.uncovered==count,"Async counter byte decode differs at "+str(count))
    packed=_counter_payload(1023,0)
    observer._accept_sun_telemetry(packed,RID(),0,4,123,9)
    observer._accept_sun_telemetry(packed,RID(),0,3,123,9)
    check(observer.sun_telemetry_latest.uncovered==1023,"Stale counter generation must not replace telemetry")
    observer._accept_sun_telemetry(packed,RID(),0,4,124,9)
    check(observer.sun_telemetry_latest.uncovered==1023,"Destroyed renderer callback must not replace telemetry")
    observer.sun_telemetry_pending=true
    observer._accept_sun_telemetry(packed,RID(),0,4,123,8)
    check(observer.sun_telemetry_pending,"Stale async request must not release newer request")
    packed=_counter_payload(1280,0)
    observer._accept_sun_telemetry(packed,RID(),0,4,123,9)
    check(observer.sun_telemetry_latest.uncovered==null,"Counter outside recovered0..1024 must be unavailable")
    observer._accept_sun_telemetry(PackedByteArray(),RID(),0,4,123,9)
    check(observer.sun_telemetry_latest.uncovered==null,"Missing async bytes must be unavailable")
    packed=_counter_payload(1023,0xA1B2C3D4)
    observer._accept_sun_telemetry(packed,RID(),0xA1B2C3D4,4,123,9)
    check(observer.sun_telemetry_latest.uncovered==1023 and observer.sun_telemetry_latest.source_frame==0xA1B2C3D4,"GPU payload producer frame byte order lost")
    check(observer.sun_telemetry_latest.source_frame_domain=="engine_process_frame","Async counter frame domain lost")
    observer._accept_sun_telemetry(packed,RID(),0xA1B2C3D5,4,123,9)
    check(observer.sun_telemetry_latest.uncovered==null and observer.sun_telemetry_latest.status=="async_source_frame_mismatch","Reused counter texture must not claim requested producer")
    packed=_counter_payload(1023,0)
    packed[4]=0
    observer._accept_sun_telemetry(packed,RID(),0,4,123,9)
    check(observer.sun_telemetry_latest.uncovered==null and observer.sun_telemetry_latest.status=="async_counter_payload_invalid","Counter RG disagreement must be unavailable")
    packed=_counter_payload(1023,0)
    packed[10]=1
    observer._accept_sun_telemetry(packed,RID(),0,4,123,9)
    check(observer.sun_telemetry_latest.uncovered==null and observer.sun_telemetry_latest.status=="async_counter_payload_invalid","Producer row disagreement must be unavailable")
    observer.sun_telemetry_textures={}
    packed=_counter_payload(1023,0)
    observer._accept_sun_telemetry(packed,RID(),0,4,123,9)
    check(observer.sun_telemetry_latest.uncovered==null,"Retired texture callback must be ignored")
    observer.sun_telemetry_enabled=true
    observer.sun_telemetry_pending=true
    var retired_request: int=observer.sun_telemetry_request
    observer._sample_sun_telemetry(null,{"telemetry_generation":5,"counter_frame_domain":"engine_process_frame"})
    check(not observer.sun_telemetry_pending and observer.sun_telemetry_request>retired_request,"Lifetime change must retire pending callback identity")
    observer._accept_sun_telemetry(packed,RID(),0,4,123,retired_request)
    check(observer.sun_telemetry_latest.uncovered==null,"Retired pending callback must not restore telemetry")
    print("PROFILE_DEBUG_CONTRACT_OK" if not failure else "PROFILE_DEBUG_CONTRACT_FAILED")
    observer.queue_free()
    scene.queue_free()
    await process_frame
    quit(1 if failure else 0)
