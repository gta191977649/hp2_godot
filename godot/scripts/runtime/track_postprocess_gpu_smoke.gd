extends Node3D
# TASK0052: byte-domain post packets on real PC GPU; no original-runtime claim.
func _material(code: String) -> ShaderMaterial:
 var shader := Shader.new()
 shader.code=code
 var material := ShaderMaterial.new()
 material.shader=shader
 return material
func _image(colour: Color, width: int=1) -> ImageTexture:
 var image := Image.create(width,1,false,Image.FORMAT_RGBA8)
 image.fill(colour)
 return ImageTexture.create_from_image(image)
func _pixel(viewport: SubViewport, point: Vector2i) -> Vector4i:
 var c := viewport.get_texture().get_image().get_pixelv(point)
 return Vector4i(roundi(c.r*255),roundi(c.g*255),roundi(c.b*255),roundi(c.a*255))
func _frames() -> void:
 for i in 3:
  await get_tree().process_frame
  await RenderingServer.frame_post_draw
func _fail(reason: String) -> void:
 push_error(reason)
 get_tree().quit(1)
func _ready() -> void:
 _run.call_deferred()
func _run() -> void:
 if DisplayServer.get_name()=="headless":
  get_tree().quit(77)
  return
 var overlay := get_node_or_null("/root/DebugTools")
 if overlay:overlay.hide()
 var code := FileAccess.get_file_as_string(ProjectSettings.globalize_path("res://../src/platform/shaders/postprocess_composite.glsl.inc")).strip_edges()
 if not code.begins_with('R"HP2(') or not code.ends_with(')HP2"'):
  _fail("Original postprocess shader source unavailable")
  return
 code=code.substr(6,code.length()-11)
 var target := SubViewport.new()
 target.size=Vector2i(64,64)
 target.transparent_bg=true
 target.disable_3d=true
 target.render_target_update_mode=SubViewport.UPDATE_ALWAYS
 add_child(target)
 var rect := TextureRect.new()
 rect.size=Vector2(64,64)
 rect.texture=_image(Color(128.0/255,0,0,1))
 var material := _material(code)
 rect.material=material
 target.add_child(rect)
 material.set_shader_parameter("colour_texture",rect.texture)
 # XYZF2 rawZ24=01FE77, independently known for viewZ2,near.5,far500.
 material.set_shader_parameter("encoded_depth",_image(Color(119.0/255,254.0/255,1.0/255,1)))
 material.set_shader_parameter("fog_palette",_image(Color(0,0,1,64.0/255),256))
 material.set_shader_parameter("focus_palette",_image(Color(128.0/255,128.0/255,128.0/255,128.0/255),256))
 material.set_shader_parameter("fog_enabled",true)
 await _frames()
 if _pixel(target,Vector2i(32,32))!=Vector4i(64,0,127,64):
  _fail("Fog GS byte blend mismatch: "+str(_pixel(target,Vector2i(32,32))))
  return
 material.set_shader_parameter("focus_enabled",true)
 await _frames()
 if _pixel(target,Vector2i(32,32))!=Vector4i(64,0,126,128):
  _fail("Focus jitter per-pass truncation mismatch")
  return
 material.set_shader_parameter("focus_enabled",false)
 material.set_shader_parameter("fog_enabled",false)
 await _frames()
 if _pixel(target,Vector2i(32,32)).w!=254:
  _fail("MainDraw depth byte must remain wrapped while MainDisp LUT index saturates")
  return
 material.set_shader_parameter("filter_enabled",true)
 material.set_shader_parameter("filter_texture",_image(Color(0,0,1,64.0/255)))
 material.set_shader_parameter("filter_colour",Vector4(128,128,128,128))
 await _frames()
 if _pixel(target,Vector2i(32,32))!=Vector4i(64,0,127,64):
  _fail("Original filter ALPHA equation mismatch")
  return
 material.set_shader_parameter("filter_enabled",false)
 var palette := Image.create(256,1,false,Image.FORMAT_RGBA8)
 for i in 256:palette.set_pixel(i,0,Color(float(i)/255,float(i)/255,float(i)/255,128.0/255))
 material.set_shader_parameter("black_white_palette",ImageTexture.create_from_image(palette))
 material.set_shader_parameter("black_white_enabled",true)
 material.set_shader_parameter("depth_into_alpha_enabled",false)
 material.set_shader_parameter("history_available",true)
 material.set_shader_parameter("previous_texture",_image(Color(0,128.0/255,0,129.0/255)))
 material.set_shader_parameter("colour_texture",_image(Color(0,170.0/255,0,1)))
 await _frames()
 if _pixel(target,Vector2i(32,32))!=Vector4i(171,171,171,128):
  _fail("Original BW green/high7 plus historical alpha LSB mismatch")
  return
 material.set_shader_parameter("previous_texture",_image(Color(0,128.0/255,0,128.0/255)))
 material.set_shader_parameter("depth_into_alpha_enabled",true)
 await _frames()
 if _pixel(target,Vector2i(32,32))!=Vector4i(171,171,171,128):
  _fail("BW must preserve current MainDisp depth LSB when depth attachment ran")
  return
 material.set_shader_parameter("depth_into_alpha_enabled",false)
 await _frames()
 if _pixel(target,Vector2i(32,32))!=Vector4i(170,170,170,128):
  _fail("BW must use stored MainDisp alpha only when depth attachment is disabled")
  return
 material.set_shader_parameter("black_white_enabled",false)
 material.set_shader_parameter("depth_into_alpha_enabled",true)
 material.set_shader_parameter("colour_texture",rect.texture)
 material.set_shader_parameter("radial_enabled",true)
 material.set_shader_parameter("previous_enabled",true)
 material.set_shader_parameter("radial_texture",_image(Color(0,0,0,128.0/255)))
 await _frames()
 var edge := _pixel(target,Vector2i(1,1))
 if edge.x!=0 or edge.y!=192 or edge.z!=0 or edge.w!=255:
  _fail("Original strip-diagonal/high-Ad history blend mismatch: "+str(edge))
  return
 print("TRACK_POSTPROCESS_GPU_OK fog/focus/filter/BW/history; host GPU only")
 get_tree().quit(0)
