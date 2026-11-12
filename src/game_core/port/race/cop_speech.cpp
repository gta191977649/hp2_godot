// Port process lifecycle and service binding; recovered bodies use exact original owners.
#include "cop_speech.hpp"
#include <stdexcept>
namespace port::race {
// Port explicit lifetime binding of original00223490/00204DC0; E-0099-W16.
// Actual constructor is in its confirmed speech/mgr.cpp owner.
void CopSpeechBindings::init(){
 if(initialized_)throw std::logic_error("Police speech binding already initialized");
 constexpr std::uint32_t manager=0x32b398;
 if(memory_.word(0x328918)==0){
  construct_manager(manager);
  memory_.word(0x328918,1);owns_manager_=true;
 }
 memory_.word(0x328c18,manager);initialized_=true;
}
// Port session resource teardown; original mgr destructor00204E18 stops FSM.
// Original SpeechNoise destructor002101E8 has no stores for its static object.
void CopSpeechBindings::shutdown(){
 if(!initialized_)return;
 if(owns_manager_){constexpr std::uint32_t manager=0x32b398;
  memory_.word(manager,0);
  for(auto it=allocations_.rbegin();it!=allocations_.rend();++it)memory_.release(*it);
  allocations_.clear();memory_.word(0x328918,0);memory_.word(0x328c18,0);owns_manager_=false;
 }
 initialized_=false;
}
}
