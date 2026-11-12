#include "nis_services.hpp"
#include "game_core/port/not_implemented.hpp"
namespace port::race {
NisServices& require_nis_services(driving::Memory& memory){const auto* bound=detail::current_nis_services;if(!bound||bound->memory!=&memory||!bound->services)hp2::not_implemented("0x001AE8F8","actual same-Memory NIS services are not bound");return *bound->services;}
}
