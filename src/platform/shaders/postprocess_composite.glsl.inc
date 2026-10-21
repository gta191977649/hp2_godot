R"HP2(shader_type canvas_item;
render_mode unshaded,blend_disabled;
uniform sampler2D colour_texture:filter_linear,repeat_disable;
uniform sampler2D encoded_depth:filter_nearest,repeat_disable;
uniform sampler2D fog_palette:filter_nearest,repeat_disable;
uniform sampler2D black_white_palette:filter_nearest,repeat_disable;
uniform bool black_white_enabled=false;
uniform bool history_available=false;
uniform sampler2D focus_palette:filter_nearest,repeat_disable;
uniform bool fog_enabled=false;
uniform bool depth_into_alpha_enabled=true;
uniform bool focus_enabled=false;
uniform ivec4 fog_abcd=ivec4(0,1,0,1);
uniform int fog_fix=128;
uniform int jitter_blend=64;
uniform int jitter_count=2;
uniform sampler2D filter_texture:filter_linear,repeat_disable;
uniform sampler2D radial_texture:filter_linear,repeat_disable;
uniform sampler2D previous_texture:filter_linear,repeat_disable;
uniform bool filter_enabled=false;
uniform bool radial_enabled=false;
uniform bool previous_enabled=false;
uniform ivec4 filter_abcd=ivec4(1,0,0,0);
uniform int filter_fix=128;
uniform int filter_tfx=0;
uniform vec4 filter_colour=vec4(128,128,128,255);
uniform vec2 radial_alpha=vec2(0,192);
uniform vec2 original_size=vec2(512.0,448.0);
vec4 gsblend(vec4 s,vec4 d,ivec4 abcd,float fix){
 vec3 a=abcd.x==0?s.rgb:(abcd.x==1?d.rgb:vec3(0));
 vec3 b=abcd.y==0?s.rgb:(abcd.y==1?d.rgb:vec3(0));
 float c=abcd.z==0?s.a:(abcd.z==1?d.a:fix);
 vec3 last=abcd.w==0?s.rgb:(abcd.w==1?d.rgb:vec3(0));
 return vec4(clamp(floor((a-b)*c/128.0)+last,vec3(0),vec3(255)),s.a);
}
uint original_z(vec2 uv){uvec3 rgb=uvec3(round(texture(encoded_depth,clamp(uv,vec2(0),vec2(1))).rgb*255.0));return rgb.r|(rgb.g<<8u)|(rgb.b<<16u);}
int slot(vec2 uv){return int(min(original_z(uv)>>8u,255u));}
vec4 fogged(vec2 uv){
 vec4 colour=clamp(round(texture(colour_texture,uv)*255.0),vec4(0),vec4(255));if(depth_into_alpha_enabled)colour.a=float((original_z(uv)>>8u)&255u);
 if(fog_enabled){vec4 f=round(texelFetch(fog_palette,ivec2(slot(uv),0),0)*255.0);return gsblend(f,colour,fog_abcd,float(fog_fix));}
 return colour;
}
vec4 focus_source(vec2 uv){
 vec4 colour=fogged(uv);colour.a=round(texelFetch(focus_palette,ivec2(slot(uv),0),0).a*255.0);return colour;
}
vec4 fogged_linear(vec2 uv){
 vec2 p=uv*original_size-vec2(0.5);vec2 b=floor(p),f=fract(p);
 vec2 first=(b+vec2(0.5))/original_size;
 return mix(mix(focus_source(first),focus_source(first+vec2(1,0)/original_size),f.x),mix(focus_source(first+vec2(0,1)/original_size),focus_source(first+vec2(1,1)/original_size),f.x),f.y);
}
vec4 halfpixel(vec2 p){
 vec2 uv=(p+vec2(0.5))/(original_size*0.5);
 vec4 accum=vec4(0);
 for(int i=0;i<2;i++)if(i<jitter_count){
  vec2 offset=i==0?vec2(0):vec2(2.0)/original_size;
  vec4 src=fogged_linear(uv+offset);
  accum.rgb=clamp(accum.rgb+floor(src.rgb*float(jitter_blend)/128.0),vec3(0),vec3(255));accum.a=src.a;
 }
 return accum;
}
vec4 focused(vec2 uv){
 vec4 outcolour=fogged(uv);
 if(focus_enabled){
  vec2 p=uv*(original_size*0.5)-vec2(0.5);vec2 base=floor(p),f=fract(p);
  vec4 blurred=mix(mix(halfpixel(base),halfpixel(base+vec2(1,0)),f.x),mix(halfpixel(base+vec2(0,1)),halfpixel(base+vec2(1,1)),f.x),f.y);
  float focus=round(texelFetch(focus_palette,ivec2(slot(uv),0),0).a*255.0);
  outcolour.rgb=clamp(floor((blurred.rgb-outcolour.rgb)*focus/128.0)+outcolour.rgb,vec3(0),vec3(255));outcolour.a=blurred.a;
 }
 return outcolour;
}
float display_alpha_at(vec2 uv){
 ivec2 size=textureSize(previous_texture,0);ivec2 p=clamp(ivec2(floor(uv*vec2(size))),ivec2(0),size-ivec2(1));
 int a=depth_into_alpha_enabled?slot(uv):(history_available?int(round(texelFetch(previous_texture,p,0).a*255.0)):128);
 if(black_white_enabled)a=(int(focused(uv).g)&254)|(a&1);
 return float(a);
}
float display_alpha_linear(vec2 uv){
 vec2 p=uv*original_size-vec2(0.5);vec2 b=floor(p),f=fract(p),first=(b+vec2(0.5))/original_size;
 return mix(mix(display_alpha_at(first),display_alpha_at(first+vec2(1,0)/original_size),f.x),mix(display_alpha_at(first+vec2(0,1)/original_size),display_alpha_at(first+vec2(1,1)/original_size),f.x),f.y);
}
void fragment(){
 vec4 outcolour=focused(UV);
 if(black_white_enabled){
  int index=int(display_alpha_at(UV));
  outcolour=round(texelFetch(black_white_palette,ivec2(index,0),0)*255.0);
 }
 if(filter_enabled){
  vec4 t=round(texture(filter_texture,UV)*255.0),c=filter_colour;
  vec4 source=clamp(floor(t*c/128.0),vec4(0),vec4(255));
  if(filter_tfx==1)source=t;
  if(filter_tfx==2||filter_tfx==3){source.rgb=clamp(floor(t.rgb*c.rgb/128.0)+vec3(c.a),vec3(0),vec3(255));source.a=filter_tfx==2?min(t.a+c.a,255.0):t.a;}
  outcolour=gsblend(source,outcolour,filter_abcd,float(filter_fix));
 }
 if(radial_enabled){
  vec2 q=vec2(1)-abs(UV*2.0-vec2(1));
  float vertex_alpha=radial_alpha.y+(radial_alpha.x-radial_alpha.y)*max(0.0,q.x+q.y-1.0);
  outcolour.a=clamp(floor(round(texture(radial_texture,q).a*255.0)*vertex_alpha/128.0),0.0,255.0);
 }
 if(previous_enabled){
  vec2 previous_uv=(vec2(0.5)+UV*(original_size-vec2(1.0)))/original_size;
  vec4 previous=round(texture(previous_texture,previous_uv)*255.0);previous.a=round(display_alpha_linear(previous_uv));
  outcolour=gsblend(previous,outcolour,ivec4(0,1,1,1),128.0);
 }
 COLOR=outcolour/255.0;
}
)HP2"