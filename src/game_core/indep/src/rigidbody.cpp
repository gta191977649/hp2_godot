
// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x0019CC58; E-0042-C095; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace port::driving::init {
void CalculateInertiaTensor(ByteView out,ByteView config){
    out.require(0,0x160);config.require(0,0x50);
    const float f1=config.scalar(0);
    std::array<float,16> matrix;
    for(std::size_t i=0;i<16;++i)matrix[i]=config.scalar(0x10+4*i);
    const auto inverse=bInvertMatrix(matrix);
    out.scalar(0x118,f1);out.scalar(0x11c,1.0f/f1);
    for(std::size_t i=0;i<16;++i)out.scalar(0x120+4*i,inverse[i]);
}
}

// Retail 0x0019C9B0; E-0042-C093; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace port::driving::init {
void Sub_0019C9B0(ByteView out,std::uint32_t config_address,ByteView config){
    out.require(0,0x17c);config.require(0,0x50);
    out.word(0x10,0);out.word(0x14,0);out.word(0x18,0);
    out.word(8,0x002b5430);out.word(0xc,config_address);
    const auto matrix=bIdentity();
    for(std::size_t i=0;i<16;++i)out.scalar(0x20+4*i,matrix[i]);
    for(auto offset:{0x60,0x64,0x68,0x70,0x74,0x78,0x80,0x90,0x94,0x98,
                    0xa0,0xa4,0xa8,0xf0,0xf4,0xf8,0x100,0x104,0x108,0x170,0x174,0x114})out.word(offset,0);
    out.scalar(0x178,1);out.word(0x110,1);
    CalculateInertiaTensor(out,config);
}
}

// Retail 0x0019CC40; E-0042-C094; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace port::driving::init {
void Calibrate(ByteView out,const std::array<std::uint32_t,4>* a1){
    if(a1){out.require(0x160,16);for(std::size_t i=0;i<4;++i)out.word(0x160+4*i,(*a1)[i]);}
}
}

// Retail 0x0019CF10; E-0041-M01; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace port::driving::native {
void ApplyImpulse(Memory& m,std::uint32_t body,const std::array<float,4>& input,const std::array<float,4>& point){
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

// Retail 0x0019D368; E-0042-U132; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace port::driving {
void bMulMatrixVector(SolverVector& out, const SolverMatrix& matrix,
                 const SolverVector& in, const SolverMathContext& context) {
    using namespace detail_group_00138f38;
    bMulMatrix(out, matrix, in, context);
}
}

// Retail 0x0019CD90; E-0041-M01; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace port::driving {
void ApplyForce(SolverBodyProjection& body, const SolverVector& a1, const SolverVector& a2) {
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

// Retail 0x0019CE80; E-0042-C097; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace port::driving {
void ApplyForceInFrame(SolverBodyProjection& body, const SolverVector& a1,
                 const SolverVector& a2, const SolverMathContext& context) {
    using namespace detail_group_00138f38;
    SolverVector first{}, second{};
    bMulMatrixVector(first, body.Field_0x20, a1, context);
    bMulMatrixVector(second, body.Field_0x20, a2, context);
    ApplyForce(body, first, second);
}
}

// Retail 0x0019CEF8; E-0042-C098; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace port::driving {
void ApplyMoment(SolverBodyProjection& body, const SolverVector& a1) {
    using namespace detail_group_00138f38;
    for (unsigned i = 0; i < 4; ++i) body.Field_0x100[i] = body.Field_0x100[i] + a1[i];
}
}

// Retail 0x0019CD90; E-0041-M01; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace port::driving::native {
void ApplyForce(Memory& m,std::uint32_t body,const Vector& force,const Vector& point) {
    using namespace detail_group_0019d4d0;
    SolverBodyProjection p;p.Field_0xf0=m.vector(body+0xf0);p.Field_0x100=m.vector(body+0x100);
    port::driving::ApplyForce(p,force,point);m.vector(body+0xf0,p.Field_0xf0);m.vector(body+0x100,p.Field_0x100);
}
}

// Retail 0x0019CE80; E-0042-C097; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace port::driving::native {
void ApplyForceInFrame(Memory& m,std::uint32_t body,const Vector& force,const Vector& point) {
    using namespace detail_group_0019d4d0;
    const auto a=m.matrix(body+0x20);ApplyForce(m,body,transform(m,a,force),transform(m,a,point));
}
}

// Retail 0x0019D370; E-0041-M01; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace port::driving::native {
Matrix VectorStarMatrix(const Vector& a,const Matrix& b) {
    using namespace detail_group_0019d4d0;
    Matrix r{};
    for(unsigned c=0;c<3;++c) {
        float f0=-a[2]*b[c*4+1],f1=a[1]*b[c*4+2];r[c*4]=f0+f1;
        f0=a[2]*b[c*4];f1=a[0]*b[c*4+2];r[c*4+1]=f0-f1;
        f0=-a[1]*b[c*4];f1=a[0]*b[c*4+1];r[c*4+2]=f0+f1;
    }r[15]=1;return r;
}
}

// Retail 0x0019D4D0; E-0041-R03; E-0042-S001; exact Debug TU ..\indep\src\rigidbody.cpp
namespace port::driving::native {
void GetDerivative(Memory& m,std::uint32_t body,std::uint32_t output) {
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
    m.vector(output,m.vector(body+0x90));m.matrix(output+0x10,VectorStarMatrix(m.vector(body+0xa0),m.matrix(body+0x20)));
    m.vector(output+0x50,m.vector(body+0xf0));m.vector(output+0x60,m.vector(body+0x100));
    const float angular_length=length(m.vector(body+0xa0));
    const float f=port::driving::Table::GetValue(m,0x00310df8,angular_length);
    add(m,output+0x60,scale(m.vector(body+0x70),-f));
    float f12=length(m.vector(body+0x90))/g(m,0xb258);const float f20=angular_length/g(m,0xb25c);if(f12<f20)f12=f20;
    const float limit=port::driving::Table::GetValue(m,0x00310e30,f12);
    m.scalar(output+0x70,m.scalar(body+0x80)<limit ? g(m,0xb268) : -g(m,0xb26c));
    if(m.word(body+0x110)!=0) {
        add(m,output+0x60,scale(m.vector(body+0x70),-g(m,0xb264)*m.scalar(body+0x80)));
        add(m,output+0x50,scale(m.vector(body+0x60),-g(m,0xb260)*m.scalar(body+0x80)));
    }
    const float f1=m.scalar(body+0x170),f2=m.scalar(body+0x108);
    if(f1!=0) {if(f1<f2)m.scalar(output+0x68,f1);else if(f2< -f1)m.scalar(output+0x68,-f1);}
}
}

#include "game_core/indep/src/rigidbody.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x0019C9B0; E-0042-C093; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0019c9b0.md.
void RigidBody::Sub_0019C9B0() {
    hp2::not_implemented("0x0019C9B0", "RigidBody::Sub_0019C9B0 (signature and full body unresolved)");
}

// ELF 0x0019CC40; E-0042-C094; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0019cc40.md.
void RigidBody::Calibrate() {
    hp2::not_implemented("0x0019CC40", "RigidBody::Calibrate (signature and full body unresolved)");
}

// ELF 0x0019CC58; E-0042-C095; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0019cc58.md.
void RigidBody::CalculateInertiaTensor() {
    hp2::not_implemented("0x0019CC58", "RigidBody::CalculateInertiaTensor (signature and full body unresolved)");
}

// ELF 0x0019CD90; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0019cd90.md.
void RigidBody::ApplyForce() {
    hp2::not_implemented("0x0019CD90", "RigidBody::ApplyForce (signature and full body unresolved)");
}

// ELF 0x0019CE80; E-0042-C097; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0019ce80.md.
void RigidBody::ApplyForceInFrame() {
    hp2::not_implemented("0x0019CE80", "RigidBody::ApplyForceInFrame (signature and full body unresolved)");
}

// ELF 0x0019CEF8; E-0042-C098; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0019cef8.md.
void RigidBody::ApplyMoment() {
    hp2::not_implemented("0x0019CEF8", "RigidBody::ApplyMoment (signature and full body unresolved)");
}

// ELF 0x0019CF10; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0019cf10.md.
void RigidBody::ApplyImpulse() {
    hp2::not_implemented("0x0019CF10", "RigidBody::ApplyImpulse (signature and full body unresolved)");
}

// ELF 0x0019CFF8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0019cff8.md.
void RigidBody::ApplyImpulseWithScaledInverseInertiaTensor() {
    hp2::not_implemented("0x0019CFF8", "RigidBody::ApplyImpulseWithScaledInverseInertiaTensor (signature and full body unresolved)");
}

// ELF 0x0019D1F0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0019d1f0.md.
void RigidBody::GetEnergy() {
    hp2::not_implemented("0x0019D1F0", "RigidBody::GetEnergy (signature and full body unresolved)");
}

// ELF 0x0019D4D0; E-0041-R03; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/0019d4d0.md.
void RigidBody::GetDerivative() {
    hp2::not_implemented("0x0019D4D0", "RigidBody::GetDerivative (signature and full body unresolved)");
}

// ELF 0x0019D980; E-0042-C100; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0019d980.md.
void RigidBody::PrintCurrentState() {
    hp2::not_implemented("0x0019D980", "RigidBody::PrintCurrentState (signature and full body unresolved)");
}
} // namespace hp2::recovered
