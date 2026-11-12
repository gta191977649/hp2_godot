#include "game_core/indep/src/rigidbody.hpp"
#include "game_core/port/driving/ee_scalar.hpp"

// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x0019CC58; E-0042-C095; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace hp2::recovered {
void RigidBody::CalculateInertiaTensor(port::driving::init::ByteView out,port::driving::init::ByteView config){using namespace port::driving;using namespace port::driving::init;
    out.require(0,0x160);config.require(0,0x50);
    const float f1=config.scalar(0);
    std::array<float,16> matrix;
    for(std::size_t i=0;i<16;++i)matrix[i]=config.scalar(0x10+4*i);
    const auto inverse=bInvertMatrix(matrix);
    out.scalar(0x118,f1);out.scalar(0x11c,1.0f/f1);
    for(std::size_t i=0;i<16;++i)out.scalar(0x120+4*i,inverse[i]);
}
}
namespace port::driving::init {
void CalculateInertiaTensor(ByteView out,ByteView config){hp2::recovered::RigidBody::CalculateInertiaTensor(out,config);}

}

// Retail 0x0019C9B0; E-0042-C093; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace hp2::recovered {
void RigidBody::port_initialize(port::driving::init::ByteView out,std::uint32_t config_address,port::driving::init::ByteView config){using namespace port::driving;using namespace port::driving::init;
    out.require(0,0x17c);config.require(0,0x50);
    out.word(0x10,0);out.word(0x14,0);out.word(0x18,0);
    out.word(8,0x002ad1f0);out.word(0xc,config_address);
    const auto matrix=bIdentity();
    for(std::size_t i=0;i<16;++i)out.scalar(0x20+4*i,matrix[i]);
    for(auto offset:{0x60,0x64,0x68,0x70,0x74,0x78,0x80,0x90,0x94,0x98,
                    0xa0,0xa4,0xa8,0xf0,0xf4,0xf8,0x100,0x104,0x108,0x170,0x174,0x114})out.word(offset,0);
    out.scalar(0x178,1);out.word(0x110,1);
    CalculateInertiaTensor(out,config);
}
}
namespace port::driving::init {
void Sub_0019C9B0(ByteView out,std::uint32_t config_address,ByteView config){hp2::recovered::RigidBody::port_initialize(out,config_address,config);}

}

// Retail 0x0019CC40; E-0042-C094; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace hp2::recovered {
void RigidBody::Calibrate(port::driving::init::ByteView out,const std::array<std::uint32_t,4>* a1){using namespace port::driving;using namespace port::driving::init;
    if(a1){out.require(0x160,16);for(std::size_t i=0;i<4;++i)out.word(0x160+4*i,(*a1)[i]);}
}
}
namespace port::driving::init {
void Calibrate(ByteView out,const std::array<std::uint32_t,4>* a1){hp2::recovered::RigidBody::Calibrate(out,a1);}

}

// Retail 0x0019CF10; E-0041-M01; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace hp2::recovered {
void RigidBody::ApplyImpulse(const port::driving::SolverVector& input,const port::driving::SolverVector& point){using namespace port::driving;using namespace port::driving::native;auto& m=m_;const auto body=address_;
    using namespace detail_group_00137100_runtime;
    // ELF0019CF10, E-0038-I28. Cross uses the capped input (unlike0019CD90).
    auto value=input;const auto magnitude=length(value);
    if(1000.0f<magnitude)for(auto& lane:value)lane=lane*(1000.0f/magnitude);
    const auto inspect=[&](float result,float left,float right,const char* pc,const char* stage,unsigned lane){if(!std::isfinite(result)||!std::isfinite(left)||!std::isfinite(right)){const auto bits=[](float x){std::uint32_t word;std::memcpy(&word,&x,4);return std::to_string(word);};const std::string why=std::string("RigidBody basic impulse producer stage=")+stage+" body="+std::to_string(body)+" lane="+std::to_string(lane)+" left_bits="+bits(left)+" right_bits="+bits(right)+" result_bits="+bits(result);hp2::not_implemented(pc,why.c_str());}};
    auto linear=m.vector(body+0x60);for(unsigned i=0;i<4;++i){const float old=linear[i];linear[i]=::port::driving::ee_vadd_finite(old,value[i],"0x0019A8A0");inspect(linear[i],old,value[i],"0x0019A8A0","linear-add",i);}m.vector(body+0x60,linear);
    auto angular=cross(point,value);const auto size=length(angular);
    if(10.0f<size)for(auto& lane:angular)lane=lane*(10.0f/size);
    const auto old=m.vector(body+0x70);for(unsigned i=0;i<4;++i){const float delta=angular[i];angular[i]=::port::driving::ee_vadd_finite(old[i],delta,"0x0019A910");inspect(angular[i],old[i],delta,"0x0019A910","angular-add",i);}m.vector(body+0x70,angular);
}
}
namespace port::driving::native {
void ApplyImpulse(Memory& m,std::uint32_t body,const std::array<float,4>& input,const std::array<float,4>& point){hp2::recovered::RigidBody(m,body).ApplyImpulse(input,point);}

}

// Retail 0x0019D368; E-0042-U132; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace port::driving {
void bMulMatrixVector(SolverVector& out, const SolverMatrix& matrix,
                 const SolverVector& in, const SolverMathContext& context) {
    using namespace detail_group_00138f38;
    bMulMatrix(out, matrix, in, context);
}
}

// Retail 0x0019CD90; E-0041-M01; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace hp2::recovered {
void RigidBody::ApplyForce(port::driving::SolverBodyProjection& body,const port::driving::SolverVector& a1,const port::driving::SolverVector& a2){using namespace port::driving;using namespace port::driving::native;
    using namespace detail_group_00138f38;
    // First A1 load precedes the +0xf0 stores; cross inputs load afterwards.
    const SolverVector original = a1;
    SolverVector limited = original;
    const float length = xyz_length(original);
    if (1200.0f < length) {
        const float factor = 1200.0f / length;
        for (float& lane : limited) lane = lane * factor;
    }
    for (unsigned i = 0; i < 4; ++i) body.Field_0xf0[i] = ee_vadd_finite(body.Field_0xf0[i],limited[i],"0x0019A720");
    // Original A1, rather than capped force, feeds the cross product.
    const SolverVector position = a2;
    SolverVector value = cross_xyz(position, a1, position[3]);
    const float cross_length = xyz_length(value);
    if (100.0f < cross_length) {
        const float factor = 100.0f / cross_length;
        for (float& lane : value) lane = lane * factor;
    }
    for (unsigned i = 0; i < 4; ++i) body.Field_0x100[i] = ee_vadd_finite(body.Field_0x100[i],value[i],"0x0019A794");
}
}
namespace port::driving {
void ApplyForce(SolverBodyProjection& body, const SolverVector& a1, const SolverVector& a2){hp2::recovered::RigidBody::ApplyForce(body,a1,a2);}

}

// Retail 0x0019CE80; E-0042-C097; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace hp2::recovered {
void RigidBody::ApplyForceInFrame(port::driving::SolverBodyProjection& body,const port::driving::SolverVector& a1,const port::driving::SolverVector& a2,const port::driving::SolverMathContext& context){using namespace port::driving;using namespace port::driving::init;
    using namespace detail_group_00138f38;
    SolverVector first{}, second{};
    bMulMatrixVector(first, body.Field_0x20, a1, context);
    bMulMatrixVector(second, body.Field_0x20, a2, context);
    ApplyForce(body, first, second);
}
}
namespace port::driving {
void ApplyForceInFrame(SolverBodyProjection& body, const SolverVector& a1,
                 const SolverVector& a2, const SolverMathContext& context){hp2::recovered::RigidBody::ApplyForceInFrame(body,a1,a2,context);}

}

// Retail 0x0019CEF8; E-0042-C098; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace hp2::recovered {
void RigidBody::ApplyMoment(port::driving::SolverBodyProjection& body,const port::driving::SolverVector& a1){using namespace port::driving;using namespace port::driving::native;
    using namespace detail_group_00138f38;
    for (unsigned i = 0; i < 4; ++i) body.Field_0x100[i] = ee_vadd_finite(body.Field_0x100[i],a1[i],"0x0019A828");
}
}
namespace port::driving {
void ApplyMoment(SolverBodyProjection& body, const SolverVector& a1){hp2::recovered::RigidBody::ApplyMoment(body,a1);}

}

// Retail 0x0019CD90; E-0041-M01; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace hp2::recovered {
void RigidBody::ApplyForce(const port::driving::SolverVector& force,const port::driving::SolverVector& point){using namespace port::driving;using namespace port::driving::native;auto& m=m_;const auto body=address_;
    using namespace detail_group_0019d4d0;
    SolverBodyProjection p;p.Field_0xf0=m.vector(body+0xf0);p.Field_0x100=m.vector(body+0x100);
    const auto before_force=p.Field_0xf0,before_moment=p.Field_0x100;port::driving::ApplyForce(p,force,point);const auto inspect=[&](const char* stage,const SolverVector& result,const SolverVector& before,const char* pc){for(unsigned lane=0;lane<4;++lane)if(!std::isfinite(result[lane])||!std::isfinite(before[lane])||!std::isfinite(force[lane])||!std::isfinite(point[lane])){const auto bits=[](float x){std::uint32_t word;std::memcpy(&word,&x,4);return std::to_string(word);};std::string why=std::string("Rigid force accumulator producer stage=")+stage+" body="+std::to_string(body)+" lane="+std::to_string(lane)+" old_bits="+bits(before[lane])+" result_bits="+bits(result[lane]);for(unsigned i=0;i<4;++i)why+=" force"+std::to_string(i)+"="+bits(force[i])+" point"+std::to_string(i)+"="+bits(point[i]);hp2::not_implemented(pc,why.c_str());}};inspect("force",p.Field_0xf0,before_force,"0x0019A720");inspect("moment",p.Field_0x100,before_moment,"0x0019A794");m.vector(body+0xf0,p.Field_0xf0);m.vector(body+0x100,p.Field_0x100);
}
}
namespace port::driving::native {
void ApplyForce(Memory& m,std::uint32_t body,const Vector& force,const Vector& point){hp2::recovered::RigidBody(m,body).ApplyForce(force,point);}

}

// Retail 0x0019CE80; E-0042-C097; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace hp2::recovered {
void RigidBody::ApplyForceInFrame(const port::driving::SolverVector& force,const port::driving::SolverVector& point){using namespace port::driving;using namespace port::driving::native;auto& m=m_;const auto body=address_;
    using namespace detail_group_0019d4d0;
    const auto a=m.matrix(body+0x20);hp2::recovered::RigidBody(m,body).ApplyForce(transform(m,a,force),transform(m,a,point));
}
}
namespace port::driving::native {
void ApplyForceInFrame(Memory& m,std::uint32_t body,const Vector& force,const Vector& point){hp2::recovered::RigidBody(m,body).ApplyForceInFrame(force,point);}

}

// Debug0x0019AC98; E-0110-RM01 audited scalar MUL/ADD/SUB stages.
// Retail 0x0019D370; E-0041-M01; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace hp2::recovered {
port::driving::SolverMatrix VectorStarMatrix(const port::driving::SolverVector& a,const port::driving::SolverMatrix& b){using namespace port::driving;using namespace port::driving::native;
    using namespace detail_group_0019d4d0;
    Matrix r{};
    const char* pcs[3][3][3]={{{"0x0019ACB8","0x0019ACB4","0x0019ACBC"},{"0x0019AD20","0x0019AD24","0x0019AD28"},{"0x0019AD88","0x0019AD8C","0x0019AD90"}},{{"0x0019ACD8","0x0019ACDC","0x0019ACE0"},{"0x0019AD40","0x0019AD44","0x0019AD48"},{"0x0019ADAC","0x0019ADB0","0x0019ADB4"}},{{"0x0019AD04","0x0019AD00","0x0019AD08"},{"0x0019AD60","0x0019AD68","0x0019AD6C"},{"0x0019ADD8","0x0019ADD0","0x0019ADE8"}}};
    for(unsigned c=0;c<3;++c) {
        const float x0=ee_mul_s_finite(-a[2],b[c*4+1],pcs[c][0][0]),x1=ee_mul_s_finite(a[1],b[c*4+2],pcs[c][0][1]);r[c*4]=ee_add_s_finite(x0,x1,pcs[c][0][2]);
        const float y0=ee_mul_s_finite(a[2],b[c*4],pcs[c][1][0]),y1=ee_mul_s_finite(a[0],b[c*4+2],pcs[c][1][1]);r[c*4+1]=ee_sub_s_finite(y0,y1,pcs[c][1][2]);
        const float z0=ee_mul_s_finite(-a[1],b[c*4],pcs[c][2][0]),z1=ee_mul_s_finite(a[0],b[c*4+1],pcs[c][2][1]);r[c*4+2]=ee_add_s_finite(z0,z1,pcs[c][2][2]);
    }r[15]=1;return r;
}
}
namespace port::driving::native {
Matrix VectorStarMatrix(const Vector& a,const Matrix& b){return hp2::recovered::VectorStarMatrix(a,b);}

}

// Retail 0x0019D4D0; E-0041-R03; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace hp2::recovered {
void RigidBody::GetDerivative(std::uint32_t output){using namespace port::driving;using namespace port::driving::native;auto& m=m_;const auto body=address_;
    using namespace detail_group_0019d4d0;
    const auto inspect_derivative=[&](const char* stage,const Vector& result,const Vector& left,const Vector& right,const char* pc){for(unsigned lane=0;lane<4;++lane)if(!std::isfinite(result[lane])||!std::isfinite(left[lane])||!std::isfinite(right[lane])){const auto bits=[](float value){std::uint32_t word;std::memcpy(&word,&value,4);return std::to_string(word);};const std::string why=std::string("Rigid derivative producer stage=")+stage+" body="+std::to_string(body)+" lane="+std::to_string(lane)+" left_bits="+bits(left[lane])+" right_bits="+bits(right[lane])+" result_bits="+bits(result[lane]);hp2::not_implemented(pc,why.c_str());}};

    if(m.scalar(body+0x174)!=0) {
        float f3=m.scalar(body+0x174)*m.scalar(body+0x178);f3=f3+g(m,0x8c68);
        if(m.word(body+0x114)!=0)f3=f3*g(m,0xb270);
        float f20=m.scalar(body+0x28),f1;
        if(0<f20) {if(g(m,0xb24c)<f20)f20=g(m,0xb24c);float f0=(1.0f-g(m,0xb248))*f20;f0=f0/g(m,0xb24c);f1=1.0f-f0;}
        else {f20=-f20;if(g(m,0xb244)<f20)f20=g(m,0xb244);float f0=(g(m,0xb240)-1.0f)*f20;f0=f0/g(m,0xb244);f1=f0+1.0f;}
        f3=f3*f1;
        const auto velocity=m.vector(body+0x90);const float x=velocity[0]*velocity[0],y=velocity[1]*velocity[1];
        if(f3<std::sqrt(x+y)*g(m,0x8c6c) && 0<dot(m.vector(body+0xf0),m.vector(body+0x20))) {
            auto local=transform(m,transpose(m.matrix(body+0x20)),m.vector(body+0xf0));
            f20=-f20*g(m,0xb254);if(g(m,0xb250)<f20)f20=g(m,0xb250);local[0]=f20;
            m.vector(body+0xf0,transform(m,m.matrix(body+0x20),local));
        }
    }
    m.vector(output,m.vector(body+0x90));m.matrix(output+0x10,hp2::recovered::VectorStarMatrix(m.vector(body+0xa0),m.matrix(body+0x20)));
    const auto source_force=m.vector(body+0xf0),source_moment=m.vector(body+0x100);inspect_derivative("force-input",source_force,source_force,source_force,"0x0019AFC4");inspect_derivative("moment-input",source_moment,source_moment,source_moment,"0x0019AFCC");m.vector(output+0x50,source_force);m.vector(output+0x60,source_moment);
    const auto finite_length=[](const Vector& value,const char* mul_pc,const char* add_y_pc,const char* add_z_pc,const char* sqrt_pc){const float x=ee_vmula_finite(value[0],value[0],mul_pc).value,y=ee_vmula_finite(value[1],value[1],mul_pc).value,z=ee_vmula_finite(value[2],value[2],mul_pc).value;const float squared=ee_vadd_finite(ee_vadd_finite(x,y,add_y_pc),z,add_z_pc);return ee_sqrt_s(squared,false,sqrt_pc).value;};
    const float angular_length=finite_length(m.vector(body+0xa0),"0x0019AFD4","0x0019AFD8","0x0019AFDC","0x0019AFE8");
    const float f=port::driving::Table::GetValue(m,0x00308518,angular_length);
    const auto angular_momentum=m.vector(body+0x70);auto angular_damping=angular_momentum;for(unsigned lane=0;lane<4;++lane)angular_damping[lane]=ee_mul_s_finite(angular_momentum[lane],-f,"0x0019B010");Vector damping_factors{{-f,-f,-f,-f}};inspect_derivative("angular-damping-mul",angular_damping,angular_momentum,damping_factors,"0x0019B010");auto moment_derivative=m.vector(output+0x60);const auto before_damping=moment_derivative;for(unsigned lane=0;lane<4;++lane)moment_derivative[lane]=ee_vadd_finite(moment_derivative[lane],angular_damping[lane],"0x0019B01C");inspect_derivative("angular-damping-add",moment_derivative,before_damping,angular_damping,"0x0019B01C");m.vector(output+0x60,moment_derivative);
    float f12=finite_length(m.vector(body+0x90),"0x0019B028","0x0019B02C","0x0019B030","0x0019B040")/g(m,0xb258);const float f20=angular_length/g(m,0xb25c);if(f12<f20)f12=f20;
    const float limit=port::driving::Table::GetValue(m,0x00308550,f12);
    m.scalar(output+0x70,m.scalar(body+0x80)<limit ? g(m,0xb268) : -g(m,0xb26c));
    if(m.word(body+0x110)!=0) {
        const auto observe_motion_damping=[&](std::uint32_t destination,std::uint32_t momentum_address,float coefficient,const char* label,const char* factor_pc,const char* mul_pc,const char* add_pc){const float motion=m.scalar(body+0x80),factor=ee_mul_s_finite(coefficient,motion,factor_pc);const Vector coefficients{{coefficient,coefficient,coefficient,coefficient}},motions{{motion,motion,motion,motion}},factors{{factor,factor,factor,factor}};inspect_derivative(label,factors,coefficients,motions,factor_pc);const auto momentum=m.vector(momentum_address);auto product=momentum;for(unsigned lane=0;lane<4;++lane)product[lane]=ee_mul_s_finite(momentum[lane],factor,mul_pc);inspect_derivative(label,product,momentum,factors,mul_pc);auto result=m.vector(destination);const auto before=result;for(unsigned lane=0;lane<4;++lane)result[lane]=ee_vadd_finite(result[lane],product[lane],add_pc);inspect_derivative(label,result,before,product,add_pc);m.vector(destination,result);};
        observe_motion_damping(output+0x60,body+0x70,-g(m,0xb264),"angular-motion-damping","0x0019B0AC","0x0019B0B8","0x0019B0C4");
        observe_motion_damping(output+0x50,body+0x60,-g(m,0xb260),"linear-motion-damping","0x0019B0E0","0x0019B0EC","0x0019B0F8");
    }
    const float f1=m.scalar(body+0x170),f2=m.scalar(body+0x108);
    if(f1!=0) {if(f1<f2)m.scalar(output+0x68,f1);else if(f2< -f1)m.scalar(output+0x68,-f1);}
}
}
namespace port::driving::native {
void GetDerivative(Memory& m,std::uint32_t body,std::uint32_t output){hp2::recovered::RigidBody(m,body).GetDerivative(output);}

}


namespace hp2::recovered {
void RigidBody::port_initialize(std::uint32_t config){RigidBody::port_initialize(m_.view(address_,0x180),config,m_.view(config,0x50));}
void RigidBody::Calibrate(std::uint32_t cog){if(cog)m_.copy(address_+0x160,cog,16);}
void RigidBody::CalculateInertiaTensor(){RigidBody::CalculateInertiaTensor(m_.view(address_,0x180),m_.view(m_.word(address_+0xc),0x50));}
void RigidBody::ComputeAuxiliaries(float unused_stack_w){using namespace port::driving;using namespace port::driving::native;auto& m=m_;const auto body=address_;

    using namespace detail_group_0019d4d0;
    const auto momentum=m.vector(body+0x60);const float inverse_mass=m.scalar(body+0x11c);
    // TASK-0112 diagnostic only: inspect the actual input before the strict
    // numerical carrier throws, so its owning car/producer can be recovered.
    for(unsigned lane=0;lane<4;++lane)if(!std::isfinite(momentum[lane])||!std::isfinite(inverse_mass)){
      const auto bits=[](float value){std::uint32_t word;std::memcpy(&word,&value,4);return std::to_string(word);};
      std::string why="RigidBody velocity input body="+std::to_string(body)+" lane="+std::to_string(lane)+" momentum_bits="+bits(momentum[lane])+" inverse_mass_bits="+bits(inverse_mass);
      const auto world=m.word(0x3272f0);if(world){const int count=m.signed_word(world+0x1c);if(count>=0&&count<=14)for(int i=0;i<count;++i){const auto car=m.word(world+0x2c+4*unsigned(i));if(!car)continue;const int mode=m.signed_word(car+0x24);if(mode!=1&&mode!=4)continue;const auto mover=m.word(car+0x30);if(mover&&m.word(mover+0x18)==body)why+=" owner_car="+std::to_string(car)+" car_index="+std::to_string(m.signed_half(car+8))+" movement_mode="+std::to_string(mode);}}
      for(unsigned k=0;k<4;++k)why+=" momentum"+std::to_string(k)+"_bits="+bits(momentum[k])+" position"+std::to_string(k)+"_bits="+bits(m.scalar(body+0x10+4*k));
      hp2::not_implemented("0x0019B1A8",why.c_str());
    }
    auto velocity=momentum;for(unsigned lane=0;lane<4;++lane)velocity[lane]=::port::driving::ee_vmul_finite(momentum[lane],inverse_mass,"0x0019B1A8");for(unsigned lane=0;lane<4;++lane)if(!std::isfinite(momentum[lane])||!std::isfinite(inverse_mass)||!std::isfinite(velocity[lane])){const auto bits=[](float x){std::uint32_t word;std::memcpy(&word,&x,4);return std::to_string(word);};const std::string why="RigidBody velocity producer body="+std::to_string(body)+" lane="+std::to_string(lane)+" momentum_bits="+bits(momentum[lane])+" inverse_mass_bits="+bits(inverse_mass)+" velocity_bits="+bits(velocity[lane]);hp2::not_implemented("0x0019B1A8",why.c_str());}m.vector(body+0x90,velocity);
    const auto inspect_matrix=[&](const char* stage,const Matrix& value,const Matrix& left,const Matrix& right,const char* pc){for(unsigned element=0;element<16;++element)if(!std::isfinite(value[element])){const auto bits=[](float x){std::uint32_t word;std::memcpy(&word,&x,4);return std::to_string(word);};const unsigned column=element/4,lane=element%4;std::string why=std::string("RigidBody matrix producer stage=")+stage+" body="+std::to_string(body)+" element="+std::to_string(element)+" result_bits="+bits(value[element]);for(unsigned k=0;k<4;++k)why+=" lhs"+std::to_string(k)+"_bits="+bits(left[k*4+lane])+" rhs"+std::to_string(k)+"_bits="+bits(right[column*4+k]);hp2::not_implemented(pc,why.c_str());}};
    const auto b=m.matrix(body+0x20),inertia=m.matrix(body+0x120),transposed=transpose(b);inspect_matrix("rotation-input",b,b,inertia,"0x0019B1C4");inspect_matrix("inertia-input",inertia,b,inertia,"0x0019B1C4");// RM01: only this reached source slice opts into the port VU ACC carrier.
    // Original bMulMatrix002541C0: VMULAx, VMADDAy, VMADDAz, VMADDw.
    const auto matrix_product=[](const Matrix& left,const Matrix& right){Matrix result{};const char* pcs[4][4]={{"0x002541E0","0x002541E4","0x002541E8","0x002541EC"},{"0x002541F0","0x002541F4","0x002541F8","0x002541FC"},{"0x00254200","0x00254204","0x00254208","0x0025420C"},{"0x00254210","0x00254214","0x00254218","0x0025421C"}};for(unsigned column=0;column<4;++column)for(unsigned lane=0;lane<4;++lane){auto acc=ee_vmula_finite(left[lane],right[column*4],pcs[column][0]);for(unsigned k=1;k<4;++k)acc=ee_vmadda_finite(acc,left[k*4+lane],right[column*4+k],pcs[column][k]);result[column*4+lane]=acc.value;}return result;};
    const auto first=matrix_product(b,inertia);inspect_matrix("first-product",first,b,inertia,"0x0019B1C4");const auto c=matrix_product(first,transposed);inspect_matrix("second-product",c,first,transposed,"0x0019B1D4");
    const auto angular_momentum=m.vector(body+0x70);auto input=m.vector(0x317b20);for(unsigned lane=0;lane<3;++lane)input[lane]=ee_vadd_finite(input[lane],angular_momentum[lane],"0x00254288");Vector angular_velocity{};for(unsigned lane=0;lane<4;++lane){auto acc=ee_vmula_finite(c[lane],input[0],"0x0025429C");acc=ee_vmadda_finite(acc,c[4+lane],input[1],"0x002542A0");acc=ee_vmadda_finite(acc,c[8+lane],input[2],"0x002542A4");acc=ee_vmadda_finite(acc,c[12+lane],input[3],"0x002542A8");angular_velocity[lane]=acc.value;}for(unsigned lane=0;lane<4;++lane)if(!std::isfinite(angular_velocity[lane])){const auto bits=[](float x){std::uint32_t word;std::memcpy(&word,&x,4);return std::to_string(word);};std::string why="RigidBody angular transform producer body="+std::to_string(body)+" lane="+std::to_string(lane)+" result_bits="+bits(angular_velocity[lane]);for(unsigned k=0;k<4;++k)why+=" matrix"+std::to_string(k)+"_bits="+bits(c[k*4+lane])+" angular_momentum"+std::to_string(k)+"_bits="+bits(angular_momentum[k]);hp2::not_implemented("0x0019B1E4",why.c_str());}m.vector(body+0xa0,angular_velocity);
    if(m.scalar(body+0x80)<0)m.scalar(body+0x80,0);
    m.matrix(body+0xb0,b);m.vector(body+0xe0,m.vector(body+0x10));m.scalar(body+0xec,1);
    m.vector(body+0xf0,{{0,0,0,unused_stack_w}});m.vector(body+0x100,{{0,0,0,unused_stack_w}});
    const Vector force{{0,0,-g(m,0xb238)*m.scalar(body+0x118),0}},point{{0,0,0,0}};
    hp2::recovered::RigidBody(m,body).ApplyForce(force,point);
}
void RigidBody::ApplyMoment(const port::driving::SolverVector& moment){auto value=m_.vector(address_+0x100);for(unsigned i=0;i<4;++i){const float old=value[i],result=old+moment[i];if(!std::isfinite(old)||!std::isfinite(moment[i])||(!std::isfinite(result)&&!port::driving::pc_sqrt_carrier_enabled())){const auto bits=[](float x){std::uint32_t word;std::memcpy(&word,&x,4);return std::to_string(word);};const std::string why="Rigid ApplyMoment producer body="+std::to_string(address_)+" lane="+std::to_string(i)+" old_bits="+bits(old)+" incoming_bits="+bits(moment[i])+" result_bits="+bits(result);hp2::not_implemented("0x0019A828",why.c_str());}value[i]=port::driving::ee_vadd_finite(old,moment[i],"0x0019A828");}m_.vector(address_+0x100,value);}
void RigidBody::ResolveForces(){}
void RigidBody::SetState(std::uint32_t state){m_.copy(address_+0x10,state,0x80);}
void RigidBody::GetState(std::uint32_t state){m_.copy(state,address_+0x10,0x80);}
std::uint32_t RigidBody::GetState(){return address_+0x10;}
int RigidBody::GetSize(){return 32;}
void RigidBody::PrintCurrentState(){hp2::not_implemented("0x0019B2A8","RigidBody debug printing service");}
}

#include "game_core/src/bMath.hpp"
#include "game_core/indep/inc/Table.hpp"
namespace hp2::recovered {
// Debug 0x0019B358; E-0044-M02; original rigidbody.cpp static initialization.
void Sub_0019B358(port::driving::Memory& m) {
    // Exact XYZ stores. The fourth lanes are untouched by this original entry.
    for(auto a:{0x003084e0u,0x003084e8u,0x003084f0u,0x003084f8u,0x003084e4u,0x003084f4u})m.word(a,0);
    Table(m,0x00308518).Sub_0018D810(0x00308500,6,0,10);
    Table(m,0x00308550).Sub_0018D810(0x00308530,8,0,1);
}
}

namespace hp2::recovered {
// Debug0019a5b0; E-0044-S01. Encoded original vector and matrix pointers.
void RigidBody::SetState(std::uint32_t position,std::uint32_t matrix,std::uint32_t velocity,std::uint32_t angular){using namespace port::driving;using namespace port::driving::native;auto& m=m_;const auto body=address_;
    using namespace detail_group_0019d4d0;
    m.vector(body+0x10,m.vector(position));m.matrix(body+0x20,m.matrix(matrix));
    // Debug0019A614 is VMULx.xyzw, not an unbounded host vector scale.
    // Retain every source lane, including the carried W, and the existing
    // explicitly scoped finite PC VU projection. Nonfinite inputs still stop.
    auto linear_momentum=m.vector(velocity);const float mass=m.scalar(body+0x118);
    for(unsigned lane=0;lane<4;++lane){const float incoming=linear_momentum[lane];if(!std::isfinite(incoming)||!std::isfinite(mass)){const auto bits=[](float value){std::uint32_t word;std::memcpy(&word,&value,4);return std::to_string(word);};const std::string why="RigidBody SetState momentum input body="+std::to_string(body)+" lane="+std::to_string(lane)+" velocity_bits="+bits(incoming)+" mass_bits="+bits(mass);hp2::not_implemented("0x0019A614",why.c_str());}linear_momentum[lane]=ee_vmul_finite(incoming,mass,"0x0019A614");}
    m.vector(body+0x60,linear_momentum);
    auto a=init::bIdentity();a[0]=1.0f/m.scalar(body+0x120);a[5]=1.0f/m.scalar(body+0x134);a[10]=1.0f/m.scalar(body+0x148);
    const auto b=m.matrix(body+0x20);const auto c=bMulMatrix(bMulMatrix(b,a),transpose(b));
    m.vector(body+0x70,transform(m,c,m.vector(angular)));
}
}

namespace hp2::recovered {
// Debug0019A920; E-0048-D04. Scale applies after the original angular impulse cap.
void RigidBody::ApplyImpulseWithScaledInverseInertiaTensor(const port::driving::SolverVector& input,const port::driving::SolverVector& point,float scale){
    using namespace port::driving::native::detail_group_00137100_runtime;
    auto value=input;const float magnitude=length(value);if(1000<magnitude)for(auto& f:value)f*=1000/magnitude;
    const auto inspect=[&](float result,float left,float right,const char* pc,const char* stage,unsigned lane){if(!std::isfinite(result)||!std::isfinite(left)||!std::isfinite(right)){const auto bits=[](float x){std::uint32_t word;std::memcpy(&word,&x,4);return std::to_string(word);};const std::string why=std::string("RigidBody impulse producer stage=")+stage+" body="+std::to_string(address_)+" lane="+std::to_string(lane)+" left_bits="+bits(left)+" right_bits="+bits(right)+" result_bits="+bits(result);hp2::not_implemented(pc,why.c_str());}};
    auto linear=m_.vector(address_+0x60);for(unsigned i=0;i<4;++i){const float old=linear[i];linear[i]=::port::driving::ee_vadd_finite(old,value[i],"0x0019A988");inspect(linear[i],old,value[i],"0x0019A988","linear-add",i);}m_.vector(address_+0x60,linear);
    auto angular=cross(point,value);angular[3]=point[3]; // E-0099-W38:0019A998 VOPMSUB.xyz retains loaded armW.
    const float size=length(angular);if(10<size)for(auto& f:angular)f*=10/size;
    auto previous=m_.vector(address_+0x70);for(unsigned i=0;i<4;++i){const float product=angular[i]*scale;inspect(product,angular[i],scale,"0x0019AA00","angular-scale",i);const float old=previous[i];previous[i]=::port::driving::ee_vadd_finite(old,product,"0x0019AA0C");inspect(previous[i],old,product,"0x0019AA0C","angular-add",i);}m_.vector(address_+0x70,previous);
}
}

// TASK-0099 E-0099-W33. Debug destructor bodies and real original pool ownership.
#include "game_core/indep/inc/ComponentPhysics.hpp"
#include "game_core/inc/bSlotPool.hpp"
namespace hp2::recovered {
void RigidBody::Sub_0019B5F8(int flags){PhysicsComponent(m_,address_).Sub_00199A70(0);if(flags&1)hp2::debug::SlotPool(m_,m_.word(0x327020)).Free(address_);}
}

#include "game_core/indep/inc/Replay.hpp"
#include "game_core/port/race/physics_construction.hpp"
#include <cstring>
namespace hp2::recovered {
// Debug0019A398; E-0099-W74; replay-frame/0019a398.json/type-RigidBodyState.json.
void RigidBody::DoSnapshot(std::uint32_t snapshot,const ::port::race::SnapshotServices& services){
 auto& m=m_;const auto a=address_;hp2::debug::ReplaySnapshot s(m,snapshot);s.BeginSection(0x2ad1e0);
 for(unsigned off:{0x10u,0x20u,0x30u,0x40u})s.MoveData(a+off,12);
 const bool taking=m.byte(snapshot+0x3c)||m.byte(snapshot+0x3d);
 if(taking){m.word(a+0x110,1);m.scalar(a+0x5c,1);for(unsigned off:{0x2cu,0x3cu,0x4cu,0x50u,0x54u,0x58u})m.word(a+off,0);}
 s.MoveData(a+0x80,4);
 ::port::driving::Memory::Temporary local(m,4);bool motionless=true;
 for(unsigned off:{0x60u,0x64u,0x68u,0x70u,0x74u,0x78u}){
  const auto bits=m.word(a+off);if((bits&0x7f800000u)==0&&(bits&0x7fffffu))hp2::not_implemented("0x0019A398","EE denormal momentum equality domain");
  if(m.scalar(a+off)!=0)motionless=false;
 }
 m.word(local.address(),motionless?1:0);s.MoveData(local.address(),1,4,0);
 if(!m.word(local.address())){s.MoveData(a+0x60,12);s.MoveData(a+0x70,12);}
 else if(taking)for(unsigned off:{0x60u,0x68u,0x64u,0x70u,0x78u,0x74u})m.word(a+off,0);
 if(taking){
  const auto table=m.word(a+8),entry=table+0x40;const auto delta=m.signed_half(entry);const auto target=m.word(entry+4);
  if(target!=0x19b170)hp2::not_implemented("0x0019A398","RigidBody snapshot ComputeAuxiliaries virtual target");
  if(!services.physics||!services.physics->port_stack_vector_lane)hp2::not_implemented("0x0019A398","Snapshot requires explicit original unwritten accumulator W policy");
  const auto word=*services.physics->port_stack_vector_lane;float lane;std::memcpy(&lane,&word,4);
  RigidBody(m,a+static_cast<std::uint32_t>(delta)).ComputeAuxiliaries(lane);
 }
 s.EndSection();
}
}
