extends Node
# Port-side integration check using configured original assets at runtime.
func _ready() -> void:
	_run.call_deferred()

func _frames(count: int = 6) -> void:
	for _i in count:
		await get_tree().process_frame
	var deadline := Time.get_ticks_msec() + 15000
	while is_instance_valid(DebugTools.scene_owner) and DebugTools.scene_owner.get("pending_load") == true:
		_check(Time.get_ticks_msec() < deadline, "Async load timeout")
		await get_tree().create_timer(0.001).timeout

func _check(value: bool, message: String) -> void:
	if not value:
		push_error("CAR_RENDER_SMOKE_FAILED: " + message)
		get_tree().quit(1)
		assert(value, message)

func _run() -> void:
	await _frames()
	var host: Object = DebugTools.host
	for argument in OS.get_cmdline_user_args():
		if argument.begins_with("--expect-debug="):
			_check(host.is_debug_build() == (argument.trim_prefix("--expect-debug=") == "true"), "Expected build configuration")
	if not host.is_debug_build():
		var rejected: Node3D = ClassDB.instantiate("Ps2CarViewer")
		add_child(rejected)
		_check(not rejected.load_car("CARS/911TURBO/GEOMETRY.BIN").ok, "Release rejects native loading")
		rejected.queue_free()
		print("CAR_RENDER_SMOKE_OK: Release rejected")
		get_tree().quit(0)
		return
	var resolver: Object = ClassDB.instantiate("Ps2TextureLoader")
	if not resolver.resolve_root().ok:
		print("CAR_RENDER_SMOKE_SKIPPED: original assets not configured")
		get_tree().quit(77)
		return
	DebugTools.frontend_scene = "res://scenes/debug/viewer_frontend_fixture.tscn"
	DebugTools.request("car")
	await _frames()
	var viewer: Node = DebugTools.scene_owner
	_check(viewer.viewer_kind == "car", "CarViewer opened")
	_check(viewer.car_model.is_class("Ps2VehicleRenderer"), "Viewer uses the shared runtime vehicle backend")
	var original_host := host
	_check(viewer.car_catalog.size() == 64, "All original catalog cars are visible to ImGui")
	var skin_count := 0
	for entry in viewer.car_catalog:
		skin_count += entry.skins.size()
	_check(skin_count == 235, "All original catalog colors are retained")
	for model in ["911TURBO", "BMWM5COP", "4DR_SEDAN", "911TURBO"]:
		DebugTools.request("load", "CARS/%s/GEOMETRY.BIN" % model)
		await _frames()
		var state: Dictionary = host.viewer_state()
		_check(state.status == "Loaded" and not viewer.fixture.visible, "Original car load " + model + ": " + str(state.diagnostic))
		var native: Dictionary = viewer.car_model.get_state()
		_check(not bool(native.show_lods) and native.lods.size() == 1, "Default near model only")
		_check(native.ok and native.triangles > 0 and native.textures > 0 and native.parts.size() > 0 and int(native.get("draw_surfaces", 0)) > 0, "Native mesh/texture counts and uploaded surfaces")
		_check(native.bounds.size.length() > 1, "Original mesh bounds")
		_check(int(native.reflection_surfaces) > 0, "Original frontend environment passes uploaded")
		# The shared backend must use world placement, not assume an identity viewer root.
		viewer.car_model.position = Vector3(2, 1, -3)
		viewer.car_model.rotation = Vector3(0, 0.6, 0)
		var world_camera := Vector3(5, 4, 9)
		var placed: Dictionary = viewer.car_model.update_view(world_camera, Vector3.FORWARD, 120.0, 0)
		_check(placed.ok, "Rigid world placement remains renderable")
		var checked_world := false
		for mesh_node in viewer.car_model.get_children():
			if not mesh_node is MeshInstance3D or not mesh_node.visible:
				continue
			var shader_material: Material = mesh_node.get_active_material(0)
			if not shader_material is ShaderMaterial:
				continue
			var local_camera: Vector3 = mesh_node.global_transform.affine_inverse() * world_camera
			var expected_camera := Vector3(local_camera.z, local_camera.x, local_camera.y)
			_check(expected_camera.is_equal_approx(shader_material.get_shader_parameter("source_camera")), "Camera uses global part transform")
			var local_light: Vector3 = (mesh_node.global_basis.transposed() * Vector3(-5, 5, 0)).normalized()
			var actual_light := Vector3(shader_material.get_shader_parameter("light_360").x, shader_material.get_shader_parameter("light_361").x, shader_material.get_shader_parameter("light_362").x)
			_check(actual_light.is_equal_approx(Vector3(local_light.z, local_light.x, local_light.y)), "Native light slice includes world rotation")
			checked_world = true
			break
		_check(checked_world, "Shared material placement checked")
		viewer.car_model.transform = Transform3D.IDENTITY
		if model == "911TURBO":
			viewer.viewer_ui_action({"action": "wheel_blur", "enabled": true})
			await _frames()
			_check(viewer.car_model.get_state().ok and viewer.viewer_ui_snapshot().wheel_blur, "Original wheel blur load and UI state")
			var blur_nodes := 0
			for child in viewer.car_model.get_children():
				if str(child.name).contains("blur wheel") and child.visible:
					blur_nodes += 1
			_check(blur_nodes == 4, "Four original blur overlays uploaded")
			var tire: Node3D
			var blur_node: Node3D
			for child in viewer.car_model.get_children():
				if str(child.name).contains("TIRE_FRONT_A") and str(child.name).contains("wheel 0"):
					tire = child
				if str(child.name).contains("blur wheel 0"):
					blur_node = child
			_check(tire != null and blur_node != null, "Original tire and independent blur nodes")
			var tire_before: Transform3D = tire.transform
			var blur_before: Transform3D = blur_node.transform
			var animation_state: Dictionary = viewer.car_model.get_state()
			if host.console_available():
				var f8 := InputEventKey.new()
				f8.keycode = KEY_F8
				f8.pressed = true
				DebugTools.handle_input(f8)
				f8.pressed = false
				DebugTools.handle_input(f8)
			await _frames()
			_check(not tire.transform.is_equal_approx(tire_before), "Tires rotate while console captures input")
			_check(blur_node.transform.is_equal_approx(blur_before), "Original blur layer does not co-rotate")
			_check(int(viewer.car_model.get_state().generation) == int(animation_state.generation), "Animation does not queue resource loads")
			if host.console_available():
				var f8 := InputEventKey.new()
				f8.keycode = KEY_F8
				f8.pressed = true
				DebugTools.handle_input(f8)
				f8.pressed = false
				DebugTools.handle_input(f8)
			viewer.viewer_ui_action({"action": "wheel_speed", "value": 0.0})
			var stopped_phase: float = viewer.car_model.get_state().wheel_phase
			await _frames()
			_check(float(viewer.car_model.get_state().wheel_phase) == stopped_phase, "Zero speed holds pose")
			viewer.viewer_ui_action({"action": "wheel_speed", "value": -25.0})
			await _frames()
			_check(viewer.viewer_ui_snapshot().wheel_speed == -25.0 and float(viewer.car_model.get_state().wheel_phase) != stopped_phase, "Speed slider drives live reverse rotation")
			_check(int(viewer.car_model.get_state().generation) == int(animation_state.generation), "Speed slider does not queue loads")
			var current_skin: int = viewer.car_model.get_state().skin_id
			viewer.viewer_ui_action({"action": "select_skin", "skin_id": current_skin})
			await _frames()
			_check(viewer.car_model.get_state().ok and viewer.car_model.get_state().wheel_blur and float(viewer.car_model.get_state().wheel_speed) == -25.0, "Skin reload preserves switch and angular velocity")
			viewer.viewer_ui_action({"action": "wheel_blur", "enabled": false})
			await _frames()
			_check(viewer.car_model.get_state().ok and not viewer.viewer_ui_snapshot().wheel_blur, "Original static wheels restored")
			_check(float(viewer.car_model.get_state().wheel_phase) == 0, "Disable resets wheel phase")
		print("CAR_LOAD_TIMING ", model, " cpu_ms=", native.cpu_ms, " upload_ms=", native.upload_ms, " max_slice_ms=", native.max_upload_slice_ms, " max_frame_ms=", viewer.max_load_frame_ms)
		viewer.viewer_ui_action({"action": "show_lods", "enabled": true})
		await _frames(2)
		var compared: Dictionary = viewer.car_model.get_state()
		_check(compared.ok and compared.show_lods and compared.lods.size() == 4, "LOD comparison enabled")
		_check(compared.bounds.size.x > native.bounds.size.x * 2, "LOD comparison separates models")
		viewer.viewer_ui_action({"action": "show_lods", "enabled": false})
		await _frames(2)
		_check(not bool(viewer.car_model.get_state().show_lods), "Return to near model")
		viewer.car_model.set_wireframe(true)
		viewer.car_model.set_bounds(true)
		var body_name := ""
		for part in viewer.car_model.get_state().parts:
			if part.visible:
				body_name = str(part.name)
				break
		_check(not body_name.is_empty(), "Visible original part")
		viewer.car_model.set_part_visible(body_name, false)
		await _frames(2)
		for part in viewer.car_model.get_state().parts:
			if part.name == body_name:
				_check(not part.visible and not part.enabled, "Part visibility override")
		viewer.car_model.set_part_visible(body_name, true)
		viewer.car_model.set_wireframe(false)
		viewer.car_model.set_bounds(false)
		DebugTools.request("clear")
		await _frames()
		_check(host.viewer_state().status == "Empty" and viewer.car_model.get_child_count() == 0, "Clear releases native nodes")
	# Exercise the same post-frame catalog/color handlers used by ImGui actions.
	DebugTools._dispatch_ui_action({"action": "select_car", "car_id": 39, "skin_id": 1})
	await _frames()
	_check(host.viewer_state().status == "Loaded", "Catalog ID load")
	var catalog_state: Dictionary = viewer.car_model.get_state()
	_check(int(catalog_state.car_id) == 39 and int(catalog_state.skin_id) == 1, "Catalog identity retained")
	var colors: Array = []
	for entry in viewer.car_catalog:
		if int(entry.id) == 39:
			colors = entry.skins
	_check(colors.size() > 1, "Original selected car has multiple skins")
	var original_body_id: int = viewer.car_model.get_child(0).get_instance_id()
	var next_skin := int(colors[0].id)
	if next_skin == 1:
		next_skin = int(colors[1].id)
	DebugTools._dispatch_ui_action({"action": "select_skin", "skin_id": next_skin})
	await _frames()
	_check(int(viewer.car_model.get_state().skin_id) == next_skin, "Original skin ID selection")
	_check(viewer.car_model.get_child(0).get_instance_id() == original_body_id, "Repaint reuses unchanged geometry node")
	var warm_state: Dictionary = viewer.car_model.get_state()
	_check(int(warm_state.geometry_reads) == 0 and int(warm_state.texture_decodes) == 0, "Warm repaint avoids geometry reads and texture decoding")
	print("CAR_LOAD_TIMING warm_cpu_ms=", warm_state.cpu_ms, " upload_ms=", warm_state.upload_ms, " max_slice_ms=", warm_state.max_upload_slice_ms)
	var preview_count := 0
	for entry in viewer.car_catalog:
		if int(entry.id) == 39:
			for skin in entry.skins:
				if skin.has("preview"):
					preview_count += 1
	_check(preview_count == colors.size(), "All selected car paint tiles have material previews")
	# Coalesce requests; only the latest generation may commit.
	viewer.viewer_ui_action({"action": "select_car", "car_id": 39, "skin_id": 1})
	viewer.viewer_ui_action({"action": "select_car", "car_id": 39, "skin_id": next_skin})
	await _frames()
	_check(int(viewer.car_model.get_state().skin_id) == next_skin, "Latest queued skin wins")
	viewer.viewer_ui_action({"action": "select_car", "car_id": 39, "skin_id": 1})
	DebugTools.request("clear")
	await _frames()
	_check(viewer.car_model.get_child_count() == 0 and not bool(viewer.car_model.get_state().loading), "Clear cancels pending upload")
	await get_tree().create_timer(0.1).timeout
	_check(viewer.car_model.get_child_count() == 0, "Cancelled completion cannot resurrect car")
	viewer.viewer_ui_action({"action": "select_car", "car_id": 39, "skin_id": 1})
	await _frames()

	DebugTools._dispatch_ui_action({"action": "inspection", "enabled": true})
	await _frames(2)
	_check(bool(viewer.car_model.get_state().inspection), "Explicit raw inspection override")
	DebugTools._dispatch_ui_action({"action": "inspection", "enabled": false})
	await _frames(2)
	_check(not bool(viewer.car_model.get_state().inspection), "Recovered base pass restored")
	_check(bool(DebugTools.last_frame.get("fps_visible", false)), "FPS uses shared ImGui frame")
	for entry in viewer.car_catalog:
		if not entry.available and entry.skins.size() > 1:
			viewer.viewer_ui_action({"action": "select_car", "car_id": int(entry.id), "skin_id": int(entry.skins[0].id)})
			await _frames(2)
			_check(host.viewer_state().status == "Failed" and int(viewer.car_model.get_state().car_id) == int(entry.id), "Missing original car remains selected")
			viewer.viewer_ui_action({"action": "select_skin", "skin_id": int(entry.skins[1].id)})
			await _frames(2)
			_check(host.viewer_state().status == "Failed" and int(viewer.car_model.get_state().car_id) == int(entry.id), "Missing original car retains its own color selection/diagnostic")
			_check(viewer.car_model.get_child_count() == 0, "Missing resource never leaves old display nodes")
			break
	DebugTools.request("load", "CARS/missing/GEOMETRY.BIN")
	await _frames()
	_check(host.viewer_state().status == "Failed" and viewer.car_model.get_child_count() == 0 and not viewer.fixture.visible, "Failed load remains empty")
	DebugTools.request("fixture")
	await _frames()
	_check(viewer.fixture.visible, "Explicit fixture restore")
	DebugTools.request("load", "CARS/911TURBO/GEOMETRY.BIN")
	await _frames()
	_check(host.viewer_state().status == "Loaded", "Reload after failure")
	DebugTools.request("track")
	await _frames()
	_check(host == original_host and DebugTools.scene_owner.viewer_kind == "track", "Scene release preserves overlay")
	DebugTools.request("close")
	await _frames()
	_check(host.viewer_state().kind == "frontend", "Return to fresh frontend")
	print("CAR_RENDER_SMOKE_OK: three original models, reload, LOD, visibility, cleanup, scene transitions")
	get_tree().quit(0)
