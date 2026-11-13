#include "race_effects_presentation.hpp"

#include "assets/mesh_converter.hpp"
#include "indep/inc/Texture.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/gpu_particles3d.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/packed_vector4_array.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace hp2 {
namespace {
using namespace godot;

constexpr auto particleProcess = R"(
shader_type particles;
render_mode disable_velocity, disable_force;
uniform sampler2D step_history : filter_nearest, repeat_disable;
uniform sampler2D group_epochs : filter_nearest, repeat_disable;

uniform float drag;
uniform float gravity;
uniform int atlas_period;
uniform int atlas_cells;

void process() {
    int identity = int(CUSTOM.w);
    int slot = identity & 1023;
    vec4 group = texelFetch(group_epochs, ivec2(slot, 0), 0);
    if (int(group.r) != (identity >> 10)) ACTIVE = false;
    for (int offset = 0; offset < 16 && ACTIVE; ++offset) {
        int packed_life = int(CUSTOM.x);
        int previous = packed_life >> 16;
        int life = packed_life & 65535;
        if (previous == (int(group.b) & 255)) break;
        int index = previous % 16;
        vec4 step = texelFetch(step_history, ivec2(slot, index), 0);
        int ticks = int(texelFetch(step_history, ivec2(slot, index + 16), 0).r);
        int remaining = life - ticks;
        if (remaining <= 0) {
            ACTIVE = false;
            break;
        }
        int packed = int(CUSTOM.z);
        int frame = packed & 255;
        if (atlas_period != 0) {
            frame = (frame + life / atlas_period - remaining / atlas_period) & (atlas_cells - 1);
        }
        vec3 velocity = VELOCITY - drag * abs(VELOCITY) * VELOCITY * step.w;
        velocity.y -= gravity * step.w;
        TRANSFORM[3].xyz += (velocity + step.xyz * float(packed >> 8) / 255.0) * step.w;
        float speed = length(velocity);
        if (speed > 0.0) {
            VELOCITY = trunc(velocity / speed * 127.0) * float(int(speed * 128.0) & 65535) / (128.0 * 127.0);
        } else {
            VELOCITY = vec3(0.0);
        }
        CUSTOM.x = float(remaining | (((previous + 1) & 255) << 16));
        CUSTOM.z = float((packed & 65280) | frame);
    }
}
)";

constexpr auto particleDraw = R"(
shader_type spatial;
render_mode unshaded, blend_mix, cull_disabled, depth_draw_never, shadows_disabled;
uniform sampler2D atlas : source_color, filter_linear, repeat_disable;
uniform vec4 color_curve[4];
uniform vec4 size_curve[4];
uniform vec2 atlas_size;
uniform float lifetime;
uniform float pixel_cap;
uniform float interest_distance;
uniform float alpha_cutoff;
uniform int texture_function;
uniform sampler2D group_epochs : filter_nearest, repeat_disable;
varying vec4 tint;
varying float visible_in_view;

void vertex() {
    float age = 1.0 - float(int(INSTANCE_CUSTOM.x) & 65535) / (1024.0 * lifetime);
    vec4 shape = ((size_curve[0] * age + size_curve[1]) * age + size_curve[2]) * age + size_curve[3];
    tint = clamp(((color_curve[0] * age + color_curve[1]) * age + color_curve[2]) * age + color_curve[3],
                 vec4(0.0), vec4(255.0)) / 128.0;
    int angle = int(INSTANCE_CUSTOM.y);
    float rotation = float(angle + (((angle & 1) == 0) ? int(shape.y) : -int(shape.y))) * TAU / 65536.0;
    vec2 offset = mat2(vec2(cos(rotation), sin(rotation)), vec2(-sin(rotation), cos(rotation))) * VERTEX.xy;
    vec4 center = MODELVIEW_MATRIX * vec4(0.0, 0.0, 0.0, 1.0);
    float depth = -center.z;
    float size = clamp(shape.x, 0.0, pixel_cap * max(depth,0.0) / (240.0 * PROJECTION_MATRIX[1][1]));
    int identity = int(INSTANCE_CUSTOM.w);
    vec4 group = texelFetch(group_epochs, ivec2(identity & 1023, 0), 0);
    int mask = int(group.g);
    int view = int(CAMERA_VISIBLE_LAYERS) & 6;
    visible_in_view = float(int(group.r) == (identity >> 10) && (mask & view) != 0 &&
        depth > 0.0 && depth < interest_distance && size > 0.0);
    POSITION = PROJECTION_MATRIX * (center + vec4(offset * size * vec2(1.0, 0.875), 0.0, 0.0));
    float frame = float(int(INSTANCE_CUSTOM.z) & 255);
    UV = (UV + vec2(mod(frame, atlas_size.x), floor(frame / atlas_size.x))) / atlas_size;
}

void fragment() {
    if (visible_in_view < 0.5) discard;
    vec4 texel = texture(atlas, UV);
    vec4 result = texel * tint;
    if (texture_function == 1) result = texel;
    if (texture_function >= 2) {
        result.rgb += tint.a;
        result.a = texel.a + (texture_function == 2 ? tint.a : 0.0);
    }
    ALBEDO = result.rgb;
    ALPHA = clamp(result.a, 0.0, 1.0);
    if (ALPHA <= alpha_cutoff) discard;
}
)";

constexpr auto sunDraw = R"(
shader_type spatial;
render_mode unshaded, blend_add, cull_disabled, depth_draw_never, depth_test_disabled, shadows_disabled;
uniform sampler2D depth_texture : hint_depth_texture, filter_nearest, repeat_disable;
uniform sampler2D flare : source_color, filter_linear, repeat_disable;
uniform vec2 sun_uv;
uniform vec2 center_uv;
uniform vec2 extent_uv;
uniform float rotation;
uniform float source_scale;
uniform vec4 tint;
uniform bool sample_sky;
uniform bool depth_test_disc;
varying float visibility;

void vertex() {
    visibility = 1.0;
    if (sample_sky || depth_test_disc) {
        visibility = 0.0;
        vec2 origin = floor(sun_uv * VIEWPORT_SIZE / source_scale - vec2(16.0));
        for (int y = 0; y < 32; ++y) {
            for (int x = 0; x < 32; ++x) {
                vec2 uv = (origin + vec2(float(x), float(y)) + 0.5) * source_scale / VIEWPORT_SIZE;
                if (all(greaterThanEqual(uv, vec2(0.0))) && all(lessThan(uv, vec2(1.0)))) {
                    visibility += float(textureLod(depth_texture, uv, 0.0).r <= 0.00000012);
                }
            }
        }
        visibility /= 1024.0;
    }
    vec2 p = mat2(vec2(cos(rotation), sin(rotation)), vec2(-sin(rotation), cos(rotation))) * (UV * 2.0 - 1.0);
    vec2 pixel = center_uv + p * extent_uv;
    POSITION = vec4(pixel.x * 2.0 - 1.0, 1.0 - pixel.y * 2.0, 1.0, 1.0);
}

void fragment() {
    vec4 texel = texture(flare, UV);
    ALBEDO = texel.rgb * tint.rgb;
    ALPHA = clamp(texel.a * tint.a * visibility, 0.0, 1.0);
}
)";

constexpr auto lightDraw = R"(
shader_type spatial;
render_mode unshaded, blend_mix, cull_disabled, depth_draw_never, shadows_disabled;
uniform sampler2D flare : source_color, filter_linear, repeat_disable;
instance uniform vec4 normal_and_sides;
instance uniform vec4 response;
instance uniform vec4 distance_scale;
instance uniform vec4 minimum_color;
instance uniform vec4 maximum_color;
varying vec4 tint;

void vertex() {
    vec3 center = MODEL_MATRIX[3].xyz;
    vec3 to_camera = normalize(INV_VIEW_MATRIX[3].xyz - center);
    float facing = dot(normal_and_sides.xyz, to_camera);
    if (normal_and_sides.w > 0.5) facing = abs(facing);
    float blend = pow(max(facing, 0.01), response.z) * response.w;
    vec4 view_center = VIEW_MATRIX * vec4(center, 1.0);
    float focal = 240.0 * PROJECTION_MATRIX[1][1];
    float size = mix(response.x, response.y, blend) *
        clamp(-view_center.z / focal, distance_scale.x, distance_scale.y);
    vec4 biased = VIEW_MATRIX * vec4(center + to_camera * distance_scale.z, 1.0);
    POSITION = PROJECTION_MATRIX * (biased + vec4(VERTEX.xy * 2.0 * size, 0.0, 0.0));
    tint = mix(minimum_color, maximum_color, blend);
}

void fragment() {
    vec4 texel = texture(flare, UV);
    ALBEDO = texel.rgb * tint.rgb;
    ALPHA = clamp(texel.a * tint.a, 0.0, 1.0);
}
)";

Ref<Shader> shader(const char* source) {
    Ref<Shader> result;
    result.instantiate();
    result->set_code(source);
    return result;
}

Ref<ShaderMaterial> material(const Ref<Shader>& code) {
    Ref<ShaderMaterial> result;
    result.instantiate();
    result->set_shader(code);
    return result;
}

BaseMaterial3D::BlendMode texture_blend(const EditorMarkerTexture& texture) {
    // GS ALPHA packs A, B, C and D for (A-B)*C+D into two-bit lanes.
    const auto alpha=texture.authoredRecord[0x76];
    if (alpha==0x48) return BaseMaterial3D::BLEND_MODE_ADD;
    if (alpha==0x42) return BaseMaterial3D::BLEND_MODE_SUB;
    return BaseMaterial3D::BLEND_MODE_MIX;
}

Ref<Shader> effect_shader(const char* source, BaseMaterial3D::BlendMode blend) {
    std::string code(source);
    const auto position=code.find("blend_mix");
    if (blend!=BaseMaterial3D::BLEND_MODE_MIX)
        code.replace(position,9,blend==BaseMaterial3D::BLEND_MODE_ADD ? "blend_add":"blend_sub");
    return shader(code.c_str());
}

Color source_color(const std::array<float,4>& rgba) {
    return Color(rgba[0]/128.0f,rgba[1]/128.0f,rgba[2]/128.0f,rgba[3]/128.0f);
}

PackedVector4Array curve(const float* values) {
    PackedVector4Array result;
    for (int i=0;i<16;i+=4) result.push_back({values[i],values[i+1],values[i+2],values[i+3]});
    return result;
}

constexpr int historySize=16;
}

struct RaceEffectsPresentation::Storage {
    struct Particle {
        GPUParticles3D* node{};
        Ref<ShaderMaterial> process;
        Ref<ShaderMaterial> draw;
        bool dirty{};
    };

    struct Mark {
        MeshInstance3D* node{};
        std::uint64_t revision{};
        std::uint8_t fade{};
    };

    explicit Storage(Node3D& scene) {
        root=memnew(Node3D);
        root->set_name("RaceEffects");
        scene.add_child(root);
        quad.instantiate();
        quad->set_size({1,1});
    }

    Node3D* root{};
    Ref<QuadMesh> quad;
    Ref<Shader> processShader=shader(particleProcess);
    std::array<Ref<Shader>,3> drawShaders{
        effect_shader(particleDraw,BaseMaterial3D::BLEND_MODE_MIX),
        effect_shader(particleDraw,BaseMaterial3D::BLEND_MODE_ADD),
        effect_shader(particleDraw,BaseMaterial3D::BLEND_MODE_SUB)};
    std::array<Ref<Shader>,3> lightShaders{
        effect_shader(lightDraw,BaseMaterial3D::BLEND_MODE_MIX),
        effect_shader(lightDraw,BaseMaterial3D::BLEND_MODE_ADD),
        effect_shader(lightDraw,BaseMaterial3D::BLEND_MODE_SUB)};
    Ref<Shader> sunAddShader=shader(sunDraw);
    Ref<Shader> sunMixShader=[] {
        std::string code(sunDraw);
        code.replace(code.find("blend_add"),9,"blend_mix");
        return shader(code.c_str());
    }();
    std::unordered_map<const EditorMarkerTexture*,Ref<Texture2D>> textures;
    std::unordered_map<const EditorMarkerTexture*,Ref<StandardMaterial3D>> skidMaterials;
    std::unordered_map<const EditorMarkerTexture*,Ref<ShaderMaterial>> lightMaterials;
    std::unordered_map<const WorldParticleDefinition*,Particle> particles;
    static constexpr int slots=800;
    struct Group {
        std::uint64_t generation{};
        Particle* particle{};
        int epoch{};
        int sequence{};
        int mask{};
    };
    std::array<Group,slots> groups;
    // Store history by source group slot. Each node consumes all groups that
    // share its authored definition without issuing a draw for every emitter.
    std::array<std::array<float,4>,slots*historySize*2> steps{};
    std::array<std::array<float,4>,slots> epochs{};
    Ref<ImageTexture> stepTexture, epochTexture;
    PackedByteArray stepBytes, epochBytes;
    bool epochsDirty{};
    std::array<Mark,128> marks, rearMarks;
    std::vector<MeshInstance3D*> lights, rearLights;
    std::vector<MeshInstance3D*> sun;

    Ref<Texture2D> texture(const EditorMarkerTexture* descriptor) {
        auto [it, inserted]=textures.try_emplace(descriptor);
        if (!inserted) return it->second;
        const auto* source=textureInfoForDescriptor(descriptor);
        if (!source || !source->pixels.Data()) throw std::runtime_error("Original effect texture is unloaded");
        std::uint32_t width{},height{};
        auto rgba=DecodeTextureMipRgba(*source,0,0,source->pixels.size,width,height);
        for (std::size_t i=3;i<rgba.size();i+=4) rgba[i]=std::min(255,int(rgba[i])*2);
        PackedByteArray bytes;
        bytes.resize(rgba.size());
        std::memcpy(bytes.ptrw(),rgba.data(),rgba.size());
        auto image=Image::create_from_data(width,height,false,Image::FORMAT_RGBA8,bytes);
        if (image.is_null()) throw std::runtime_error("Godot could not decode an effect texture");
        it->second=ImageTexture::create_from_image(image);
        return it->second;
    }

    Ref<StandardMaterial3D> skid_material(const EditorMarkerTexture* descriptor) {
        auto [it, inserted]=skidMaterials.try_emplace(descriptor);
        if (!inserted) return it->second;
        auto& mat=it->second;
        mat.instantiate();
        mat->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
        mat->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
        mat->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
        mat->set_depth_draw_mode(BaseMaterial3D::DEPTH_DRAW_DISABLED);
        mat->set_texture(BaseMaterial3D::TEXTURE_ALBEDO,texture(descriptor));
        mat->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR,true);
        mat->set_blend_mode(texture_blend(*descriptor));
        return mat;
    }

    Ref<ShaderMaterial> light_material(const EditorMarkerTexture* descriptor) {
        auto [it, inserted]=lightMaterials.try_emplace(descriptor);
        if (inserted) {
            it->second=material(lightShaders.at(texture_blend(*descriptor)));
            it->second->set_shader_parameter("flare",texture(descriptor));
        }
        return it->second;
    }

    MeshInstance3D& mesh() {
        auto* instance=memnew(MeshInstance3D);
        root->add_child(instance);
        instance->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
        instance->set_layer_mask(2);
        return *instance;
    }

    void upload_texture(Ref<ImageTexture>& target,PackedByteArray& bytes,const void* data,
        int width,int height) {
        bytes.resize(width*height*4*sizeof(float));
        std::memcpy(bytes.ptrw(),data,bytes.size());
        auto image=Image::create_from_data(width,height,false,Image::FORMAT_RGBAF,bytes);
        if (target.is_null()) target=ImageTexture::create_from_image(image);
        else target->update(image);
    }

    Particle& acquire(const RaceParticleEvent& request) {
        const auto& definition=*request.event.definition;
        auto [it,added]=particles.try_emplace(&definition);
        auto& particle=it->second;
        if (!added) return particle;
        if (definition.atlasWidthPower<0 || definition.atlasHeightPower<0 ||
            definition.atlasWidthPower+definition.atlasHeightPower>8 || definition.lifetime<=0)
            throw std::runtime_error("Invalid authored AcidFX particle definition");
        if (stepTexture.is_null()) {
            upload_texture(stepTexture,stepBytes,steps.data(),slots,historySize*2);
            upload_texture(epochTexture,epochBytes,epochs.data(),slots,1);
        }
        auto draw=material(drawShaders.at(texture_blend(*request.texture)));
        particle.draw=draw;
        draw->set_shader_parameter("atlas",texture(request.texture));
        draw->set_shader_parameter("color_curve",curve(definition.colorCurve));
        draw->set_shader_parameter("size_curve",curve(definition.sizeRotationCurve));
        draw->set_shader_parameter("atlas_size",Vector2(1u<<definition.atlasWidthPower,1u<<definition.atlasHeightPower));
        draw->set_shader_parameter("lifetime",definition.lifetime);
        draw->set_shader_parameter("pixel_cap",definition.maximumPixelSize==0 ? 512.0f:definition.maximumPixelSize);
        draw->set_shader_parameter("interest_distance",definition.interestDistance);
        const auto* source=textureInfoForDescriptor(request.texture);
        draw->set_shader_parameter("alpha_cutoff",request.texture->authoredRecord[0x4f] ||
            source->drawFlag4Enabled ? -1.0f:48.0f/128.0f);
        draw->set_shader_parameter("texture_function",request.texture->authoredRecord[0x4e]);
        draw->set_shader_parameter("group_epochs",epochTexture);
        auto* node=memnew(GPUParticles3D);
        particle.node=node;
        particle.process=material(processShader);
        particle.process->set_shader_parameter("step_history",stepTexture);
        particle.process->set_shader_parameter("group_epochs",epochTexture);
        particle.process->set_shader_parameter("drag",definition.drag);
        particle.process->set_shader_parameter("gravity",definition.gravity);
        particle.process->set_shader_parameter("atlas_cells",(1u<<definition.atlasWidthPower)*(1u<<definition.atlasHeightPower));
        particle.process->set_shader_parameter("atlas_period",definition.animationRate ? 1024/definition.animationRate:0);
        node->set_process_material(particle.process);
        node->set_material_override(draw);
        node->set_emitting(false);
        node->set_amount(1000);
        node->set_lifetime(60);
        node->set_speed_scale(0);
        node->set_fixed_fps(0);
        node->set_interpolate(false);
        node->set_fractional_delta(false);
        node->set_use_local_coordinates(true);
        node->set_draw_pass_mesh(0,quad);
        node->set_draw_order(GPUParticles3D::DRAW_ORDER_VIEW_DEPTH);
        node->set_layer_mask(6);
        node->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
        node->set_visibility_aabb(AABB(Vector3(-16000,-16000,-16000),Vector3(32000,32000,32000)));
        root->add_child(node);
        return particle;
    }

    void update_particles(const RaceEffectsFrame& frame,std::uint64_t generation) {
        bool stepsDirty=false;
        for (const auto& request:frame.particles) {
            if (request.world!=generation) continue;
            const auto& event=request.event;
            const int slot=event.slot+(request.frameEffects ? slots/2:0);
            auto& group=groups.at(slot);
            if (event.operation==ParticleEffectOperation::Spawn) {
                if (!request.texture) continue;
                if (group.generation!=event.generation) {
                    group.particle=&acquire(request);
                    group.generation=event.generation;
                    group.sequence=0;
                    group.mask=generation==0 ? 2:0;
                    group.epoch=(group.epoch+1)&0x3fff;
                    epochs[slot]={float(group.epoch),float(group.mask),0,0};
                    epochsDirty=true;
                }
                auto& particle=*group.particle;
                const auto& birth=event.birth;
                if (birth.remainingTicks<=0) continue;
                const float scale=static_cast<float>(birth.speed)/(128.0f*127.0f);
                const Vector3 velocity(birth.direction[0]*scale,birth.direction[2]*scale,-birth.direction[1]*scale);
                const int packed=birth.atlasFrame+256*birth.carMotionScale;
                const int life=birth.remainingTicks|((group.sequence&255)<<16);
                const int identity=slot|(group.epoch<<10);
                particle.node->emit_particle(Transform3D(Basis(),race_vector(birth.position)),velocity,
                    Color(1,1,1,1),Color(life,birth.rotation,packed,identity),31);
                particle.dirty=true;
            } else if (group.particle && group.generation==event.generation) {
                group.particle->dirty=true;
                if (event.operation==ParticleEffectOperation::Remove) {
                    group.particle=nullptr;
                    group.generation=0;
                    group.epoch=(group.epoch+1)&0x3fff;
                    epochs[slot]={float(group.epoch),0,0,0};
                    epochsDirty=true;
                    continue;
                }
                const auto velocity=race_vector(event.inherited);
                const int index=group.sequence++%historySize;
                steps[index*slots+slot]={velocity.x,velocity.y,velocity.z,event.seconds};
                steps[(index+historySize)*slots+slot]={float(event.ticks),0,0,0};
                epochs[slot][2]=float(group.sequence);
                epochsDirty=stepsDirty=true;
            }
        }
        for (const auto& visible:frame.visibility) {
            const auto slot=visible.slot+(visible.frameEffects ? slots/2:0);
            auto& group=groups.at(slot);
            if (!group.particle || group.generation!=visible.generation) continue;
            const int mask=(visible.main ? 2:0)|(visible.rear ? 4:0);
            if (group.mask==mask) continue;
            group.mask=mask;
            epochs[slot][1]=float(mask);
            epochsDirty=true;
        }
        if (stepsDirty) upload_texture(stepTexture,stepBytes,steps.data(),slots,historySize*2);
        if (epochsDirty && epochTexture.is_valid()) upload_texture(epochTexture,epochBytes,epochs.data(),slots,1);
        epochsDirty=false;
        for (auto& [definition,particle]:particles) {
            if (!particle.dirty) continue;
            if (!generation) {
                particle.draw->set_shader_parameter("color_curve",curve(definition->colorCurve));
                particle.draw->set_shader_parameter("size_curve",curve(definition->sizeRotationCurve));
            }
            RenderingServer::get_singleton()->particles_request_process(particle.node->get_base());
            particle.dirty=false;
        }
    }

    void update_skids(const std::vector<SkidRenderQuad>& skids,std::array<Mark,128>& entries,int layer) {
        std::array<bool,128> visible{};
        for (std::size_t start=0;start<skids.size();) {
            const auto& first=skids[start];
            auto end=start+1;
            while (end<skids.size() && skids[end].trail==first.trail) ++end;
            auto& mark=entries.at(first.trail);
            visible[first.trail]=true;
            if (!mark.node) {
                mark.node=&mesh();
                mark.node->set_layer_mask(layer);
            }
            mark.node->show();
            if (mark.revision!=first.revision || mark.fade!=first.fade) {
                PackedVector3Array vertices;
                PackedVector2Array uv;
                PackedColorArray colors;
                PackedInt32Array indices;
                for (auto i=start;i<end;++i) {
                    const auto index=vertices.size();
                    for (const auto& vertex:skids[i].vertices) {
                        vertices.push_back(race_vector(vertex.position.data()));
                        uv.push_back({vertex.textureCoordinate[0],vertex.textureCoordinate[1]});
                        colors.push_back({1,1,1,float(vertex.color>>24)/128.0f});
                    }
                    for (int offset:{0,1,2,0,2,3}) indices.push_back(index+offset);
                }
                Array arrays;
                arrays.resize(Mesh::ARRAY_MAX);
                arrays[Mesh::ARRAY_VERTEX]=vertices;
                arrays[Mesh::ARRAY_TEX_UV]=uv;
                arrays[Mesh::ARRAY_COLOR]=colors;
                arrays[Mesh::ARRAY_INDEX]=indices;
                Ref<ArrayMesh> strip;
                strip.instantiate();
                strip->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES,arrays);
                mark.node->set_mesh(strip);
                mark.node->set_material_override(skid_material(first.texture));
                mark.revision=first.revision;
                mark.fade=first.fade;
            }
            start=end;
        }
        for (std::size_t i=0;i<entries.size();++i)
            if (entries[i].node && !visible[i]) entries[i].node->hide();
    }

    void update_lights(const std::vector<RaceLightEffect>& effects,Camera3D& camera,
        std::vector<MeshInstance3D*>& sprites,int view,int layer) {
        std::size_t count=0;
        for (const auto& light:effects) {
            if (light.view!=view) continue;
            const auto position=race_vector(light.position.data());
            if (!camera.is_position_in_frustum(position)) continue;
            if (count==sprites.size()) {
                auto& sprite=mesh();
                sprite.set_mesh(quad);
                sprite.set_layer_mask(layer);
                sprite.set_extra_cull_margin(8.0f);
                sprites.push_back(&sprite);
            }
            auto& sprite=*sprites[count++];
            const auto normal=race_vector(light.normal.data());
            const auto mat=light_material(light.texture);
            if (sprite.get_material_override()!=mat) sprite.set_material_override(mat);
            sprite.set_instance_shader_parameter("normal_and_sides",Vector4(normal.x,normal.y,normal.z,light.twoSided));
            sprite.set_instance_shader_parameter("response",Vector4(light.minimumSize,light.maximumSize,light.exponent,light.intensity));
            sprite.set_instance_shader_parameter("distance_scale",Vector4(light.minimumScale,light.maximumScale,light.bias,0));
            sprite.set_instance_shader_parameter("minimum_color",source_color(light.minimumColor));
            sprite.set_instance_shader_parameter("maximum_color",source_color(light.maximumColor));
            sprite.set_position(position);
            sprite.show();
        }
        for (;count<sprites.size();++count) sprites[count]->hide();
    }

    void update_sun(const std::vector<RaceSunEffect>& effects,Camera3D& camera) {
        const auto viewport=camera.get_viewport()->get_visible_rect().size;
        if (viewport.x<=0 || viewport.y<=0) return;
        const float scale=viewport.y/480.0f;
        std::size_t count=0;
        for (const auto& effect:effects) {
            const auto position=race_vector(effect.position.data());
            if (camera.is_position_behind(position)) continue;
            auto projected=camera.unproject_position(position);
            if (effect.positionOverride[0]!=0 || effect.positionOverride[1]!=0)
                projected=Vector2(effect.positionOverride[0],effect.positionOverride[1])*scale;
            const auto& row=effect.parameters;
            const float halfSize=row.size*effect.sizeScale*scale*0.5f;
            const float margin=effect.margin*scale;
            if (!Rect2(Vector2(-margin,-margin),viewport+Vector2(2*margin,2*margin)).has_point(projected)) continue;
            const auto center=projected+Vector2(row.offset[0],row.offset[1])*scale;
            if (!Rect2(-Vector2(halfSize,halfSize),viewport+Vector2(2*halfSize,2*halfSize)).has_point(center)) continue;
            if (count==sun.size()) {
                auto& sprite=mesh();
                sprite.set_name(String("SunFlare")+String::num_uint64(count));
                sprite.set_mesh(quad);
                sprite.set_material_override(material(row.enabled ? sunAddShader:sunMixShader));
                sprite.set_custom_aabb(AABB(Vector3(-16000,-16000,-16000),Vector3(32000,32000,32000)));
                sprite.set_extra_cull_margin(1.0f);
                sun.push_back(&sprite);
            }
            auto& sprite=*sun[count++];
            Ref<ShaderMaterial> mat=sprite.get_material_override();
            const auto& shader=row.enabled ? sunAddShader:sunMixShader;
            if (mat->get_shader()!=shader) mat->set_shader(shader);
            mat->set_shader_parameter("flare",texture(effect.texture));
            mat->set_shader_parameter("sun_uv",projected/viewport);
            mat->set_shader_parameter("center_uv",center/viewport);
            mat->set_shader_parameter("extent_uv",Vector2(halfSize,halfSize)/viewport);
            const float angleFraction=(projected.x+margin)/(viewport.x+2*margin);
            mat->set_shader_parameter("rotation",(row.angle/65536.0f+row.parameter20*angleFraction)*6.283185307f);
            mat->set_shader_parameter("tint",Color(row.color[0]/128.0f,row.color[1]/128.0f,row.color[2]/128.0f,effect.opacityScale));
            mat->set_shader_parameter("source_scale",scale);
            mat->set_shader_parameter("sample_sky",effect.sampleSky);
            mat->set_shader_parameter("depth_test_disc",effect.depthTestDisc);
            sprite.show();
        }
        for (;count<sun.size();++count) sun[count]->hide();
    }
};

RaceEffectsPresentation::RaceEffectsPresentation(Node3D& scene):storage_(std::make_unique<Storage>(scene)) {}
RaceEffectsPresentation::~RaceEffectsPresentation()=default;

void RaceEffectsPresentation::update(const RaceEffectsFrame& frame,Camera3D& camera,Camera3D* rear,
    std::uint64_t generation) {
    storage_->update_particles(frame,generation);
    storage_->update_skids(frame.skids,storage_->marks,2);
    storage_->update_lights(frame.lights,camera,storage_->lights,1,2);
    storage_->update_skids(frame.rearSkids,storage_->rearMarks,4);
    if (rear) storage_->update_lights(frame.lights,*rear,storage_->rearLights,3,4);
    storage_->update_sun(frame.sun,camera);
}

void RaceEffectsPresentation::update_preview_lights(const std::vector<RaceLightEffect>& lights,Camera3D& camera,int view) {
    storage_->update_lights(lights,camera,storage_->lights,view,1);
}

void RaceEffectsPresentation::update_preview_particles(const RaceEffectsFrame& frame) {
    storage_->update_particles(frame,0);
}

void RaceEffectsPresentation::reset() {
    for (auto& [definition,particle]:storage_->particles) {
        particle.node->restart();
        particle.node->set_emitting(false);
    }
    for (auto& group:storage_->groups) {
        if (!group.generation) continue;
        const auto slot=&group-storage_->groups.data();
        group.particle=nullptr;
        group.generation=0;
        group.epoch=(group.epoch+1)&0x3fff;
        storage_->epochs[slot]={float(group.epoch),0,0,0};
        storage_->epochsDirty=true;
    }
}
}
