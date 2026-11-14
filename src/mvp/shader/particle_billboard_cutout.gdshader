shader_type spatial;
render_mode unshaded, cull_disabled, depth_draw_opaque, shadows_disabled;
#define PARTICLE_CUTOUT
#include "res://shader/particle_billboard.gdshaderinc"
