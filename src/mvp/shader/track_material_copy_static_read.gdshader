shader_type spatial;
render_mode unshaded, fog_disabled, shadows_disabled, cull_disabled, depth_draw_never;

#define GS_TRACK
#define STATIC_TRACK
#include "mesh_surface.gdshaderinc"
