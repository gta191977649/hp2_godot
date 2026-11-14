shader_type spatial;
render_mode unshaded, fog_disabled, shadows_disabled, cull_disabled, blend_add, depth_draw_always;

#define GS_ADD
#define GS_BIASED
#define GS_TRACK
#include "mesh_surface.gdshaderinc"
