extends Node3D
# Original Track11 first nondegenerate mask-triangle centroids (B03 asset probe).
const CASES := [
	{"name": "water", "hash": 0x0CCA6E99, "center": Vector3(-70.8232015, -75.7048314, -170.320435)},
	{"name": "tunnel", "hash": 0xDB00D526, "center": Vector3(198.257009, -68.5316238, 80.0596873)}
]
var renderer: Node3D
var camera: Camera3D

func _ready() -> void:
	_run.call_deferred()

func _fail(reason: String) -> void:
	push_error(reason)
	get_tree().quit(1)

func _run() -> void:
	if DisplayServer.get_name() == "headless":
		get_tree().quit(77)
		return
	get_window().size = Vector2i(1280, 720)
	DebugTools.set("visible", false)
	DebugTools.set_process(false)
	camera = Camera3D.new()
	camera.near = 0.05
	camera.far = 20000
	camera.keep_aspect = Camera3D.KEEP_WIDTH
	camera.fov = float(0x32DC) * 360.0 / 65536.0
	add_child(camera)
	camera.current = true
	renderer = ClassDB.instantiate("Ps2TrackViewer" if "--mask-viewer" in OS.get_cmdline_user_args() else "Ps2TrackRenderer")
	add_child(renderer)
	var state: Dictionary = renderer.request_track(11)
	var deadline := Time.get_ticks_msec() + 120000
	while state.get("loading", false):
		if Time.get_ticks_msec() > deadline:
			_fail("Original mask load timed out")
			return
		state = renderer.pump_load()
		await get_tree().process_frame
	if not state.get("ok", false):
		_fail(str(state))
		return
	for sample in CASES:
		if "--water-only" in OS.get_cmdline_user_args() and sample.name != "water":
			continue
		for shift in [-1.0, 0.0, 1.0]:
			camera.position = sample.center + Vector3(-7.0 + shift, 3.5, -3.0)
			camera.look_at(sample.center, Vector3.UP)
			state = renderer.update_view(camera.position)
			if not state.get("ok", false):
				_fail(str(state))
				return
			var found := 0
			for node in renderer.get_children():
				if node is MeshInstance3D and node.visible and int(node.get_meta("track_texture_hash", 0)) == int(sample.hash):
					found += 1
					var material: ShaderMaterial = node.material_override
					if material.get_shader_parameter("specular_texture") == null or not material.shader.code.contains("gs_rgba.a=hp2_track_alpha40"):
						_fail("Mask is missing its original alpha/320 state")
						return
			if found == 0:
				_fail("No native visible mask commands at original " + sample.name + " position")
				return
			await get_tree().process_frame
			await RenderingServer.frame_post_draw
			await get_tree().process_frame
			await RenderingServer.frame_post_draw
			var image := get_viewport().get_texture().get_image()
			if image.is_empty():
				_fail("Original mask GPU capture is empty")
				return
			image.save_png(ProjectSettings.globalize_path("res://../local/task0051/%s-mask-%d.png" % [sample.name, int(shift)]))
			print("TRACK_MASK_FRAME ", sample.name, " shift=", shift, " masks=", found, " digest=", state.packet_digest)
	print("TRACK_MASK_GPU_OK")
	if "--hold-mask-window" not in OS.get_cmdline_user_args():
		get_tree().quit(0)
