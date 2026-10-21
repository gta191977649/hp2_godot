extends RefCounted
# TASK0088 checks original horizontal bAngle projection through engine matrices.
static func check(camera: Camera3D, tree: SceneTree, fov_bangle: int = 0x32DC) -> void:
	var expected := float(fov_bangle) * 360.0 / 65536.0
	var actual: Projection = camera.get_camera_projection()
	var horizontal := rad_to_deg(2.0 * atan(1.0 / actual.x.x))
	assert(camera.keep_aspect == Camera3D.KEEP_WIDTH and absf(horizontal - expected) < 0.0001, "Driving camera differs from original horizontal projection")
	for size in [Vector2i(800, 600), Vector2i(1280, 720), Vector2i(600, 800)]:
		var viewport := SubViewport.new()
		viewport.size = size
		tree.root.add_child(viewport)
		var sample := Camera3D.new()
		sample.fov = camera.fov
		sample.keep_aspect = camera.keep_aspect
		sample.near = camera.near
		sample.far = camera.far
		viewport.add_child(sample)
		sample.current = true
		await tree.process_frame
		var projection: Projection = sample.get_camera_projection()
		var horizontal_degrees := rad_to_deg(2.0 * atan(1.0 / projection.x.x))
		var vertical_degrees := rad_to_deg(2.0 * atan(1.0 / projection.y.y))
		assert(absf(horizontal_degrees - expected) < 0.0001, "Horizontal FOV changed with viewport aspect " + str(size))
		var expected_vertical := rad_to_deg(2.0 * atan(tan(deg_to_rad(expected) * 0.5) * float(size.y) / float(size.x)))
		assert(absf(vertical_degrees - expected_vertical) < 0.0001, "Vertical projection does not follow original horizontal FOV")
		viewport.queue_free()
		await tree.process_frame
	print("DRIVING_PROJECTION_OK: original horizontal FOV across 4:3, 16:9 and portrait")
