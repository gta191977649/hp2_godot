#include "inc/dbFiles.hpp"
#include <algorithm>
#include <climits>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace {
cDBFileSys& Owner(BMemcardFileSys* base) {return *static_cast<cDBFileSys*>(base);}
void Destroy(BMemcardFileSys* base) {delete static_cast<cDBFileSys*>(base);}
void Service(BMemcardFileSys* base,int port,int slot) {Owner(base).Service(port,slot);}
void Data(BMemcardFileSys* base,const char* name,void* data,int bytes) {Owner(base).SetFileData(name,data,bytes);}
int Required(BMemcardFileSys* base,int bytes) {return Owner(base).CalculateFreeSpaceRequired(bytes);}
int Space(BMemcardFileSys* base) {return Owner(base).GetFreeSpaceOnDevice();}
int State(BMemcardFileSys* base,int state,BMemcardDone done,void* parameter) {return Owner(base).SetState(state,done,parameter);}
const BMemcardFileSysVtbl NativeMethods{Destroy,Service,Data,Required,Space,State};
}

void cDBFilesBase::SetFileData(const char* name,void* data,std::int32_t bytes)
{
    name_=name ? name : "";data_=data;bytes_=bytes;
}

cDBFileSys::cDBFileSys(const char* directory):card_(directory)
{
    vt=&NativeMethods;lastError=HP2_BMC_OK;fileExists=0;
}

bool cDBFileSys::SetState(int operation,BMemcardDone done,void* parameter)
{
    if(Busy())return false;
    if(operation!=HP2_BMC_FS_SAVE && operation!=HP2_BMC_FS_LOAD &&
       operation!=HP2_BMC_FS_CHECK_EXISTENCE && operation!=HP2_BMC_FS_GET_DIR && operation!=HP2_BMC_FS_VERIFY) {
        // Formatting/unformatting a Sony card is excluded on a PC. No disk
        // formatter, silent success or erased user directory replaces it.
        lastError=cMC::Unsupported;return false;
    }
    state_=operation;done_=done;parameter_=parameter;lastError=HP2_BMC_IN_PROGRESS;return true;
}

void cDBFileSys::Service(int port,int slot)
{
    if(!Busy())return;
    const auto operation=state_;
    if(port!=0 || slot!=0)lastError=cMC::Unsupported;
    else if(bytes_<0)lastError=cMC::InvalidSize;
    else {
        switch(operation) {
        case HP2_BMC_FS_SAVE:
            // The previous validated text reaches durable storage before
            // the new primary is atomically replaced. Either interruption
            // boundary therefore retains at least one complete profile.
            lastError=backupName_.empty()?cMC::Ok:
                card_.MCWrite(backupName_.c_str(),backupBytes_.data(),backupBytes_.size());
            if(lastError==cMC::Ok)lastError=card_.MCWrite(name_.c_str(),data_,static_cast<std::size_t>(bytes_));
            if(lastError==0)fileExists=1;
            break;
        case HP2_BMC_FS_LOAD:
            lastError=card_.MCRead(name_.c_str(),data_,static_cast<std::size_t>(bytes_));
            if(lastError==0)fileExists=1;
            break;
        case HP2_BMC_FS_CHECK_EXISTENCE: {
            bool exists=false;
            lastError=card_.FileExists(name_.c_str(),exists);fileExists=exists;
            break;
        }
        case HP2_BMC_FS_GET_DIR:
            lastError=card_.MCGetDir(names_);
            break;
        case HP2_BMC_FS_VERIFY: {
            std::uint64_t freeBytes=0;
            lastError=card_.MCGetInfo(freeBytes);
            break;
        }
        default:
            lastError=cMC::Unsupported;
            break;
        }
    }
    auto callback=done_;void* argument=parameter_;
    state_=HP2_BMC_FS_IDLE;done_=nullptr;parameter_=nullptr;
    backupName_.clear();backupBytes_.clear();
    // Detach before invoking: an actual completion is allowed to enqueue the
    // next operation, with no nested owner or phantom finished transaction.
    if(callback)callback(argument);
}

cMC::Result cDBFileSys::ReadFile(const char* name,std::vector<std::uint8_t>& bytes,std::size_t maximumBytes)
{
    if(Busy())return cMC::InProgress;
    const auto result=card_.MCReadFile(name,bytes,maximumBytes);lastError=result;fileExists=result==cMC::Ok;
    return result;
}

cMC::Result cDBFileSys::WriteNewFile(const char* name,const void* data,std::size_t bytes)
{
    if(Busy())return cMC::InProgress;
    const auto result=card_.MCWrite(name,data,bytes,false);lastError=result;fileExists=result==cMC::Ok;
    return result;
}

void cDBFileSys::SetBackupFile(const char* name,std::vector<std::uint8_t> bytes)
{
    if(Busy())throw std::logic_error("Cannot replace an active profile backup transfer");
    backupName_=name?name:"";backupBytes_=std::move(bytes);
}

int cDBFileSys::CalculateFreeSpaceRequired(std::int32_t bytes) const
{
    if(bytes<0 || bytes>INT_MAX-1023)return -1;
    // PAL12D1A0 rounds payload to Sony1KiB clusters then includes five icon/
    // directory clusters. Retained only as the original UI capacity unit;
    // actual write authorization uses the native filesystem's byte count.
    return (bytes+1023)/1024+5;
}

int cDBFileSys::GetFreeSpaceOnDevice()
{
    std::uint64_t bytes=0;lastError=card_.MCGetInfo(bytes);
    if(lastError!=0)return -1;
    return static_cast<int>(std::min<std::uint64_t>(bytes/1024,INT_MAX));
}

void cDBFileSys::WaitUntilFunctionCompletes()
{
    // Native operations have no Sony polling thread. Service performs the
    // pending real I/O once, and its result remains available to the caller.
    if(Busy())Service();
}

const char* cDBFileSys::GetFileName(std::size_t index) const
{
    if(index>=names_.size())throw std::out_of_range("Native save directory entry index");
    return names_[index].c_str();
}

extern "C" BMemcardFileSys* hp2CreateNativeMemoryCard(const char* directory)
{
    try {return new cDBFileSys(directory);}catch(...) {return nullptr;}
}
