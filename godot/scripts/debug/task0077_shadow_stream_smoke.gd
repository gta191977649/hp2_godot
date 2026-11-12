extends SceneTree
## Asset-free host upload and raster comparison for TASK-0077.

func _initialize() -> void:
    _run.call_deferred()

func _fail(message: String) -> void:
    push_error(message)
    quit(1)

func _run() -> void:
    if DisplayServer.get_name()=="headless":
        quit(77)
        return
    var fixture: Dictionary=ClassDB.class_call_static("Ps2TrackRenderer","test_shadow_mesh_stream")
    if not fixture.get("ok",false):
        _fail("Shadow stream transport failed: %s" % fixture)
        return

    var view := SubViewport.new()
    view.size=Vector2i(160,120)
    view.own_world_3d=true
    view.render_target_update_mode=SubViewport.UPDATE_ALWAYS
    root.add_child(view)

    var camera := Camera3D.new()
    camera.position.z=3.0
    var environment := Environment.new()
    environment.background_mode=Environment.BG_COLOR
    environment.background_color=Color.BLACK
    camera.environment=environment
    view.add_child(camera)
    camera.make_current()

    var node := MeshInstance3D.new()
    var shader := Shader.new()
    shader.code="""shader_type spatial;
render_mode unshaded, cull_disabled;
void fragment() {
    ALBEDO = COLOR.rgb * vec3(UV, 1.0);
    ALPHA = COLOR.a;
}
"""
    var material := ShaderMaterial.new()
    material.shader=shader
    node.material_override=material
    view.add_child(node)
    node.mesh=fixture.baseline
    await RenderingServer.frame_post_draw
    await RenderingServer.frame_post_draw
    var reference := view.get_texture().get_image()
    node.mesh=fixture.updated
    await RenderingServer.frame_post_draw
    await RenderingServer.frame_post_draw
    var candidate := view.get_texture().get_image()
    if reference.get_pixel(80,60).b<0.01 or reference.get_data()!=candidate.get_data():
        _fail("Shadow stream raster was empty or differed from the rebuilt strip")
        return
    print("TASK0077_SHADOW_STREAM_GPU_OK unchanged/position/attributes/indices/bounds/raster")
    quit()
