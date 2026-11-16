shader_type spatial;
render_mode unshaded, blend_add, cull_disabled, depth_draw_never, shadows_disabled;

#define GS_FLARE_ADD
#include "light_flare.gdshaderinc"
