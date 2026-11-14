#include "race_effects_presentation.hpp"

#include "assets/shader_resource.hpp"

#include "assets/mesh_converter.hpp"
#include "indep/inc/Texture.hpp"
#include "bware/inc/bWare.hpp"
#include "bware/inc/bMath.hpp"
#include "systems/track/track_presentation.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/gpu_particles3d.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/gradient.hpp>
#include <godot_cpp/classes/gradient_texture1_d.hpp>
#include <godot_cpp/classes/particle_process_material.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/world3d.hpp>
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

// StuffSun / StuffSunGlare and ePoly::Render (Debug 125430, 125A70, 10FE48).
// Composite the complete ordered stack over Godot's opaque scene in GS byte
// space. Forward+ uses linear color, so convert only at the pass boundary.
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

Color source_color(const std::array<float,4>& rgba) {
    return Color(rgba[0]/128.0f,rgba[1]/128.0f,rgba[2]/128.0f,rgba[3]/128.0f);
}

PackedVector4Array curve(const float* values) {
    PackedVector4Array result;
    for (int i=0;i<16;i+=4) result.push_back({values[i],values[i+1],values[i+2],values[i+3]});
    return result;
}

constexpr int historySize=16;
constexpr int maxSunLayers = 12; // Six glare and six track rows.
enum SunFlags { SunAdd = 4, SunDisc = 8, SunSampleSky = 16 };
}

struct RaceEffectsPresentation::Storage {
    struct Particle {
        GPUParticles3D* node{};
        Ref<ShaderMaterial> process;
        Ref<ShaderMaterial> draw;
        bool dirty{};
        int groups{};
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
    SubViewport* skyViewport{};
    Camera3D* skyCamera{};
    Ref<QuadMesh> quad;
    Ref<Shader> processShader=load_shader("res://shader/particle_process.gdshader");
    std::array<Ref<Shader>,3> drawShaders{
        load_shader("res://shader/particle_billboard_mix.gdshader"),
        load_shader("res://shader/particle_billboard_add.gdshader"),
        load_shader("res://shader/particle_billboard_sub.gdshader")};
    std::array<Ref<Shader>,3> lightShaders{
        load_shader("res://shader/light_flare_mix.gdshader"),
        load_shader("res://shader/light_flare_add.gdshader"),
        load_shader("res://shader/light_flare_sub.gdshader")};
    Ref<Shader> sunShader=load_shader("res://shader/sun_occlusion.gdshader");
    std::unordered_map<const EditorMarkerTexture*,Ref<Texture2D>> textures, sunTextures;
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
    Ref<Image> stepImage, epochImage;
    GPUParticles3D* exhaust{};
    struct ExhaustClock {
        std::uint64_t car{};
        std::size_t outlet{};
        float remainder{};
    };
    std::vector<ExhaustClock> exhaustClocks;
    int exhaustFrame{-1};
    PackedByteArray stepBytes, epochBytes;
    bool epochsDirty{};
    bool particlesVisible{true};
    std::array<Mark,128> marks, rearMarks;
    std::vector<MeshInstance3D*> lights, rearLights;
    MeshInstance3D* sun{};
    Ref<ShaderMaterial> sunMaterial;
    std::array<const EditorMarkerTexture*,5> sunBindings{};
    PackedVector4Array sunRects, sunColors, sunStates;

    Ref<Texture2D> texture(const EditorMarkerTexture* descriptor, bool rawGs = false) {
        auto [it, inserted]=(rawGs ? sunTextures : textures).try_emplace(descriptor);
        if (!inserted) return it->second;
        const auto* source=textureInfoForDescriptor(descriptor);
        if (!source || !source->pixels.Data()) throw std::runtime_error("Original effect texture is unloaded");
        std::uint32_t width{},height{};
        auto rgba=DecodeTextureMipRgba(*source,0,0,source->pixels.size,width,height);
        if (!rawGs) {
            for (std::size_t i=3;i<rgba.size();i+=4) rgba[i]=std::min(255,int(rgba[i])*2);
        }
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

    void upload_texture(Ref<ImageTexture>& target,Ref<Image>& image,PackedByteArray& bytes,const void* data,
        int width,int height) {
        bytes.resize(width*height*4*sizeof(float));
        std::memcpy(bytes.ptrw(),data,bytes.size());
        if (target.is_null()) {
            image=Image::create_from_data(width,height,false,Image::FORMAT_RGBAF,bytes);
            target=ImageTexture::create_from_image(image);
        } else {
            image->set_data(width,height,false,Image::FORMAT_RGBAF,bytes);
            target->update(image);
        }
    }

    Particle& acquire(const WorldParticleDefinition& definition, const EditorMarkerTexture& descriptor) {
        auto [it,added]=particles.try_emplace(&definition);
        auto& particle=it->second;
        if (!added) return particle;
        if (definition.atlasWidthPower<0 || definition.atlasHeightPower<0 ||
            definition.atlasWidthPower+definition.atlasHeightPower>8 || definition.lifetime<=0)
            throw std::runtime_error("Invalid authored AcidFX particle definition");
        if (stepTexture.is_null()) {
            upload_texture(stepTexture,stepImage,stepBytes,steps.data(),slots,historySize*2);
            upload_texture(epochTexture,epochImage,epochBytes,epochs.data(),slots,1);
        }
        auto draw=material(drawShaders.at(texture_blend(descriptor)));
        particle.draw=draw;
        draw->set_shader_parameter("atlas",texture(&descriptor));
        draw->set_shader_parameter("color_curve",curve(definition.colorCurve));
        draw->set_shader_parameter("size_curve",curve(definition.sizeRotationCurve));
        draw->set_shader_parameter("atlas_size",Vector2(1u<<definition.atlasWidthPower,1u<<definition.atlasHeightPower));
        draw->set_shader_parameter("lifetime",definition.lifetime);
        draw->set_shader_parameter("pixel_cap",definition.maximumPixelSize==0 ? 512.0f:definition.maximumPixelSize);
        draw->set_shader_parameter("interest_distance",definition.interestDistance);
        const auto* source=textureInfoForDescriptor(&descriptor);
        draw->set_shader_parameter("alpha_cutoff",descriptor.authoredRecord[0x4f] ||
            source->drawFlag4Enabled ? -1.0f:48.0f/128.0f);
        draw->set_shader_parameter("texture_function",descriptor.authoredRecord[0x4e]);
        draw->set_shader_parameter("group_epochs",epochTexture);
        auto* node=memnew(GPUParticles3D);
        particle.node=node;
        node->set_meta("source_effect_group", static_cast<std::int64_t>(definition.groupKey));
        node->set_meta("source_effect_key", static_cast<std::int64_t>(definition.key));
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
        node->hide();
        return particle;
    }

    void prepare_exhaust(const EditorMarkerTexture& descriptor) {
        // The shipped CARFX_EXHAUST_SMOKE slot has no emitter definition.
        // Use its authored tailpipe markers and reuse the original smoke atlas.
        Ref<ParticleProcessMaterial> process;
        process.instantiate();
        process->set_gravity(Vector3(0, 0.35f, 0));
        process->set_param(ParticleProcessMaterial::PARAM_SCALE, Vector2(0.75f, 1.2f));
        Ref<Gradient> fade;
        fade.instantiate();
        fade->set_color(0, Color(0.72f, 0.72f, 0.72f, 0.24f));
        fade->set_color(1, Color(0.72f, 0.72f, 0.72f, 0));
        Ref<GradientTexture1D> ramp;
        ramp.instantiate();
        ramp->set_gradient(fade);
        process->set_color_ramp(ramp);

        Ref<StandardMaterial3D> draw;
        draw.instantiate();
        draw->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
        draw->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
        draw->set_depth_draw_mode(BaseMaterial3D::DEPTH_DRAW_DISABLED);
        draw->set_proximity_fade_enabled(true);
        draw->set_proximity_fade_distance(0.3f);
        draw->set_billboard_mode(BaseMaterial3D::BILLBOARD_PARTICLES);
        draw->set_particles_anim_h_frames(4);
        draw->set_particles_anim_v_frames(4);
        draw->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
        draw->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, texture(&descriptor));
        Ref<QuadMesh> mesh;
        mesh.instantiate();
        mesh->set_size(Vector2(0.38f, 0.38f));
        exhaust = memnew(GPUParticles3D);
        exhaust->set_name("TailpipeExhaust");
        exhaust->set_process_material(process);
        exhaust->set_material_override(draw);
        exhaust->set_draw_pass_mesh(0, mesh);
        exhaust->set_amount(192);
        exhaust->set_lifetime(0.65);
        exhaust->set_emitting(false);
        exhaust->set_layer_mask(6);
        exhaust->set_use_local_coordinates(false);
        exhaust->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
        exhaust->set_visibility_aabb(AABB(Vector3(-16000,-16000,-16000),Vector3(32000,32000,32000)));
        root->add_child(exhaust);
        exhaustClocks.reserve(64);
    }

    void update_exhaust(const std::vector<RaceExhaustOutlet>& outlets, Camera3D& camera, int worldFrames) {
        if (!exhaust) return;
        const int elapsed = exhaustFrame < 0 ? 0 : std::clamp(worldFrames - exhaustFrame, 0, 6);
        exhaustFrame = worldFrames;
        exhaust->set_speed_scale(elapsed * (1.0 / 60.0) / std::max(1.0e-4, camera.get_process_delta_time()));
        if (!elapsed) return;
        const auto eye = camera.get_global_position();
        exhaustClocks.resize(outlets.size());
        for (std::size_t i = 0; i < outlets.size(); ++i) {
            const auto& outlet = outlets[i];
            auto& clock = exhaustClocks[i];
            if (clock.car != outlet.car || clock.outlet != outlet.outlet)
                clock = {outlet.car, outlet.outlet, 0};
            const auto position = race_vector(outlet.position.data());
            if (!outlet.active || position.distance_squared_to(eye) > 3600.0f) {
                clock.remainder = 0;
                continue;
            }
            clock.remainder += elapsed * (4.0f + 12.0f * outlet.throttle) / 60.0f;
            const int count = static_cast<int>(clock.remainder);
            clock.remainder -= count;
            const auto velocity = race_vector(outlet.velocity.data()) * 0.45f +
                race_vector(outlet.direction.data()) * (0.9f + 1.8f * outlet.throttle);
            for (int i = 0; i < count; ++i)
                exhaust->emit_particle(Transform3D(Basis(), position), velocity, Color(1,1,1), Color(0,0,0,0), 31);
        }
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
                    if (group.particle && --group.particle->groups == 0) group.particle->node->hide();
                    group.particle=&acquire(*event.definition, *request.texture);
                    if (group.particle->groups++ == 0 && particlesVisible) group.particle->node->show();
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
                    if (--group.particle->groups == 0) group.particle->node->hide();
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
        if (stepsDirty) upload_texture(stepTexture,stepImage,stepBytes,steps.data(),slots,historySize*2);
        if (epochsDirty && epochTexture.is_valid()) upload_texture(epochTexture,epochImage,epochBytes,epochs.data(),slots,1);
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

    void update_sun_sky(Camera3D& camera, Vector2i viewport) {
        if (!skyViewport) {
            if (!root->get_parent()->has_node("TrackSky")) return;
            skyViewport = memnew(SubViewport);
            skyViewport->set_name("SunSkyTarget");
            skyViewport->set_world_3d(camera.get_world_3d());
            root->add_child(skyViewport);
            RenderingServer::get_singleton()->viewport_set_disable_2d(skyViewport->get_viewport_rid(), true);
            skyCamera = memnew(Camera3D);
            skyViewport->add_child(skyCamera);
            skyCamera->set_cull_mask(trackSkyLayer);
            skyCamera->make_current();
            sunMaterial->set_shader_parameter("sky_color", skyViewport->get_texture());
            sunMaterial->set_shader_parameter("has_sky", true);
        }
        if (skyViewport->get_size() != viewport) skyViewport->set_size(viewport);
        skyCamera->set_global_transform(camera.get_global_transform());
        skyCamera->set_keep_aspect_mode(camera.get_keep_aspect_mode());
        skyCamera->set_perspective(camera.get_fov(), camera.get_near(), camera.get_far());
        skyViewport->set_update_mode(SubViewport::UPDATE_ALWAYS);
    }

    void update_sun(const std::vector<RaceSunEffect>& effects, Camera3D& camera, bool ntsc) {
        const auto viewport = camera.get_viewport()->get_visible_rect().size;
        if (viewport.x <= 0 || viewport.y <= 0) return;
        // Same source focal ratio as the primary Godot camera and ProjectSun.
        const float scale = viewport.y * (ntsc ? 1.0714286566f / 448.0f : 1.285f / 512.0f);
        Rect2 bounds;
        Vector2 sunUv;
        bool sampleSky = false;
        int count = 0;
        int discLayer = -1;
        for (const auto& effect : effects) {
            const auto position = race_vector(effect.position.data());
            if (camera.is_position_behind(position)) continue;
            auto projected = camera.unproject_position(position);
            if (effect.positionOverride[0] != 0 || effect.positionOverride[1] != 0)
                projected = Vector2(effect.positionOverride[0], effect.positionOverride[1]) * scale;
            const auto& row = effect.parameters;
            const float halfSize = row.size * effect.sizeScale * scale * 0.5f;
            const float margin = effect.margin * scale;
            if (!Rect2(Vector2(-margin, -margin), viewport + Vector2(2*margin, 2*margin)).has_point(projected))
                continue;
            const auto center = projected + Vector2(row.offset[0], row.offset[1]) * scale;
            const float fraction = (projected.x + margin) / (viewport.x + 2*margin);
            const auto sweep = static_cast<int>(row.parameter20 * fraction * 65536.0f);
            const auto angle = static_cast<std::uint16_t>(row.angle + sweep);
            const float cosine = vectorCosineAngle(angle);
            const float sine = rotationSineAngle(angle);
            const float radius = halfSize * (std::abs(cosine) + std::abs(sine));
            const Vector2 halfBounds(radius, radius);
            const Rect2 clipped = Rect2(center - halfBounds, 2 * halfBounds).intersection(Rect2({}, viewport));
            if (!clipped.has_area()) continue;

            if (!sun) {
                sun = &mesh();
                sun->set_name("SunFlare0");
                sun->set_mesh(quad);
                sun->set_custom_aabb(AABB(Vector3(-16000,-16000,-16000), Vector3(32000,32000,32000)));
                sunMaterial = material(sunShader);
                sunMaterial->set_render_priority(Material::RENDER_PRIORITY_MIN + 1);
                sun->set_material_override(sunMaterial);
                sunRects.resize(maxSunLayers);
                sunColors.resize(maxSunLayers);
                sunStates.resize(maxSunLayers);
            }
            const int slot = row.kind;
            if (count >= maxSunLayers || slot < 0 || slot >= static_cast<int>(sunBindings.size()))
                throw std::runtime_error("Sun layers exceed the authored bank");
            if (sunBindings[slot] != effect.texture) {
                const auto image = texture(effect.texture, true);
                const auto* wire = effect.texture->authoredRecord;
                // TEX0, TEX1 and TEST from the actual sun bank. Keep
                // unsupported resource states visible instead of inventing
                // a different filter or blend path.
                if (wire[0x4e] != 0 || wire[0x4f] == 0 || wire[0x50] != 0 ||
                    wire[0x6e] != 0 || wire[0x6f] != 1 || wire[0x70] != 3 || wire[0x71] != 0 ||
                    wire[0x72] != 0 || wire[0x73] != 0)
                    throw std::runtime_error("Unsupported authored GS state for sun texture");
                sunMaterial->set_shader_parameter(String("flare_") + String::num_int64(slot), image);
                sunBindings[slot] = effect.texture;
            }
            int flags = row.enabled ? SunAdd : 0;
            if (effect.depthTestDisc) {
                flags |= SunDisc;
                discLayer = count;
            }
            if (effect.sampleSky) {
                flags |= SunSampleSky;
                sampleSky = true;
                sunUv = projected / viewport;
            }

            const Vector2 extent = Vector2(halfSize, halfSize) / viewport;
            sunRects.set(count, Vector4(center.x / viewport.x, center.y / viewport.y, extent.x, extent.y));
            sunColors.set(count, Vector4(row.color[0], row.color[1], row.color[2], effect.opacityScale * 128.0f));
            sunStates.set(count, Vector4(cosine, sine, slot, flags));
            bounds = count == 0 ? clipped : bounds.merge(clipped);
            ++count;
        }
        if (!count) {
            if (sun) sun->hide();
            if (skyViewport) skyViewport->set_update_mode(SubViewport::UPDATE_DISABLED);
            return;
        }
        const auto start = bounds.position / viewport;
        const auto end = bounds.get_end() / viewport;
        sunMaterial->set_shader_parameter("bounds_uv", Vector4(start.x, start.y, end.x, end.y));
        sunMaterial->set_shader_parameter("layer_rect", sunRects);
        sunMaterial->set_shader_parameter("layer_color", sunColors);
        sunMaterial->set_shader_parameter("layer_state", sunStates);
        sunMaterial->set_shader_parameter("layer_count", count);
        sunMaterial->set_shader_parameter("disc_layer", discLayer);
        sunMaterial->set_shader_parameter("sun_uv", sunUv);
        sunMaterial->set_shader_parameter("source_scale", scale);
        sunMaterial->set_shader_parameter("sample_sky", sampleSky);
        update_sun_sky(camera, Vector2i(viewport));
        sun->show();
    }
};

RaceEffectsPresentation::RaceEffectsPresentation(Node3D& scene):storage_(std::make_unique<Storage>(scene)) {}
RaceEffectsPresentation::~RaceEffectsPresentation()=default;

void RaceEffectsPresentation::prepare_particles(const std::vector<RaceParticleAsset>& assets) {
    const auto smokeKey = hashString("SMOKE_CS");
    for (const auto& asset : assets) {
        storage_->acquire(*asset.definition, *asset.texture);
        if (!storage_->exhaust && asset.texture->key == smokeKey)
            storage_->prepare_exhaust(*asset.texture);
    }
}

void RaceEffectsPresentation::update(const RaceEffectsFrame& frame,Camera3D& camera,Camera3D* rear,
    std::uint64_t generation, bool ntsc, int worldFrames, bool particlesVisible, bool sunGlareVisible) {
    if (storage_->particlesVisible != particlesVisible) {
        storage_->particlesVisible = particlesVisible;
        for (auto& entry : storage_->particles)
            entry.second.node->set_visible(particlesVisible && entry.second.groups > 0);
        if (storage_->exhaust) storage_->exhaust->set_visible(particlesVisible);
    }
    storage_->update_particles(frame,generation);
    storage_->update_exhaust(frame.exhaust, camera, worldFrames);
    storage_->update_skids(frame.skids,storage_->marks,2);
    storage_->update_lights(frame.lights,camera,storage_->lights,1,2);
    storage_->update_skids(frame.rearSkids,storage_->rearMarks,4);
    if (rear) storage_->update_lights(frame.lights,*rear,storage_->rearLights,3,4);
    if (sunGlareVisible) {
        storage_->update_sun(frame.sun, camera, ntsc);
    } else {
        if (storage_->sun) storage_->sun->hide();
        if (storage_->skyViewport) storage_->skyViewport->set_update_mode(SubViewport::UPDATE_DISABLED);
    }
}

void RaceEffectsPresentation::update_preview_lights(const std::vector<RaceLightEffect>& lights,Camera3D& camera,int view) {
    storage_->update_lights(lights,camera,storage_->lights,view,1);
}

void RaceEffectsPresentation::update_preview_particles(const RaceEffectsFrame& frame) {
    storage_->update_particles(frame,0);
}

void RaceEffectsPresentation::reset() {
    if (storage_->sun) storage_->sun->hide();
    if (storage_->skyViewport) storage_->skyViewport->set_update_mode(SubViewport::UPDATE_DISABLED);
    if (storage_->exhaust) {
        storage_->exhaust->restart();
        storage_->exhaust->set_emitting(false);
        storage_->exhaustFrame = -1;
        storage_->exhaustClocks.clear();
    }
    for (auto& [definition,particle]:storage_->particles) {
        particle.groups = 0;
        particle.node->hide();
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
