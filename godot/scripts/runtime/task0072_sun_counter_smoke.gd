extends Node2D
# Port-only GPU transport fixture. Shared alpha include is production code;
# reduction is extracted from the explicitly supplied production C++ source.
# This does not establish original-game equivalence or native scene scheduling.
const INTENSITIES = [0.0,-0.125,-0.5,-1.0,-2.0,-3.0,0.0001,0.1,0.5,1.0,1.25,1.9999,2.0,2.5,3.9999,8.0]
var failed := false
var reduction_code := ""
var source_path := ""
var output_path := ""
var checks := 0
func _ready() -> void:_run.call_deferred()
func _check(condition: bool, message: String) -> void:
    checks+=1
    if not condition:
        failed=true
        push_error(message)
func _float32(value: float) -> float:return PackedFloat32Array([value])[0]
func _alpha_reference(count: int, intensity: float) -> int:
    var original := _float32(intensity)
    var alpha: float
    if original<0.0:alpha=_float32(-original*128.0)
    else:
        var coverage := _float32(float(count)*_float32(1.0/1024.0))
        alpha=_float32(_float32(coverage*original)*128.0)
    return int(alpha)&255
func _target(size: Vector2i, code: String) -> Dictionary:
    var viewport := SubViewport.new()
    viewport.size=size
    viewport.disable_3d=true
    viewport.transparent_bg=true
    viewport.render_target_update_mode=SubViewport.UPDATE_DISABLED
    add_child(viewport)
    var rect := ColorRect.new()
    rect.size=Vector2(size)
    viewport.add_child(rect)
    var shader := Shader.new()
    shader.code=code
    var material := ShaderMaterial.new()
    material.shader=shader
    rect.material=material
    return {"viewport":viewport,"rect":rect,"material":material}
func _render_target(target: Dictionary) -> Image:
    target.viewport.render_target_update_mode=SubViewport.UPDATE_ONCE
    await get_tree().process_frame
    await RenderingServer.frame_post_draw
    var image: Image=target.viewport.get_texture().get_image()
    image.convert(Image.FORMAT_RGBA8)
    return image
func _alpha_exhaustive() -> void:
    var code := "shader_type canvas_item;render_mode unshaded,blend_disabled;\n#include \"res://shaders/sun_counter.gdshaderinc\"\nuniform sampler2D counts : filter_nearest,repeat_disable;uniform float intensities[16];varying float alpha_byte;void vertex(){int count=hp2_sun_count(texelFetch(counts,ivec2(int(UV.x+0.5),0),0));alpha_byte=hp2_sun_alpha(count,intensities[int(UV.y+0.5)]);}void fragment(){COLOR=vec4(vec3(alpha_byte/255.0),1.0);}"
    var target := _target(Vector2i(2050,32),code)
    target.rect.hide()
    var packed := PackedByteArray()
    packed.resize(1025*4)
    for count in 1025:
        packed[count*4]=count&255
        packed[count*4+1]=count>>8
        packed[count*4+3]=255
    var texture := ImageTexture.create_from_image(Image.create_from_data(1025,1,false,Image.FORMAT_RGBA8,packed))
    target.material.set_shader_parameter("counts",texture)
    target.material.set_shader_parameter("intensities",PackedFloat32Array(INTENSITIES))
    var vertices := PackedVector3Array()
    var coordinates := PackedVector2Array()
    var indices := PackedInt32Array()
    for row in INTENSITIES.size():
        for count in 1025:
            var base := vertices.size()
            for offset in [Vector2(0,0),Vector2(2,0),Vector2(2,2),Vector2(0,2)]:
                vertices.append(Vector3(count*2+offset.x,row*2+offset.y,0))
                coordinates.append(Vector2(count,row))
            for index in [0,1,2,0,2,3]:indices.append(base+index)
    var arrays := []
    arrays.resize(Mesh.ARRAY_MAX)
    arrays[Mesh.ARRAY_VERTEX]=vertices
    arrays[Mesh.ARRAY_TEX_UV]=coordinates
    arrays[Mesh.ARRAY_INDEX]=indices
    var mesh := ArrayMesh.new()
    mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES,arrays)
    var node := MeshInstance2D.new()
    node.mesh=mesh
    node.material=target.material
    target.viewport.add_child(node)
    var image: Image=await _render_target(target)
    for row in INTENSITIES.size():
        for count in 1025:
            var actual := int(round(image.get_pixel(count*2+1,row*2+1).r*255.0))
            var expected := _alpha_reference(count,INTENSITIES[row])
            _check(actual==expected,"Sun vertex alpha mismatch count=%d intensity=%s actual=%d expected=%d"%[count,str(INTENSITIES[row]),actual,expected])
    target.viewport.queue_free()
func _depth_image(mode: int) -> Image:
    var bytes := PackedByteArray()
    bytes.resize(512*448*4)
    for y in 448:
        for x in 512:
            var raw := _raw_depth(x,y,mode)
            var index := (y*512+x)*4
            bytes[index]=raw&255
            bytes[index+1]=(raw>>8)&255
            bytes[index+2]=(raw>>16)&255
            bytes[index+3]=255
    return Image.create_from_data(512,448,false,Image.FORMAT_RGBA8,bytes)
func _raw_depth(x: int, y: int, mode: int) -> int:
    if mode==1:return 0
    if mode==2:return 2
    return [0,1,2,3,255,256,65536,16777215][(x+3*y)%8]
func _count_reference(origin: Vector2i, first: Vector2i, last: Vector2i, mode: int) -> int:
    var count := 0
    for y in range(first.y,last.y):
        for x in range(first.x,last.x):
            var point := origin+Vector2i(x,y)
            var raw := _raw_depth(point.x,point.y,mode) if point.x>=0 and point.y>=0 and point.x<512 and point.y<448 else 0
            if raw<2:count+=1
    return count
func _setup_reduction(target: Dictionary, texture: Texture2D, origin: Vector2i, first: Vector2i, last: Vector2i) -> void:
    target.material.set_shader_parameter("raw_depth",texture)
    target.material.set_shader_parameter("source_origin",origin)
    target.material.set_shader_parameter("clip_start",first)
    target.material.set_shader_parameter("clip_end",last)
func _check_count(image: Image, expected: int, label: String) -> void:
    var bytes := image.get_data()
    for pixel in 4:
        var actual := int(bytes[pixel*4])+(int(bytes[pixel*4+1])<<8)
        _check(actual==expected,"Sun reduction %s pixel=%d count=%d expected=%d"%[label,pixel,actual,expected])
func _reduction_cases() -> void:
    var target := _target(Vector2i(2,2),reduction_code)
    var pattern := ImageTexture.create_from_image(_depth_image(0))
    var cases := [
        [Vector2i(240,200),Vector2i.ZERO,Vector2i(32,32)],
        [Vector2i.ZERO,Vector2i.ZERO,Vector2i(32,32)],
        [Vector2i(479,415),Vector2i(17,17),Vector2i(32,32)],
        [Vector2i.ZERO,Vector2i.ZERO,Vector2i(16,16)],
        [Vector2i(500,440),Vector2i.ZERO,Vector2i(32,32)],
        [Vector2i(-16,-16),Vector2i.ZERO,Vector2i(32,32)],
        [Vector2i(512,448),Vector2i.ZERO,Vector2i(32,32)],
        [Vector2i(240,200),Vector2i(16,16),Vector2i(16,16)]
    ]
    for index in cases.size():
        var item: Array=cases[index]
        _setup_reduction(target,pattern,item[0],item[1],item[2])
        var image: Image=await _render_target(target)
        _check_count(image,_count_reference(item[0],item[1],item[2],0),"case"+str(index))
    for mode in [1,2]:
        pattern.update(_depth_image(mode))
        _setup_reduction(target,pattern,Vector2i(200,200),Vector2i.ZERO,Vector2i(32,32))
        var image: Image=await _render_target(target)
        _check_count(image,1024 if mode==1 else 0,"threshold_mode"+str(mode))
    for producer in [0,1,65535,65536,0x12345678,0x7fffffff]:
        target.material.set_shader_parameter("producer_frame",producer)
        var stamped: Image=await _render_target(target)
        var bytes := stamped.get_data()
        var decoded := int(bytes[2])|(int(bytes[3])<<8)|(int(bytes[6])<<16)|(int(bytes[7])<<24)
        _check(decoded==producer,"GPU Sun producer frame payload mismatch")
        for byte in 8:_check(bytes[byte]==bytes[byte+8],"GPU Sun producer payload row mismatch")
    target.viewport.queue_free()
func _pingpong() -> void:
    var buffers := [_target(Vector2i(2,2),reduction_code),_target(Vector2i(2,2),reduction_code)]
    var code := "shader_type canvas_item;render_mode unshaded,blend_disabled;\n#include \"res://shaders/sun_counter.gdshaderinc\"\nuniform sampler2D previous_count:filter_nearest,repeat_disable;varying float a;void vertex(){a=hp2_sun_alpha(hp2_sun_count(textureLod(previous_count,vec2(0.5),0.0)),1.0);}void fragment(){COLOR=vec4(vec3(a/255.0),1.0);}"
    var consumer := _target(Vector2i(2,2),code)
    var zero := Image.create(2,2,false,Image.FORMAT_RGBA8)
    zero.fill(Color(0,0,0,1))
    var previous: Texture2D=ImageTexture.create_from_image(zero)
    var pattern := ImageTexture.create_from_image(_depth_image(0))
    var previous_count := 0
    var next_buffer := 0
    # Initial0; alternating outputs1024,0,256; hold when no request; resume.
    for stage in [-1,1,2,0,-1,1,2]:
        consumer.material.set_shader_parameter("previous_count",previous)
        var write: Dictionary={}
        var expected_new := previous_count
        if stage>=0:
            pattern.update(_depth_image(stage))
            write=buffers[next_buffer]
            _setup_reduction(write,pattern,Vector2i(200,200),Vector2i.ZERO,Vector2i(32,32))
            write.viewport.render_target_update_mode=SubViewport.UPDATE_ONCE
            expected_new=_count_reference(Vector2i(200,200),Vector2i.ZERO,Vector2i(32,32),stage)
        var consumed: Image=await _render_target(consumer)
        var actual := int(round(consumed.get_pixel(0,0).r*255.0))
        _check(actual==_alpha_reference(previous_count,1.0),"Pingpong consumer lost previous completed count stage="+str(stage))
        if not write.is_empty():
            var generated: Image=write.viewport.get_texture().get_image()
            generated.convert(Image.FORMAT_RGBA8)
            _check_count(generated,expected_new,"pingpong")
            previous=write.viewport.get_texture()
            previous_count=expected_new
            next_buffer=1-next_buffer
    for target in buffers:target.viewport.queue_free()
    consumer.viewport.queue_free()
func _run() -> void:
    if DisplayServer.get_name()=="headless":
        print("TASK0072_SUN_COUNTER_SKIPPED graphical device required")
        get_tree().quit(77)
        return
    for argument in OS.get_cmdline_user_args():
        if argument.begins_with("--sun-counter-source="):source_path=argument.trim_prefix("--sun-counter-source=")
        if argument.begins_with("--sun-counter-output="):output_path=argument.trim_prefix("--sun-counter-output=")
    if source_path.is_empty():source_path=ProjectSettings.globalize_path("res://../src/platform/godot_bridge/track_sun_renderer.cpp")
    if not FileAccess.file_exists(source_path):
        push_error("Supply --sun-counter-source=<production track_sun_renderer.cpp>; fixture will not duplicate reduction code")
        get_tree().quit(77)
        return
    var expression := RegEx.new()
    expression.compile('sun_reduce_shader\\s*=\\s*R"\\(([\\s\\S]*?)\\)";')
    var found := expression.search(FileAccess.get_file_as_string(source_path))
    if found==null:
        push_error("Production sun_reduce_shader raw string not found")
        get_tree().quit(1)
        return
    reduction_code=found.get_string(1)
    var debug := get_node_or_null("/root/DebugTools")
    if debug:debug.hide()
    await _alpha_exhaustive()
    await _reduction_cases()
    await _pingpong()
    var report := {"ok":not failed,"checks":checks,"backend":RenderingServer.get_current_rendering_driver_name(),"method":RenderingServer.get_current_rendering_method(),"engine":Engine.get_version_info(),"production_cpp_sha256":FileAccess.get_sha256(source_path),"shared_alpha_sha256":FileAccess.get_sha256("res://shaders/sun_counter.gdshaderinc"),"notes":"Synthetic GPU transport cases; no original equivalence claim; fixture-owned pingpong scheduling does not prove native renderer scheduling."}
    if not output_path.is_empty():
        DirAccess.make_dir_recursive_absolute(output_path.get_base_dir())
        var file := FileAccess.open(output_path,FileAccess.WRITE)
        if file:file.store_string(JSON.stringify(report,"  "))
        else:_check(false,"Cannot write Sun counter report")
    print("TASK0072_SUN_COUNTER_GPU_", "FAILED" if failed else "OK", " ",JSON.stringify(report))
    get_tree().quit(1 if failed else 0)

