shader_type spatial;
render_mode unshaded, fog_disabled, shadows_disabled, cull_disabled, blend_mix, depth_draw_always;

#define GS_MIX
#define GS_TRACK
#include "mesh_surface.gdshaderinc"
