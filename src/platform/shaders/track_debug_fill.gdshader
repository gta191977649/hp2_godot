shader_type spatial;
render_mode unshaded, fog_disabled, shadows_disabled, cull_disabled, depth_draw_never, blend_mix;
uniform bool material_mode=false;
void fragment(){ALBEDO=material_mode?COLOR.rgb:vec3(0.05,0.65,1.0);ALPHA=material_mode?0.88:0.26;}
