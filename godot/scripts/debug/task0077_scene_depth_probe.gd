extends CompositorEffect
## Diagnostic only. Borrow scene depth during the callback and own the encoded copy.

signal captured(image: Image)
signal released

const SOURCE := """
#version 450
layout(local_size_x=8, local_size_y=8) in;
layout(set=0, binding=0) uniform sampler2D scene_depth;
layout(rgba8, set=0, binding=1) writeonly uniform image2D encoded;
layout(push_constant, std430) uniform Params {
    vec4 inverse_z_w;
    vec2 near_far;
} params;
void main() {
    ivec2 pixel=ivec2(gl_GlobalInvocationID.xy);
    if (any(greaterThanEqual(pixel,imageSize(encoded)))) return;
    float hardware=texelFetch(scene_depth,pixel,0).r;
    uint depth=0;
    if (hardware>0.0) {
        vec4 inverse=params.inverse_z_w;
        float distance=-(inverse.x*hardware+inverse.y)/(inverse.z*hardware+inverse.w);
        float lo=uintBitsToFloat(0x40000010u);
        float hi=uintBitsToFloat(0x40800000u);
        float n=params.near_far.x, f=params.near_far.y;
        float a=(-hi*n+lo*f)/(f-n);
        float b=f*n*(hi-lo)/(f-n);
        depth=(floatBitsToUint(a+b/max(distance,0.000001))>>4u)&0xffffffu;
    }
    imageStore(encoded,pixel,vec4(vec3(depth&255u,(depth>>8u)&255u,(depth>>16u)&255u)/255.0,1.0));
}
"""

var _rd: RenderingDevice
var _shader := RID()
var _pipeline := RID()
var _sampler := RID()
var _texture := RID()
var _size := Vector2i.ZERO
var _near_far: Vector2
var _attempted := false

func _init(stage: int, near_far: Vector2) -> void:
	effect_callback_type = stage
	access_resolved_depth = true
	_near_far = near_far

func _render_callback(stage: int, data: RenderData) -> void:
	if stage != effect_callback_type:
		return
	var buffers := data.get_render_scene_buffers() as RenderSceneBuffersRD
	if buffers == null or buffers.get_view_count() != 1:
		return
	if not _attempted:
		_attempted = true
		_rd = RenderingServer.get_rendering_device()
		var source := RDShaderSource.new()
		source.source_compute = SOURCE
		var spirv := _rd.shader_compile_spirv_from_source(source)
		if not spirv.compile_error_compute.is_empty():
			push_error(spirv.compile_error_compute)
			return
		_shader = _rd.shader_create_from_spirv(spirv)
		_pipeline = _rd.compute_pipeline_create(_shader)
		_sampler = _rd.sampler_create(RDSamplerState.new())
	if not _pipeline.is_valid():
		return
	var size := buffers.get_internal_size()
	var depth := buffers.get_depth_layer(0)
	if size.x < 1 or size.y < 1 or not depth.is_valid():
		return
	if _texture.is_valid() and _size != size:
		_rd.free_rid(_texture)
		_texture = RID()
	if not _texture.is_valid():
		_size = size
		var format := RDTextureFormat.new()
		format.width = size.x
		format.height = size.y
		format.format = RenderingDevice.DATA_FORMAT_R8G8B8A8_UNORM
		format.usage_bits = RenderingDevice.TEXTURE_USAGE_STORAGE_BIT | RenderingDevice.TEXTURE_USAGE_CAN_COPY_FROM_BIT
		_texture = _rd.texture_create(format, RDTextureView.new())
	var input := RDUniform.new()
	input.uniform_type = RenderingDevice.UNIFORM_TYPE_SAMPLER_WITH_TEXTURE
	input.binding = 0
	input.add_id(_sampler)
	input.add_id(depth)
	var output := RDUniform.new()
	output.uniform_type = RenderingDevice.UNIFORM_TYPE_IMAGE
	output.binding = 1
	output.add_id(_texture)
	var uniform_set := UniformSetCacheRD.get_cache(_shader, 0, [input, output])
	var inverse := data.get_render_scene_data().get_cam_projection().inverse()
	var params := PackedFloat32Array([inverse.z.z, inverse.w.z, inverse.z.w, inverse.w.w,
		_near_far.x, _near_far.y]).to_byte_array()
	var list := _rd.compute_list_begin()
	_rd.compute_list_bind_compute_pipeline(list, _pipeline)
	_rd.compute_list_bind_uniform_set(list, uniform_set, 0)
	_rd.compute_list_set_push_constant(list, params, params.size())
	_rd.compute_list_dispatch(list, ceili(size.x / 8.0), ceili(size.y / 8.0), 1)
	_rd.compute_list_end()

func request_image() -> void:
	RenderingServer.call_on_render_thread(_read_image)

func _read_image() -> void:
	var image := Image.new()
	if _texture.is_valid():
		image = Image.create_from_data(_size.x, _size.y, false, Image.FORMAT_RGBA8, _rd.texture_get_data(_texture, 0))
	_deliver.call_deferred(image)

func _deliver(image: Image) -> void:
	captured.emit(image)

func close() -> void:
	RenderingServer.call_on_render_thread(_release_render_resources)

func _release_render_resources() -> void:
	for rid in [_texture, _sampler, _shader]:
		if rid.is_valid():
			_rd.free_rid(rid)
	_texture = RID()
	_sampler = RID()
	_shader = RID()
	_pipeline = RID()
	_finish_release.call_deferred()

func _finish_release() -> void:
	released.emit()
