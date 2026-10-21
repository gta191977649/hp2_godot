extends RefCounted
# TASK0084 port-side mouse override. Native HP2 pose remains the default.
# User-requested host policy; these constants are not recovered PS2 tuning.
const MOUSE_RADIANS := 0.0026179938779915 # 0.15 degrees per mouse pixel.
const IDLE_SECONDS := 1.5
const RETURN_RATE := 6.0
const RETURN_EPSILON := 0.0001
var look_back := false
var yaw := 0.0
var pitch := 0.0
var idle := IDLE_SECONDS
var manual := false

func reset() -> void:
	look_back = false
	clear_mouse()

func clear_mouse() -> void:
	yaw = 0
	pitch = 0
	idle = IDLE_SECONDS
	manual = false

func set_look_back(pressed: bool) -> void:
	if look_back != pressed:
		look_back = pressed
		clear_mouse()

func clear_held_view() -> void:
	set_look_back(false)
	clear_mouse()

func mouse_motion(relative: Vector2) -> void:
	if relative.is_zero_approx(): return
	yaw = wrapf(yaw - relative.x * MOUSE_RADIANS, -PI, PI)
	pitch = clampf(pitch + relative.y * MOUSE_RADIANS, deg_to_rad(-75), deg_to_rad(75))
	idle = 0
	manual = true

func configure_projection(camera: Camera3D, frame: Dictionary) -> void:
	# TASK0088: original CurrentKey.FieldOfView is a horizontal bAngle.
	camera.keep_aspect = Camera3D.KEEP_WIDTH
	camera.fov = float(frame.fov_bangle) * 360.0 / 65536.0
	camera.near = float(frame.near_z)

func apply(camera: Camera3D, frame: Dictionary, delta: float) -> void:
	configure_projection(camera, frame)
	idle += maxf(delta, 0)
	if manual and idle >= IDLE_SECONDS:
		var weight := 1.0 - exp(-RETURN_RATE * maxf(delta, 0))
		yaw = lerp_angle(yaw, 0, weight)
		pitch = lerpf(pitch, 0, weight)
		if absf(yaw) < RETURN_EPSILON and absf(pitch) < RETURN_EPSILON:
			clear_mouse()
	var position: Vector3 = frame.position
	var target: Vector3 = frame.target
	var up: Vector3 = frame.up
	if manual:
		# Exterior mouse movement orbits the native target. Bumper mouse movement
		# swivels around the eye; both are explicit host controls, not PS2 tuning.
		var outside := int(frame.pov_type) == 2
		var offset := position - target if outside else target - position
		var radius := offset.length()
		var horizontal := Vector3(offset.x, 0, offset.z)
		if radius > 0.000001 and horizontal.length_squared() > 0.000001:
			var elevation := clampf(asin(clampf(offset.y / radius, -1, 1)) + pitch, deg_to_rad(-80), deg_to_rad(80))
			var rotated := radius * (horizontal.normalized().rotated(Vector3.UP, yaw) * cos(elevation) + Vector3.UP * sin(elevation))
			if outside: position = target + rotated
			else: target = position + rotated
			up = Vector3.UP
	camera.position = position
	camera.look_at(target, up)
