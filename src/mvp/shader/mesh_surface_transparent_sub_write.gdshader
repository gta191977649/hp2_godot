shader_type spatial;
render_mode unshaded, cull_disabled, blend_sub, depth_draw_always;

#define TRANSPARENT
#include "mesh_surface.gdshaderinc"
