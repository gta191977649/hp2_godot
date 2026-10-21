extends Node3D
# TASK0055 asset-free GPU buffer transport/parity test, not PS2 certification.
func _ready(): _run.call_deferred()
func _fail(reason:String):push_error(reason);get_tree().quit(1)
func _run():
 if DisplayServer.get_name()=="headless":get_tree().quit(77);return
 if not ClassDB.class_has_method("Ps2TrackRenderer","test_particle_mesh_reuse"):_fail("Particle reuse test binding missing");return
 var fixture:Dictionary=ClassDB.class_call_static("Ps2TrackRenderer","test_particle_mesh_reuse")
 if not fixture.get("ok",false):_fail(str(fixture.get("error","Particle mesh fixture failed")));return
 var vp=SubViewport.new();vp.use_hdr_2d=RenderingServer.get_current_rendering_method()!="gl_compatibility";vp.size=Vector2i(512,448);vp.own_world_3d=true;vp.render_target_update_mode=SubViewport.UPDATE_ALWAYS;add_child(vp)
 var camera=Camera3D.new();camera.position.z=3;vp.add_child(camera);camera.current=true
 var environment=Environment.new();environment.background_mode=Environment.BG_COLOR;environment.background_color=Color.BLACK;camera.environment=environment
 var node=MeshInstance3D.new();vp.add_child(node);node.custom_aabb=AABB(Vector3(-100,-100,-100),Vector3(200,200,200))
 for depth_mode in [false,true]:
  var shader=Shader.new()
  var fragment="ALBEDO=clamp(vec3(UV,0.5),vec3(0.0),vec3(1.0))*COLOR.rgb;ALPHA=COLOR.a;DEPTH=0.5;"
  if depth_mode:fragment="uint z=uint(packet_z);ALBEDO=vec3(float(z&255u),float((z>>8u)&255u),float((z>>16u)&255u))/255.0;ALPHA=1.0;DEPTH=0.5;"
  shader.code="shader_type spatial;render_mode unshaded,cull_disabled,fog_disabled,shadows_disabled,depth_draw_always,blend_mix;varying flat float packet_z;"+str(fixture.screen_position_shader)+"void vertex(){packet_z=VERTEX.z;POSITION=hp2_host_screen_position((VERTEX.xy/16.0-vec2(1792.0,1824.0))/vec2(512.0,448.0),PROJECTION_MATRIX[1][1]);}void fragment(){"+fragment+"}"
  var material=ShaderMaterial.new();material.shader=shader;node.material_override=material
  var reference:PackedByteArray
  for reused in [false,true]:
   node.mesh=fixture.reused if reused else fixture.baseline
   await RenderingServer.frame_post_draw
   await RenderingServer.frame_post_draw
   var image=vp.get_texture().get_image();image.convert(Image.FORMAT_RGBA8)
   var bytes=image.get_data()
   if not reused:
    reference=bytes
    var nonempty=0
    for offset in range(0,bytes.size(),4):
     if bytes[offset]!=0 or bytes[offset+1]!=0 or bytes[offset+2]!=0:nonempty+=1
    if nonempty<500 or nonempty>1100:_fail("Particle fixture did not rasterize expected original quad");return
   elif reference!=bytes:_fail("Particle capacity update changed GPU colour/depth output or added inactive quad pixels");return
 print("PARTICLE_MESH_REUSE_GPU_OK capacity=",fixture.capacity_quads," active=",fixture.active_quads," raw-layout/all256-colours/grow/shrink/colour/depth")
 get_tree().quit(0)
