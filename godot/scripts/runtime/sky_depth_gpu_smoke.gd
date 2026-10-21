extends Node3D
func _ready(): run.call_deferred()
func shader(source:String,fragment:String,transparent:bool)->ShaderMaterial:
 var s=Shader.new()
 s.code="shader_type spatial;render_mode unshaded,cull_disabled,fog_disabled,shadows_disabled,"+("depth_draw_never,blend_mix" if transparent else "depth_draw_opaque")+";void vertex(){"+source+"}void fragment(){"+fragment+"}"
 var m=ShaderMaterial.new();m.shader=s;return m
func quad(vp:SubViewport,size:Vector2,position:Vector3,material:Material)->MeshInstance3D:
 var node=MeshInstance3D.new();var mesh=QuadMesh.new();mesh.size=size;node.mesh=mesh;node.position=position;node.material_override=material;vp.add_child(node);return node
func run():
 if DisplayServer.get_name()=="headless":get_tree().quit(77);return
 var text=FileAccess.get_file_as_string("res://../src/platform/shaders/sky_depth.hpp")
 var start=text.find('R"glsl(');var end=text.find(')glsl"',start)
 if start<0 or end<0:push_error("Production sky clip adapter unavailable");get_tree().quit(1);return
 var code=text.substr(start+7,end-start-7)
 var vp=SubViewport.new();vp.size=Vector2i(64,32);vp.own_world_3d=true;vp.render_target_update_mode=SubViewport.UPDATE_ALWAYS;add_child(vp)
 var camera=Camera3D.new();camera.projection=Camera3D.PROJECTION_ORTHOGONAL;camera.keep_aspect=Camera3D.KEEP_WIDTH;camera.size=4;camera.position.z=3;vp.add_child(camera);camera.current=true
 var opaque=shader("","ALBEDO=vec3(1,0,0);",false)
 var transparent=shader("","ALBEDO=vec3(0,1,0);ALPHA=1.0;",true);transparent.render_priority=0
 quad(vp,Vector2(.8,.8),Vector3(-1,0,.1),opaque)
 quad(vp,Vector2(.8,.8),Vector3(1,0,.1),transparent)
 var background=quad(vp,Vector2(4,2),Vector3.ZERO,null)
 for negative in [true,false]:
  var active=code
  if negative:active=active.replace("CLIP_SPACE_FAR*POSITION.w","POSITION.w")
  var mat=shader(active,"ALBEDO=vec3(0,0,1);ALPHA=1.0;",true);mat.render_priority=-128;background.material_override=mat
  await RenderingServer.frame_post_draw
  await RenderingServer.frame_post_draw
  var image=vp.get_texture().get_image();var left=image.get_pixel(16,16);var right=image.get_pixel(48,16);var empty=image.get_pixel(32,16)
  print("SKY_DEPTH_GPU negative=",negative," opaque=",left," transparent=",right," background=",empty)
  var correct=left.r>.99 and left.g<.01 and left.b<.01 and right.g>.99 and right.r<.01 and right.b<.01 and empty.b>.99
  if correct==negative:push_error("Sky depth did not preserve foreground or negative control failed");get_tree().quit(1);return
 print("SKY_DEPTH_GPU_OK");get_tree().quit(0)
