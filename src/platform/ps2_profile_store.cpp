// PORT-SIDE FILE. No original source path for this module was recovered from the ELF, so its
// name and location are this project's, not the original project's.
#include "ps2_profile_store.hpp"

#include "ps2_asset_root.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <limits>
#include <climits>
#include <cstdio>
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace hp2::platform {
namespace {
namespace fs = std::filesystem;

std::optional<std::string> env_value(const char* name) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') return std::nullopt;
    return std::string(value);
}

// The per-user fallback. Nothing here is recovered: the original writes to a memory card
// through the PS2 driver, which has no host equivalent, so a host convention is chosen and
// labelled as such.
fs::path default_save_root() {
    if (auto local_app_data = env_value("LOCALAPPDATA")) {
        return fs::path(*local_app_data) / "hp2_godot" / "saves";
    }
    if (auto xdg = env_value("XDG_DATA_HOME")) {
        return fs::path(*xdg) / "hp2_godot" / "saves";
    }
    if (auto home = env_value("HOME")) {
        return fs::path(*home) / ".local" / "share" / "hp2_godot" / "saves";
    }
    return fs::path("local") / "saves";
}

fs::path save_root(const std::string& search_start_dir) {
    if (auto configured = env_value(kSaveRootEnvVar)) return fs::path(*configured);
    if (auto configured = resolve_local_config_value(search_start_dir, "port_save_root")) {
        return fs::path(*configured);
    }
    return default_save_root();
}

std::optional<std::vector<std::uint8_t>> read_whole_file(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return std::nullopt;
    return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(file)),
                                     std::istreambuf_iterator<char>());
}

struct FileSnapshot {bool exists=false,ok=true;std::vector<std::uint8_t> bytes;std::string error;};
FileSnapshot snapshot(const fs::path& path){
    FileSnapshot s;std::error_code ec;s.exists=fs::exists(path,ec);
    if(ec){s.ok=false;s.error="cannot inspect endpoint: "+ec.message();return s;}
    if(!s.exists)return s;
    if(!fs::is_regular_file(path,ec)||ec){s.ok=false;s.error="endpoint is not a readable regular file";return s;}
    std::ifstream f(path,std::ios::binary);if(!f){s.ok=false;s.error="cannot read endpoint";return s;}
    s.bytes.assign(std::istreambuf_iterator<char>(f),{});if(f.bad()){s.ok=false;s.error="endpoint read failed";}return s;
}
bool path_equal(const fs::path& a,const fs::path& b){
#ifdef _WIN32
    const auto x=a.native(),y=b.native();
    if(x.size()>std::size_t(INT_MAX)||y.size()>std::size_t(INT_MAX))throw std::runtime_error("protected path exceeds Win32 comparison domain");
    return CompareStringOrdinal(x.c_str(),int(x.size()),y.c_str(),int(y.size()),TRUE)==CSTR_EQUAL;
#else
    return a==b;
#endif
}
std::string protect(const fs::path& target,const std::vector<fs::path>& inputs){
    for(const auto& input:inputs){
        std::error_code ec;const auto a=fs::absolute(input,ec);if(ec)return "cannot resolve configured read-only profile: "+ec.message();
        const auto b=fs::absolute(target,ec);if(ec)return "cannot resolve private output: "+ec.message();
        if(path_equal(a.lexically_normal(),b.lexically_normal()))return "refusing to overwrite configured read-only profile input";
        const auto ca=fs::weakly_canonical(a,ec);if(ec)return "cannot canonicalize read-only profile input: "+ec.message();
        const auto cb=fs::weakly_canonical(b,ec);if(ec)return "cannot canonicalize private output: "+ec.message();
        if(path_equal(ca,cb))return "refusing to overwrite canonical read-only profile alias";
        const bool input_exists=fs::exists(a,ec);if(ec)return "cannot inspect configured read-only profile: "+ec.message();
        const bool output_exists=fs::exists(b,ec);if(ec)return "cannot inspect private profile output: "+ec.message();
        if(input_exists&&output_exists){const bool same=fs::equivalent(a,b,ec);if(ec)return "cannot compare protected profile identity: "+ec.message();if(same)return "refusing to overwrite read-only profile file identity alias";}
    }
    return {};
}
std::atomic<std::uint64_t> temp_sequence{0};
struct TempFile {
    fs::path path;
#ifdef _WIN32
    HANDLE handle=INVALID_HANDLE_VALUE;
    bool open()const{return handle!=INVALID_HANDLE_VALUE;}
    bool close(){if(!open())return true;const auto h=handle;handle=INVALID_HANDLE_VALUE;return CloseHandle(h)!=0;}
#else
    int handle=-1;
    bool open()const{return handle>=0;}
    bool close(){if(!open())return true;const int h=handle;handle=-1;return ::close(h)==0;}
#endif
    ~TempFile(){close();if(!path.empty()){std::error_code ignored;fs::remove(path,ignored);}}
};
std::string native_error(){
#ifdef _WIN32
    return "Win32 error "+std::to_string(GetLastError());
#else
    return std::strerror(errno);
#endif
}
std::string create_temp(TempFile& file,const fs::path& target){
#ifdef _WIN32
    const auto pid=GetCurrentProcessId();
#else
    const auto pid=::getpid();
#endif
    for(unsigned attempt=0;attempt<64;++attempt){
        const auto id=std::to_string(pid)+"-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(++temp_sequence);
        fs::path name(".");name+=target.filename();name+=fs::path(".hp2-atomic-"+id+".tmp");const auto candidate=target.parent_path()/name;
#ifdef _WIN32
        file.handle=CreateFileW(candidate.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(!file.open()){const auto error=GetLastError();if(error==ERROR_FILE_EXISTS||error==ERROR_ALREADY_EXISTS)continue;return "cannot create unique private temp: Win32 error "+std::to_string(error);}
#else
        file.handle=::open(candidate.c_str(),O_WRONLY|O_CREAT|O_EXCL,0600);
        if(!file.open()){if(errno==EEXIST)continue;return "cannot create unique private temp: "+native_error();}
#endif
        file.path=candidate;return {};
    }
    return "unique private temp name attempts exhausted";
}
std::string write_temp(TempFile& file,const std::vector<std::uint8_t>& data,const AtomicFileTestFaults& fault){
    std::size_t offset=0;
    while(offset<data.size()){
        if(fault.fail_after_bytes&&offset>=*fault.fail_after_bytes)return "injected short-write failure";
        std::size_t count=std::min<std::size_t>(data.size()-offset,0x40000000u);
        if(fault.fail_after_bytes)count=std::min(count,*fault.fail_after_bytes-offset);
#ifdef _WIN32
        DWORD written=0;if(!WriteFile(file.handle,data.data()+offset,DWORD(count),&written,nullptr))return "private temp write failed: "+native_error();
        if(!written)return "private temp write made no progress";
        offset+=written;
#else
        const auto written=::write(file.handle,data.data()+offset,count);if(written<0){if(errno==EINTR)continue;return "private temp write failed: "+native_error();}
        if(!written)return "private temp write made no progress";
        offset+=std::size_t(written);
#endif
    }
    if(fault.fail_flush)return "injected flush failure";
#ifdef _WIN32
    if(!FlushFileBuffers(file.handle))return "private temp flush failed: "+native_error();
#else
    if(::fsync(file.handle)!=0)return "private temp flush failed: "+native_error();
#endif
    if(!file.close())return "private temp close failed: "+native_error();
    if(fault.fail_close)return "injected close failure";
    const auto check=snapshot(file.path);if(!check.ok||!check.exists||check.bytes!=data)return "private temp readback does not match complete intended bytes";
    return {};
}

} // namespace

AtomicFileResult write_private_atomic_file(const fs::path& requested,const std::vector<std::uint8_t>& data,
    const std::vector<fs::path>& protected_inputs,const AtomicFileTestFaults& fault){
    bool attempted=false;TempFile temp;
    try{
        std::error_code ec;const auto target=fs::absolute(requested,ec).lexically_normal();
        if(ec||target.filename().empty())return {AtomicFileStatus::NotCommitted,"invalid private endpoint path"};
        if(const auto error=protect(target,protected_inputs);!error.empty())return {AtomicFileStatus::NotCommitted,error};
        const auto old=snapshot(target);if(!old.ok)return {AtomicFileStatus::NotCommitted,old.error};
        fs::create_directories(target.parent_path(),ec);if(ec)return {AtomicFileStatus::NotCommitted,"cannot create private save directory: "+ec.message()};
        if(const auto error=create_temp(temp,target);!error.empty())return {AtomicFileStatus::NotCommitted,error};
        if(const auto error=write_temp(temp,data,fault);!error.empty())return {AtomicFileStatus::NotCommitted,error};
        if(const auto error=protect(target,protected_inputs);!error.empty())return {AtomicFileStatus::NotCommitted,error};
        if(fault.fail_before_replace)return {AtomicFileStatus::NotCommitted,"injected pre-replace failure"};
        attempted=true;bool replaced=false;std::string replacement_error;
        if(fault.fail_replace)replacement_error="injected replace failure";
        else{
#ifdef _WIN32
            replaced=MoveFileExW(temp.path.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
            replaced=::rename(temp.path.c_str(),target.c_str())==0;
#endif
            if(!replaced)replacement_error="private endpoint replace failed: "+native_error();
            if(replaced&&fault.replace_error_after_success){replaced=false;replacement_error="injected lost replace acknowledgement";}
        }
        if(fault.fail_post_replace_verification)return {AtomicFileStatus::Indeterminate,"private replace state indeterminate: injected endpoint verification failure"};
        const auto final=snapshot(target);
        if(final.ok&&final.exists&&final.bytes==data)return {AtomicFileStatus::Committed,{}};
        if(!replaced&&final.ok&&final.exists==old.exists&&(!final.exists||final.bytes==old.bytes))return {AtomicFileStatus::NotCommitted,replacement_error+"; previous endpoint verified"};
        return {AtomicFileStatus::Indeterminate,"private replace state indeterminate; endpoint is not verified intended/prior state: "+replacement_error+(final.ok?"":"; "+final.error)};
    }catch(const std::exception& e){return {attempted?AtomicFileStatus::Indeterminate:AtomicFileStatus::NotCommitted,std::string(attempted?"private replace state indeterminate: ":"private write not committed: ")+e.what()};}
}

std::string profile_store_path(const std::string& search_start_dir, int region) {
    const fs::path path = save_root(search_start_dir) /
                          hp2::memorycard::Sub_0012DEE0(region) /
                          hp2::memorycard::kProfileFileName_002BEBE0;
    return path.generic_string();
}

std::optional<std::string> resolve_profile_path(const std::string& search_start_dir, int region) {
    if (auto configured = env_value(kProfilePathEnvVar)) return configured;
    if (auto configured = resolve_local_config_value(search_start_dir, "profile_path")) {
        return configured;
    }
    const std::string own = profile_store_path(search_start_dir, region);
    std::error_code ec;
    if (fs::is_regular_file(fs::path(own), ec)) return own;
    return std::nullopt;
}

ProfileLoad load_profile(const std::string& search_start_dir, int region,
                         hp2::memorycard::Profile_002FC050& profile) {
    ProfileLoad result;
    auto path = resolve_profile_path(search_start_dir, region);
    if (!path) {
        result.error = "not configured";
        return result;
    }
    result.path = *path;

    auto payload = read_whole_file(fs::path(*path));
    if (!payload) {
        result.error = "cannot open " + *path;
        return result;
    }

    // The recovered half decides what a valid payload is; this file only supplies the bytes.
    switch (hp2::memorycard::Load_00245D30(*payload, profile)) {
    case hp2::memorycard::LoadResult_00245780::kOk:
        result.loaded = true;
        return result;
    case hp2::memorycard::LoadResult_00245780::kWrongLength:
        result.error = "wrong length";
        return result;
    case hp2::memorycard::LoadResult_00245780::kBadChecksum:
        result.error = "bad checksum";
        return result;
    }
    result.error = "unknown load result";
    return result;
}

std::string store_profile_for_test(const std::string& search_start_dir,int region,
                          hp2::memorycard::Profile_002FC050& profile,const AtomicFileTestFaults& fault) {
    const fs::path path(profile_store_path(search_start_dir, region));
    std::vector<fs::path> inputs;
    if(const auto input=env_value(kProfilePathEnvVar))inputs.emplace_back(*input);
    if(const auto input=resolve_local_config_value(search_start_dir,"profile_path"))inputs.emplace_back(*input);
    auto candidate=profile;const auto payload=hp2::memorycard::Store_00245738(candidate);
    const auto result=write_private_atomic_file(path,payload,inputs,fault);
    if(result.status==AtomicFileStatus::Committed){profile=std::move(candidate);return {};}
    return result.error.empty()?"private profile write was not confirmed":result.error;
}
std::string store_profile(const std::string& search_start_dir,int region,hp2::memorycard::Profile_002FC050& profile){return store_profile_for_test(search_start_dir,region,profile,{});}

} // namespace hp2::platform
