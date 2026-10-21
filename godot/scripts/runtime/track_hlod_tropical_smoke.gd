extends "res://scripts/runtime/track_hlod_smoke.gd"
# E0056T01: original default TextureInfo binding, not generated placeholder pixels.
func _run() -> void:
 var overlay := root.get_node_or_null("DebugTools")
 if overlay:overlay.hide()
 root.size=Vector2i(1280,720)
 world=Node3D.new()
 root.add_child(world)
 camera=Camera3D.new()
 camera.keep_aspect=Camera3D.KEEP_WIDTH
 camera.fov=float(0x32dc)*360/65536
 world.add_child(camera)
 camera.current=true
 renderer=ClassDB.instantiate("Ps2TrackRenderer")
 world.add_child(renderer)
 if not _check(renderer.configure_lod_backend("godot_hlod").ok,"Tropical HLOD setup failed"):return
 if not await _load(61):return
 var state: Dictionary=renderer.get_lod_state()
 if not _check(int(state.original_texture_fixups)==1,"Expected original missing-name/default binding"):return
 var matched: MeshInstance3D
 for node in renderer.get_children():
  if node is MeshInstance3D and int(node.get_meta("track_texture_hash",0))==0x19006835:
   matched=node
   break
 if not _check(matched!=null,"Original DEFAULTTEXTURE not submitted"):return
 var material: ShaderMaterial=matched.material_override
 var texture: Texture2D=material.get_shader_parameter("base_texture")
 if not _check(texture!=null and texture.get_width()==32 and texture.get_height()==32,"Default bitmap did not come from original32-square resource"):return
 var binding: Dictionary=renderer.get_lod_ranges(matched.get_meta("hp2_hlod_instance"),0)
 var target: Vector3=binding.anchor+Vector3.UP
 for offset in [-1.0,0.0,1.0]:
  camera.position=target+Vector3(-8+offset,3,-5)
  camera.look_at(target)
  var frame: Dictionary=renderer.update_view(camera.position)
  if not _check(frame.ok,"Tropical frame failed: "+str(frame.get("error"))):return
  await _frames(3)
 var output := "user://track-hlod-tropical.png"
 for argument in OS.get_cmdline_user_args():
  if argument.begins_with("--hlod-tropical-capture="):output=argument.trim_prefix("--hlod-tropical-capture=")
 root.get_texture().get_image().save_png(output)
 print("TRACK_HLOD_TROPICAL_GPU_OK ",renderer.get_lod_state())
 renderer.clear_track()
 quit(0)
