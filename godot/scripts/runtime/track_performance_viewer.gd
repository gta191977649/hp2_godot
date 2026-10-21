extends "res://scripts/debug/asset_viewer.gd"
# Port benchmark adapter around the actual TrackViewer startup/UI/process path.
# Benchmark owns startup selection; production requests and rendering remain in use.
var benchmark_context := "viewer"
var benchmark_route := "drive"
var benchmark_frame_index := 0
var last_driving_frame: Dictionary = {}
var startup_driving_state: Dictionary = {}
var saved_debug_input := {}
var benchmark_start_allowed := false
var benchmark_viewport := Vector2i(1920,1080)
var benchmark_car_code := "CORVETTE"
var track_id := 11
var track_renderer: Node3D
var vehicle_renderer: Node3D
var phase := "Idle"
var diagnostic := ""

func _ready() -> void:
 viewer_kind="track"
 super._ready()
 # Benchmark input is deterministic: retain production drawing/UI while ignoring
 # external camera/control events for the complete benchmark lifetime.
 # No frame has yielded yet. Replace the automatic first-track startup request
 # with begin_session's explicit benchmark request through the same dispatcher.
 var debug_tools := get_node("/root/DebugTools")
 saved_debug_input={"input":debug_tools.is_processing_input(),"unhandled":debug_tools.is_processing_unhandled_input()}
 if benchmark_context=="driving":
  debug_tools.set_process_input(false)
  debug_tools.set_process_unhandled_input(false)
  focused=true
 if debug_tools.host!=null:
  var startup: Dictionary=debug_tools.host.viewer_next()
  if startup.has("action") and str(startup.action)!="load":
   phase="Failed"
   diagnostic="Unexpected TrackViewer startup action: "+str(startup)
 track_renderer=track_model
 vehicle_renderer=car_model
 get_window().size=benchmark_viewport
 set_process(false)
 set_process_input(false)
 set_process_unhandled_input(false)

func _select_initial_car() -> void:
 # Base startup defers its first available car; benchmark selects the requested car.
 pass

func begin_session() -> void:
 if not benchmark_start_allowed or phase=="Failed": return
 if track_model==null or car_model==null:
  phase="Failed"
  diagnostic="Actual TrackViewer requires Debug host and native viewer bridges"
  return
 phase="Loading"
 # Same production DebugTools dispatch and viewer_action request flow.
 get_node("/root/DebugTools")._dispatch_request({"action":"load","source":"TRACKS/TRACKB%d.LZC"%track_id})
 var found := false
 for entry in car_catalog:
  if str(entry.get("code",""))!=benchmark_car_code or not bool(entry.get("available",false)): continue
  for skin in entry.get("skins",[]):
   if int(skin.get("id",-1))<0: continue
   viewer_ui_action({"action":"select_car","car_id":int(entry.id),"skin_id":int(skin.id)})
   found=true
   break
  break
 if not found:
  phase="Failed"
  diagnostic="TrackViewer benchmark selected car is unavailable: "+benchmark_car_code

func _process(_delta: float) -> void:
 if phase!="Loading": return
 # Production pumping and publish/refresh path, with no gameplay timestep.
 super._process(0.0)
 if not track_pending and not bool(track_state.get("ok",false)):
  phase="Failed"
  diagnostic="Actual TrackViewer load failed: "+str(track_state)
 elif not pending_load and not bool(car_state.get("ok",false)):
  phase="Failed"
  diagnostic="Actual TrackViewer car load failed: "+str(car_state)
 elif not track_pending and not pending_load:
  if benchmark_context=="driving":
   focused=true
   viewer_ui_action({"action":"test_drive"})
   if driving==null or not driving.engaged() or not driving.active:
    phase="Failed"
    diagnostic="Actual Test drive failed to start: "+str(driving.error if driving!=null else "missing driving_lab")
    return
   # Only initial benchmark setup owns this elapsed clock. Production startup
   # may have marked a focus-transition discard before our fixed clock begins.
   startup_driving_state={"discard_elapsed_before":driving.discard_elapsed,"native_step":driving.state.get("step"),"native_tick_calls":driving.native_tick_calls,"phase":int(driving.phase),"focused":focused,"elapsed_policy":"Fixed benchmark elapsed begins after successful production Test drive start; no sampled frame is skipped"}
   driving.discard_elapsed=false
   startup_driving_state["discard_elapsed_after"]=driving.discard_elapsed
   driving.capture_wanted=false
   benchmark_frame_index=0
   phase="Running"
  elif driving!=null and driving.engaged():
   phase="Failed"
   diagnostic="TrackViewer benchmark unexpectedly entered driving mode"
  elif car_model.visible:
   phase="Failed"
   diagnostic="TrackViewer free-browse car must remain hidden"
  else:
   phase="Running"

static func driving_frame_error(before: int, native: Dictionary, controls: Dictionary, tick_calls: int) -> String:
 if tick_calls!=1: return "Native driving tick invocation count differs from one production call"
 if not bool(native.get("ok",false)) or bool(native.get("paused",true)) or str(native.get("status","")) in ["Blocked","Failed"]:
  return "Native driving state is failed, blocked or paused"
 if not bool(native.get("body_known",false)) or not bool(native.get("wheels_known",false)):
  return "Native driving body/wheels are unavailable"
 if int(native.get("step",-1))<=0 or int(native.get("step",-1))-before not in [0,1]:
  return "Native driving step changed outside the original hold/advance scheduler"
 if not bool(native.get("dt_known",false)) or not is_finite(float(native.get("dt",0))) or float(native.get("dt",0))<=0:
  return "Native driving timestep is unavailable or invalid"
 var body= native.get("body")
 if not body is Transform3D or not body.is_finite(): return "Native driving body transform is invalid"
 for pair in [["throttle",0.35],["brake",0.0],["steering",0.0],["handbrake",0.0]]:
  if not controls.has(pair[0]) or not is_equal_approx(float(controls[pair[0]]),float(pair[1])):
   return "Deterministic semantic input was cleared or changed: "+str(pair[0])
 return ""

func _semantic_throttle() -> void:
 var event := InputEventAction.new()
 event.action="hp2_drive_throttle"
 event.pressed=true
 event.strength=0.35
 driving.input_adapter.handle_input(event,false)

func _driving_lifecycle_evidence() -> Dictionary:
 var tools := get_node("/root/DebugTools")
 var native: Dictionary=driving.state if driving!=null else {}
 return {"phase":int(driving.phase) if driving!=null else -1,"active":bool(driving.active) if driving!=null else false,"driving_error":str(driving.error) if driving!=null else "missing driving_lab","focused":focused,"console_open":tools.is_console_open(),"keyboard_capture":tools.host.viewer_keyboard_capture() if tools.host!=null else null,"native_step":native.get("step"),"native_paused":native.get("paused"),"native_status":native.get("status"),"native_error":native.get("error"),"native_diagnostic":native.get("diagnostic"),"discard_elapsed":driving.discard_elapsed if driving!=null else null}

func benchmark_frame(delta: float) -> Dictionary:
 if benchmark_context!="driving":
  # Exactly one actual free-browse process, including hidden native car and UI.
  super._process(delta)
  if driving!=null and driving.engaged(): return {"ok":false,"error":"TrackViewer benchmark entered driving mode"}
  if car_model.visible: return {"ok":false,"error":"TrackViewer free-browse car became visible"}
  return track_state
 focused=true
 if driving==null or driving.phase!=driving.Phase.DRIVING or not driving.active:
  var evidence := _driving_lifecycle_evidence()
  var message := "Production driving lifecycle is not DRIVING; no automatic restart: "+JSON.stringify(evidence)
  if driving!=null: driving._fail(message)
  phase="Failed"
  diagnostic=message
  return {"ok":false,"error":message,"driving_lifecycle":evidence}
 var previous_step := int(driving.state.get("step",-1))
 var previous_tick_calls := int(driving.native_tick_calls)
 var discarded_elapsed_before := bool(driving.discard_elapsed)
 _semantic_throttle()
 if benchmark_route=="drive_spin":
  # Synthetic mouse-look yaw changes only the existing port camera, not physics.
  driving.driving_camera.yaw=PI*sin(float(benchmark_frame_index)*delta*TAU/4.0)
  driving.driving_camera.manual=true
  driving.driving_camera.idle=0.0
 driving.capture_wanted=false
 super._process(delta)
 var tick_calls := int(driving.native_tick_calls)-previous_tick_calls
 var reason := driving_frame_error(previous_step,driving.state,driving.last_input_frame,tick_calls)
 if discarded_elapsed_before or driving.discard_elapsed: reason="Driving lifecycle discarded elapsed benchmark time"
 if driving.phase!=driving.Phase.DRIVING or not driving.active or not car_model.visible:
  reason="Production driving stopped or car became hidden: "+str(driving.error)
 if not reason.is_empty():
  reason+=" Lifecycle: "+JSON.stringify(_driving_lifecycle_evidence())
  driving._fail(reason)
  phase="Failed"
  diagnostic=reason
  return {"ok":false,"error":reason}
 var body: Transform3D=driving.state.body
 var p := body.origin
 var c := camera.global_position
 var wheel_rows: Array=[]
 for wheel in driving.state.get("wheels",[]):
  var grip: Dictionary=wheel.get("grip",{})
  wheel_rows.append({"contact":wheel.get("contact"),"terrain":grip.get("terrain"),"grip_available":not grip.is_empty()})
 var powertrain: Dictionary=driving.state.get("powertrain",{})
 last_driving_frame={"native_tick_calls":tick_calls,"tick_calls_before":previous_tick_calls,"tick_calls_after":int(driving.native_tick_calls),"discard_elapsed_before":discarded_elapsed_before,"discard_elapsed_after":driving.discard_elapsed,"wheels":wheel_rows,"speed":powertrain.get("speed"),"forward_speed":powertrain.get("forward_speed"),"continuous_frame":benchmark_frame_index,"step_before":previous_step,"step_after":int(driving.state.step),"step_delta":int(driving.state.step)-previous_step,"elapsed":delta,"native_dt":driving.state.dt,"input":driving.last_input_frame.duplicate(true),"position_host":[p.x,p.y,p.z],"position_hp2":[p.z,p.x,p.y],"body_bits":var_to_bytes(body).hex_encode(),"camera_position_host":[c.x,c.y,c.z],"camera_bits":var_to_bytes(camera.global_transform).hex_encode(),"camera_yaw":driving.driving_camera.yaw,"phase":"DRIVING","forced_benchmark_focus":true}
 benchmark_frame_index+=1
 return track_state

func _viewer_notification(what: int) -> void:
 if benchmark_context=="driving":
  # Godot delivers inherited _notification handlers too. Override this explicit
  # virtual hook instead; overriding _notification cannot suppress the base.
  # The deterministic wrapper isolates all asynchronous lifecycle dispatch; the
  # actual production driving process still performs its normal lifecycle check.
  focused=true
  return
 super._viewer_notification(what)

func _update_camera() -> void:
 # Actual _publish_track_result already selected the production start camera.
 pass

func _release() -> void:
 release_camera()
 var debug_tools := get_node_or_null("/root/DebugTools")
 if benchmark_context=="driving" and debug_tools!=null and not saved_debug_input.is_empty():
  debug_tools.set_process_input(bool(saved_debug_input.input))
  debug_tools.set_process_unhandled_input(bool(saved_debug_input.unhandled))
 # Actual inherited _exit_tree clears native resources and unregisters UI owner.
