// Native Windows backing for the actual bOpen/read/service boundary.
// The existing physical range owner is preserved; no PS2 SDK body or layout
// is asserted by these host adaptation methods.
#include "bware/inc/bFile.hpp"
#include "bware/inc/bWare.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <limits>
#include <sstream>

namespace hp2 {

namespace {
void logPhysicalFile(const char* event,const std::filesystem::path& path,
    std::uint64_t offset,std::uint64_t bytes,std::int64_t transferred=-1) noexcept
{
    try {
        const auto name=path.u8string();
        char detail[1536]{};
        std::snprintf(detail,sizeof(detail),"path=\"%.1100s\" offset=%llu bytes=%llu transferred=%lld",
            name.c_str(),static_cast<unsigned long long>(offset),
            static_cast<unsigned long long>(bytes),static_cast<long long>(transferred));
        RecordSessionEvent(event,detail);
    } catch (...) {} // Logging must not alter the physical I/O result.
}
}

RandomAccessFile::RandomAccessFile(std::filesystem::path path) : path_(std::move(path)) {
    Require(std::filesystem::is_regular_file(path_), "Not a regular file: " + path_.u8string());
    size_ = std::filesystem::file_size(path_);
    stream_.open(path_, std::ios::binary);
    Require(stream_.is_open(), "Cannot open: " + path_.u8string());
    logPhysicalFile("file.open",path_,0,size_);
}

void RandomAccessFile::Read(std::uint64_t offset, void* buffer, std::size_t length) const {
    if (!Inside(offset,length,size_))logPhysicalFile("file.read.invalid_range",path_,offset,length);
    Require(Inside(offset, length, size_), "Read beyond end of: " + path_.u8string());
    Require(length <= static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()),
            "Read too large");
    Require(offset <= static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max()),
            "File offset does not fit streamoff");
    if (length == 0) return;
    std::lock_guard<std::mutex> guard(lock_);
    stream_.clear();
    stream_.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    if (!stream_)logPhysicalFile("file.read.seek_failed",path_,offset,length);
    Require(static_cast<bool>(stream_), "Seek failed: " + path_.u8string());
    stream_.read(static_cast<char*>(buffer), static_cast<std::streamsize>(length));
    if (stream_.gcount()!=static_cast<std::streamsize>(length))
        logPhysicalFile("file.read.short",path_,offset,length,stream_.gcount());
    Require(stream_.gcount() == static_cast<std::streamsize>(length),
            "Short read: " + path_.u8string());
}

FileRange::FileRange(std::shared_ptr<RandomAccessFile> file,
                     std::uint64_t offset, std::uint64_t size)
    : file_(std::move(file)), offset_(offset), size_(size) {
    Require(file_ != nullptr && Inside(offset_, size_, file_->Size()),
            "Invalid bounded file range");
}

std::vector<std::uint8_t> FileRange::Read(std::uint64_t offset, std::size_t length) const {
    Require(Valid(), "File range not initialized");
    Require(Inside(offset, length, size_), "Out-of-bounds range read");
    std::vector<std::uint8_t> bytes(length);
    Read(offset, bytes.data(), length);
    return bytes;
}

void FileRange::Read(std::uint64_t offset, void* buffer, std::size_t length) const {
    Require(Valid(), "File range not initialized");
    Require(Inside(offset, length, size_), "Out-of-bounds range read");
    file_->Read(offset_ + offset, buffer, length);
}

FileRange FileRange::Subrange(std::uint64_t offset, std::uint64_t length) const {
    Require(Valid() && Inside(offset, length, size_), "Invalid subrange");
    return FileRange(file_, offset_ + offset, length);
}

void FileRange::SaveTo(const std::filesystem::path& filename) const {
    Require(Valid(), "File range not initialized");
    Require(!std::filesystem::exists(filename) || std::filesystem::is_regular_file(filename),
            "Destination is not a normal file");
    if (filename.has_parent_path()) std::filesystem::create_directories(filename.parent_path());
    // Write to temporary sibling, then replace. Source data is never mutated.
    auto temporary = filename;
    temporary += ".partial";
    Require(!std::filesystem::exists(filename), "Destination already exists: " + filename.u8string());
    Require(!std::filesystem::exists(temporary), "Temporary output already exists: " + temporary.u8string());
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    Require(output.is_open(), "Cannot create destination: " + temporary.u8string());
    try {
        std::array<std::uint8_t, 1 << 20> buffer{};
        for (std::uint64_t pos = 0; pos < size_;) {
            const auto batch = static_cast<std::size_t>(std::min<std::uint64_t>(buffer.size(), size_ - pos));
            Read(pos, buffer.data(), batch);
            output.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(batch));
            Require(static_cast<bool>(output), "Write failed: " + temporary.u8string());
            pos += batch;
        }
        output.close();
        Require(static_cast<bool>(output), "Close failed: " + temporary.u8string());
        // No rename over existing destination: prevents accidental loss of user data.
        Require(!std::filesystem::exists(filename), "Destination already exists: " + filename.u8string());
        std::filesystem::rename(temporary, filename);
    } catch (...) {
        output.close();
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        throw;
    }
}

std::uint64_t FileRange::Fnv1a64() const {
    Require(Valid(), "File range not initialized");
    std::array<std::uint8_t, 1 << 20> buffer{};
    std::uint64_t hash = 14695981039346656037ull;
    for (std::uint64_t pos = 0; pos < size_;) {
        const auto batch = static_cast<std::size_t>(std::min<std::uint64_t>(buffer.size(), size_ - pos));
        Read(pos, buffer.data(), batch);
        for (std::size_t i = 0; i < batch; ++i) {
            hash = (hash ^ buffer[i]) * 1099511628211ull;
        }
        pos += batch;
    }
    return hash;
}

std::string NormalizePath(const std::string& logicalName) {
    Require(!logicalName.empty(), "Empty logical path");
    Require(logicalName[0] != '/' && logicalName[0] != '\\', "Absolute logical path rejected");
    std::vector<std::string> segments;
    std::string segment;
    auto push = [&] {
        if (segment.empty() || segment == ".") { segment.clear(); return; }
        Require(segment != "..", "Parent traversal rejected");
        const auto semicolon = segment.rfind(';');
        if (semicolon != std::string::npos && semicolon + 1 < segment.size()) {
            if (std::all_of(segment.begin() + static_cast<std::ptrdiff_t>(semicolon + 1), segment.end(),
                            [](unsigned char c) { return c >= '0' && c <= '9'; })) {
                segment.erase(semicolon);
            }
        }
        Require(segment.find(':') == std::string::npos, "Drive specifier rejected");
        for (char& c : segment) {
            if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
        }
        segments.push_back(segment);
        segment.clear();
    };
    for (const unsigned char c : logicalName) {
        Require(c >= 0x20 && c != 0x7F, "Invalid control character in logical path");
        if (c == '/' || c == '\\') push();
        else segment.push_back(static_cast<char>(c));
    }
    push();
    Require(!segments.empty(), "Empty normalized logical path");
    std::string result;
    for (const auto& part : segments) {
        if (!result.empty()) result += '/';
        result += part;
    }
    return result;
}

std::uint32_t PathHash(const std::string& logicalName) {
    const auto logicalPath = NormalizePath(logicalName);
    std::string archiveName = logicalPath;
    for (char& character : archiveName)
        if (character == '/') character = '\\';
    // File names are normalized before hashing.
    return hashString(archiveName.c_str());
}

} // namespace hp2

#include <chrono>
#include <future>
#include <list>

namespace hp2 {

class FileReadOperation final {
    friend class FileIoRuntime;
    FileIoRuntime* owner{};
    FileReadState state{FileReadState::Pending};
    std::future<void> transfer;
};

struct FileIoRuntime::Storage {
    std::list<std::shared_ptr<FileReadOperation>> ordered;
};

FileIoRuntime::FileIoRuntime():storage_(std::make_unique<Storage>()) {}

FileIoRuntime::~FileIoRuntime()
{
    while (!storage_->ordered.empty()) {
        const auto operation=storage_->ordered.front();Wait(operation);
    }
}

std::shared_ptr<FileReadOperation> FileIoRuntime::Read(const FileRange& range,
    std::uint64_t offset,void* destination,std::size_t bytes)
{
    Require(range.Valid() && (destination || !bytes) && Inside(offset,bytes,range.Size()),
        "Physical async read has an invalid owner, destination or file extent");
    auto operation=std::make_shared<FileReadOperation>();operation->owner=this;
    // This worker performs real physical I/O. Completion is published only
    // after future::get synchronizes the actual destination writes.
    operation->transfer=std::async(std::launch::async,[range,offset,destination,bytes] {
        range.Read(offset,destination,bytes);
    });
    storage_->ordered.push_back(operation);
    return operation;
}

void FileIoRuntime::Service()
{
    while (!storage_->ordered.empty()) {
        auto& operation=*storage_->ordered.front();
        if (operation.transfer.wait_for(std::chrono::seconds(0))!=std::future_status::ready)break;
        try {operation.transfer.get();operation.state=FileReadState::Complete;}
        catch (...) {operation.state=FileReadState::Failed;}
        storage_->ordered.pop_front();
    }
}

void FileIoRuntime::Wait(const std::shared_ptr<FileReadOperation>& operation)
{
    Require(operation && operation->owner==this,"Async file operation belongs to another SDK owner");
    while (operation->state==FileReadState::Pending) {
        Require(!storage_->ordered.empty(),"Pending physical read has no scheduled SDK job");
        storage_->ordered.front()->transfer.wait();Service();
    }
}

FileReadState FileIoRuntime::State(const std::shared_ptr<FileReadOperation>& operation) const
{
    Require(operation && operation->owner==this,"Async file operation has no matching SDK owner");
    return operation->state;
}

bool FileIoRuntime::Empty() const noexcept {return storage_->ordered.empty();}

} // namespace hp2
