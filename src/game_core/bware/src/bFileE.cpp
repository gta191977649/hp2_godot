// Native mounted-directory/physical-resource adaptation for bFileE.
// One existing disc/archive owner is retained; source SDK CD/IOP routines
// are not declared complete by the Windows implementation.

#include "bware/inc/bFile.hpp"
#include "bware/inc/bWare.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <set>
#include <sstream>

namespace hp2 {
namespace {
constexpr std::uint64_t kSectorSize = 2048;
constexpr std::uint64_t kPrimaryVolumeSector = 16;

void logResourceSource(const char* event,const std::string& logical,
    const std::filesystem::path& physical,std::uint32_t hash,std::uint64_t bytes,
    bool expanded,std::int64_t entry=-1,std::uint64_t archiveOffset=0) noexcept
{
    try {
        const auto path=physical.u8string();
        char detail[1792]{};
        std::snprintf(detail,sizeof(detail),
            "logical=\"%.500s\" source=\"%.950s\" hash=%08X bytes=%llu encoding=%s archive_entry=%lld archive_offset=%llu",
            logical.c_str(),path.c_str(),static_cast<unsigned>(hash),
            static_cast<unsigned long long>(bytes),expanded ? "expanded" : "original",
            static_cast<long long>(entry),static_cast<unsigned long long>(archiveOffset));
        RecordSessionEvent(event,detail);
    } catch (...) {}
}

std::string TrimRight(std::string s) {
    const auto last = s.find_last_not_of(std::string(" \0", 2));
    s.erase(last == std::string::npos ? 0 : last + 1);
    return s;
}

class IsoReader final {
public:
    explicit IsoReader(std::shared_ptr<RandomAccessFile> image,
                       std::map<std::string, DiscEntry>& files)
        : image_(std::move(image)), files_(files) {
        Require(image_->Size() >= 17 * kSectorSize, "ISO too small for primary volume descriptor");
        const auto pvd = Sector(kPrimaryVolumeSector);
        Require(pvd[0] == 1 && std::memcmp(pvd.data() + 1, "CD001", 5) == 0 && pvd[6] == 1,
                "Unsupported image: ISO9660 primary volume descriptor not found");
        const auto volumeSectors = ReadU32(pvd.data() + 80);
        Require(volumeSectors > kPrimaryVolumeSector &&
                Inside(0, static_cast<std::uint64_t>(volumeSectors) * kSectorSize, image_->Size()),
                "Invalid ISO volume size");
        volumeSize_ = static_cast<std::uint64_t>(volumeSectors) * kSectorSize;
        Require(ReadU16(pvd.data() + 128) == kSectorSize,
                "Unsupported ISO logical block size (expected 2048)");
        volumeName_ = TrimRight(std::string(reinterpret_cast<const char*>(pvd.data() + 40), 32));
        // ISO9660 root directory record is embedded at PVD offset 156.
        const auto rootSize = ReadU32(pvd.data() + 156 + 10);
        const auto rootSector = ReadU32(pvd.data() + 156 + 2);
        VisitDirectory(rootSector, rootSize, "", 0);
    }

    const std::string& VolumeName() const noexcept { return volumeName_; }

private:
    std::array<std::uint8_t, kSectorSize> Sector(std::uint64_t number) const {
        std::array<std::uint8_t, kSectorSize> sector{};
        image_->Read(number * kSectorSize, sector.data(), sector.size());
        return sector;
    }

    void VisitDirectory(std::uint32_t lba, std::uint32_t length,
                        const std::string& prefix, unsigned depth) {
        Require(depth < 20, "Excessive ISO directory depth");
        Require(Inside(static_cast<std::uint64_t>(lba) * kSectorSize, length, volumeSize_),
                "Invalid ISO directory extent");
        Require(length <= (64u << 20), "Unreasonably large ISO directory");
        Require(visitedDirs_.insert({lba, length}).second, "Repeated ISO directory extent");
        // Read complete directory; this disc's directories are small, but multi-sector is supported.
        std::vector<std::uint8_t> directory(length);
        image_->Read(static_cast<std::uint64_t>(lba) * kSectorSize, directory.data(), directory.size());
        for (std::size_t pos = 0; pos < directory.size();) {
            const auto recordLength = directory[pos];
            if (recordLength == 0) {
                pos = ((pos / kSectorSize) + 1) * kSectorSize;
                continue;
            }
            Require(recordLength >= 34 && recordLength <= directory.size() - pos,
                    "Invalid ISO directory record length");
            const auto* record = directory.data() + pos;
            const auto nameLength = record[32];
            Require(static_cast<unsigned>(nameLength) + 33 <= recordLength,
                    "ISO directory name exceeds record length");
            if (nameLength == 1 && (record[33] == 0 || record[33] == 1)) {
                pos += recordLength;
                continue;
            }
            const std::string rawName(reinterpret_cast<const char*>(record + 33), nameLength);
            const std::string name = NormalizePath(rawName);
            const std::string path = prefix.empty() ? name : prefix + "/" + name;
            const auto extent = ReadU32(record + 2);
            const auto fileSize = ReadU32(record + 10);
            Require(Inside(static_cast<std::uint64_t>(extent) * kSectorSize, fileSize, volumeSize_),
                    "ISO file extent out of bounds: " + path);
            Require((record[25] & 0x80u) == 0, "ISO multi-extent file not yet supported: " + path);
            if ((record[25] & 2u) != 0) {
                VisitDirectory(extent, fileSize, path, depth + 1);
            } else {
                Require(files_.find(path) == files_.end(), "Duplicate ISO path: " + path);
                files_.emplace(path, DiscEntry{path,
                    FileRange(image_, static_cast<std::uint64_t>(extent) * kSectorSize, fileSize),
                    extent});
            }
            pos += recordLength;
        }
    }

    std::shared_ptr<RandomAccessFile> image_;
    std::map<std::string, DiscEntry>& files_;
    std::string volumeName_;
    std::uint64_t volumeSize_ = 0;
    std::set<std::pair<std::uint32_t, std::uint32_t>> visitedDirs_;
};
} // namespace

GameFileSystem::GameFileSystem(const std::filesystem::path& source) {
    Require(std::filesystem::exists(source), "Data source does not exist: " + source.u8string());
    if (std::filesystem::is_directory(source)) LoadDirectory(source);
    else LoadIso(source);
    Require(Contains("NFSHP2/ZDIR.BIN") && Contains("NFSHP2/ZZDATA.BIN"),
            "Not an HP2 PS2 disc tree: NFSHP2/ZDIR.BIN or ZZDATA.BIN missing");
}

GameFileSystem::GameFileSystem(const std::filesystem::path& index,
                               const std::filesystem::path& data) {
    Require(std::filesystem::is_regular_file(index),
            "Archive index is not a regular file: " + index.u8string());
    Require(std::filesystem::is_regular_file(data),
            "Archive data is not a regular file: " + data.u8string());
    Require(!std::filesystem::equivalent(index, data),
            "Archive index and data refer to the same file: " + data.u8string());
    isImage_ = false;
    volumeName_ = data.u8string() + " (index " + index.u8string() + ")";
    const auto indexFile = std::make_shared<RandomAccessFile>(index);
    const auto dataFile = std::make_shared<RandomAccessFile>(data);
    files_.emplace("NFSHP2/ZDIR.BIN", DiscEntry{"NFSHP2/ZDIR.BIN",
        FileRange(indexFile, 0, indexFile->Size()), 0});
    files_.emplace("NFSHP2/ZZDATA.BIN", DiscEntry{"NFSHP2/ZZDATA.BIN",
        FileRange(dataFile, 0, dataFile->Size()), 0});
}

void GameFileSystem::LoadIso(const std::filesystem::path& image) {
    isImage_ = true;
    const auto file = std::make_shared<RandomAccessFile>(image);
    IsoReader reader(file, files_);
    volumeName_ = reader.VolumeName();
}

void GameFileSystem::LoadDirectory(const std::filesystem::path& root) {
    isImage_ = false;
    volumeName_ = "DIRECTORY";
    for (std::filesystem::recursive_directory_iterator it(root), end; it != end; ++it) {
        if (it->is_symlink()) { it.disable_recursion_pending(); continue; }
        if (!it->is_regular_file()) continue;
        const auto relative = std::filesystem::relative(it->path(), root);
        const std::string path = NormalizePath(relative.generic_string());
        Require(files_.find(path) == files_.end(), "Case-insensitive duplicate path: " + path);
        const auto file = std::make_shared<RandomAccessFile>(it->path());
        files_.emplace(path, DiscEntry{path, FileRange(file, 0, file->Size()), 0});
    }
}

const DiscEntry& GameFileSystem::Get(const std::string& logicalName) const {
    const auto path = NormalizePath(logicalName);
    const auto it = files_.find(path);
    Require(it != files_.end(), "Missing disc file: " + path);
    return it->second;
}

bool GameFileSystem::Contains(const std::string& logicalName) const {
    return files_.find(NormalizePath(logicalName)) != files_.end();
}

std::string GameFileSystem::DiscBootExecutable() const {
    if (!Contains("SYSTEM.CNF")) return {};
    const auto& range = Get("SYSTEM.CNF").range;
    Require(range.Size() <= 4096, "SYSTEM.CNF unexpectedly large");
    const auto bytes = range.Read(0, static_cast<std::size_t>(range.Size()));
    const std::string configuration(bytes.begin(), bytes.end());
    const auto boot = configuration.find("BOOT2");
    if (boot == std::string::npos) return {};
    const auto slash = configuration.find_last_of("\\/", configuration.find(';', boot));
    if (slash == std::string::npos) return {};
    const auto end = configuration.find_first_of(";\r\n", slash + 1);
    if (end == std::string::npos) return {};
    return NormalizePath(configuration.substr(slash + 1, end - slash - 1));
}

} // namespace hp2

#include "bware/inc/bFile.hpp"
#include "bware/inc/bWare.hpp"

#include <algorithm>
#include <cstring>
#include <map>
#include <cstdio>
#include <set>
#include <sstream>

namespace hp2 {
namespace {
constexpr std::uint64_t kSector = 2048;

// Candidate names are native physical-index discovery only. A name is
// retained below only when its original hash is actually in this ZDIR.
std::vector<std::string> DiscoverCandidateNames() {
    std::vector<std::string> names{
        "GLOBAL\\GLOBALA.BUN", "GLOBAL\\GLOBALB.BUN", "GLOBAL\\GLOBALB.LZC",
        "GLOBAL\\INGAMEA.BUN", "GLOBAL\\INGAMEB.BUN", "GLOBAL\\INGAMEB.LZC",
        "GLOBAL\\DYNTEX.BIN", "GLOBAL\\HUDTEX.BIN", "GLOBAL\\DRIVTEX.BIN",
        "GLOBAL\\PRERACEANIM.LZC", "CARS\\TEXTURES.BIN",
        "FRONTEND\\FRONTA.BUN", "FRONTEND\\FRONTB.BUN",
        "FRONTEND\\FRONTB.LZC", "FRONTEND\\TSELECT.BIN",
        "LANGUAGES\\FE_ENGLISH.BIN", "LANGUAGES\\FE_FRENCH.BIN",
        "LANGUAGES\\FE_GERMAN.BIN", "LANGUAGES\\FE_ITALIAN.BIN",
        "LANGUAGES\\FE_SPANISH.BIN", "LANGUAGES\\FE_SWEDISH.BIN",
        "LANGUAGES\\FE_LONGEST.BIN", "LANGUAGES\\FE_ID.BIN",
        "SOUND\\ENGINE.CTB", "SOUND\\ENGINE.LTB", "SOUND\\JUKEBOXM.JBC",
        "SOUND\\SPEECH\\EVENTS.EVT", "NFS6.ICO", "PAL.TAG",
        "MOVIES\\HOTPURSUIT_MOVIE.AVI", "MOVIES\\MAINMENU_MOVIE.AVI",
        "MOVIES\\OPTIONS_MOVIE.AVI", "MOVIES\\OPTIONS_MOVIE_NEW.AVI",
        "MOVIES\\INTRO\\BBGLOGO_EN.AVI", "MOVIES\\INTRO\\MAKEOF_EN.AVI",
    };
    for (const char* language : {"EN", "FR", "GE", "IT", "SP", "SW"}) {
        names.emplace_back(std::string("MOVIES\\INTRO\\EALOGO_") + language + ".AVI");
    }
    for (const char* language : {"EN", "FR"}) {
        names.emplace_back(std::string("MOVIES\\INTRO\\NFSINTRO_") + language + ".AVI");
        names.emplace_back(std::string("MOVIES\\INTRO\\REWARD_") + language + ".AVI");
    }
    // Original ELF uses decimal track IDs (e.g. TRACKA11, TRACKB12).
    for (int track = 0; track < 100; ++track) {
        for (const char* pattern : {
                "TRACKS\\TRACKA%d.BUN", "TRACKS\\TRACKB%d.BUN", "TRACKS\\TRACKB%d.LZC",
                "TRACKS\\TEX%dLOCATION.BIN", "TRACKS\\TEX%dTRACK.BIN",
                "MARKERS\\MARKERS%d.BIN"}) {
            char value[96]{};
            std::snprintf(value, sizeof(value), pattern, track);
            names.emplace_back(value);
        }
    }
    return names;
}
}

ZDirArchive::ZDirArchive(const GameFileSystem& disc)
    : zzdata_(disc.Get("NFSHP2/ZZDATA.BIN").range) {
    const auto& index = disc.Get("NFSHP2/ZDIR.BIN").range;
    Require(index.Size() > 0 && index.Size() <= (32u << 20) && index.Size() % 12 == 0,
            "Invalid HP2 old-ZDIR index length " + std::to_string(index.Size()) +
            " (expected nonempty 12-byte records): " + disc.VolumeName());
    const auto indexBytes = index.Read(0, static_cast<std::size_t>(index.Size()));
    entries_.reserve(indexBytes.size() / 12);
    for (std::size_t offset = 0; offset < indexBytes.size(); offset += 12) {
        const auto hash = ReadU32(indexBytes.data() + offset);
        const auto sector = ReadU32(indexBytes.data() + offset + 4);
        const auto length = ReadU32(indexBytes.data() + offset + 8);
        const auto byteOffset = static_cast<std::uint64_t>(sector) * kSector;
        Require(Inside(byteOffset, length, zzdata_.Size()),
                "Truncated archive: ZDIR entry " + std::to_string(offset / 12) +
                " requires " + std::to_string(length) + " bytes at offset " +
                std::to_string(byteOffset) + ", but data size is " +
                std::to_string(zzdata_.Size()) + ": " + disc.VolumeName());
        const auto idx = entries_.size();
        entries_.push_back(ArchiveEntry{idx, hash, sector, length, {}});
        indexByHash_[hash].push_back(idx);
    }
    // File hashes are case-insensitive 33*x + ASCII, seeded with 0xffffffff.
    // Keep every matching name because hashes may collide and the index has aliases.
    for (const auto& name : DiscoverCandidateNames()) {
        const auto found = indexByHash_.find(PathHash(name));
        if (found == indexByHash_.end()) continue;
        const auto normalized = NormalizePath(name);
        for (const auto id : found->second) {
            auto& names = entries_[id].knownNames;
            if (std::find(names.begin(), names.end(), normalized) == names.end()) {
                names.push_back(normalized);
            }
        }
    }
}

const std::vector<std::size_t>& ZDirArchive::FindHash(std::uint32_t hash) const {
    static const std::vector<std::size_t> missing;
    const auto it = indexByHash_.find(hash);
    return it == indexByHash_.end() ? missing : it->second;
}

const std::vector<std::size_t>& ZDirArchive::Find(const std::string& archivePath) const {
    return FindHash(PathHash(archivePath));
}

FileRange ZDirArchive::Get(std::size_t index) const {
    Require(index < entries_.size(), "ZDIR index out of range");
    const auto& e = entries_[index];
    return zzdata_.Subrange(static_cast<std::uint64_t>(e.sectorOffset) * kSector, e.length);
}

FileRange ZDirArchive::GetUnique(const std::string& archivePath) const {
    const auto& matches = Find(archivePath);
    Require(!matches.empty(), "Resource hash not found for " + archivePath);
    const auto& first = entries_[matches.front()];
    const auto primary = Get(matches.front());
    if (matches.size() == 1) return primary;
    // A duplicate pair is immutable while the image is mounted. Verify each
    // distinct physical extent once; later opens must not read megabytes
    // again just to establish that identical archive aliases match.
    std::lock_guard<std::mutex> verify(verificationMutex_);
    if (verifiedDuplicateHashes_.count(first.nameHash)) return primary;
    // The disc archive contains two byte-identical copies of the entire
    // CARS/TEXTURES.BIN pack at different physical positions. A hash collision
    // must still fail, but duplicated content is a valid original resource.
    // Compare bounded blocks; never allocate both potentially huge ranges.
    constexpr std::size_t blockSize = 1024 * 1024;
    for (const auto id : matches) {
        const auto& entry = entries_[id];
        Require(entry.length == first.length,
                "Ambiguous resource hash (different lengths): " + archivePath);
        if (entry.sectorOffset == first.sectorOffset) continue;
        const auto candidate = Get(id);
        for (std::uint64_t offset = 0; offset < primary.Size(); offset += blockSize) {
            const auto length = static_cast<std::size_t>(std::min<std::uint64_t>(blockSize,primary.Size()-offset));
            Require(primary.Read(offset,length) == candidate.Read(offset,length),
                    "Ambiguous resource hash (different contents): " + archivePath);
        }
    }
    verifiedDuplicateHashes_.insert(first.nameHash);
    return primary;
}

bool ZDirArchive::CompInfo(std::size_t index, CompHeader& out) const {
    const auto& e = entries_.at(index);
    if (e.length < 16) return false;
    const auto bytes = Get(index).Read(0, 16);
    if (std::memcmp(bytes.data(), "COMP", 4) != 0) return false;
    out.version = ReadU32(bytes.data() + 4);
    out.uncompressedBytes = ReadU32(bytes.data() + 8);
    out.compressedBytes = ReadU32(bytes.data() + 12);
    Require(out.version == 0x1001 && out.compressedBytes == e.length &&
                out.uncompressedBytes > 0 && out.uncompressedBytes <= (128u << 20),
            "Unexpected COMP header at ZDIR index " + std::to_string(index));
    return true;
}

std::size_t ZDirArchive::ValidatePayloadHeaders() const {
    std::size_t compressed = 0;
    for (const auto& entry : entries_) {
        if (entry.length == 0) continue;
        const auto resource = Get(entry.index);
        const auto count = static_cast<std::size_t>(std::min<std::uint64_t>(resource.Size(), 32));
        (void)resource.Read(0, count);
        if (resource.Size() >= 32) (void)resource.Read(resource.Size() - 32, 32);
        CompHeader header{};
        if (CompInfo(entry.index, header)) ++compressed;
    }
    return compressed;
}

} // namespace hp2

#include "bware/inc/bFile.hpp"
#include "bware/inc/bChunk.hpp"
#include "bware/inc/bWare.hpp"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

namespace hp2 {
namespace {
namespace fs = std::filesystem;

std::string UpperAscii(const std::string& input) {
    std::string output = input;
    for (auto& ch : output) {
        const auto value = static_cast<unsigned char>(ch);
        if (value >= 'a' && value <= 'z') ch = static_cast<char>(value - 'a' + 'A');
    }
    return output;
}

fs::path LoosePath(const fs::path& root, const std::string& logicalName) {
    const auto normalized = NormalizePath(logicalName);
    fs::path result = root;
    auto begin = std::size_t{0};
    while (begin < normalized.size()) {
        const auto slash = normalized.find('/', begin);
        const auto end = slash == std::string::npos ? normalized.size() : slash;
        const auto component = normalized.substr(begin, end - begin);
        fs::path found;
        if (!fs::is_directory(result)) return {};
        for (const auto& entry : fs::directory_iterator(result)) {
            if (UpperAscii(entry.path().filename().u8string()) != component) continue;
            Require(!fs::is_symlink(entry.symlink_status()),
                    "Resource path is a symbolic link: " + entry.path().u8string());
            Require(found.empty(), "Case-insensitive duplicate resource path: " +
                    result.u8string() + "/" + component);
            found = entry.path();
        }
        if (found.empty()) return {};
        result = std::move(found);
        begin = end + 1;
    }
    return result;
}

std::vector<std::uint8_t> LoadDecoded(const FileRange& file) {
    Require(file.Size() <= (512ull << 20), "Decoded resource input exceeds 512 MiB");
    if (file.Size() >= 4 && file.Read(0, 4) ==
        std::vector<std::uint8_t>({'C','O','M','P'})) return DecodeComp(file);
    return file.Read(0, static_cast<std::size_t>(file.Size()));
}
} // namespace

GameResources::GameResources(const fs::path& input, GameResourceSource source) {
    Require(!input.empty(), "No game resource source was selected");
    sourcePath_ = fs::absolute(input).lexically_normal();
    Require(fs::exists(sourcePath_), "Game resource source does not exist: " + sourcePath_.u8string());
    if (source == GameResourceSource::LooseDirectory) {
        Require(fs::is_directory(sourcePath_),
                "Loose resource source is not a directory: " + sourcePath_.u8string());
        looseRoot_ = sourcePath_;
        LoadLooseIndex();
        return;
    }
    if (source == GameResourceSource::ArchiveData) {
        Require(fs::is_regular_file(sourcePath_),
                "Archive data source must be a file: " + sourcePath_.u8string());
        const auto index = LoosePath(sourcePath_.parent_path(), "ZDIR.BIN");
        Require(!index.empty(), "ZDIR.BIN is required beside the archive data: " + sourcePath_.u8string());
        disc_ = std::make_unique<GameFileSystem>(index, sourcePath_);
    } else if (fs::is_regular_file(sourcePath_)) {
        const auto name = UpperAscii(sourcePath_.filename().u8string());
        if (name == "ZZDATA.BIN") {
            const auto index = LoosePath(sourcePath_.parent_path(), "ZDIR.BIN");
            Require(!index.empty(), "ZDIR.BIN is required beside the archive data: " + sourcePath_.u8string());
            disc_ = std::make_unique<GameFileSystem>(index, sourcePath_);
        } else if (name == "ZDIR.BIN") {
            const auto data = LoosePath(sourcePath_.parent_path(), "ZZDATA.BIN");
            Require(!data.empty(), "ZZDATA.BIN is required beside the archive index: " + sourcePath_.u8string());
            disc_ = std::make_unique<GameFileSystem>(sourcePath_, data);
        } else {
            disc_ = std::make_unique<GameFileSystem>(sourcePath_);
        }
    } else {
        Require(fs::is_directory(sourcePath_),
                "Game resource source is neither a file nor a directory: " + sourcePath_.u8string());
        auto index = LoosePath(sourcePath_, "ZDIR.BIN");
        auto data = LoosePath(sourcePath_, "ZZDATA.BIN");
        if (data.empty()) {
            index = LoosePath(sourcePath_, "NFSHP2/ZDIR.BIN");
            data = LoosePath(sourcePath_, "NFSHP2/ZZDATA.BIN");
        }
        if (!data.empty()) {
            Require(!index.empty(), "Archive data has no adjacent ZDIR.BIN: " + data.u8string());
            // The game needs the two physical archive owners, not handles for
            // every file under the chosen project/save/extraction directory.
            disc_ = std::make_unique<GameFileSystem>(index, data);
        } else {
            looseRoot_ = sourcePath_;
        }
    }
    if (disc_) archive_ = std::make_unique<ZDirArchive>(*disc_);
    else LoadLooseIndex();
}

void GameResources::LoadLooseIndex() {
    looseIndexPath_ = LoosePath(looseRoot_, "__archive/ZDIR.BIN");
    if (looseIndexPath_.empty()) return;
    const auto index = std::make_shared<RandomAccessFile>(looseIndexPath_);
    Require(index->Size() > 0 && index->Size() <= (32u << 20) && index->Size() % 12 == 0,
            "Invalid extracted old-ZDIR index length " + std::to_string(index->Size()) +
            " (expected nonempty 12-byte records): " + looseIndexPath_.u8string());
    const auto bytes = FileRange(index, 0, index->Size()).Read(0,
        static_cast<std::size_t>(index->Size()));
    for (std::size_t offset = 0; offset < bytes.size(); offset += 12) {
        const auto hash = ReadU32(bytes.data() + offset);
        const auto size = ReadU32(bytes.data() + offset + 8);
        const auto inserted = looseLengths_.emplace(hash, size);
        if (!inserted.second && inserted.first->second != size)
            looseAmbiguousHashes_.insert(hash);
    }
    // The extractor compares every original duplicate extent before publishing
    // the directory. Equal-content aliases share one physical file; conflicting
    // payloads are retained separately and their hashes must never be selected.
    const auto markerPath = LoosePath(looseRoot_, "__archive/AMBIGUOUS_HASHES.BIN");
    if (markerPath.empty()) return;
    const auto marker = std::make_shared<RandomAccessFile>(markerPath);
    Require(marker->Size() % 4 == 0 && marker->Size() <= looseLengths_.size() * 4ull,
            "Invalid extracted ambiguity table length: " + markerPath.u8string());
    const auto hashes = FileRange(marker, 0, marker->Size()).Read(0,
        static_cast<std::size_t>(marker->Size()));
    std::uint32_t previous = 0;
    for (std::size_t offset = 0; offset < hashes.size(); offset += 4) {
        const auto hash = ReadU32(hashes.data() + offset);
        Require((offset == 0 || hash > previous) && looseLengths_.count(hash) != 0,
                "Invalid extracted ambiguity table hash/order: " + markerPath.u8string());
        looseAmbiguousHashes_.insert(hash);
        previous = hash;
    }
}

PhysicalResource GameResources::OpenPhysical(const std::string& logicalName) const {
    auto found=TryOpenPhysical(logicalName);
    if (!found)logResourceSource("resource.missing",logicalName,sourcePath_,0,0,false);
    Require(found.has_value(), "Game resource not found: " + logicalName +
            " (source " + sourcePath_.u8string() + ")");
    return std::move(*found);
}

std::optional<PhysicalResource> GameResources::TryOpenPhysical(const std::string& logicalName) const {
    if (archive_) {
        const auto& matches=archive_->Find(logicalName);
        if (matches.empty())return std::nullopt;
        auto range=archive_->GetUnique(logicalName);
        const auto& entry=archive_->Entries().at(matches.front());
        logResourceSource("resource.source",logicalName,sourcePath_,entry.nameHash,range.Size(),false,
            static_cast<std::int64_t>(entry.index),std::uint64_t(entry.sectorOffset)*kSectorSize);
        return PhysicalResource{std::move(range),false};
    }
    const auto hash = PathHash(logicalName);
    Require(looseAmbiguousHashes_.count(hash) == 0,
            "Ambiguous extracted archive hash for " + logicalName +
            " (index " + looseIndexPath_.u8string() + ")");
    const auto expected = looseLengths_.find(hash);
    auto path = LoosePath(looseRoot_, logicalName);
    bool expanded = false;
    if (path.empty() && expected != looseLengths_.end()) {
        std::string hashedName = "__Unknown/00000000.bin";
        constexpr char hexadecimal[] = "0123456789ABCDEF";
        for (unsigned digit = 0; digit < 8; ++digit)
            hashedName[10 + digit] = hexadecimal[(hash >> (28 - digit * 4)) & 15u];
        path = LoosePath(looseRoot_, hashedName);
    }
    if (path.empty()) {
        const auto normalized = NormalizePath(logicalName);
        if (normalized.size() > 4 &&
            normalized.compare(normalized.size() - 4, 4, ".LZC") == 0) {
            auto fallback = normalized;
            fallback.replace(fallback.size() - 4, 4, ".BUN");
            path = LoosePath(looseRoot_, fallback);
            expanded = !path.empty();
        }
    }
    if (path.empty())return std::nullopt;
    Require(fs::is_regular_file(path),"Game resource is not a physical file: " + logicalName);
    auto file = std::make_shared<RandomAccessFile>(path);
    if (expected != looseLengths_.end() && !expanded) {
        Require(file->Size() == expected->second,
                "Extracted resource size mismatch: " + path.u8string() + " for " +
                logicalName + "; ZDIR expects " + std::to_string(expected->second) +
                " bytes, found " + std::to_string(file->Size()));
    }
    logResourceSource("resource.source",logicalName,path,hash,file->Size(),expanded);
    return PhysicalResource{FileRange(file, 0, file->Size()), expanded};
}

std::vector<std::uint8_t> GameResources::Load(const std::string& logicalName) const {
    return LoadDecoded(OpenPhysical(logicalName).range);
}

} // namespace hp2
