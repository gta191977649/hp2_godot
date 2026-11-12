// TASK-0112 E-0112-NC01. Include once at end original nis.cpp.
#include "game_core/indep/inc/CameraCoordinator.hpp"
namespace hp2::debug {
std::uint32_t NisManager::CreateNisCameraCoordinator(int view){const auto coord=m_.allocate(0x24);NisCameraCoordinator(m_,coord).Sub_00198DF8(view,m_.word(address_+0x54),m_.word(address_+0x58),m_.signed_word(address_+0x50),m_.scalar(address_+0x90),m_.signed_word(address_+0x44));EnableOverrideRendering(view,m_.word(address_+0x54),m_.word(address_+0x58),coord);return coord;}
void NisManager::DeleteNisCameraCoordinator(std::uint32_t coord){DisableNisCameraCoordinator(coord);if(coord)NisCameraCoordinator(m_,coord).Sub_00198F18(3);}
void NisManager::DisableNisCameraCoordinator(std::uint32_t coord){if(!m_.word(coord+0xc)){DisableOverrideRendering(m_.signed_word(coord+0x10),m_.word(coord+0x1c),m_.word(coord+0x20));NisCameraCoordinator(m_,coord).Disable();}}
void NisManager::EnableNisCameraCoordinator(std::uint32_t coord){if(m_.word(coord+0xc)){EnableOverrideRendering(m_.signed_word(coord+0x10),m_.word(coord+0x1c),m_.word(coord+0x20),coord);NisCameraCoordinator(m_,coord).Enable();}}
}
