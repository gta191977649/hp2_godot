shader_type spatial;
render_mode unshaded, cull_disabled, blend_add, depth_draw_never, shadows_disabled;

#define TRACK_SKY
#include "mesh_surface.gdshaderinc"
