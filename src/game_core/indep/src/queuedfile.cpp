#include "bware/inc/bFile.hpp"
#include "bware/inc/bChunk.hpp"
#include "Texture.hpp"
#include "Joylog.hpp"
#include "bware/inc/bWare.hpp"
#include <algorithm>
#include <cstring>
#include <list>
#include <vector>

namespace hp2 {

// PAL1AD6D8..1ADCB4: one ordered queue and one active physical read.
// Slot allocation is a native adaptation of the growable SDK pool. Request
// addresses remain stable, including while a completion queues more files.
struct QueuedFileService::Storage {
    const GameResources& files;
    FileIoRuntime& io;
    ReplayIntegerLog& log;
    std::vector<std::unique_ptr<QueuedFile>> slots;
    std::list<QueuedFile*> ordered;
    QueuedFile* active{};
    std::int32_t nextId{1};
    bool shuttingDown{};
    std::exception_ptr shutdownFailure;
    Storage(const GameResources& input,FileIoRuntime& transport,ReplayIntegerLog& replay)
        :files(input),io(transport),log(replay),slots(16) {}
};

QueuedFileService::QueuedFileService(const GameResources& files,FileIoRuntime& io,ReplayIntegerLog& log)
    :storage_(std::make_unique<Storage>(files,io,log))
{
    Require(log.clock && (!log.replaying || log.stream),"Queued files require the actual clock/Joylog owner");
}

QueuedFileService::~QueuedFileService()
{
    (void)Shutdown();
}

std::int32_t QueuedFileService::Add(void* destination,const char* filename,std::int32_t offset,
    std::int32_t bytes,QueuedFileCompletion completion)
{
    Require(filename && *filename && std::strlen(filename)<48 && offset>=0 && bytes>=0 &&
        (destination || !bytes),"Queued file has an invalid filename, destination or extent");
    Require((completion.kind==QueuedFileCompletionKind::None && !completion.resourceFile && !completion.texturePack) ||
        (completion.kind==QueuedFileCompletionKind::ResourceFileTransfer && completion.resourceFile && !completion.texturePack) ||
        (completion.kind==QueuedFileCompletionKind::TextureRegion && completion.texturePack && !completion.resourceFile),
        "Queued file completion has no matching actual owner");
    auto& s=*storage_;
    Require(!s.shuttingDown,"Queued file service is shutting down");
    auto slot=std::find_if(s.slots.begin(),s.slots.end(),[](const auto& p){return !p;});
    if (slot==s.slots.end()) {
        const auto count=s.slots.size();
        s.slots.resize(count+count/4+1);slot=s.slots.begin()+static_cast<std::ptrdiff_t>(count);
    }
    auto request=std::make_unique<QueuedFile>();
    request->destination_=static_cast<std::uint8_t*>(destination);request->filename_=filename;
    request->offset_=offset;request->total_=bytes;request->completion_=completion;
    // The retail sequence wraps at 100000. A long-running native session
    // can retain requests across that wrap: do not give two live requests the
    // same ID or Abort/ChangeParams can target the wrong ResourceFile owner.
    auto identifier=s.nextId;
    const auto originalIdentifier=identifier;
    while (std::any_of(s.ordered.begin(),s.ordered.end(),
            [identifier](const QueuedFile* pending){return pending->id_==identifier;})) {
        if (++identifier>100000) identifier=1;
        Require(identifier!=originalIdentifier,"Queued file identifier pool exhausted");
    }
    request->id_=identifier;
    auto* identity=request.get();
    // Publish the slot only after the ordered-list node exists. A native
    // allocation exception must leave no retained request pointing at the
    // ResourceFile/TexturePack that its caller is about to unwind.
    s.ordered.push_back(identity);
    *slot=std::move(request);
    s.nextId=identifier<100000 ? identifier+1 : 1;
    return identity->id_;
}

void QueuedFileService::BeginRead(QueuedFile& request)
{
    auto& s=*storage_;
    try {
        const auto file=s.files.OpenPhysical(request.filename_);
        const auto count=std::min(request.total_-request.scheduled_,request.chunkBytes_);
        const auto offset=static_cast<std::uint64_t>(request.offset_)+static_cast<std::uint32_t>(request.scheduled_);
        Require(count>=0 && offset<=file.range.Size() && static_cast<std::uint32_t>(count)<=file.range.Size()-offset,
            "Queued read exceeds its actual physical file");
        request.operation_=s.io.Read(file.range,offset,
            request.destination_ ? request.destination_+request.scheduled_ : nullptr,static_cast<std::size_t>(count));
        request.state_=QueuedFileState::Reading;request.scheduled_+=count;
    } catch (...) {request.state_=QueuedFileState::Failed;}
}

void QueuedFileService::ObserveRead(QueuedFile& request)
{
    if (request.state_!=QueuedFileState::Reading)return;
    const auto state=storage_->io.State(request.operation_);
    if (state==FileReadState::Complete)request.state_=QueuedFileState::Complete;
    if (state==FileReadState::Failed)request.state_=QueuedFileState::Failed;
}

void QueuedFileService::Retire(QueuedFile& request,bool notify,bool failed)
{
    auto& s=*storage_;
    const auto slot=std::find_if(s.slots.begin(),s.slots.end(),[&](const auto& p){return p.get()==&request;});
    Require(slot!=s.slots.end(),"Queued request has no owning SDK slot");
    const auto index=static_cast<std::size_t>(slot-s.slots.begin());
    s.ordered.remove(&request);if (s.active==&request)s.active=nullptr;
    const auto completion=request.completion_;
    // The source releases the slot after the callback. Keep this allocation
    // occupied even if callback Add() grows the vector of slot owners.
    try {
        if (notify)switch (completion.kind) {
        case QueuedFileCompletionKind::None:break;
        case QueuedFileCompletionKind::ResourceFileTransfer:completion.resourceFile->QueuedReadDone(failed);break;
        case QueuedFileCompletionKind::TextureRegion:completion.texturePack->QueuedReadDone(failed);break;
        }
    } catch (...) {s.slots[index].reset();throw;}
    s.slots[index].reset();
}

void QueuedFileService::Service()
{
    auto& s=*storage_;
    if(s.shuttingDown)return;
    s.io.Service();
    if (s.active) {
        auto& request=*s.active;ObserveRead(request);
        auto observed=request.state_;
        if (s.log.replaying) {
            const auto errors=s.log.readFailures;
            observed=static_cast<QueuedFileState>(replayIntegerLogRead(&s.log,4,5));
            Require(errors==s.log.readFailures,"Queued file playback status is truncated");
            if (observed!=QueuedFileState::Reading && request.state_==QueuedFileState::Reading) {
                s.io.Wait(request.operation_);ObserveRead(request);
            }
            // A different/missing physical asset cannot become loaded merely
            // because a recorded status says that its original read finished.
            Require(observed!=QueuedFileState::Complete || request.state_==QueuedFileState::Complete,
                "Queued playback completed without actual physical bytes");
        } else replayIntegerLogWrite(&s.log,static_cast<std::uint32_t>(observed),4,5);
        if (observed==QueuedFileState::Complete) {
            s.active=nullptr;request.state_=QueuedFileState::Waiting;request.operation_.reset();
            if (request.abortRequested_)Retire(request,false,false);
            else if (request.scheduled_==request.total_)Retire(request,true,false);
        } else if (observed==QueuedFileState::Failed) {
            // Replay may report a failure before the physical OS read has
            // ended. Join it before allowing the destination to be released.
            if (request.operation_)s.io.Wait(request.operation_);
            // An owner can be retired after requesting Abort(). Even if the
            // OS read fails, a completion into that destroyed owner is unsafe.
            Retire(request,!request.abortRequested_,true);
        }
    }
    if (!s.active && !s.ordered.empty()) {
        s.ordered.sort([](const auto* a,const auto* b){return a->priority_<b->priority_;});
        s.active=s.ordered.front();BeginRead(*s.active);
    }
}

bool QueuedFileService::Abort(std::int32_t identifier)
{
    auto& s=*storage_;
    const auto found=std::find_if(s.ordered.begin(),s.ordered.end(),[&](const auto* p){return p->id_==identifier;});
    if (found==s.ordered.end())return true;
    auto& request=**found;
    if (request.state_==QueuedFileState::Waiting) {Retire(request,false,false);return true;}
    request.abortRequested_=true;return false;
}

void QueuedFileService::ChangeParams(std::int32_t identifier,std::int32_t priority,std::int32_t chunkBytes)
{
    Require(chunkBytes>0,"Queued file chunk size must make forward progress");
    for (auto* request:storage_->ordered)if (request->id_==identifier) {
        request->priority_=priority;request->chunkBytes_=chunkBytes;return;
    }
}

void QueuedFileService::CancelAndWait(std::int32_t identifier)
{
    auto& s=*storage_;
    const auto found=std::find_if(s.ordered.begin(),s.ordered.end(),
        [identifier](const auto* request){return request->id_==identifier;});
    if(found==s.ordered.end())return;
    auto& request=**found;
    request.completion_={};request.abortRequested_=true;
    // A recorded completion is not the end of the actual destination write.
    // Cancellation neither consults Joylog nor starts a subsequent request.
    if(request.operation_)s.io.Wait(request.operation_);
    Retire(request,false,false);
}

void QueuedFileService::BeginShutdown() noexcept
{
    auto& s=*storage_;s.shuttingDown=true;
    for(auto* request:s.ordered) {
        request->completion_={};request->abortRequested_=true;
    }
}

std::exception_ptr QueuedFileService::Shutdown() noexcept
{
    BeginShutdown();auto& s=*storage_;
    while(!s.ordered.empty()) {
        try {CancelAndWait(s.ordered.front()->id_);}
        catch(...) {
            if(!s.shutdownFailure)s.shutdownFailure=std::current_exception();
            // Physical failures finish the operation normally. A failure of
            // Wait itself means a broken private transport-owner invariant:
            // keep the destination owned instead of freeing an active write.
            break;
        }
    }
    return s.shutdownFailure;
}

const QueuedFile* QueuedFileService::Find(std::int32_t identifier) const noexcept
{
    for (const auto* request:storage_->ordered)if (request->id_==identifier)return request;
    return nullptr;
}
bool QueuedFileService::Empty() const noexcept {return storage_->ordered.empty();}
const GameResources& QueuedFileService::Files() const noexcept {return storage_->files;}

} // namespace hp2
