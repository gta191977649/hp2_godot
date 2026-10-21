extends SceneTree
# Pure host contract tests: no viewer/native physics instantiated and no GPU needed.
var failures: Array[String] = []
func _initialize() -> void:
 _run.call_deferred()
func _expect(value: bool, message: String) -> void:
 if not value: failures.append(message)
func _native() -> Dictionary:
 return {"ok":true,"paused":false,"status":"Running","body_known":true,"wheels_known":true,"step":11,"dt_known":true,"dt":1.0/60.0,"body":Transform3D.IDENTITY}
func _run() -> void:
 var viewer=load("res://scripts/runtime/track_performance_viewer.gd")
 var benchmark=load("res://scripts/runtime/track_performance.gd")
 if viewer==null or benchmark==null:
  push_error("Driving harness scripts did not compile")
  quit(1)
  return
 var instance=viewer.new()
 instance.benchmark_context="driving"
 instance.focused=true
 instance._notification(Node.NOTIFICATION_APPLICATION_FOCUS_OUT)
 _expect(instance.focused,"Inherited notification bypassed deterministic driving lifecycle hook")
 instance.free()
 var adapter=load("res://scripts/debug/driving_input.gd").new()
 var event := InputEventAction.new()
 event.action="hp2_drive_throttle"
 event.pressed=true
 event.strength=0.35
 adapter.handle_input(event,false)
 var controls: Dictionary=adapter.frame()
 _expect(is_equal_approx(float(controls.throttle),0.35) and float(controls.steering)==0,"Semantic throttle/steering differs")
 _expect(viewer.driving_frame_error(10,_native(),controls,1).is_empty(),"Valid single production step rejected")
 var held := _native()
 held.step=10
 _expect(viewer.driving_frame_error(10,held,controls,1).is_empty(),"Original minimum-frame-elapsed scheduler hold rejected")
 for calls in [0,2]:
  _expect(not viewer.driving_frame_error(10,held,controls,calls).is_empty(),"Missing/multiple native tick calls accepted on held step")
  _expect(not viewer.driving_frame_error(10,_native(),controls,calls).is_empty(),"Missing/multiple native tick calls accepted on advanced step")
 # Invalid physics/lifecycle cannot masquerade as a timed valid route.
 for defect in ["paused","blocked","failed","body","wheels","backwards_step","double_step","dt","nonfinite_dt","missing_transform"]:
  var native := _native()
  match defect:
   "paused": native.paused=true
   "blocked": native.status="Blocked"
   "failed": native.status="Failed"
   "body": native.body_known=false
   "wheels": native.wheels_known=false
   "backwards_step": native.step=9
   "double_step": native.step=12
   "dt": native.dt=0
   "nonfinite_dt": native.dt=NAN
   "missing_transform": native.erase("body")
  _expect(not viewer.driving_frame_error(10,native,controls,1).is_empty(),"Invalid driving evidence accepted: "+defect)
 adapter.clear()
 _expect(not viewer.driving_frame_error(10,_native(),adapter.frame(),1).is_empty(),"Cleared semantic throttle accepted")
 var changed := controls.duplicate()
 changed.steering=0.2
 _expect(not viewer.driving_frame_error(10,_native(),changed,1).is_empty(),"External steering contamination accepted")
 var delta: Dictionary=benchmark.pipeline_delta({"canvas":1,"mesh":3},{"canvas":1,"mesh":5})
 _expect(delta.status=="monotonic_engine_counters" and delta.delta=={"canvas":0,"mesh":2},"Pipeline deltas differ")
 _expect(benchmark.pipeline_delta({"mesh":3},{"mesh":2}).delta==null,"Counter reset reported as compilation saving")
 _expect(benchmark.pipeline_delta({"mesh":3},{}).delta==null,"Unavailable counter reported as zero")
 var route_summary := {}
 benchmark.accumulate_driving_evidence(route_summary,{"position_host":[1,2,3],"speed":4,"wheels":[{"contact":true,"terrain":1},{"contact":true,"terrain":1},{"contact":true,"terrain":1},{"contact":true,"terrain":1}]})
 benchmark.accumulate_driving_evidence(route_summary,{"position_host":[5,-2,9],"speed":8,"wheels":[{"contact":false},{"contact":false},{"contact":false},{"contact":false}]})
 benchmark.accumulate_driving_evidence(route_summary,{"position_host":[0,0,4],"speed":null,"wheels":[]})
 _expect(route_summary.position_min_host==[0.0,-2.0,3.0] and route_summary.position_max_host==[5.0,2.0,9.0],"Driving position range differs: "+str(route_summary))
 _expect(route_summary.zero_known_contacts_frames==1 and route_summary.unknown_contact_frames==1 and route_summary.all_contacts_known_frames==2,"Unknown contacts conflated with no road contact")
 _expect(route_summary.speed_min==4 and route_summary.speed_max==8 and route_summary.terrain_samples.get("1")==4,"Raw speed/terrain summary differs")
 if not failures.is_empty():
  for failure in failures: push_error(failure)
  quit(1)
  return
 print("TASK0072_DRIVING_HARNESS_CONTRACT_OK semantic input, single tick invocation, legitimate scheduler hold, failure and pipeline delta rejection; route not GPU validated")
 quit(0)
