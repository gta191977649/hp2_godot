extends SceneTree
# TASK-0114 deterministic host observation checks; no native World/rule writes.
var failures: Array[String] = []
func check(value: bool, message: String) -> void:
	if not value: failures.append(message)
func _initialize() -> void: call_deferred("run")
func run() -> void:
	var camera := Camera3D.new()
	root.add_child(camera)
	var adapter := preload("res://scripts/debug/driving_camera.gd").new()
	var center := Vector3(10, 2, 20)
	var frame := {"position": Vector3(10, 5, 12), "target": Vector3(50, 0, 100), "up": Vector3.UP, "orbit_center": center, "orbit_back": Vector3.BACK, "pov_type": 2, "fov_bangle": 12000, "near_z": 0.5}
	var original := var_to_bytes(frame)
	adapter.apply(camera, frame, 0)
	check(camera.position == frame.position, "default source eye changed")
	check((-camera.basis.z).dot((frame.target - frame.position).normalized()) > .99999, "default source aim changed")
	adapter.set_dragging(true)
	adapter.mouse_motion(Vector2(-600, 0))
	adapter.apply(camera, frame, 0)
	var radius: float = (frame.position - center).length()
	check(is_equal_approx(camera.position.distance_to(center), radius), "orbit radius changed")
	check(adapter.orbit_target == center and (-camera.basis.z).dot((center - camera.position).normalized()) > .99999, "orbit retained native lookahead target instead of vehicle center")
	check(camera.position.distance_to(center + (frame.position - center).rotated(Vector3.UP, PI * .5)) < .0001, "90 degree mouse yaw is not around vehicle center")
	var shifted: Dictionary = frame.duplicate()
	var shift := Vector3(40, 3, -8)
	shifted.orbit_center = center + shift
	shifted.position = frame.position + Vector3(2, 0, 0) # Native chase lag may change independently.
	shifted.target = frame.target + shift
	var before := camera.position
	adapter.apply(camera, shifted, 10)
	check(adapter.manual and camera.position.distance_to(before + shift) < .0001, "held orbit changed radius/offset or failed moving vehicle follow")
	adapter.mouse_motion(Vector2(100000, 100000))
	adapter.apply(camera, shifted, 0)
	check(camera.position.is_finite() and camera.basis.is_finite() and absf(adapter.yaw) <= PI and absf(adapter.pitch) <= deg_to_rad(75), "large mouse input bypassed yaw wrap/pitch clamp")
	check(is_equal_approx(camera.position.distance_to(shifted.orbit_center), radius), "pitch clamp changed radius")
	adapter.set_dragging(false)
	adapter.apply(camera, shifted, 1)
	check(adapter.manual and adapter.return_strength == 1, "orbit returned before idle delay")
	for i in 400: adapter.apply(camera, shifted, 1.0 / 60)
	check(not adapter.manual and camera.position == shifted.position, "idle did not restore exact current source eye")
	check((-camera.basis.z).dot((shifted.target - shifted.position).normalized()) > .99999, "idle did not restore source aim")
	adapter.reset()
	var bumper: Dictionary = frame.duplicate()
	bumper.position = center + Vector3(0, 0, .3)
	bumper.pov_type = 0
	adapter.apply(camera, bumper, 0)
	check(camera.position == bumper.position, "default bumper eye changed")
	adapter.mouse_motion(Vector2(30, 20))
	adapter.apply(camera, bumper, 0)
	check(adapter.orbit_target == center and camera.position.distance_to(center) > 5.9, "manual bumper still swivels at eye instead of exterior orbit")
	adapter.clear_held_view()
	adapter.apply(camera, frame, 0)
	check(not adapter.manual and not adapter.dragging and camera.position == frame.position, "focus/reset did not restore source camera")
	check(var_to_bytes(frame) == original, "observation mutated immutable source frame")
	camera.queue_free()
	if failures.is_empty():
		print("PASS TASK-0114 vehicle-center/radius/yaw/pitch/motion/hold/idle/bumper/reset/source immutability")
		quit(0)
	else:
		for failure in failures: push_error(failure)
		quit(1)
