
vec2 tex_size=vec2(textureSize(base_texture,0));
vec2 dx=dFdx(UV*tex_size),dy=dFdy(UV*tex_size);
vec4 texel;
if(max(dot(dx,dx),dot(dy,dy))>1.0){
  ivec2 coord=ivec2(floor(UV*tex_size));
  texel=texelFetch(base_texture,clamp(coord,ivec2(0),ivec2(tex_size)-ivec2(1)),0);
}else{texel=texture(base_texture,UV);}
vec4 combined=floor(texel*255.0*gs_rgba/128.0);
combined=clamp(combined,vec4(0.0),vec4(255.0));
