extends RefCounted
# TASK-0114 user-authorized vehicle-centered host orbit. Native pose is default.
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
const CLOSE_EYE_RADIUS := 2.0 # Host-only threshold for bumper observation.
const BUMPER_ORBIT_RADIUS := 6.0 # Host-only exterior observation distance.
var dragging := false
var orbit_origin := Vector3.ZERO
var orbit_target := Vector3.ZERO
var orbit_radius := 0.0
var orbit_offset := Vector3.ZERO
var orbit_initialized := false
var return_strength := 1.0
var widescreen_hor_plus := false # TASK-0112: explicit host projection policy.

func reset() -> void:
	look_back = false
	clear_mouse()

func clear_mouse() -> void:
	yaw = 0
	pitch = 0
	idle = IDLE_SECONDS
	manual = false
	dragging = false
	orbit_initialized = false
	return_strength = 1.0

func set_dragging(pressed: bool) -> void:
	dragging = pressed
	if pressed: idle = 0

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
	return_strength = 1.0
	manual = true

func configure_projection(camera: Camera3D, frame: Dictionary) -> void:
	# TASK0088: original CurrentKey.FieldOfView is a horizontal bAngle.
	var horizontal := float(frame.fov_bangle) * 360.0 / 65536.0
	if widescreen_hor_plus:
		camera.keep_aspect = Camera3D.KEEP_HEIGHT
		camera.fov = rad_to_deg(2.0 * atan(tan(deg_to_rad(horizontal) * 0.5) / (4.0 / 3.0)))
	else:
		camera.keep_aspect = Camera3D.KEEP_WIDTH
		camera.fov = horizontal
	camera.near = float(frame.near_z)

func apply(camera: Camera3D, frame: Dictionary, delta: float) -> void:
	configure_projection(camera, frame)
	if not dragging: idle += maxf(delta, 0)
	if manual and idle >= IDLE_SECONDS:
		return_strength *= exp(-RETURN_RATE * maxf(delta, 0))
		if return_strength < RETURN_EPSILON:
			clear_mouse()
	var position: Vector3 = frame.position
	var target: Vector3 = frame.target
	var up: Vector3 = frame.up
	orbit_origin = frame.get("orbit_center", target)
	if manual:
		if not orbit_initialized:
			orbit_offset = position - orbit_origin
			if orbit_offset.length() < CLOSE_EYE_RADIUS:
				var back: Vector3 = frame.get("orbit_back", Vector3.BACK)
				orbit_offset = (back.normalized() + Vector3.UP * 0.25).normalized() * BUMPER_ORBIT_RADIUS
			orbit_radius = orbit_offset.length()
			orbit_initialized = true
		var horizontal := Vector3(orbit_offset.x, 0, orbit_offset.z)
		if horizontal.length_squared() < 0.000001: horizontal = Vector3.BACK
		var elevation := clampf(asin(clampf(orbit_offset.y / orbit_radius, -1, 1)) + pitch, deg_to_rad(-80), deg_to_rad(80))
		var rotated := orbit_radius * (horizontal.normalized().rotated(Vector3.UP, yaw) * cos(elevation) + Vector3.UP * sin(elevation))
		position = position.lerp(orbit_origin + rotated, return_strength)
		target = target.lerp(orbit_origin, return_strength)
		up = up.lerp(Vector3.UP, return_strength).normalized()
	else:
		orbit_radius = 0
	orbit_target = target
	camera.position = position
	camera.look_at(target, up)
