extends "res://scripts/debug/track_smoke.gd"
# TASK0058 one-track viewer and Test drive integration.
func _run() -> void:
 var viewer: Node=get_parent()
 if not await _wait(viewer) or not viewer.track_state.get("ok",false):
  _fail("Track11 viewer load failed");return
 if int(viewer.track_state.track_id)!=11:
  _fail("Expected only representative Track11");return
 for enabled in [true,false]:
  viewer.viewer_ui_action({"action":"show_lod","enabled":enabled})
  await get_tree().process_frame
  if viewer.track_model.get_state().show_lod!=enabled:
   _fail("Viewer Show LOD action chain failed");return
 viewer.viewer_ui_action({"action":"track_drive"})
 await get_tree().process_frame
 if not viewer.driving.engaged() or not viewer.driving.state.get("real_track",false):
  _fail("Track11 Test drive entry failed: "+viewer.driving.error);return
 for i in 8:await get_tree().process_frame
 var lod: Dictionary=viewer.track_model.get_lod_state()
 if lod.backend!="original" or lod.host_lod_selection or not lod.native_culling_views[0].native_selection:
  _fail("Test drive bypassed original selection");return
 viewer.driving.stop()
 await get_tree().process_frame
 if not viewer.track_state.get("ok",false):
  _fail("Return to TrackViewer failed");return
 DebugTools.request("clear")
 if not await _wait(viewer) or viewer.track_model.get_state().get("ok",false):
  _fail("Clear retained track");return
 print("TRACK_NATIVE_VIEWER_DRIVE_OK")
 get_tree().quit(0)
