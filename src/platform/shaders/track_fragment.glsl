vec4 texel=sample_track(UV);
vec4 combined=clamp(floor(texel*255.0*gs_rgba/128.0),vec4(0.0),vec4(255.0));
@TRACK_ALPHA_TEST@
@TRACK_FUSION@
ALBEDO=hp2_framebuffer_rgb(combined.rgb/255.0);
@TRACK_ALPHA_OUTPUT@
@TRACK_DEPTH_OUTPUT@
