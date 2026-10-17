#include "game_core/indep/src/rigidbody.hpp"

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
    auto linear=m.vector(body+0x60);for(unsigned i=0;i<4;++i)linear[i]=linear[i]+value[i];m.vector(body+0x60,linear);
    auto angular=cross(point,value);const auto size=length(angular);
    if(10.0f<size)for(auto& lane:angular)lane=lane*(10.0f/size);
    const auto old=m.vector(body+0x70);for(unsigned i=0;i<4;++i)angular[i]=old[i]+angular[i];m.vector(body+0x70,angular);
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
    for (unsigned i = 0; i < 4; ++i) body.Field_0xf0[i] = body.Field_0xf0[i] + limited[i];
    // Original A1, rather than capped force, feeds the cross product.
    const SolverVector position = a2;
    SolverVector value = cross_xyz(position, a1, position[3]);
    const float cross_length = xyz_length(value);
    if (100.0f < cross_length) {
        const float factor = 100.0f / cross_length;
        for (float& lane : value) lane = lane * factor;
    }
    for (unsigned i = 0; i < 4; ++i) body.Field_0x100[i] = body.Field_0x100[i] + value[i];
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
    for (unsigned i = 0; i < 4; ++i) body.Field_0x100[i] = body.Field_0x100[i] + a1[i];
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
    port::driving::ApplyForce(p,force,point);m.vector(body+0xf0,p.Field_0xf0);m.vector(body+0x100,p.Field_0x100);
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

// Retail 0x0019D370; E-0041-M01; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace hp2::recovered {
port::driving::SolverMatrix VectorStarMatrix(const port::driving::SolverVector& a,const port::driving::SolverMatrix& b){using namespace port::driving;using namespace port::driving::native;
    using namespace detail_group_0019d4d0;
    Matrix r{};
    for(unsigned c=0;c<3;++c) {
        float f0=-a[2]*b[c*4+1],f1=a[1]*b[c*4+2];r[c*4]=f0+f1;
        f0=a[2]*b[c*4];f1=a[0]*b[c*4+2];r[c*4+1]=f0-f1;
        f0=-a[1]*b[c*4];f1=a[0]*b[c*4+1];r[c*4+2]=f0+f1;
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
    m.vector(output+0x50,m.vector(body+0xf0));m.vector(output+0x60,m.vector(body+0x100));
    const float angular_length=length(m.vector(body+0xa0));
    const float f=port::driving::Table::GetValue(m,0x00308518,angular_length);
    add(m,output+0x60,scale(m.vector(body+0x70),-f));
    float f12=length(m.vector(body+0x90))/g(m,0xb258);const float f20=angular_length/g(m,0xb25c);if(f12<f20)f12=f20;
    const float limit=port::driving::Table::GetValue(m,0x00308550,f12);
    m.scalar(output+0x70,m.scalar(body+0x80)<limit ? g(m,0xb268) : -g(m,0xb26c));
    if(m.word(body+0x110)!=0) {
        add(m,output+0x60,scale(m.vector(body+0x70),-g(m,0xb264)*m.scalar(body+0x80)));
        add(m,output+0x50,scale(m.vector(body+0x60),-g(m,0xb260)*m.scalar(body+0x80)));
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
    m.vector(body+0x90,scale(m.vector(body+0x60),m.scalar(body+0x11c)));
    const auto b=m.matrix(body+0x20);const auto c=bMulMatrix(bMulMatrix(b,m.matrix(body+0x120)),transpose(b));
    m.vector(body+0xa0,transform(m,c,m.vector(body+0x70)));
    if(m.scalar(body+0x80)<0)m.scalar(body+0x80,0);
    m.matrix(body+0xb0,b);m.vector(body+0xe0,m.vector(body+0x10));m.scalar(body+0xec,1);
    m.vector(body+0xf0,{{0,0,0,unused_stack_w}});m.vector(body+0x100,{{0,0,0,unused_stack_w}});
    const Vector force{{0,0,-g(m,0xb238)*m.scalar(body+0x118),0}},point{{0,0,0,0}};
    hp2::recovered::RigidBody(m,body).ApplyForce(force,point);
}
void RigidBody::ApplyMoment(const port::driving::SolverVector& moment){auto value=m_.vector(address_+0x100);for(unsigned i=0;i<4;++i)value[i]=value[i]+moment[i];m_.vector(address_+0x100,value);}
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
    m.vector(body+0x60,scale(m.vector(velocity),m.scalar(body+0x118)));
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
    auto linear=m_.vector(address_+0x60);for(unsigned i=0;i<4;++i)linear[i]+=value[i];m_.vector(address_+0x60,linear);
    auto angular=cross(point,value);const float size=length(angular);if(10<size)for(auto& f:angular)f*=10/size;
    auto previous=m_.vector(address_+0x70);for(unsigned i=0;i<4;++i)previous[i]+=angular[i]*scale;m_.vector(address_+0x70,previous);
}
}
