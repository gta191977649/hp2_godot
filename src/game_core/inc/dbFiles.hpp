#pragma once
#include "indep/inc/dbFilesBase.hpp"
#include "inc/memorycard.hpp"
#include <vector>
// cDBFileSys, its live state/callback contract, and this original header are
// individually identified in saves57. Native service performs real file I/O
// and publishes completion only after read validation or atomic replacement.
class cDBFileSys final : public cDBFilesBase {
public:
    explicit cDBFileSys(const char* utf8Directory=nullptr);
    bool SetState(int state,BMemcardDone done=nullptr,void* parameter=nullptr);
    void Service(int port=0,int slot=0);
    int CalculateFreeSpaceRequired(std::int32_t bytes) const;
    int GetFreeSpaceOnDevice();
    void WaitUntilFunctionCompletes();
    bool Busy() const noexcept {return state_!=HP2_BMC_FS_IDLE;}
    const char* GetFileName(std::size_t index) const;
    std::size_t GetFileCount() const noexcept {return names_.size();}
    const std::string& LastErrorMessage() const noexcept {return card_.LastError();}
    const std::filesystem::path& Directory() const noexcept {return card_.Directory();}
    // Variable-size PC text transfers keep the same native file owner.
    cMC::Result ReadFile(const char* name,std::vector<std::uint8_t>& bytes,
        std::size_t maximumBytes=4u*1024u*1024u);
    cMC::Result WriteNewFile(const char* name,const void* data,std::size_t bytes);
    void SetBackupFile(const char* name,std::vector<std::uint8_t> bytes);
private:
    cMC card_;
    int state_{HP2_BMC_FS_IDLE};
    BMemcardDone done_{};
    void* parameter_{};
    std::vector<std::string> names_;
    std::string backupName_;
    std::vector<std::uint8_t> backupBytes_;
};
