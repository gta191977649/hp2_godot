shader_type spatial;
render_mode unshaded, fog_disabled, shadows_disabled, cull_disabled, blend_sub, depth_draw_always;

#define GS_SUB
#define GS_TRACK
#include "mesh_surface.gdshaderinc"
