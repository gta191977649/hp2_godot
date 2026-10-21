extends SceneTree
# TASK0057 port-only feasibility probe; no generated result is applied to live meshes.
func _initialize():call_deferred("run")
func run():
 var overlay=root.get_node_or_null("DebugTools")
 if overlay:overlay.hide()
 print("GENERATOR_AVAILABLE ",ClassDB.class_exists("ImporterMesh"))
 var grid=PlaneMesh.new()
 grid.subdivide_width=30
 grid.subdivide_depth=30
 var reference=ImporterMesh.new()
 reference.add_surface(Mesh.PRIMITIVE_TRIANGLES,grid.get_mesh_arrays())
 reference.generate_lods(25,60,[])
 print("SYNTHETIC_GENERATOR_REFERENCE lod_count=",reference.get_surface_lod_count(0))

 # Requires TASK0056 --lod-inventory output; private assets/results stay local.
 var folder=ProjectSettings.globalize_path("res://../local/task0057")
 DirAccess.make_dir_recursive_absolute(folder)
 var names={}
 var inventory=ProjectSettings.globalize_path("res://../local/task0056/lod-inventory-track11.log")
 for line in FileAccess.get_file_as_string(inventory).split("\n"):
  if not line.begins_with("LOD_INVENTORY"):continue
  var parts=line.split(" | ")
  for i in range(1,parts.size()):
   var pair=parts[i].split(":",true,1)
   names[str(pair[0].hex_to_int())]=pair[1].strip_edges()
 var world=Node3D.new()
 root.add_child(world)
 var camera=Camera3D.new()
 world.add_child(camera)
 camera.current=true
 var renderer=ClassDB.instantiate("Ps2TrackRenderer")
 world.add_child(renderer)
 var state: Dictionary=renderer.request_track(11)
 while state.get("loading",false):
  state=renderer.pump_load()
  await process_frame
 if not state.ok:push_error(str(state));quit(1);return
 var start: Vector3=state.start_position
 var forward: Vector3=state.start_forward
 camera.position=start-forward*8+Vector3.UP*3
 camera.look_at(start)
 state=renderer.update_view(camera.position)
 print("ORIGINAL_CULL ",state.get("lod")," triangles=",state.get("triangles"))
 renderer.set_show_lod(true)
 state=renderer.update_view(camera.position)
 var tested={}
 var result=[]
 for node in renderer.get_children():
  if not node is MeshInstance3D or not node.visible or not node.has_meta("hp2_hlod_instance"):continue
  var ranges: Dictionary=renderer.get_lod_ranges(node.get_meta("hp2_hlod_instance"),0)
  var name=""
  for level in ranges.get("levels",[]):
   if level.slot==node.get_meta("hp2_hlod_slot"):name=names.get(str(int(level.model_hash)),"")
  if not (name.begins_with("RD_SECTION") or name.begins_with("TRN_SECTION")):continue
  var mesh: ArrayMesh=node.mesh
  if tested.has(mesh.get_rid()):continue
  tested[mesh.get_rid()]=true
  var arrays=mesh.surface_get_arrays(0)
  var count=mesh.surface_get_array_index_len(0)
  if count<90:continue
  var importer=ImporterMesh.new()
  importer.add_surface(Mesh.PRIMITIVE_TRIANGLES,arrays,[],{},null,"",mesh.surface_get_format(0))
  var before=Time.get_ticks_usec()
  importer.generate_lods(25,60,[])
  var sizes=[]
  for i in importer.get_surface_lod_count(0):sizes.append(importer.get_surface_lod_size(0,i))
  var output=importer.get_mesh()
  var out_arrays=output.surface_get_arrays(0)
  var record={"name":name,"base_indices":count,"lod_count":sizes.size(),"errors":sizes,"ms":(Time.get_ticks_usec()-before)/1000.0,"same_color":arrays[Mesh.ARRAY_COLOR]==out_arrays[Mesh.ARRAY_COLOR],"same_custom0":arrays[Mesh.ARRAY_CUSTOM0]==out_arrays[Mesh.ARRAY_CUSTOM0]}
  var indices=[]
  for i in importer.get_surface_lod_count(0):indices.append(importer.get_surface_lod_indices(0,i).size())
  record["lod_indices"]=indices
  result.append(record)
  print("GENERATED_LOD ",record)
  if result.size()>=24:break
 FileAccess.open(folder.path_join("generator-results.json"),FileAccess.WRITE).store_string(JSON.stringify(result,"  "))
 renderer.clear_track()
 print("GENERATOR_PROBE_OK samples=",result.size())
 quit(0)
