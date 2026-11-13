#pragma once
// Source declaration licence: saves57/individual_headers57.json. Native
// filesystem replacement for the individually identified cMC I/O owner;
// Sony SDK handles, card formatting and PS2 icon resources are not emulated.
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
class cMC final {
public:
    enum Result : int {Ok=0,NotFound=5,InProgress=15,IoError=16,
        InvalidSize=17,NoSpace=18,Unsupported=19,Corrupt=20,ReadOnly=21};
    explicit cMC(const char* utf8Directory=nullptr);
    Result MCGetInfo(std::uint64_t& freeBytes);
    Result MCRead(const char* name,void* destination,std::size_t bytes);
    Result MCReadFile(const char* name,std::vector<std::uint8_t>& destination,
        std::size_t maximumBytes=4u*1024u*1024u);
    Result MCWrite(const char* name,const void* source,std::size_t bytes,bool replaceExisting=true);
    Result MCDelete(const char* name);
    Result MCGetDir(std::vector<std::string>& names);
    Result FileExists(const char* name,bool& exists) const;
    const std::filesystem::path& Directory() const noexcept {return root_;}
    const std::string& LastError() const noexcept {return error_;}
private:
    std::filesystem::path FilePath(const char* name) const;
    Result Fail(const std::string& operation,int nativeError) const;
    Result Fail(const std::string& operation,const std::error_code& error) const;
    std::filesystem::path root_;
    mutable std::string error_;
};
