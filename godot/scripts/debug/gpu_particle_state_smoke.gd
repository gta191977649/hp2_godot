extends SceneTree
# TASK0085: actual production process shader, test-only GPU AABB readback.
var owner: Node3D
var shader: Shader

func _initialize() -> void:
 _run.call_deferred()

func find_gpu(node: Node) -> GPUParticles3D:
 if node is GPUParticles3D:return node
 for child in node.get_children():
  var found := find_gpu(child)
  if found:return found
 return null

func settle(count: int = 5) -> void:
 for frame in range(count):await RenderingServer.frame_post_draw

func make_probe(state_projection: bool = false) -> GPUParticles3D:
 var p := GPUParticles3D.new()
 p.emitting=false
 p.amount=1000
 p.lifetime=60
 p.speed_scale=0
 p.fixed_fps=0
 p.interpolate=false
 p.fract_delta=false
 p.local_coords=true
 p.draw_pass_1=QuadMesh.new()
 p.visibility_aabb=AABB(Vector3(-100000,-100000,-100000),Vector3(200000,200000,200000))
 var material := ShaderMaterial.new()
 material.shader=shader
 if state_projection:
  var projected := Shader.new()
  var where := shader.code.rfind("}")
  projected.code=shader.code.insert(where,"TRANSFORM[3].xyz=vec3(CUSTOM.x,CUSTOM.z,CUSTOM.w);\n")
  material.shader=projected
 material.set_shader_parameter("gravity",4.0 if not state_projection else 0.0)
 material.set_shader_parameter("drag",0.1 if not state_projection else 0.0)
 material.set_shader_parameter("inherit_enabled",not state_projection)
 material.set_shader_parameter("inherited",Vector3(.5,1,2))
 material.set_shader_parameter("interval",256 if state_projection else 0)
 material.set_shader_parameter("frame_mask",15)
 p.process_material=material
 root.add_child(p)
 return p

func close(actual: Vector3, expected: Vector3, label: String) -> bool:
 if actual.distance_to(expected)>0.0001:
  printerr("GPU_STATE_FAIL ",label," actual=",actual," expected=",expected)
  return false
 print("GPU_STATE ",label," ",actual)
 return true

func _run() -> void:
 var args := OS.get_cmdline_user_args()
 if not args.has("--particle-backend=godot_gpu"):
  printerr("Requires --particle-backend=godot_gpu")
  quit(1)
  return
 owner=Node3D.new()
 root.add_child(owner)
 var camera := Camera3D.new()
 owner.add_child(camera)
 camera.position=Vector3(5,4,-8)
 camera.look_at(Vector3.ZERO)
 camera.current=true
 var driving: RefCounted=ClassDB.instantiate("Ps2DrivingRuntime")
 var loaded: Dictionary=driving.load_config("911TURBO")
 if not loaded.get("ok",false):printerr(loaded);quit(1);return
 driving.action("run")
 for frame in range(300):
  driving.set_driving_input(1.0,0.0,1.0,0.0)
  var state: Dictionary=driving.tick(1.0/60.0)
  if not state.get("ok",false):printerr(state);quit(1);return
  var result: Dictionary=driving.present_particles(owner,camera)
  if not result.get("ok",false):printerr(result);quit(1);return
  await settle(1)
  var gpu := find_gpu(owner)
  if gpu:
   shader=(gpu.process_material as ShaderMaterial).shader
   break
 if not shader:printerr("No production GPU particle node");quit(1);return
 owner.hide()
 var okay := true
 for projected in [false,true]:
  var p := make_probe(projected)
  await settle()
  var material := p.process_material as ShaderMaterial
  p.hide()
  p.emit_particle(Transform3D(Basis(),Vector3(1,2,3)),Vector3(1,2,3),Color.WHITE,Color(10240,57,32771 if projected else 32768,0),31)
  material.set_shader_parameter("epoch",1)
  material.set_shader_parameter("timestep",.25)
  RenderingServer.particles_request_process(p.get_base())
  await settle()
  var first := p.capture_aabb().position+Vector3.ONE
  okay=close(first,Vector3(9984,32772,1) if projected else Vector3(1.306495,2.600490,3.694730),"state1" if projected else "motion1") and okay
  material.set_shader_parameter("epoch",2)
  RenderingServer.particles_request_process(p.get_base())
  await settle()
  var second := p.capture_aabb().position+Vector3.ONE
  okay=close(second,Vector3(9728,32773,2) if projected else Vector3(1.603689,3.173455,4.118376),"state2" if projected else "motion2") and okay
  await settle(10)
  okay=close(p.capture_aabb().position+Vector3.ONE,second,"idle") and okay
  p.show()
  await settle()
  okay=close(p.capture_aabb().position+Vector3.ONE,second,"off_to_on") and okay
  p.queue_free()
 driving.clear_particles()
 print("GPU_PARTICLE_STATE_OK" if okay else "GPU_PARTICLE_STATE_FAIL")
 quit(0 if okay else 1)
