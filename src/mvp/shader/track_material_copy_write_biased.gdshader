shader_type spatial;
render_mode unshaded, fog_disabled, shadows_disabled, cull_disabled;

#define GS_BIASED
#define GS_TRACK
#include "mesh_surface.gdshaderinc"
