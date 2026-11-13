#ifndef NFSHP2_JOYLOG_HPP
#define NFSHP2_JOYLOG_HPP
#include "bRepeat.hpp"
#include "indep/inc/Timer.hpp"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct ReplayIntegerLog {
    BitStream* stream;
    const GameTime* clock;
    int32_t replaying;
    int32_t recording;
    uint32_t readFailures;
} ReplayIntegerLog;

uint32_t replayIntegerLogRead(ReplayIntegerLog* log, uint32_t bits, uint32_t channel);
uint32_t replayIntegerLogReadUnsigned(ReplayIntegerLog* log, uint32_t bits, uint32_t channel);
void replayIntegerLogReadBytes(ReplayIntegerLog* log, uint8_t* bytes, int32_t count, uint32_t channel);
void replayIntegerLogWrite(ReplayIntegerLog* log, uint32_t value, uint32_t bits, uint32_t channel);
void replayIntegerLogWriteBytes(ReplayIntegerLog* log, const uint8_t* bytes, int32_t count, uint32_t channel);

#ifdef __cplusplus
}
#include <memory>
#include <string>
/* Direct references to the 31 source launch/config cells, in the exact
 * LoadConfigItems/SaveConfigItems order. Slot6 is the separate float cell.
 * These are borrowed application owners, never a second settings snapshot. */
struct JoylogConfigItems {
    int32_t* integers[31]{};
    float* numericOption{};
    /* Slots11/12/24/25 are real float cells moved as raw words. This view
     * borrows those same floats without an int32_t alias or shadow storage. */
    float* floatBits[31]{};
};
bool joylogConfigBindingsValid(const JoylogConfigItems& items) noexcept;
bool joylogLoadConfigItems(ReplayIntegerLog& log,JoylogConfigItems& items);
bool joylogSaveConfigItems(ReplayIntegerLog& log,const JoylogConfigItems& items);

enum class JoylogResult { Ok,InvalidBinding,InvalidInput,FileFailure,StreamFailure };
/* Native retained allocation/transport for the original Joylog.Init. Empty
 * input selects recording; a named unreadable input is an explicit error. */
class JoylogFile {
public:
    explicit JoylogFile(const GameTime& clock);
    ~JoylogFile();
    JoylogFile(const JoylogFile&)=delete;
    JoylogFile& operator=(const JoylogFile&)=delete;
    JoylogResult Initialize(int32_t applicationMode,int32_t enabled,
        const std::string& input,const std::string& output,JoylogConfigItems* config);
    JoylogResult Finish();
    ReplayIntegerLog& Log() noexcept;
private:
    struct Storage;
    std::unique_ptr<Storage> storage_;
};
#endif
#endif
