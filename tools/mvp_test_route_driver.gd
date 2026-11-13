extends RefCounted

# Test-only steering via the same key events as a human. The route is a local
# capture of the original road course, in native X/Y and cumulative metres.
var course: Array
var furthest := 0.0
var cursor := -1
var drive_step := -1
var stalled := -1
var reverse_until := -1
var report_at := 0
var held := {}
var send_key: Callable

func load_course(path: String, key_input: Callable) -> bool:
    var capture: Variant = JSON.parse_string(FileAccess.get_file_as_string(path))
    if not capture is Dictionary or not capture.get("points") is Array:
        return false
    course = capture["points"]
    if course.size() < 2:
        return false
    var previous := -INF
    for point in course:
        if not point is Array or point.size() < 3:
            return false
        for value in point.slice(0, 3):
            if not (value is float or value is int) or not is_finite(float(value)):
                return false
        if point[2] <= previous:
            return false
        previous = point[2]
    send_key = key_input
    return true

func release() -> void:
    _buttons(false, false, false, false)

func reached_finish() -> bool:
    # Reject a race-over caused by failure/timeout before the finish area.
    return furthest >= float(course.back()[2]) - 80.0

func _point(index: int) -> Vector2:
    return Vector2(course[index][0],course[index][1])

func _at(distance: float) -> Vector2:
    var left := 0
    var right := course.size() - 1
    while right - left > 1:
        var mid := (left + right) >> 1
        if float(course[mid][2]) < distance: left = mid
        else: right = mid
    var fraction := clampf((distance-float(course[left][2]))/(float(course[right][2])-float(course[left][2])),0,1)
    return _point(left).lerp(_point(right),fraction)

func _buttons(w: bool,s: bool,a: bool,d: bool) -> void:
    var states := {KEY_W:w,KEY_S:s,KEY_A:a,KEY_D:d}
    for key in states:
        if states[key] != held.get(key,false):
            held[key] = states[key]
            send_key.call(key, states[key])

func drive(sample: Dictionary) -> void:
    var step: int = sample["step"]
    if step == drive_step:
        return
    drive_step = step
    var position: Vector3 = sample["position"]
    var direction: Vector3 = sample["forward"]
    var here := Vector2(position.x,position.y)
    var forward := Vector2(direction.x,direction.y)
    var speed: float = absf(sample["speed"])
    var first := 0 if cursor < 0 else maxi(0,cursor-80)
    var last := course.size()-1 if cursor < 0 else mini(course.size()-1,cursor+80)
    var best := INF
    var distance := 0.0
    for i in range(first,last):
        var a := _point(i)
        var b := _point(i+1)
        var seg := b-a
        var fraction := clampf((here-a).dot(seg)/seg.length_squared(),0,1)
        var error := here.distance_squared_to(a+seg*fraction)
        if error < best:
            best = error
            cursor = i
            distance = lerpf(course[i][2],course[i+1][2],fraction)
    furthest = maxf(furthest, distance)
    var look := clampf(8.0+speed*0.9,12.0,36.0)
    var target := _at(distance+look) - here
    var error := atan2(forward.cross(target),forward.dot(target))
    var braking_limit := 62.0
    var previous := _at(distance-12.0)
    var current := _at(distance)
    for ahead in range(0,109,12):
        var following := _at(distance+ahead+12.0)
        var a := current-previous
        var b := following-current
        if a.length_squared()>1.0 and b.length_squared()>1.0:
            var curvature := maxf(absf(a.angle_to(b))/12.0,0.001)
            braking_limit = minf(braking_limit,sqrt(12.0/curvature+18.0*ahead))
        previous = current
        current = following
    var cross_track := sqrt(best)
    var desired := maxf(15.0,braking_limit-absf(error)*15.0-cross_track*0.55)
    if cross_track > 8.0 or absf(error) > 0.8: desired = minf(desired,7.0)
    var duty := clampf(error*1.55,-0.8,0.8)
    var steer := absf(error)>0.025 and step%12 < absf(duty)*12.0
    if speed > 1.0: stalled = -1
    elif stalled < 0: stalled = step
    if stalled >= 0 and step-stalled >= 120 and step >= reverse_until:
        reverse_until = step+120
        stalled = -1
    if step < reverse_until:
        _buttons(false,true,error < -0.15,error > 0.15)
    else:
        _buttons(speed < desired-0.5 and (absf(error)<1.25 or speed<2.5),
            speed > desired+1.5,steer and error>0.0,steer and error<0.0)
    if step >= report_at:
        print("DRIVE step=",step," distance=",distance," cross=",cross_track," speed=",speed," pos=",here," heading=",forward," error=",error)
        report_at = step + 300


