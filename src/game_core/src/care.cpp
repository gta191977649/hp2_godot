// TASK-0042 E-0042-S001: exact Debug source marker(s): src\care.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): CarPartCuller::CullParts() [0x002AC920]
#include "game_core/CarPartCuller.hpp"
#include "game_core/indep/inc/Car.hpp"
#include "game_core/port/driving/car_effects.hpp"
#include "game_core/port/driving/vehicle_lights.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/inc/bSlotPool.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/port/race/world_construction.hpp"
#include "game_core/port/not_implemented.hpp"

namespace hp2::debug {
// Debug0011DDB8, E-0099-W06. The recovered routine always creates a new pool;
// unlike InitClans/InitSkids it has no lazy-allocation test. Original padding in
// TestSprite and bytes outside these exact stores remain untouched.
void InitCarEffects(::port::driving::Memory& memory){
    memory.word(0x00326cd8,bNewSlotPool(memory,0x10,0x15e,0x002a4c30,0x10));
    for(unsigned i=0;i<26;++i){
        const auto row=0x002d7bb0+i*8;
        const auto name=memory.word(row);
        if(!name)hp2::not_implemented("0x0024C8B0","CarEffectParameters.Name is not bound");
        std::string text;
        for(unsigned offset=0;;++offset){
            if(offset==4096)hp2::not_implemented("0x0024C8B0","unterminated CarEffectParameters.Name");
            const auto c=memory.byte(name+offset);if(!c)break;text.push_back(static_cast<char>(c));
        }
        memory.word(row+4,hp2::frontend::bStringHash(text.c_str()));
    }
    // Original eIdentity0011B558 produces the same identity carrier as the
    // existing canonical recovered bIdentity. No engine matrix enters here.
    memory.matrix(0x002d7b70,hp2::recovered::bIdentity());
    const auto sprite=0x002d7c90;
    memory.word(sprite,0);memory.word(sprite+0x28,0x80808080);
    memory.scalar(sprite+0x20,1);memory.word(sprite+4,0);memory.word(sprite+8,0);
    memory.scalar(sprite+0xc,1);memory.word(sprite+0x10,0);memory.word(sprite+0x14,0);
    memory.scalar(sprite+0x18,1);memory.scalar(sprite+0x1c,1);memory.word(sprite+0x24,0);
}
}

namespace port::driving {
// Debug00120350; E-0082-L01, reverse/task0082/lights.md. NFSHP2.ELF SHA-256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
VehicleLightState vehicle_light_state(const Memory& m,std::uint32_t car,
    int time_of_day,bool frontend,std::uint32_t car_timebase_start){
    VehicleLightState result;
    const bool night=std::uint32_t(std::int16_t(time_of_day))-2u<2u;
    if(car){
        if(m.word(car+0x830)!=0)result.headlight_left=result.headlight_right=true;
        else if(m.word(car+0x808)!=0){
            const auto phase=(car_timebase_start+m.word(0x00327d04))&15u;
            result.headlight_left=m.byte(0x002d7ae8+phase)!=0;
            result.headlight_right=!result.headlight_left;
        }
        const auto driver=m.word(car+0x28);
        if(driver)result.brakelights=m.scalar(0x00324b68)<m.scalar(driver+0x218);
    }
    if(night||m.word(0x00326c80)!=0)result.headlight_left=result.headlight_right=true;
    if(night||m.word(0x00326c84)!=0||frontend)result.brakelights=true;
    return result;
}
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): CarRenderInfo::Render() [0x002ACDF0]
// Diagnostic text (not a callable signature): CarRenderInfo::RenderDashboard() [0x002ACE48]
#include "game_core/CarRenderInfo.hpp"

#include "game_core/port/not_implemented.hpp"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace hp2::recovered {
// Debug0011E4E0; E-0075-C01, reverse/task0075/terrain-producer.md.
// Host service binding of wheel smoke/spew and TASK0089 DriveOver slices.
// E0089W05, reverse/task0089/water-research.md; unsupported marker lists throw.
void Car::TriggerEffect(int effect,float intensity,int parameter,const std::array<float,4>* override_velocity,port::driving::CarEffectServices& services){
 if(!services.render_info_available||intensity==0||effect==0||services.override_view_bits!=0)return;
 if(effect<1||effect>4)hp2::not_implemented("0x0011E4E0","Car::TriggerEffect unsupported effect binding");
 const int position=parameter>>16;
 if(effect>=3&&(position<5||position>8))hp2::not_implemented("0x0011E4E0","Car::TriggerEffect non-wheel emitter position");
 if(!services.emitter_list||!services.wheel_position||!services.emit)hp2::not_implemented("0x0011E4E0","Car::TriggerEffect missing emitter services");
 auto local_world=m_.matrix(address_+0x90);
 const auto body_position=m_.vector(address_+(effect<=2?0x50:0x60));
 for(unsigned i=0;i<3;++i)local_world[12+i]=body_position[i];
 local_world[15]=1;
 auto velocity=override_velocity?*override_velocity:m_.vector(address_+0x70);velocity[3]=0;
 auto& emitters=services.emitter_list(effect,static_cast<unsigned>(parameter)&0xffffu);
 for(auto& emitter:emitters){
  auto orientation=emitter.local_world;
  std::array<float,3> point{};
  if(effect>=3)point=services.wheel_position(static_cast<unsigned>(position-5));
  else if(emitter.car_position>=1&&emitter.car_position<=4){
   // InitRenderInfo0011DEA8 exact pair table, E0089W05.
   constexpr unsigned pairs[4][2]{{0,1},{3,2},{0,3},{1,2}};
   const auto a=services.wheel_position(pairs[emitter.car_position-1][0]);
   const auto b=services.wheel_position(pairs[emitter.car_position-1][1]);
   for(unsigned i=0;i<3;++i)point[i]=(a[i]+b[i])*0.5f;
  }else if(emitter.car_position>=5&&emitter.car_position<=8)
   point=services.wheel_position(emitter.car_position-5);
  else if(emitter.car_position!=0)hp2::not_implemented("0x0011E4E0","DriveOver CarPosition marker list not recovered");
  for(unsigned i=0;i<3;++i)orientation[12+i]=point[i];
  orientation[15]=1;
  std::array<float,16> emitter_world{};hp2::recovered::eMulMatrix(emitter_world,orientation,local_world);
  emitter.local_world=orientation;
  services.emit(emitter,-intensity,emitter_world,velocity);
 }
}
}
#include "game_core/port/assets/elab_chunks.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include <limits>
#include <stdexcept>

namespace port {
namespace {
// Port-side matrix value carrier for original0011B248 multiplication order.
std::array<float,16> attachment_product(const std::array<float,16>& left,const std::array<float,16>& right) {
    std::array<float,16> out{};
    for(std::size_t row=0;row<4;++row)for(std::size_t col=0;col<4;++col)
        out[row*4+col]=left[row*4]*right[col]+left[row*4+1]*right[4+col]+
            left[row*4+2]*right[8+col]+left[row*4+3]*right[12+col];
    return out;
}
float float_bits(std::uint32_t bits) {
    float result;
    std::memcpy(&result,&bits,4);
    return result;
}
}
std::array<float,16> Sub_00116828(const std::array<float,19>& p) {
    for(const auto value:p)if(!std::isfinite(value))throw std::invalid_argument("Non-finite original material parameter");
    const auto signed_bits=[](float value) {
        std::int32_t bits;std::memcpy(&bits,&value,4);return static_cast<float>(bits);
    };
    float first=p[0]*p[2], first_affine=0;
    const float first_product=p[1]*p[3];
    if(first_product==0)first=0;
    else first_affine=signed_bits(first_product)-first*float_bits(0x4e7e0000);
    float second=p[12]*p[14],second_affine=0;
    const float second_product=p[13]*p[15];
    if(second_product==0)second=0;
    else second_affine=signed_bits(second_product)-second*float_bits(0x4e7e0000);
    const float half=(p[11]-p[7])*0.5f;
    return {{first,first_affine,float_bits(0x3dcccccd),p[18]*float_bits(0x3b808081),
        p[4],p[5],p[6],(p[7]+half)*128.0f,p[8],p[9],p[10],half*-128.0f,
        second,second_affine,p[16],p[17]-p[16]}};
}
CarMaterial read_car_material(const std::vector<std::uint8_t>& bytes,std::uint32_t hash) {
    const auto chunks=hp2::assets::parse_chunks_top(bytes);
    const auto* table=hp2::assets::find_first(chunks,0x3401e);
    if(!table)throw std::runtime_error("Original material registry chunk0x3401e missing");
    const auto base=(table->header_offset+0x17u)&~std::size_t(15);
    const auto end=table->payload_offset()+table->size;
    const auto u32=[&](std::size_t at) {
        if(at>end || end-at<4)throw std::runtime_error("Original material registry truncated");
        return std::uint32_t(bytes.at(at)) | std::uint32_t(bytes.at(at+1))<<8 |
            std::uint32_t(bytes.at(at+2))<<16 | std::uint32_t(bytes.at(at+3))<<24;
    };
    if(u32(base+8)!=1)throw std::runtime_error("Unsupported original material registry version");
    const auto count=u32(base+12);
    if(base+16>end || count>(end-base-16)/0x140)throw std::runtime_error("Original material registry count exceeds chunk");
    std::size_t selected=end, fallback=end;
    for(std::size_t i=0;i<count;++i) {
        const auto row=base+16+i*0x140;
        if(u32(row+8)==0x2c420d64)fallback=row;
        if(u32(row+8)==hash)selected=row;
    }
    if(selected==end)selected=fallback;
    if(selected==end)throw std::runtime_error("Original material and Default fallback absent");
    CarMaterial result;result.hash=u32(selected+8);
    for(std::size_t i=0;i<20 && bytes.at(selected+12+i);++i)result.name+=static_cast<char>(bytes[selected+12+i]);
    for(std::size_t i=0;i<40;++i) {
        const auto bits=u32(selected+0x20+i*4);std::memcpy(&result.parameters[i],&bits,4);
        if(!std::isfinite(result.parameters[i]))throw std::runtime_error("Non-finite original material registry parameter");
    }
    for(std::size_t block=0;block<2;++block) {
        std::array<float,19> input{};
        std::copy_n(result.parameters.begin()+block*20,19,input.begin());
        const auto output=Sub_00116828(input);
        std::copy(output.begin(),output.end(),result.blocks.begin()+block*16);
    }
    return result;
}
std::array<float,28> Sub_00115978(const std::array<float,16>& matrix) {
    for(const auto value:matrix)if(!std::isfinite(value))throw std::invalid_argument("Non-finite original part matrix");
    // 00117098 initialized vectors at002D5B70/80/90, including exact GP float bits.
    const std::array<std::array<float,3>,3> directions{{{{0,-5,5}},
        {{float_bits(0x4089999a),2.5f,2.5f}},{{float_bits(0xc089999a),2.5f,2.5f}}}};
    std::array<float,28> output{{128,128,128,0, 64,64,64,0, 64,64,64,0, 0,0,0,128}};
    for(std::size_t light=0;light<3;++light) {
        std::array<float,3> direction{};
        for(std::size_t axis=0;axis<3;++axis)direction[axis]=directions[light][0]*matrix[axis*4]+
            directions[light][1]*matrix[axis*4+1]+directions[light][2]*matrix[axis*4+2];
        const float squared=direction[0]*direction[0]+direction[1]*direction[1]+direction[2]*direction[2];
        if(!(squared>0))throw std::invalid_argument("Degenerate original lighting direction");
        const float reciprocal=1.0f/std::sqrt(squared);
        for(std::size_t axis=0;axis<3;++axis)output[16+axis*4+light]=direction[axis]*reciprocal;
    }
    return output;
}
CarUvFactors FixStripSTs(std::uint16_t width, std::uint16_t height,
                         std::uint16_t Field_0x4, std::uint16_t Field_0x6) {
    if(!width || !height || !Field_0x4 || !Field_0x6)
        throw std::invalid_argument("Invalid original texture UV scale");
    CarUvFactors factors;
    const std::array<std::uint16_t,2> dimensions{{width,height}},stored{{Field_0x4,Field_0x6}};
    for(std::size_t axis=0;axis<2;++axis) {
        std::uint32_t padded=1;
        while(padded<dimensions[axis])padded*=2;
        const float desired=static_cast<float>(dimensions[axis])/static_cast<float>(padded);
        const float previous=static_cast<float>(stored[axis])*0x1p-15f;
        factors.original[axis]=static_cast<std::uint32_t>(desired*32768.0f)==stored[axis] ?
            1.0f : (1.0f/previous)*desired;
        // The native decoder crops to width/height. Map GS allocation UVs into
        // that cropped image after the original factor; this is host adaptation.
        factors.cropped_image[axis]=factors.original[axis]/desired;
    }
    return factors;
}
float Sub_0011B6E0(float angle) {
    // E-0033-B05, exact single-precision constants at 0032DA2C..48.
    if (!std::isfinite(angle)) throw std::invalid_argument("Non-finite original trig input");
    const float turn=float_bits(0x40c90fdb), half=float_bits(0x40490fdb), quarter=float_bits(0x3fc90fdb);
    while (turn < angle) angle=angle-turn;
    while (angle < 0.0f) angle=angle+turn;
    float sign=1.0f;
    if (half <= angle) {angle=angle-half;sign=-1.0f;}
    if (quarter <= angle) angle=half-angle;
    const float squared=angle*angle;
    const float fifth=angle*squared*squared;
    const float seventh=fifth*squared;
    return (angle + angle*squared*float_bits(0xbe2aaaa4) + fifth*float_bits(0x3c08873e) +
        seventh*float_bits(0xb94fb21f) + seventh*squared*float_bits(0x362e9c14))*sign;
}
std::array<float,16> Sub_0011B7D8(std::uint16_t angle) {
    const float radians=static_cast<float>(angle)*float_bits(0x38c90fdb);
    const float cosine=Sub_0011B6E0(radians+float_bits(0x3fc90fdc));
    const float sine=Sub_0011B6E0(radians);
    return {{cosine,sine,0,0, -sine,cosine,0,0, 0,0,1,0, 0,0,0,1}};
}
CarRenderConfig read_car_render_config(const std::vector<std::uint8_t>& bytes,
                                      const std::string& code, std::uint8_t skin) {
    const auto chunks = hp2::assets::parse_chunks_top(bytes);
    const auto* table = hp2::assets::find_first(chunks, 0x34600);
    if (!table) throw std::runtime_error("Original car config chunk0x34600 missing");
    const std::size_t base = (table->header_offset + 0x17u) & ~std::size_t(15);
    if (base + 64u*0x560u > table->payload_offset()+table->size)
        throw std::runtime_error("Original car config table truncated");
    const auto text = [&](std::size_t offset) {
        std::size_t length = 0;
        while (length < 32 && bytes.at(offset+length) != 0) ++length;
        if (length == 32) throw std::runtime_error("Original car config string not terminated");
        return std::string(reinterpret_cast<const char*>(bytes.data()+offset),length);
    };
    const auto u32 = [&](std::size_t offset) {
        return std::uint32_t(bytes.at(offset)) | (std::uint32_t(bytes.at(offset+1))<<8) |
            (std::uint32_t(bytes.at(offset+2))<<16) | (std::uint32_t(bytes.at(offset+3))<<24);
    };
    for (std::size_t i = 0; i < 64; ++i) {
        const auto row = base + i*0x560u;
        if (text(row) != code) continue;
        CarRenderConfig result;
        result.code = code;
        result.car_index = static_cast<std::uint32_t>(i);
        const auto* skins=hp2::assets::find_first(chunks,0x34601);
        if(!skins || skins->size<64 || skins->size%64)
            throw std::runtime_error("Original car skin catalog missing or malformed");
        const auto skin_base=skins->payload_offset(); // 00187F98: deliberately unaligned.
        result.material_hash=u32(skin_base+0x2c); // 00188180 permitted-miss returns first row.
        for(std::size_t index=0;index<skins->size/64;++index) {
            const auto row_skin=skin_base+index*64;
            if(u32(row_skin)==i && u32(row_skin+4)==skin) {
                result.material_hash=u32(row_skin+0x2c);break;
            }
        }
        result.prefix = text(row+0x20);
        result.Field_0x538 = static_cast<std::int32_t>(u32(row+0x538));
        for (std::size_t j = 0; j < 4; ++j) for (std::size_t k = 0; k < 4; ++k) {
            const auto bits = u32(row+0x120+j*0x20+k*4);
            std::memcpy(&result.Field_0x120_0x180[j][k], &bits, 4);
            if (!std::isfinite(result.Field_0x120_0x180[j][k]))
                throw std::runtime_error("Original car config vector is not finite");
        }
        for(std::size_t j=0;j<4;++j){const auto bits=u32(row+0x138+j*0x20);std::memcpy(&result.Field_0x138_0x198[j],&bits,4);if(!std::isfinite(result.Field_0x138_0x198[j]))throw std::runtime_error("Non-finite original wheel config offset");}
        return result;
    }
    throw std::runtime_error("Requested car code absent from original car config");
}
int GetPixelSize(float radius, float projection_scale,
                 const std::array<float, 3>& position,
                 const std::array<float, 3>& camera_position,
                 const std::array<float, 3>& camera_forward) {
    // E-0033-B03. Port validation avoids undefined native float-to-int conversions.
    if (!std::isfinite(radius) || radius <= 0 || !std::isfinite(projection_scale) || projection_scale < 0)
        throw std::invalid_argument("Invalid projection inputs for 0x00107600");
    std::array<float, 3> delta{};
    for (std::size_t i = 0; i < 3; ++i) {
        if (!std::isfinite(position[i]) || !std::isfinite(camera_position[i]) || !std::isfinite(camera_forward[i]))
            throw std::invalid_argument("Non-finite projection vector for 0x00107600");
        delta[i] = position[i] - camera_position[i];
    }
    const float dot = delta[0]*camera_forward[0] + delta[1]*camera_forward[1] + delta[2]*camera_forward[2];
    if (dot < -radius) return 0;
    const float distance = std::sqrt(delta[0]*delta[0] + delta[1]*delta[1] + delta[2]*delta[2]) - radius;
    const float result = radius*0.5f < distance ? radius*projection_scale/distance : projection_scale+projection_scale;
    if (!std::isfinite(result) || static_cast<double>(result) > std::numeric_limits<int>::max())
        throw std::invalid_argument("Projection result outside native int range");
    return static_cast<int>(result);
}
}
namespace hp2::recovered {
std::array<float,28> CarRenderInfo::Sub_00120EF8(const std::array<float,16>& source_part_transform) {
    // Original CarRenderInfo render entry delegates context construction to 00115978;
    // all coefficient math remains shared with its independent native callers.
    return port::Sub_00115978(source_part_transform);
}
port::CarRenderBindings CarRenderInfo::Sub_0011E860(const std::string& prefix,
                                                  std::int32_t Field_0x538, std::uint8_t skin) {
    port::CarRenderBindings result;
    for (std::uint32_t i = 0; i < 4; ++i) {
        const auto letter = static_cast<char>('A' + (Field_0x538 == 2 && i > 0 ? i-1 : i));
        result.body[i] = {0x40u + i*24u, prefix + "_" + letter, {}, {}};
        const auto hash=[](const std::string& value){return hp2::frontend::bStringHash(value.c_str());};
        const auto skin_name=prefix+"_SKIN"+std::to_string(skin);
        const bool use_b=i>1 && Field_0x538!=2;
        auto& replacements=result.body[i].texture_replacements;
        // E-0033-B07: flattened result of the pre-binding SKIN1->SKIN1B
        // rewrite at 0010C7F0 followed by the constructor's texture patch table.
        replacements.push_back({hash(prefix+"_SKIN1"),hash(skin_name+(use_b?"B":""))});
        replacements.push_back({hash(prefix+"_SKIN1B"),hash(skin_name+"B")});
        if(i<3) {
            // E-0033-B10: 0011FF68 null-car path selects WINDOW table entry0
            // at 002E0380 for all six original address-anchored material keys.
            for(const auto key:{0x7b220ddfu,0x1b0763a0u,0xe7e4ef49u,0x4cdebfcau,0x60f8b13cu,0x0ab88f5du})
                replacements.push_back({key,0x834b8017u});
            // E-0033-B09, 00120130 null-car path with no environment override.
            // Static viewer explicitly selects the original lights-off material state.
            for(const auto* side:{"LEFT","RIGHT"}) {
                replacements.push_back({hash(prefix+"_HEADLIGHT_"+side),hash(prefix+"_HEADLIGHT")});
                replacements.push_back({hash(prefix+"_BRAKELIGHT_"+side),hash(prefix+"_BRAKELIGHT")});
            }
        }
    }
    for (std::uint32_t i = 0; i < 3; ++i) {
        const auto rear_letter = static_cast<char>('A' + (Field_0x538 == 2 && i == 2 ? 1 : i));
        const auto front_letter = static_cast<char>('A' + (Field_0x538 == 2 && i == 2 ? 1 : i));
        const auto front_name = prefix + "_TIRE_FRONT_" + front_letter;
        result.tire_front[i] = {0xa0u + i*48u, front_name, {}, {}};
        result.tire_rear[i] = {0xb8u + i*48u, prefix + "_TIRE_REAR_" + rear_letter, front_name, {}};
    }
    // E-0034-W01: 0011E860 states +0x130/+0x148, shared solid then F/R fallback.
    result.wheel_blur={{{0x130,prefix+"_WHEEL_BLUR",prefix+"_WHEEL_BLUR_F",{}},
                        {0x148,prefix+"_WHEEL_BLUR",prefix+"_WHEEL_BLUR_R",{}}}};
    result.accessories = {
        {0x160, prefix + "_BRAKE_FRONT", {}, {}}, {0x178, prefix + "_BRAKE_REAR", prefix + "_BRAKE_FRONT", {}},
        {0x190, prefix + "_WIPER_LEFT", {}, {}}, {0x1a8, prefix + "_WIPER_RIGHT", {}, {}},
        {0x1c0, prefix + "_SIDE_MIRROR_LEFT", {}, {}}, {0x1d8, prefix + "_SIDE_MIRROR_RIGHT", {}, {}},
        {0x1f0, prefix + "_LICENSE_PLATE_FRONT", {}, {}}, {0x208, prefix + "_LICENSE_PLATE_REAR", {}, {}}
    };
    // E-0035-M01: 0011E860 binds the two-entry table at object+0x528
    // to both mirror objects (+0x1c0/+0x1d8). 0011FE60 fills its skin pairs.
    for(auto& binding:result.accessories)if(binding.object_offset==0x1c0||binding.object_offset==0x1d8)
        binding.texture_replacements.assign(result.body[0].texture_replacements.begin(),result.body[0].texture_replacements.begin()+2);
    return result;
}
port::CarLodSelection CarRenderInfo::Sub_00120EF8(int projected_size,
                                                const std::array<bool, 4>& presence) {
    port::CarLodSelection result;
    if (projected_size < 4) return result;
    result.visible = true;
    // Raw float table at ELF 0x002AD0E8: 42f00000,42340000,41a00000,00000000.
    constexpr std::array<float, 4> thresholds{{120.0f,45.0f,20.0f,0.0f}};
    const float size = static_cast<float>(projected_size);
    if (size < thresholds[0] && presence[1]) {
        do {
            ++result.body;
            if (thresholds[static_cast<std::size_t>(result.body)] <= size) break;
        } while (result.body < 3 && presence[static_cast<std::size_t>(result.body)+1]);
    }
    result.tire = result.body < 2 ? result.body : 2;
    result.mirrors = result.body < 2;
    result.license_plates = result.body < 3;
    return result;
}
port::CarAttachmentMatrices CarRenderInfo::Sub_00120EF8(const port::CarRenderConfig& config,
                                                       std::uint16_t steering) {
    auto static_config=config;static_config.Field_0x138_0x198.fill(0);
    return port::CalculateWheelMatrices(static_config,{{0,0,0,0}},steering);
}
} // namespace hp2::recovered
namespace port {
std::array<float,16> Sub_0011B950(std::uint16_t angle){
    const auto z=Sub_0011B7D8(angle);
    return {{z[0],0,-z[1],0, 0,1,0,0, z[1],0,z[0],0, 0,0,0,1}};
}
std::array<float,16> eCreateLookAtMatrix(const std::array<float,3>& eye,const std::array<float,3>& target,const std::array<float,3>& up){
    const auto normalize=[](std::array<float,3> a){float square=a[0]*a[0];square+=a[1]*a[1];square+=a[2]*a[2];if(!(square>0)||!std::isfinite(square))throw std::invalid_argument("Degenerate original environment view");const float inverse=1.f/std::sqrt(square);for(auto& v:a)v*=inverse;return a;};
    const auto cross=[](const std::array<float,3>& a,const std::array<float,3>& b){return std::array<float,3>{{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]}};};
    auto forward=normalize({{target[0]-eye[0],target[1]-eye[1],target[2]-eye[2]}});
    auto right=cross({{-up[0],-up[1],-up[2]}},forward);
    auto vertical=cross(forward,right);right=normalize(right);vertical=normalize(vertical);
    std::array<float,16> matrix{};matrix[15]=1;
    for(int i=0;i<3;++i){matrix[i*4]=right[i];matrix[i*4+1]=vertical[i];matrix[i*4+2]=forward[i];}
    for(int i=0;i<3;++i)matrix[12+i]=-eye[0]*matrix[i]-eye[1]*matrix[4+i]-eye[2]*matrix[8+i];
    return matrix;
}
float Sub_00120EA8(float velocity){
    if(!std::isfinite(velocity))throw std::invalid_argument("Non-finite original wheel angular velocity");
    return std::min(1.f,std::max(0.f,std::fabs(velocity)*float_bits(0x3ca3d70a)));
}
CarAttachmentMatrices CalculateWheelMatrices(const CarRenderConfig& config,const std::array<float,4>& phases,
                                  std::uint16_t steering) {
    // Port containers of exact source matrices; not a reconstruction of frontend animation.
    const std::array<float,16> identity{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
    port::CarAttachmentMatrices result;
    for(std::size_t i=0;i<4;++i) {
        const float scaled=phases[i]*float_bits(0x4622f983);
        if(!std::isfinite(scaled)||double(scaled)<=std::numeric_limits<int>::min()||double(scaled)>=std::numeric_limits<int>::max())throw std::invalid_argument("Original wheel phase outside conversion range");
        auto tire=Sub_0011B950(static_cast<std::uint16_t>(static_cast<int>(scaled)));
        auto brake=identity;
        if(i<2) {const auto turn=port::Sub_0011B7D8(steering);tire=attachment_product(tire,turn);brake=attachment_product(brake,turn);}
        for(std::size_t lane=0;lane<3;++lane) {
            const auto value=config.Field_0x120_0x180[i][lane];
            if(!std::isfinite(value))throw std::invalid_argument("Non-finite original attachment translation");
            tire[12+lane]=value;brake[12+lane]=value;
        }
        tire[13]+=config.Field_0x138_0x198[i];brake[13]+=config.Field_0x138_0x198[i];
        result.tire[i]=tire;
        // Original second matrix group has steering/translation but no wheel spin.
        result.blur[i]=brake;
        result.brake[i]=brake;
    }
    return hp2::recovered::CarRenderInfo::Sub_00120EF8(result);
}
} // namespace port
namespace hp2::recovered {
// ELF00122E88..00122FE8 and00123294..001233D4; E-0038-W01.
port::CarAttachmentMatrices CarRenderInfo::Sub_00120EF8(const port::CarAttachmentMatrices& source) {
    auto result=source;
    const auto reverse=port::Sub_0011B7D8(0x8000);
    const std::array<float,16> mirror{{1,0,0,0,0,-1,0,0,0,0,1,0,0,0,0,1}};
    for(const std::size_t i:{std::size_t(0),std::size_t(3)}) {
        result.tire[i]=port::attachment_product(reverse,source.tire[i]);
        result.blur[i]=port::attachment_product(reverse,source.blur[i]);
        result.brake[i]=port::attachment_product(mirror,source.brake[i]);
    }
    return result;
}
void CarRenderInfo::Sub_0011E860() {
    hp2::not_implemented("0x0011E860", "CarRenderInfo::Sub_0011E860 full constructor: allocation, materials, markers and state");
}
void CarRenderInfo::Sub_00120EF8() {
    hp2::not_implemented("0x00120EF8", "CarRenderInfo::Sub_00120EF8 full render: PS2 submission, live-car state and effects");
}
}

// TASK0052 E-0052-E06: original src/care.cpp free-function shadow island.
#include "game_core/port/track/shadow.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/indep/src/track.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace hp2::recovered {
int cmpl(const port::track::EnvironmentVector* a,const port::track::EnvironmentVector* b){float difference=(*a)[0]-(*b)[0];if(difference>0)return 1;if(difference<0)return -1;difference=(*b)[1]-(*a)[1];return difference>0?1:difference<0?-1:0;}
int cmph(const port::track::EnvironmentVector* a,const port::track::EnvironmentVector* b){return cmpl(b,a);}
int make_chain(std::vector<const port::track::EnvironmentVector*>& points,std::size_t offset,int count,bool high){
 if(count<1||offset+std::size_t(count)>points.size())throw std::invalid_argument("Invalid original hull chain span");
 std::sort(points.begin()+offset,points.begin()+offset+count,[&](const auto* a,const auto* b){return (high?cmph(a,b):cmpl(a,b))<0;});
 int current=1,last=1;for(int next=2;next<count;++next){if(current<1)++current;else{int previous=current;do{const int index=previous;--previous;const auto& v=*points[offset+index];const auto& before=*points[offset+previous];const auto& n=*points[offset+next];current=index+1;if((n[0]-v[0])*(before[1]-v[1])-(n[1]-v[1])*(before[0]-v[0])>0)break;current=index;}while(previous>0);}std::swap(points[offset+current],points[offset+next]);last=current;}return last;
}
std::vector<port::track::EnvironmentVector> ch2d(const std::vector<port::track::EnvironmentVector>& input){
 if(input.size()<3)return input;if(input.size()>64)throw std::runtime_error("Original shadow hull capacity exceeded");
 std::vector<const port::track::EnvironmentVector*> points;for(const auto& v:input)points.push_back(&v);points.push_back(nullptr);
 const int first=make_chain(points,0,int(input.size()),false);points[input.size()]=points[0];const int second=make_chain(points,first,int(input.size())-first+1,true);
 std::vector<port::track::EnvironmentVector> out;for(int i=0;i<first+second;++i)out.push_back(*points[i]);return out;
}
std::vector<port::track::EnvironmentVector> convex_hull(const std::vector<port::track::EnvironmentVector>& input,const port::track::Topology& topology,float z,float bias,const port::track::EnvironmentVector& origin,const port::track::EnvironmentVector& normal){
 auto hull=ch2d(input);std::vector<port::track::EnvironmentVector> out;
 for(auto point:hull){point[2]=z;const auto contact=TopologyCoordinate::GetElevation(topology,point);point[2]=contact.height+bias;if(!contact.found||std::abs(point[2]-z)>4.f)continue;const float distance=(point[0]-origin[0])*normal[0]+(point[1]-origin[1])*normal[1]+(point[2]-origin[2])*normal[2];if(std::abs(distance)>=1.5f)continue;out.push_back(point);}return out;
}
std::vector<port::track::EnvironmentVector> smooth_shadow_corners(const std::vector<port::track::EnvironmentVector>& input){
 std::vector<port::track::EnvironmentVector> out;if(input.empty())return out;if(input.size()>64)throw std::runtime_error("Original shadow hull capacity exceeded");
 constexpr float scale=0.3330000042915344f;port::track::EnvironmentVector previous{};
 for(unsigned k=0;k<3;++k)previous[k]=(input[0][k]-input.back()[k])*scale;
 for(std::size_t i=0;i<input.size();++i){port::track::EnvironmentVector current{},a{},b{},c{};for(unsigned k=0;k<3;++k){current[k]=(input[(i+1)%input.size()][k]-input[i][k])*scale;a[k]=input[i][k]-previous[k];b[k]=input[i][k]+(current[k]-previous[k])*scale;c[k]=input[i][k]+current[k];}out.push_back(a);out.push_back(b);out.push_back(c);previous=current;}return out;
}
port::track::ShadowLight sh_Setup(port::driving::Memory& math,const port::track::SunResources& sun,const port::track::EnvironmentVector& car){
 const port::track::EnvironmentVector light=sun.available?sun.shadow_position:port::track::EnvironmentVector{{-16384,0,7000}};port::track::ShadowLight out;
 for(unsigned k=0;k<3;++k)out.direction[k]=car[k]-light[k];auto& v=out.direction;
 const auto angle=std::uint16_t(hp2::recovered::bATan(math,std::sqrt(v[0]*v[0]+v[1]*v[1]),-v[2]));
 if(angle<0x11f8){const float length=std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]),denominator=std::abs(v[0])+std::abs(v[1]);if(denominator==0)throw std::runtime_error("Singular original shadow light direction");const float sine=hp2::recovered::bSin(math,std::uint16_t(0x11f8)),cosine=hp2::recovered::bSin(math,std::uint16_t(0x51f8));v[2]=-length*sine;v[1]=(v[1]/denominator)*length*cosine;v[0]=(v[0]/denominator)*length*cosine;}
 if(v[2]!=0)out.inverse_z=1.f/v[2];return out;
}
}

#include "game_core/port/track/assets.hpp"
namespace hp2::recovered {
// Retail00120EF8 / Debug001210F0, live-car static-track shadow branches.
// Host input supplies original geometry-space CV, pose, body0 bounds, wheels and
// selectedLOD. It deliberately does not recover damage/part-culler decisions.
port::track::ShadowPacket CarRenderInfo::Render(port::driving::Memory& math,const port::track::Scene& scene,const port::track::ShadowInput& input){
 using namespace port::track;ShadowPacket out;if(!input.topology)throw std::invalid_argument("Shadow requires original topology");
 const auto& topology=*input.topology;const auto transform=[&](const EnvironmentVector& p){EnvironmentVector v{};for(unsigned k=0;k<3;++k)v[k]=input.local_world[k]*p[0]+input.local_world[4+k]*p[1]+input.local_world[8+k]*p[2]+input.local_world[12+k];return v;};
 auto origin=input.car_position;const auto ground=TopologyCoordinate::GetElevation(topology,origin);if(!ground.found)return out;origin[2]=ground.height;EnvironmentVector normal{{ground.plane[0],ground.plane[1],ground.plane[2]}};
 eLightQuery lights;QueryLightDatabase(lights,scene.light_packs,input.car_position);const float sunlight=std::clamp(lights.LightScale,0.f,1.f);
 if(input.fancy&&input.car_lod<3&&!input.collision_vertices.empty()){
 const auto light=sh_Setup(math,scene.environment.sun,input.car_position);std::vector<EnvironmentVector> projected;
 for(auto p:input.collision_vertices){p[0]*=.85f;p[1]*=.85f;p[2]+=.2750000059604645f;p=transform(p);const float shift=(input.car_position[2]-p[2])*light.inverse_z;p[0]+=shift*light.direction[0];p[1]+=shift*light.direction[1];projected.push_back(p);}
 // Retail uses the recovered ground height+bias as query reference Z.
 const float bias=.05999999865889549f;auto hull=smooth_shadow_corners(convex_hull(projected,topology,origin[2]+bias,bias,origin,normal));
 if(hull.size()>2){EnvironmentVector center{};for(unsigned k=0;k<3;++k)center[k]=(hull[0][k]+hull[hull.size()/2][k])*.5f;
 float alpha=90,edge=1.1f;if(sunlight<.4f){alpha=0;edge=1.4f;}else if(sunlight<=1.f){const float scale=(sunlight-.4f)/.6f;alpha=scale*90;edge=scale*(-.2999999523162842f)+1.4f;}
 if(input.car_lod==2){if(input.lod_swap1==input.lod_swap2)throw std::invalid_argument("Singular original shadow LOD interval");alpha*=std::clamp((input.projected_size-input.lod_swap2)/(input.lod_swap1-input.lod_swap2),0.f,1.f);}
 const std::array<std::uint8_t,4> colour{{128,128,128,static_cast<std::uint8_t>(std::min(int(alpha),255))}};
 auto vertex=[](const EnvironmentVector& p,float u){return ShadowVertex{p,{{u,0}}};};
 const int n=int(hull.size()),limit=(n&~1)-1;for(int i=0;i<limit;i+=2){ShadowStrip strip;strip.colour=colour;strip.vertices={vertex(hull[i],.155f),vertex(center,0),vertex(hull[i+1],.155f),vertex(hull[(i+2)%n],.155f)};out.strips.push_back(std::move(strip));}
 if(n&1){ShadowStrip strip;strip.colour=colour;strip.vertices={vertex(hull.back(),.155f),vertex(center,0),vertex(hull.front(),.155f)};out.strips.push_back(std::move(strip));}
 const int third=n/3;int begin=0;for(int group=0;group<3;++group){const int count=group==2?n-begin:third;ShadowStrip strip;strip.colour=colour;for(int j=0;j<=count;++j){const auto& p=hull[(begin+j)%n];auto expanded=p;expanded[0]=edge*(p[0]-center[0])+center[0];expanded[1]=edge*(p[1]-center[1])+center[1];strip.vertices.push_back(vertex(p,.155f));strip.vertices.push_back(vertex(expanded,1));}out.strips.push_back(std::move(strip));begin+=third;}
 }
 }
 if(input.fallback){
 constexpr float step=.3333333432674408f;std::array<EnvironmentVector,16> points{};std::array<std::array<float,2>,16> uv{};std::array<unsigned,4> bad{};std::array<Contact,4> wheels{};std::array<EnvironmentVector,4> wheel_pos{};
 for(unsigned i=0;i<4;++i){wheel_pos[i]=transform(input.wheels[i]);wheels[i]=TopologyCoordinate::GetElevation(topology,wheel_pos[i]);wheel_pos[i][2]=wheels[i].height;}
 constexpr unsigned wheel_index[16]={2,2,1,1,2,2,1,1,3,3,0,0,3,3,0,0};const float xmin=input.bounds_min[0]*.95f,xmax=input.bounds_max[0]*.95f,ymin=input.bounds_min[1]*.95f,ymax=input.bounds_max[1]*.95f;float previous=transform({{0,0,0}})[2];
 for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col){const unsigned i=row*4+col;auto p=transform({{xmin+float(col)*(xmax-xmin)*step,ymin+float(row)*(ymax-ymin)*step,0}});p[2]=input.car_position[2];auto contact=TopologyCoordinate::GetElevation(topology,p);p[2]=contact.height;
 if(!contact.found){const unsigned wheel=wheel_index[i];if(wheels[wheel].found){const auto& plane=wheels[wheel].plane;if(plane[2]==0)throw std::runtime_error("Singular original shadow wheel plane");p[2]=wheel_pos[wheel][2]-(p[0]-wheel_pos[wheel][0])*plane[0]/plane[2]-(p[1]-wheel_pos[wheel][1])*plane[1]/plane[2];contact.found=true;}}
 if(((i^(i>>1))&5)==5)p[2]+=.08f;float distance=(p[0]-origin[0])*normal[0]+(p[1]-origin[1])*normal[1]+(p[2]-origin[2])*normal[2];
 if(!contact.found||std::abs(p[2]-origin[2])>4.f||std::abs(distance)>=1.5f){p[2]=previous+.04f;++bad[row];}else {previous=p[2];p[2]+=.04f;}
 points[i]=p;uv[i]={{float(col)*step,float(row)*step}};}
 float alpha=70;if(sunlight>=.4f)alpha=sunlight<=1.f?70+(140-70)*((sunlight-.4f)/.6f):140;
 for(unsigned row=0;row<3;++row){if(bad[row]+bad[row+1]>=4)continue;ShadowStrip strip;strip.ramp=false;strip.colour={{128,128,128,static_cast<std::uint8_t>(std::min(unsigned(alpha),255u))}};for(unsigned col=0;col<4;++col)for(unsigned r=0;r<2;++r){const auto i=(row+r)*4+col;strip.vertices.push_back({points[i],uv[i]});}out.strips.push_back(std::move(strip));}
 }
 return out;
}
}

namespace hp2::debug {
// Debug0011DE78, E-0099-W18. Original deliberately retains the stale global.
void CloseCarEffects(::port::driving::Memory& m){bDeleteSlotPool(m,m.word(0x326cd8));}
}

#include "game_core/port/race/car_render_resources.hpp"
#include "game_core/port/race/damage_resources.hpp"
#include "game_core/port/track/texture_bindings.hpp"
#include "game_core/port/track/emitter_bindings.hpp"
#include <limits>
namespace hp2::recovered {
// Debug0011E7B0; E-0110-CE01. Original src/care.cpp; raw RideInfo skin
// byte and existing render resource names/pointers, without host GPU operations.
void Car::SwitchSkin(std::uint32_t skin){
 const auto ride=m_.word(address_+0x10);m_.byte(ride+5,static_cast<std::uint8_t>(skin));
 port::race::CarRenderInfo(m_,m_.word(address_+0x4b0)).SwitchSkin(ride);
 port::race::CarRenderInfo(m_,m_.word(address_+0x4b0)).UpdateSkinTextures();
}
}
namespace port::race {
// Debug00120080; E-0099-W25. Actual raw RideInfo/texture-name producer.
void CarRenderInfo::SwitchSkin(std::uint32_t ride){
 m_.word(address_+4,ride);GetUsedCarTextureInfo(m_,address_+0x2b0,ride,0);
 const auto used=address_+0x2b0,first=m_.word(used+0x44),second=m_.word(used+0x48),replace=m_.word(used+0x70),replace_b=m_.word(used+0x74);
 for(auto table:{address_+0x528,address_+0x560}){m_.word(table,first);m_.word(table+4,replace);m_.word(table+28,second);m_.word(table+32,replace_b);}
 m_.word(address_+0x6cc,first);m_.word(address_+0x6d0,replace);
}
// Debug001200F0; E-0110-CE01. Exact replacement row NewNameHash+4 and
// pTextureInfo+8; lookup retains the original missing-texture fallback.
void CarRenderInfo::UpdateTextureInfo(std::uint32_t entry){
 m_.word(entry+8,hp2::debug::GetTextureInfo(m_,m_.word(entry+4),1,0));
}
// Debug00120128; E-0110-CE01. These five rows only, in original call order.
void CarRenderInfo::UpdateSkinTextures(){
 for(const auto offset:{0x528u,0x544u,0x560u,0x57cu,0x6ccu})UpdateTextureInfo(address_+offset);
}
// Debug00120188; E-0099-W25. Original breakable item bit extraction and state tables.
void CarRenderInfo::UpdateCarReplacementTextures(){
 const auto s=address_;auto bits=[&](){const auto car=m_.word(s);if(!car)return 0u;const auto damage=m_.word(car+0x844);return damage?m_.word(damage+0xcc):0u;};
 for(unsigned i=0;i<6;++i)m_.word(s+0x560+28*(i+2)+4,m_.word(0x2d7c80+4*((bits()>>(2*(i+8)))&3)));
 m_.word(s+0x374,0);
 for(unsigned i=0;i<4;++i){const auto table=s+0x560+28*(i+8);const auto amount=(bits()>>(2*(i+4)))&3;
  if(amount<2)m_.word(table+4,m_.word(table));else{m_.word(table+4,m_.word(s+0x2b0+(i<2?0x84:0x90)));m_.word(s+0x374,m_.word(s+0x374)|m_.word(0x2d7aa0+4*i));}
 }
 const auto broken=m_.word(s+0x374);if((broken&m_.word(0x2d7aa8))&&(broken&m_.word(0x2d7aac)))m_.word(s+0x374,broken|m_.word(0x2d7ab0));
 for(unsigned i=0;i<3;++i)eModel(m_,s+0x40+24*i).ReconnectTextureTable();
}
// Debug00120558; E-0099-W25. Exact marker hashes, normal-table index and pool records.
void CarRenderInfo::CreateCarLightFlares(){
 const auto s=address_,info=m_.word(s+8);if(!info)return;const auto base=render_resource_text(m_,info+32);const auto hash=hp2::frontend::bStringHash(base.c_str());
 for(auto marker=hp2::debug::eFindPositionMarker(m_,0,hash);marker;marker=hp2::debug::eFindPositionMarker(m_,0,hash,marker)){
  const auto name=m_.word(marker+8);int type=-1;unsigned flags=0;
  switch(name){case 0x4ae7f96f:case 0xacec665c:type=0;break;case 0x34970013:case 0x5f09c5e2:case 0x7efcf06f:type=1;break;case 0x944e5339:case 0xbea9aee6:type=2;break;case 0x1e4150b4:type=4;flags=1;break;case 0xe662c161:type=5;flags=1;break;case 0xb4348dba:type=6;break;case 0xa532fc46:type=7;break;case 0xa7e6ea53:type=8;break;default:break;}
  if(type<0)continue;const auto pool=m_.word(0x326710);if(!pool)hp2::not_implemented("0x00109E18","Original eLightFlareSlotPool component startup is required");
  const auto p=hp2::debug::bOMalloc(m_,pool);m_.byte(p+13,static_cast<std::uint8_t>(type));m_.byte(p+12,1);m_.byte(p+16,0);m_.word(p+8,name);m_.half(p+14,static_cast<std::uint16_t>(flags));
  const auto tail=m_.word(s+0x36c);m_.word(tail,p);m_.word(s+0x36c,p);m_.word(p+4,tail);m_.word(p,s+0x368);
  for(unsigned axis=0;axis<3;++axis){m_.scalar(p+48+4*axis,m_.scalar(marker+16+4*axis)-m_.scalar(s+48+4*axis));m_.word(p+64+4*axis,m_.word(0x2d7af8+12*m_.byte(marker+29)+4*axis));}
  m_.word(p+60,0);m_.word(p+76,0);
 }
}
// Debug0011D7A8; E-0099-W25. Shared plane normals mutate only if original tolerance fails.
static void InitPart(Memory&m,std::uint32_t culler,unsigned part,const std::array<float,4>&position){
 const auto info=0x2d7640+80*part;for(int i=0;i<m.signed_word(info+12);++i){const auto p=info+16+16*static_cast<unsigned>(i);auto normal=m.vector(p);const float length=std::sqrt((normal[0]*normal[0]+normal[1]*normal[1])+normal[2]*normal[2]);if(m.scalar(0x324b60)<std::fabs(1.f-length)){const float inverse=1.f/length;for(auto&v:normal)v=v*inverse;m.vector(p,normal);}}
 m.vector(culler+32*part,position);
}
}

namespace port::race {
// Debug0011EA88; E-0099-W25. Full required constructor sequence in original care.cpp.
void CarRenderInfo::Sub_0011EA88(std::uint32_t ride,std::uint32_t car){
 const auto s=address_;auto& m=m_;
 for(unsigned i=0;i<26;++i){m.word(s+0x40+24*i+8,0);m.word(s+0x40+24*i+12,0);}
 m.word(s+0x368,s+0x368);m.word(s+0x36c,s+0x368);
 for(unsigned i=0;i<26;++i){const auto list=s+0x378+8*i;m.word(list,list);m.word(list+4,list);}
 for(unsigned i=0;i<28;++i){const auto list=s+0x448+8*i;m.word(list,list);m.word(list+4,list);}
 for(unsigned i=0;i<0x1c0;++i)m.byte(s+0x760+i,0);
 m.word(s,car);m.word(s+0x920,0);m.word(s+4,ride);m.word(s+0x370,hp2::recovered::bRandom(m,8));
 if(!m.word(0x3278e0))hp2::not_implemented("0x00186148","Original CarInfo resource loader has not published its array");
 const auto info=m.word(0x3278e0)+0x560*m.word(ride),skin=GetSkinInfo(m,m.signed_word(ride),m.signed_byte(ride+5),1);m.word(s+8,info);
 if(!skin)hp2::not_implemented("0x00186330","Original SkinInfo resource table is absent");
 const auto base=render_resource_text(m,info+32);auto hash=[&](unsigned fmt,int a=0,int b=0){return render_format_hash(m,fmt,base,a,b);};
 SwitchSkin(ride);const auto used=s+0x2b0;
 const unsigned window_hashes[]={0x7b220ddf,0x1b0763a0,0xe7e4ef49,0x4cdebfca,0x60f8b13c,0x0ab88f5d};
 m.word(s+0x560,m.word(used+0x44));m.word(s+0x57c,m.word(used+0x48));
 for(unsigned i=0;i<6;++i)m.word(s+0x560+28*(i+2),window_hashes[i]);for(unsigned i=0;i<4;++i)m.word(s+0x560+28*(i+8),m.word(used+0x50+4*i));
 UpdateCarReplacementTextures();
 m.word(s+0x6b0,m.word(used+0x4c));m.word(s+0x6b4,m.word(used+0x78));
 m.word(s+0x6e8,m.word(used+0x64));m.word(s+0x6ec,m.word(used+0xa0));m.word(s+0x704,m.word(used+0x68));m.word(s+0x708,m.word(used+0xa4));
 const auto material_hash=m.word(skin+0x2c);m.word(s+0x364,elGetLightMaterial(m,material_hash));const bool traffic=m.word(info+0x538)==2;
 for(unsigned i=0;i<4;++i){const int level=traffic&&i?int(i-1):int(i);const bool damaged=m.signed_byte(ride+7)&&m.signed_byte(ride+4)&&i<2;const auto model=s+0x40+24*i;
  eModel view(m,model);view.Init(hash(damaged?0x326ce8:0x326cf0,'A'+level,m.signed_byte(ride+4)));
  if(i>1&&!traffic)view.RenameTexture(m.word(used+0x44),m.word(used+0x48));view.AttachReplacementTextureTable(s+(i<3?0x560:0x528),i<3?12:2,i<3?int(i):0);
 }
 const unsigned tire_formats[]={0x2a4c70,0x2a4c80,0x2a4c90,0x2a4ca0,0x2a4cb0,0x2a4cc0};std::array<unsigned,6> tire_hash{};for(unsigned i=0;i<6;++i)tire_hash[i]=hash(tire_formats[i]);if(traffic){tire_hash[2]=tire_hash[1];tire_hash[5]=tire_hash[4];}
 for(unsigned side=0;side<2;++side)for(unsigned lod=0;lod<3;++lod)eModel(m,s+0xa0+48*lod+24*side).Init(tire_hash[side*3+lod]);
 for(unsigned lod=0;lod<3;++lod){const auto rear=s+0xb8+48*lod;if(!m.word(rear+12)){eModel(m,rear).UnInit();eModel(m,rear).Init(tire_hash[lod]);}}
 for(unsigned side=0;side<2;++side)for(unsigned lod=0;lod<3;++lod)eModel(m,s+0xa0+48*lod+24*side).AttachReplacementTextureTable(s+0x6cc,3,int(side*3+lod));
 for(unsigned side=0;side<2;++side)for(unsigned lod=0;lod<3;++lod)eModel(m,s+0xa0+48*lod+24*side).ReplaceLightMaterial(0xd6d6080a,material_hash);
 const auto brake_front=hash(0x2a4cd0);eModel(m,s+0x160).Init(brake_front);eModel(m,s+0x178).Init(hash(0x2a4ce0));if(!m.word(s+0x184)){eModel(m,s+0x178).UnInit();eModel(m,s+0x178).Init(brake_front);}
 for(unsigned i=0;i<2;++i)eModel(m,s+0x160+24*i).AttachReplacementTextureTable(s+0x528,2,int(i+1));for(unsigned i=0;i<2;++i)eModel(m,s+0x160+24*i).ReplaceLightMaterial(0xd6d6080a,material_hash);
 const unsigned pair_offsets[]={0x190,0x1c0,0x1f0};const unsigned pair_formats[][2]={{0x2a4cf0,0x2a4d00},{0x2a4d10,0x2a4d28},{0x2a4d40,0x2a4d58}};
 for(unsigned pair=0;pair<3;++pair){for(unsigned i=0;i<2;++i)eModel(m,s+pair_offsets[pair]+24*i).Init(hash(pair_formats[pair][i]));if(pair<2)for(unsigned i=0;i<2;++i)eModel(m,s+pair_offsets[pair]+24*i).AttachReplacementTextureTable(s+0x528,2,int(3+pair*2+i));for(unsigned i=0;i<2;++i)eModel(m,s+pair_offsets[pair]+24*i).ReplaceLightMaterial(0xd6d6080a,material_hash);}
 eModel(m,s+0x220).Init(hash(m.signed_byte(ride+4)?0x2a4d70:0x2a4d80,m.signed_byte(ride+4)));
 const unsigned dash_offsets[]={0x238,0x268,0x250,0x280,0x298},dash_formats[]={0x326cf8,0x2a4d90,0x2a4da0,0x2a4db0,0x2a4dc0};for(unsigned i=0;i<5;++i)eModel(m,s+dash_offsets[i]).Init(hash(dash_formats[i]));
 eModel(m,s+0x238).AttachReplacementTextureTable(s+0x6b0,1,0);eModel(m,s+0x268).AttachReplacementTextureTable(s+0x6b0,1,1);for(unsigned i=0;i<3;++i)eModel(m,s+dash_offsets[i]).ReplaceLightMaterial(0xd6d6080a,material_hash);
 const auto blur=hash(0x2a4dd0);for(unsigned i=0;i<2;++i)eModel(m,s+0x130+24*i).Init(blur);for(unsigned i=0;i<2;++i)if(!m.word(s+0x13c+24*i)){eModel(m,s+0x130+24*i).UnInit();eModel(m,s+0x130+24*i).Init(hash(i?0x2a4df0:0x2a4de0));}for(unsigned i=0;i<2;++i)eModel(m,s+0x130+24*i).AttachReplacementTextureTable(s+0x6cc,3,int(i+6));
 const bool frontend=m.word(0x3275ac)==3;const auto shadow=hp2::debug::GetTextureInfo(m,hash(frontend?0x2a4e00:0x2a4e10),0,0);m.word(s+0x35c,shadow);m.word(s+0x360,hp2::debug::GetTextureInfo(m,0xbadb4475,0,0));if(shadow)m.byte(shadow+0x29,frontend?5:3);
 if(car){hp2::recovered::Car(m,car).AssignAnimatedDriver(base);if(const auto damage=m.word(car+0x844)){
  hp2::debug::BodyDamage body(m,damage+8);const auto name=base+"_A";body.SetALevelBodyDamageInfo(name.c_str());
  if(!m.signed_byte(ride+7)){const auto scuffs=base+"_SCUFFS";body.AddModel(s+0x220,scuffs.c_str(),1);}else{for(unsigned i=0;i<3;++i){const auto level=base+"_"+char('A'+i);body.AddModel(s+0x40+24*i,level.c_str(),0);}const auto scuffs=base+"_SCUFFS";body.AddModel(s+0x220,scuffs.c_str(),0);}
 }}
 const auto body=m.word(s+0x4c);for(unsigned axis=0;axis<3;++axis)m.scalar(s+0x30+4*axis,body?m.scalar(body+0x90+4*axis):0.f);
 CreateCarLightFlares();if(!body)hp2::not_implemented("0x0010CDA0","Missing original body solid leaves constructor bounding-box temporary undefined");
 // Original named AABB components are XYZ. Uninitialized temporary W padding
 // has no recovered value; the explicit host carrier preserves its padding.
 std::array<float,3> lo{},hi{};for(unsigned axis=0;axis<3;++axis){lo[axis]=m.scalar(body+0x40+4*axis);hi[axis]=m.scalar(body+0x50+4*axis);}
 for(unsigned model:{s+0x40,s+0x1c0,s+0x1d8}){if(const auto solid=m.word(model+12))for(unsigned axis=0;axis<3;++axis){lo[axis]=m.scalar(solid+0x40+4*axis);hi[axis]=m.scalar(solid+0x50+4*axis);}for(unsigned axis=0;axis<3;++axis){if(model==s+0x40){m.scalar(s+0x10+4*axis,lo[axis]);m.scalar(s+0x20+4*axis,hi[axis]);}else{m.scalar(s+0x10+4*axis,std::min(m.scalar(s+0x10+4*axis),lo[axis]));m.scalar(s+0x20+4*axis,std::max(m.scalar(s+0x20+4*axis),hi[axis]));}}}
 for(unsigned axis=0;axis<3;++axis){m.scalar(s+0x10+4*axis,m.scalar(s+0x10+4*axis)-m.scalar(0x324b64));m.scalar(s+0x20+4*axis,m.scalar(s+0x20+4*axis)+m.scalar(0x324b64));}
 std::array<std::array<float,4>,4> wheels{};for(unsigned i=0;i<4;++i)wheels[i]=m.vector(info+0x120+32*i);const auto culler=s+0x760;for(unsigned i=0;i<4;++i)InitPart(m,culler,i,wheels[i]);
 std::array<float,4> left{},right{},under{};for(unsigned axis=0;axis<4;++axis){left[axis]=(wheels[0][axis]+wheels[3][axis])*0.5f;right[axis]=(wheels[1][axis]+wheels[2][axis])*0.5f;under[axis]=(left[axis]+right[axis])*0.5f;}
 InitPart(m,culler,4,left);InitPart(m,culler,5,right);for(unsigned i=0;i<4;++i)InitPart(m,culler,6+i,wheels[i]);InitPart(m,culler,10,wheels[0]);InitPart(m,culler,11,wheels[2]);InitPart(m,culler,12,under);
 std::array<float,4> front{},side{};for(unsigned axis=0;axis<4;++axis){front[axis]=wheels[0][axis]-wheels[1][axis];side[axis]=wheels[1][axis]-wheels[2][axis];}
 std::array<float,4> normal{{front[1]*side[2]-front[2]*side[1],front[2]*side[0]-front[0]*side[2],front[0]*side[1]-front[1]*side[0],front[3]}};const float inverse=1.f/std::sqrt((normal[0]*normal[0]+normal[1]*normal[1])+normal[2]*normal[2]);for(unsigned axis=0;axis<4;++axis)under[axis]=under[axis]+(normal[axis]*inverse)*wheels[0][3];InitPart(m,culler,13,under);
}
}

namespace port::race {
// Debug0011FCE8; E-0099-W25. Fresh alpha=null and original idle queue branch.
// EI/unused COP0 Count read are CPU-platform boundaries; active rendering cannot
// be replaced by an idle success and therefore throws before freeing resources.
void CarRenderInfo::Sub_0011FCE8(){
 auto&m=m_;const auto s=address_;
 if(m.word(0x3266d0)||m.word(0x3266d4))hp2::not_implemented("0x001121E0","Active original render/buffer-swap synchronization is not bound");
 if(const auto car=m.word(s))hp2::recovered::Car(m,car).UnAssignAnimatedDriver();
 if(const auto alpha=m.word(s+0x920))m.release(alpha);
 for(int i=27;i>=0;--i){const auto list=s+0x448+8*static_cast<unsigned>(i);while(m.word(list)!=list){const auto p=m.word(list);m.word(list,m.word(p));if(m.word(list+4)==p)m.word(list+4,list);hp2::debug::SlotPool(m,m.word(0x326cd8)).Free(p);}}
 for(int i=25;i>=0;--i)hp2::debug::afxDeleteEmitters(m,s+0x378+8*static_cast<unsigned>(i));
 const auto list=s+0x368;while(m.word(list)!=list){const auto p=m.word(list),next=m.word(p),prev=m.word(p+4);m.word(prev,next);m.word(next+4,prev);hp2::debug::SlotPool(m,m.word(0x326710)).Free(p);}
 for(int i=25;i>=0;--i)eModel(m,s+0x40+24*static_cast<unsigned>(i)).UnInit();
}
}
namespace hp2::recovered {
// Debug0011DEA8; E-0099-W25. Actual owned object, pool/list order, marker
// mapping and original26 emitter groups; no host eModel pointer substitutions.
void Car::InitRenderInfo(std::uint32_t ride){
 auto&m=m_;const auto car=address_;const auto render=m.allocate(0x930);
 ::port::race::CarRenderInfo(m,render).Sub_0011EA88(ride,car);const auto info=m.word(car+12);m.word(car+0x4b0,render);
 const auto base=::port::race::render_resource_text(m,info+32);const auto type=hp2::frontend::bStringHash(base.c_str());
 auto append=[&](unsigned group,const std::array<float,3>&position){const auto pool=m.word(0x326cd8);if(!pool)hp2::not_implemented("0x0011DDB8","Original CarEmitterPositionSlotPool startup is required");const auto p=hp2::debug::bOMalloc(m,pool),list=render+0x448+8*group,tail=m.word(list+4);m.word(list+4,p);m.word(tail,p);m.word(p,list);for(unsigned axis=0;axis<3;++axis)m.scalar(p+4+4*axis,position[axis]);};
 append(0,{{0,0,0}});
 const unsigned pairs[][2]={{0,1},{3,2},{0,3},{1,2}};
 for(unsigned i=0;i<4;++i){std::array<float,3> position{};for(unsigned axis=0;axis<3;++axis)position[axis]=(m.scalar(info+0x120+32*pairs[i][0]+4*axis)+m.scalar(info+0x120+32*pairs[i][1]+4*axis))*0.5f;append(i+1,position);}
 for(unsigned i=0;i<4;++i)append(i+5,{{m.scalar(info+0x120+32*i),m.scalar(info+0x124+32*i),m.scalar(info+0x128+32*i)}});
 for(auto marker=hp2::debug::eFindPositionMarker(m,0,type);marker;marker=hp2::debug::eFindPositionMarker(m,0,type,marker)){
  int first=-1,second=-1;switch(m.word(marker+8)){
  case 0x725758bf:first=26;break;case 0xa7e6ea53:first=18;second=20;break;case 0xa2799c2b:case 0xa5b28001:case 0xf21be91e:first=10;break;case 0x7b220ddf:first=12;break;case 0xa532fc46:first=18;second=21;break;case 0xd947f346:first=19;second=22;break;case 0xc52bf01b:first=25;break;case 0xe7e4ef49:first=14;break;case 0x494ed280:first=27;break;case 0x59a55db5:first=9;break;case 0x4cdebfca:first=15;break;case 0x60f8b13c:first=16;break;case 0x68100815:first=11;break;case 0x05f63788:first=24;break;case 0x0ab88f5d:first=17;break;case 0x1b0763a0:first=13;break;case 0x02b52399:first=19;second=23;break;default:break;
  }
  std::array<float,3> position{};for(unsigned axis=0;axis<3;++axis)position[axis]=m.scalar(marker+16+4*axis)-m.scalar(render+48+4*axis);if(first>=0)append(static_cast<unsigned>(first),position);if(second>=0)append(static_cast<unsigned>(second),position);
 }
 for(unsigned i=0;i<26;++i)hp2::debug::afxGetEmitters(m,render+0x378+8*i,m.word(0x2d7bb4+8*i));
 m.word(0x326cdc,hp2::debug::GetTextureInfo(m,0x2340,1,0));
}
// Debug0011E4A8; E-0099-W25. Original deleting destructor and final null store.
void Car::CloseRenderInfo(){if(const auto render=m_.word(address_+0x4b0)){::port::race::CarRenderInfo(m_,render).Sub_0011FCE8();m_.release(render);}m_.word(address_+0x4b0,0);}
}

namespace port::race {
// Debug00123B18; E-0099-W25. Actual primitive2-byte alpha/flags save array.
void CarRenderInfo::OverRideAlpha(std::uint8_t alpha,bool semi,bool write_z){
 const auto s=address_;auto&m=m_;if(m.word(s+0x920))return;const auto count=m.word(s+0x2f0),saved=m.allocate(count*2);m.word(s+0x920,saved);
 for(unsigned i=0;i<count;++i)if(const auto texture=hp2::debug::GetTextureInfo(m,m.word(s+0x2b0+4*i),1,0)){
  m.byte(saved+2*i,m.byte(texture+0x76));m.half(saved+2*i,static_cast<std::uint16_t>((m.half(saved+2*i)&0xfeff)|((m.byte(texture+0x4f)&1)<<8)));
  m.half(saved+2*i,static_cast<std::uint16_t>((m.half(saved+2*i)&0xfdff)|((m.byte(texture+0x50)&1)<<9)));
  m.byte(texture+0x50,write_z?1:0);m.byte(texture+0x76,alpha);m.byte(texture+0x4f,semi?1:0);
 }
}
// Debug00123C40; E-0099-W25. Restores original record flags then frees/save-pointer null.
void CarRenderInfo::RestoreAlpha(){const auto s=address_;auto&m=m_;const auto saved=m.word(s+0x920);if(!saved)return;
 for(unsigned i=0;i<m.word(s+0x2f0);++i)if(const auto texture=hp2::debug::GetTextureInfo(m,m.word(s+0x2b0+4*i),1,0)){
  m.byte(texture+0x76,m.byte(saved+2*i));m.byte(texture+0x4f,m.byte(saved+2*i+1)&1);m.byte(texture+0x50,(m.byte(saved+2*i+1)>>1)&1);
 }
 m.release(saved);m.word(s+0x920,0);
}
}

#include "game_core/indep/inc/AcidFX.hpp"
namespace hp2::recovered {
// Debug0011E4E0, E-0099-W38; exact src/care.cpp owner. Original linked emitter
// and position records remain in shared Memory; active emission uses raw AcidFX.
void Car::TriggerEffect(int effect,float intensity,int parameter,std::uint32_t override_velocity){
 TriggerEffect(effect,intensity,parameter,override_velocity,std::nullopt);
}
void Car::TriggerEffect(int effect,float intensity,int parameter,std::uint32_t override_velocity,std::optional<std::uint32_t> port_stack_vector_lane){
 const auto render=m_.word(address_+0x4b0);if(!render||intensity==0||effect==0||m_.word(address_+0x510))return;
 if(effect<1||effect>=26)hp2::not_implemented("0x0011E4E0","Original CarEffect index outside recovered table");
 auto list=render+0x378+static_cast<unsigned>(effect)*8;const auto terrain=m_.word(0x328b58);
 if(static_cast<unsigned>(effect-1)<5&&terrain)list=terrain+(static_cast<unsigned>(parameter)&0xffffu)*8+static_cast<unsigned>(effect-1)*0x108;
 int position=0;if(static_cast<unsigned>(effect-3)<2||effect==8||effect==9)position=parameter>>16;
 auto local_world=m_.matrix(address_+0x90);const auto origin=address_+(position>0&&position<9?0x60:0x50);
 for(unsigned axis=0;axis<3;++axis)local_world[12+axis]=m_.scalar(origin+axis*4);local_world[15]=1;
 const auto velocity=override_velocity?override_velocity:address_+0x70;
 for(auto emitter=m_.word(list);emitter!=list;emitter=m_.word(emitter)){
  auto orientation=m_.matrix(emitter+0x30);const auto data=m_.word(emitter+0x1c);
  if(!data)hp2::not_implemented("0x0011E4E0","Original Car emitter requires loaded EmitterData before CarPosition read");
  const int id=position?position:m_.signed_word(data+0x58);if(id<0||id>=28)hp2::not_implemented("0x0011E4E0","Original CarEffectPosition index outside recovered table");
  const auto head=render+0x448+static_cast<unsigned>(id)*8;
  for(auto node=m_.word(head);node!=head;node=m_.word(node)){
   for(unsigned axis=0;axis<3;++axis)orientation[12+axis]=m_.scalar(node+4+axis*4);orientation[15]=1;
   std::array<float,16> world;eMulMatrix(world,orientation,local_world);m_.matrix(emitter+0x30,world);
   for(unsigned axis=0;axis<3;++axis)m_.word(emitter+0x70+axis*4,m_.word(velocity+axis*4));m_.word(emitter+0x7c,0);
   hp2::debug::AcidEffectSystem(m_,0x2e6f40).TriggerEmitter(emitter,-intensity,address_,port_stack_vector_lane);
  }
  // Source restores the orientation carrying the LAST position visited, not
  // the emitter's entry translation. Empty position lists preserve all16 lanes.
  m_.matrix(emitter+0x30,orientation);
 }
}
}
