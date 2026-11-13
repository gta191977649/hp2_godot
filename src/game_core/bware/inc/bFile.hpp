#ifndef NFSHP2_BWARE_INC_BFILE_HPP
#define NFSHP2_BWARE_INC_BFILE_HPP

#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>

struct ReplayIntegerLog;

// Native Windows physical-file/directory types retain one existing owner.
// This observed bWare header arrangement asserts no beta packed layout or
// original inline placement. Original bFile/bFileE SDK methods are separate.
namespace hp2 {

class RandomAccessFile final {
public:
    explicit RandomAccessFile(std::filesystem::path path);
    std::uint64_t Size() const noexcept { return size_; }
    const std::filesystem::path& Path() const noexcept { return path_; }
    void Read(std::uint64_t offset, void* buffer, std::size_t length) const;

private:
    std::filesystem::path path_;
    std::uint64_t size_ = 0;
    mutable std::ifstream stream_;
    mutable std::mutex lock_;
};

class FileRange final {
public:
    FileRange() = default;
    FileRange(std::shared_ptr<RandomAccessFile> file, std::uint64_t offset, std::uint64_t size);
    std::uint64_t Size() const noexcept { return size_; }
    bool Valid() const noexcept { return static_cast<bool>(file_); }
    std::vector<std::uint8_t> Read(std::uint64_t offset, std::size_t length) const;
    void Read(std::uint64_t offset, void* buffer, std::size_t length) const;
    FileRange Subrange(std::uint64_t offset, std::uint64_t length) const;
    void SaveTo(const std::filesystem::path& filename) const;
    std::uint64_t Fnv1a64() const;

private:
    std::shared_ptr<RandomAccessFile> file_;
    std::uint64_t offset_ = 0;
    std::uint64_t size_ = 0;
};

std::string NormalizePath(const std::string& logicalName);
std::uint32_t PathHash(const std::string& logicalName);

struct DiscEntry {
    std::string path;
    FileRange range;
    std::uint64_t discSector = 0; // meaningful only for ISO source
};

class GameFileSystem final {
public:
    explicit GameFileSystem(const std::filesystem::path& source);
    GameFileSystem(const std::filesystem::path& index, const std::filesystem::path& data);
    bool IsImage() const noexcept { return isImage_; }
    const std::string& VolumeName() const noexcept { return volumeName_; }
    const std::map<std::string, DiscEntry>& Files() const noexcept { return files_; }
    const DiscEntry& Get(const std::string& logicalName) const;
    bool Contains(const std::string& logicalName) const;
    std::string DiscBootExecutable() const;

private:
    void LoadIso(const std::filesystem::path& image);
    void LoadDirectory(const std::filesystem::path& root);
    bool isImage_ = false;
    std::string volumeName_;
    std::map<std::string, DiscEntry> files_;
};

struct ArchiveEntry {
    std::size_t index = 0;
    std::uint32_t nameHash = 0;
    std::uint32_t sectorOffset = 0;
    std::uint32_t length = 0;
    std::vector<std::string> knownNames;
};

struct CompHeader {
    std::uint32_t version = 0;
    std::uint32_t uncompressedBytes = 0;
    std::uint32_t compressedBytes = 0;
};

class ZDirArchive final {
public:
    explicit ZDirArchive(const GameFileSystem& disc);
    const std::vector<ArchiveEntry>& Entries() const noexcept { return entries_; }
    const std::vector<std::size_t>& Find(const std::string& archivePath) const;
    const std::vector<std::size_t>& FindHash(std::uint32_t hash) const;
    FileRange Get(std::size_t index) const;
    FileRange GetUnique(const std::string& archivePath) const;
    bool CompInfo(std::size_t index, CompHeader& out) const;
    std::size_t ValidatePayloadHeaders() const;

private:
    FileRange zzdata_;
    std::vector<ArchiveEntry> entries_;
    std::map<std::uint32_t, std::vector<std::size_t>> indexByHash_;
    mutable std::mutex verificationMutex_;
    mutable std::set<std::uint32_t> verifiedDuplicateHashes_;
};

// A file range preserving its source encoding and length.
struct PhysicalResource {
    FileRange range;
    bool expanded = false; // true ONLY for extracted .BUN substituting .LZC
};

// A physical source selection, not a second game-resource owner. Auto accepts
// loose files, a disc directory/image, or an adjacent ZDIR/ZZDATA pair. Explicit
// ArchiveData also accepts a renamed data binary and still requires its index.
enum class GameResourceSource { Auto, ArchiveData, LooseDirectory };

class GameResources final {
public:
    explicit GameResources(const std::filesystem::path& input,
                           GameResourceSource source = GameResourceSource::Auto);
    const std::filesystem::path& SourcePath() const noexcept { return sourcePath_; }
    bool IsLooseArchive() const noexcept { return !disc_; }
    PhysicalResource OpenPhysical(const std::string& logicalName) const;
    std::optional<PhysicalResource> TryOpenPhysical(const std::string& logicalName) const;

    // Decoding is explicit; normal file reads preserve compressed data.
    std::vector<std::uint8_t> Load(const std::string& logicalName) const;

private:
    std::filesystem::path sourcePath_;
    std::filesystem::path looseRoot_;
    // Optional original extraction index; only physical-file metadata, not a
    // second game resource owner. Unknown names resolve through its hashes.
    std::filesystem::path looseIndexPath_;
    std::map<std::uint32_t, std::uint32_t> looseLengths_;
    std::set<std::uint32_t> looseAmbiguousHashes_;
    std::unique_ptr<GameFileSystem> disc_;
    std::unique_ptr<ZDirArchive> archive_;
    void LoadLooseIndex();
};

// Windows transport for the actual asynchronous file-service boundary.
// These handles own physical reads, not game completion/loaded flags.
enum class FileReadState { Pending, Complete, Failed };
class FileReadOperation;
class FileIoRuntime final {
public:
    FileIoRuntime();
    ~FileIoRuntime();
    FileIoRuntime(const FileIoRuntime&)=delete;
    FileIoRuntime& operator=(const FileIoRuntime&)=delete;
    std::shared_ptr<FileReadOperation> Read(const FileRange& range,std::uint64_t offset,
        void* destination,std::size_t bytes);
    void Service();
    void Wait(const std::shared_ptr<FileReadOperation>& operation);
    FileReadState State(const std::shared_ptr<FileReadOperation>& operation) const;
    bool Empty() const noexcept;
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};

class ResourceFile;
class TexturePack;
enum class QueuedFileState : std::int32_t { Waiting=0,Reading=1,Complete=2,Failed=3 };
enum class QueuedFileCompletionKind { None,ResourceFileTransfer,TextureRegion };
struct QueuedFileCompletion {
    QueuedFileCompletionKind kind{QueuedFileCompletionKind::None};
    ResourceFile* resourceFile{};
    TexturePack* texturePack{};
};

// One retained request; accessors borrow this exact object until retirement.
// Native arrangement only, with no overlay of the original100-byte record.
class QueuedFile final {
public:
    std::int32_t Id() const noexcept {return id_;}
    const std::string& Filename() const noexcept {return filename_;}
    QueuedFileState State() const noexcept {return state_;}
    std::int32_t BytesScheduled() const noexcept {return scheduled_;}
    std::int32_t TotalBytes() const noexcept {return total_;}
private:
    friend class QueuedFileService;
    std::uint8_t* destination_{};
    std::string filename_;
    std::int32_t offset_{},total_{},scheduled_{};
    std::int32_t chunkBytes_{0x100000},priority_{5},id_{};
    QueuedFileState state_{QueuedFileState::Waiting};
    bool abortRequested_{};
    QueuedFileCompletion completion_;
    std::shared_ptr<FileReadOperation> operation_;
};

class QueuedFileService final {
public:
    QueuedFileService(const GameResources& files,FileIoRuntime& io,::ReplayIntegerLog& log);
    ~QueuedFileService();
    QueuedFileService(const QueuedFileService&)=delete;
    QueuedFileService& operator=(const QueuedFileService&)=delete;
    std::int32_t Add(void* destination,const char* filename,std::int32_t offset,
        std::int32_t bytes,QueuedFileCompletion completion);
    void Service();
    bool Abort(std::int32_t identifier);
    // Join any issued write without dispatching the revoked owner callback.
    void CancelAndWait(std::int32_t identifier);
    void BeginShutdown() noexcept;
    std::exception_ptr Shutdown() noexcept;
    void ChangeParams(std::int32_t identifier,std::int32_t priority,std::int32_t chunkBytes);
    const QueuedFile* Find(std::int32_t identifier) const noexcept;
    bool Empty() const noexcept;
    const GameResources& Files() const noexcept;
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
    void BeginRead(QueuedFile& request);
    void ObserveRead(QueuedFile& request);
    void Retire(QueuedFile& request,bool notify,bool failed);
};

} // namespace hp2
#endif
