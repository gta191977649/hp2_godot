shader_type spatial;
render_mode unshaded, blend_mix, cull_disabled, depth_draw_never, shadows_disabled;

#define PARTICLE_MIX
#include "particle_billboard.gdshaderinc"
