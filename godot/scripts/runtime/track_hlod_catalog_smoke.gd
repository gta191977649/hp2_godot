extends SceneTree
# TASK0056 complete host upload/lifetime check; headless is not a raster comparison.
func _initialize() -> void:call_deferred("_run")
func _run() -> void:
 var world := Node3D.new()
 root.add_child(world)
 var camera := Camera3D.new()
 camera.keep_aspect=Camera3D.KEEP_WIDTH
 camera.fov=float(0x32dc)*360/65536
 world.add_child(camera)
 camera.current=true
 var renderer: Node3D=ClassDB.instantiate("Ps2TrackRenderer")
 world.add_child(renderer)
 if not renderer.configure_lod_backend("godot_hlod").ok:
  push_error("Catalogue HLOD configuration rejected")
  quit(1)
  return
 var baseline_nodes := int(Performance.get_monitor(Performance.OBJECT_NODE_COUNT))
 var failures := []
 var rows := []
 var catalog: Array=renderer.get_catalog()
 for entry in catalog:
  var before: int=renderer.get_lod_state().visual_frame_counter
  var state: Dictionary=renderer.request_track(entry.id)
  var deadline := Time.get_ticks_msec()+180000
  while state.get("loading",false) and Time.get_ticks_msec()<deadline:
   state=renderer.pump_load()
   await process_frame
  if state.get("ok",false):
   if int(renderer.get_lod_state().visual_frame_counter)!=before:
    failures.append({"track":entry.id,"error":"Warm-up changed visual frame counter"})
   var start: Vector3=state.start_position
   var forward: Vector3=state.start_forward
   camera.position=start-forward*8+Vector3.UP*3
   camera.look_at(start)
   state=renderer.update_view(camera.position)
  if not state.get("ok",false):failures.append({"track":entry.id,"error":state.get("error","Host load timeout")})
  var row := {"track":entry.id,"ok":state.get("ok",false),"lod":renderer.get_lod_state(),"error":state.get("error","")}
  renderer.clear_track()
  await process_frame
  await process_frame
  var nodes := int(Performance.get_monitor(Performance.OBJECT_NODE_COUNT))
  row.nodes_after_clear=nodes
  if nodes>baseline_nodes:
   failures.append({"track":entry.id,"error":"HLOD node leak","baseline":baseline_nodes,"nodes":nodes})
  rows.append(row)
  print("TRACK_HLOD_CATALOG_ENTRY ",JSON.stringify(row))
 var path := "user://track-hlod-catalog.json"
 for argument in OS.get_cmdline_user_args():
  if argument.begins_with("--hlod-catalog-output="):path=argument.trim_prefix("--hlod-catalog-output=")
 var file := FileAccess.open(path,FileAccess.WRITE)
 if file:file.store_string(JSON.stringify({"tracks":rows,"failures":failures,"baseline_nodes":baseline_nodes},"  "))
 if not failures.is_empty() or catalog.size()!=30:
  push_error("HLOD catalogue failed: "+str(failures))
  quit(1)
  return
 print("TRACK_HLOD_CATALOG_ALL30_OK")
 quit(0)

