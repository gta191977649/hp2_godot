extends RefCounted
# TASK-0046 thin renderer: all geometry, UV, intensity and lifetime originate in
# canonical SkidSet/RenderSkids. This class only uploads complete draw commands.
var node: MeshInstance3D
var count := 0

func _init(parent: Node3D) -> void:
	node = MeshInstance3D.new()
	node.name = "OriginalSkidCommands"
	parent.add_child(node)

func clear() -> void:
	count = 0
	if is_instance_valid(node):
		node.queue_free()

func _surface(next: ArrayMesh, positions: PackedVector3Array, uv: PackedVector2Array, colours: PackedColorArray, indices: PackedInt32Array, material: Material) -> void:
	if positions.is_empty():
		return
	var arrays := []
	arrays.resize(Mesh.ARRAY_MAX)
	arrays[Mesh.ARRAY_VERTEX] = positions
	arrays[Mesh.ARRAY_TEX_UV] = uv
	arrays[Mesh.ARRAY_COLOR] = colours
	arrays[Mesh.ARRAY_INDEX] = indices
	next.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
	next.surface_set_material(next.get_surface_count() - 1, material)

func publish(commands: Array) -> void:
	var next := ArrayMesh.new()
	var positions := PackedVector3Array()
	var uv := PackedVector2Array()
	var colours := PackedColorArray()
	var indices := PackedInt32Array()
	var material: Material
	# Combine consecutive commands only: retain original draw order and avoid a
	# separate Godot surface per quad. Original pool limits this to 128 sets.
	for command in commands:
		if material != command.material:
			_surface(next, positions, uv, colours, indices, material)
			positions.clear()
			uv.clear()
			colours.clear()
			indices.clear()
			material = command.material
		var base := positions.size()
		positions.append_array(command.vertices)
		uv.append_array(command.uv)
		colours.append_array(command.colours)
		# Original Render3D strip 0,1,3,2 projected to host triangle indices.
		for index in [0, 1, 3, 1, 2, 3]:
			indices.append(base + index)
	_surface(next, positions, uv, colours, indices, material)
	node.mesh = next
	count = commands.size()
