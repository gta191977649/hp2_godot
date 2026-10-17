
// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail None; explicit host partition; original TU unconfirmed
namespace port::driving::native {
void integrate(Memory& m,std::uint32_t list,float dt,InitServices& services){
    using namespace detail_group_00138e08;
    IntegrationCalls calls;
    calls.derivative=[](Memory& state,std::uint32_t object,std::uint32_t out){
        const auto table=state.word(object+8),target=state.word(table+0x3c);const auto adjusted=object+static_cast<std::int32_t>(state.signed_half(table+0x38));
        switch(target){
        case 0x0019d4d0:GetDerivative(state,adjusted,out);return;
        case 0x001a46b0:port::driving::Wheel::GetDerivative(state,adjusted,out);return;
        case 0x001a63c0:port::driving::Sub_001A63C0(state,adjusted,out);return;
        case 0x0018a9d8:port::driving::Engine::GetDerivative(state,adjusted,out);return;
        case 0x0018b698:port::driving::Sub_0018B698(state,adjusted,out);return;
        default:hp2::not_implemented("0x0019C670","untranslated derivative target");}
    };
    calls.after=[&](Memory& state,std::uint32_t head){const auto table=state.word(head+0xc),target=state.word(table+0x14);if(target!=0x00137788)hp2::not_implemented("0x0019C290","unknown post-integrate target");PhysicsCar::ComputeAuxiliaries(state,head+static_cast<std::int32_t>(state.signed_half(table+0x10)),services);};
    IntegrateEuler(m,dt,list,calls);
}
}

// Retail None; explicit host partition; original TU unconfirmed
namespace port::driving {
std::int32_t cvt_w_s(float f) {
    using namespace detail_group_0018f808;
    if(!std::isfinite(f) || static_cast<double>(f)<-2147483648.0 || static_cast<double>(f)>=2147483648.0)
        hp2::not_implemented("0x0018F878","CVT.W.S exceptional or out-of-range operand");
    return static_cast<std::int32_t>(f);
}
}

// Retail None; explicit host partition; original TU unconfirmed
namespace port::driving::native {
Matrix transpose(const Matrix& a) {
    using namespace detail_group_0019d4d0;Matrix b;port::driving::bTransposeMatrix(b,a);return b;}
}

// Retail None; explicit host partition; original TU unconfirmed
namespace port::driving::native {
Vector transform(Memory& m,const Matrix& a,const Vector& b) {
    using namespace detail_group_0019d4d0;Vector c;port::driving::bMulMatrix(c,a,b,{m.vector(0x00320a60)});return c;}
}
