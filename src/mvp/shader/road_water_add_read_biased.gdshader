shader_type spatial;
render_mode unshaded, fog_disabled, shadows_disabled, cull_disabled, blend_add, depth_draw_never;

#define GS_ADD
#define GS_TRACK
#define GS_ROAD_WATER
#define GS_BIASED
#include "mesh_surface.gdshaderinc"
