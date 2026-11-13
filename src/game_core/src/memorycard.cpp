#include "inc/memorycard.hpp"
#include "indep/inc/Flags.hpp"
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <system_error>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace {
constexpr std::size_t MaximumSaveBytes=4u*1024u*1024u;
std::atomic<std::uint64_t> nextTemporaryFile{0};
std::filesystem::path DefaultDirectory()
{
    // Explicit directory overrides still belong to this one file owner.
    // The normal native game keeps writable state in the Saves child.
    return hp2::nativeExecutablePath().parent_path()/"Saves";
}
}

cMC::cMC(const char* directory)
    :root_(directory && *directory ? std::filesystem::u8path(directory) : DefaultDirectory())
{
    std::error_code ec;
    root_=std::filesystem::absolute(root_,ec).lexically_normal();
    if(ec)throw std::system_error(ec,"Resolve native save directory");
}

std::filesystem::path cMC::FilePath(const char* name) const
{
    if(!name || !*name || std::strlen(name)>80 || !std::strcmp(name,".") || !std::strcmp(name,".."))
        throw std::invalid_argument("Invalid save filename");
    for(const auto* p=reinterpret_cast<const unsigned char*>(name);*p;++p)
        if(!((*p>='a' && *p<='z') || (*p>='A' && *p<='Z') || (*p>='0' && *p<='9') || *p=='_' || *p=='-' || *p=='.'))
            throw std::invalid_argument("Save filename escapes its native directory");
    return root_/std::filesystem::u8path(name);
}

cMC::Result cMC::Fail(const std::string& operation,int value) const
{
    // Native API failures use GetLastError on Windows and errno on POSIX.
    // The overload below preserves the different category of filesystem/CRT
    // errors. In particular, Windows CRT EIO=5 must not be decoded as ERROR_ACCESS_DENIED.
    return Fail(operation,std::error_code(value,std::system_category()));
}

cMC::Result cMC::Fail(const std::string& operation,const std::error_code& code) const
{
    error_=operation+": "+code.message()+" ["+code.category().name()+":"+
        std::to_string(code.value())+"]\nDirectory: "+root_.u8string();
#ifdef _WIN32
    if(code.category()==std::system_category()) {
        const auto value=code.value();
        if(value==ERROR_FILE_NOT_FOUND || value==ERROR_PATH_NOT_FOUND)return NotFound;
        if(value==ERROR_DISK_FULL || value==ERROR_HANDLE_DISK_FULL)return NoSpace;
        if(value==ERROR_ACCESS_DENIED || value==ERROR_WRITE_PROTECT)return ReadOnly;
    }
#else
    if((code.category()==std::system_category() || code.category()==std::generic_category()) &&
       code.value()==EDQUOT)return NoSpace;
#endif
    if(code==std::errc::no_such_file_or_directory)return NotFound;
    if(code==std::errc::no_space_on_device)return NoSpace;
    if(code==std::errc::permission_denied || code==std::errc::operation_not_permitted ||
       code==std::errc::read_only_file_system)return ReadOnly;
    return IoError;
}

cMC::Result cMC::MCGetInfo(std::uint64_t& freeBytes)
{
    freeBytes=0;error_.clear();std::error_code ec;
    std::filesystem::create_directories(root_,ec);
    if(ec)return Fail("Prepare native save directory",ec);
    const auto space=std::filesystem::space(root_,ec);
    if(ec)return Fail("Read available save space",ec);
    freeBytes=space.available;return Ok;
}

cMC::Result cMC::FileExists(const char* name,bool& exists) const
{
    exists=false;error_.clear();
    try {
        const auto path=FilePath(name);std::error_code ec;
        const auto status=std::filesystem::symlink_status(path,ec);
        if(ec)return Fail("Inspect save file",ec);
        if(!std::filesystem::exists(status))return NotFound;
        if(!std::filesystem::is_regular_file(status)){error_="Save path is not an ordinary file";return IoError;}
        exists=true;return Ok;
    } catch(const std::exception& e) {error_=e.what();return IoError;}
}

cMC::Result cMC::MCReadFile(const char* name,std::vector<std::uint8_t>& destination,std::size_t maximumBytes)
{
    error_.clear();
    if(!maximumBytes || maximumBytes>MaximumSaveBytes){error_="Invalid maximum native save size";return InvalidSize;}
    try {
        const auto path=FilePath(name);std::vector<std::uint8_t> image;std::size_t size=0;int failure=0;
#ifdef _WIN32
        const HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,
            OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
        if(file==INVALID_HANDLE_VALUE)return Fail("Open native save",static_cast<int>(GetLastError()));
        struct Close {HANDLE file;~Close(){if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);}} close{file};
        BY_HANDLE_FILE_INFORMATION info{};LARGE_INTEGER length{};
        if(!GetFileInformationByHandle(file,&info) || !GetFileSizeEx(file,&length))return Fail("Inspect open native save",static_cast<int>(GetLastError()));
        if((info.dwFileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY))!=0) {
            error_="Save path is not an ordinary file";return IoError;
        }
        if(length.QuadPart<=0 || static_cast<std::uint64_t>(length.QuadPart)>maximumBytes) {
            error_="Native save length exceeds the allowed profile size";return InvalidSize;
        }
        size=static_cast<std::size_t>(length.QuadPart);image.resize(size);std::size_t position=0;
        while(position<size) {
            DWORD received=0;
            if(!ReadFile(file,image.data()+position,static_cast<DWORD>(size-position),&received,nullptr)) {
                failure=static_cast<int>(GetLastError());break;
            }
            if(!received){failure=ERROR_HANDLE_EOF;break;}position+=received;
        }
        std::uint8_t extra{};DWORD received=0;
        if(!failure && !ReadFile(file,&extra,1,&received,nullptr))failure=static_cast<int>(GetLastError());
        if(!failure && received)failure=ERROR_INVALID_DATA;
        if(!CloseHandle(file) && !failure)failure=static_cast<int>(GetLastError());
        close.file=INVALID_HANDLE_VALUE;
#else
        const int file=open(path.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC);
        if(file<0)return Fail("Open native save",errno);
        struct Close {int file;~Close(){if(file>=0)::close(file);}} close{file};struct stat info{};
        if(fstat(file,&info))return Fail("Inspect open native save",errno);
        if(!S_ISREG(info.st_mode)){error_="Save path is not an ordinary file";return IoError;}
        if(info.st_size<=0 || static_cast<std::uint64_t>(info.st_size)>maximumBytes) {
            error_="Native save length exceeds the allowed profile size";return InvalidSize;
        }
        size=static_cast<std::size_t>(info.st_size);image.resize(size);std::size_t position=0;
        while(position<size) {
            const auto received=read(file,image.data()+position,size-position);
            if(received<0 && errno==EINTR)continue;
            if(received<=0){failure=received<0?errno:EIO;break;}position+=static_cast<std::size_t>(received);
        }
        if(!failure) {
            std::uint8_t extra{};ssize_t received=0;
            do {received=read(file,&extra,1);}while(received<0 && errno==EINTR);
            if(received!=0)failure=received<0?errno:EIO;
        }
        if(::close(file) && !failure)failure=errno;
        close.file=-1;
#endif
        if(failure)return Fail("Read complete native profile",failure);
        destination=std::move(image);return Ok;
    } catch(const std::exception& e){error_=e.what();return IoError;}
}

cMC::Result cMC::MCRead(const char* name,void* destination,std::size_t bytes)
{
    error_.clear();
    if(!destination || !bytes || bytes>MaximumSaveBytes){error_="Invalid native save buffer size";return InvalidSize;}
    std::vector<std::uint8_t> image;const auto result=MCReadFile(name,image,MaximumSaveBytes);if(result!=Ok)return result;
    if(image.size()!=bytes){error_="Native save length differs from the requested profile version";return InvalidSize;}
    std::memcpy(destination,image.data(),bytes);return Ok;
}

cMC::Result cMC::MCWrite(const char* name,const void* source,std::size_t bytes,bool replaceExisting)
{
    error_.clear();
    if(!source || !bytes || bytes>MaximumSaveBytes){error_="Invalid native save buffer size";return InvalidSize;}
    std::filesystem::path temporary;
    try {
        const auto target=FilePath(name);std::uint64_t available=0;
        const auto info=MCGetInfo(available);if(info!=Ok)return info;
        if(available<bytes){error_="Insufficient native filesystem space";return NoSpace;}
        bool exists=false;const auto status=FileExists(name,exists);
        if(status!=Ok && status!=NotFound)return status;
        if(exists && !replaceExisting){error_="A native profile already exists; migration cannot replace it";return IoError;}
#ifdef _WIN32
        const auto process=static_cast<std::uint64_t>(GetCurrentProcessId());
#else
        const auto process=static_cast<std::uint64_t>(getpid());
#endif
        temporary=root_/("."+std::string(name)+".pending."+std::to_string(process)+"."+std::to_string(nextTemporaryFile.fetch_add(1)));
#ifdef _WIN32
        HANDLE file=CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file==INVALID_HANDLE_VALUE)return Fail("Create atomic save transfer",static_cast<int>(GetLastError()));
        std::size_t position=0;int failure=0;
        while(position<bytes) {
            DWORD written=0;const auto remaining=static_cast<DWORD>(std::min<std::size_t>(bytes-position,MAXDWORD));
            if(!WriteFile(file,static_cast<const std::uint8_t*>(source)+position,remaining,&written,nullptr) || !written) {
                failure=static_cast<int>(GetLastError());if(!failure)failure=ERROR_WRITE_FAULT;break;
            }
            position+=written;
        }
        if(!failure && !FlushFileBuffers(file))failure=static_cast<int>(GetLastError());
        if(!CloseHandle(file) && !failure)failure=static_cast<int>(GetLastError());
        if(!failure && !MoveFileExW(temporary.c_str(),target.c_str(),(replaceExisting?MOVEFILE_REPLACE_EXISTING:0)|MOVEFILE_WRITE_THROUGH))failure=static_cast<int>(GetLastError());
#else
        const int file=open(temporary.c_str(),O_CREAT|O_EXCL|O_WRONLY|O_NOFOLLOW,0600);
        if(file<0)return Fail("Create atomic save transfer",errno);
        std::size_t position=0;int failure=0;
        while(position<bytes) {
            const auto written=write(file,static_cast<const std::uint8_t*>(source)+position,bytes-position);
            if(written<0 && errno==EINTR)continue;
            if(written<=0){failure=written<0 ? errno : EIO;break;}
            position+=static_cast<std::size_t>(written);
        }
        if(!failure && fsync(file))failure=errno;
        if(close(file) && !failure)failure=errno;
        if(!failure) {
            if(replaceExisting) {if(rename(temporary.c_str(),target.c_str()))failure=errno;}
            else {
                // link is an atomic create-if-absent publication on the same
                // filesystem. It cannot overwrite a concurrently created save.
                if(link(temporary.c_str(),target.c_str()))failure=errno;
                else if(unlink(temporary.c_str()))failure=errno;
            }
        }
        if(!failure) {
            const int directory=open(root_.c_str(),O_RDONLY|O_DIRECTORY);
            if(directory<0)failure=errno;
            else {if(fsync(directory))failure=errno;if(close(directory) && !failure)failure=errno;}
        }
#endif
        if(failure){std::error_code ignored;std::filesystem::remove(temporary,ignored);return Fail("Commit atomic native save",failure);}
        error_.clear();return Ok;
    } catch(const std::exception& e) {
        if(!temporary.empty()){std::error_code ignored;std::filesystem::remove(temporary,ignored);}
        error_=e.what();return IoError;
    }
}

cMC::Result cMC::MCDelete(const char* name)
{
    error_.clear();
    try {
        const auto path=FilePath(name);bool exists=false;const auto state=FileExists(name,exists);if(state!=Ok)return state;
        std::error_code ec;const bool removed=std::filesystem::remove(path,ec);
        if(ec)return Fail("Delete selected native save",ec);
        return removed ? Ok : NotFound;
    } catch(const std::exception& e) {error_=e.what();return IoError;}
}

cMC::Result cMC::MCGetDir(std::vector<std::string>& names)
{
    error_.clear();names.clear();std::error_code ec;
    if(!std::filesystem::exists(root_,ec))return ec ? Fail("Inspect save directory",ec) : NotFound;
    std::filesystem::directory_iterator iterator(root_,ec),end;
    if(ec)return Fail("Enumerate native save directory",ec);
    while(iterator!=end) {
        const auto status=iterator->symlink_status(ec);if(ec)return Fail("Inspect native save entry",ec);
        const auto name=iterator->path().filename().u8string();
        if(std::filesystem::is_regular_file(status) && !name.empty() && name[0]!='.')names.push_back(name);
        iterator.increment(ec);if(ec)return Fail("Read next native save entry",ec);
    }
    std::sort(names.begin(),names.end());return Ok;
}
