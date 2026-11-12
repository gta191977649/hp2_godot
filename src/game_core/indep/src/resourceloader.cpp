// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\resourceloader.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/ResourceFile.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001995E0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001995e0.md.
void ResourceFile::GetSize() {
    hp2::not_implemented("0x001995E0", "ResourceFile::GetSize (signature and full body unresolved)");
}
} // namespace hp2::recovered

// TASK-0099 E-0099-W62, Debug authority815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
namespace hp2::debug {
void Sub_00197508(::port::driving::Memory& m){m.word(0x327b7c,0x327b78);m.word(0x327b78,0x327b78);}
std::uint32_t FindResourceFile(::port::driving::Memory& m,std::uint32_t filename){
 auto file=m.word(0x327b7c);if(!file)hp2::not_implemented("0x001972E8","original ResourceFileList static initialization is required");
 for(;file!=0x327b78;file=m.word(file+4)){
  std::uint32_t i=0;for(;;++i){const auto left=m.byte(file+0x10+i),right=m.byte(filename+i);if(left!=right)break;if(!left)return file;}
 }
 return 0;
}
}
