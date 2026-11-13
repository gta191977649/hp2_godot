#ifndef NFSHP2_DBFILESBASE_HPP
#define NFSHP2_DBFILESBASE_HPP
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
// Retained native C callers borrow this one file-system owner. These ABI
// adapters are not names or bodies recovered from the final ELF.
typedef struct BMemcardFileSys BMemcardFileSys;
typedef void (*BMemcardDone)(void* parameter);
typedef struct BMemcardFileSysVtbl {
    void (*destroy)(BMemcardFileSys*);
    void (*service)(BMemcardFileSys*,int port,int slot);
    void (*setFileData)(BMemcardFileSys*,const char* name,void* data,int size);
    int (*calcFreeSpaceRequired)(BMemcardFileSys*,int size);
    int (*getFreeSpace)(BMemcardFileSys*);
    int (*setState)(BMemcardFileSys*,int state,BMemcardDone done,void* parameter);
} BMemcardFileSysVtbl;
struct BMemcardFileSys {const BMemcardFileSysVtbl* vt;int lastError;int fileExists;};
enum {HP2_BMC_FS_IDLE=0,HP2_BMC_FS_SAVE=1,HP2_BMC_FS_LOAD=2,
    HP2_BMC_FS_CHECK_EXISTENCE=3,HP2_BMC_FS_GET_DIR=4,HP2_BMC_FS_FORMAT=5,
    HP2_BMC_FS_UNFORMAT=6,HP2_BMC_FS_VERIFY=7,HP2_BMC_FS_WAITING=8};
enum {HP2_BMC_OK=0,HP2_BMC_FILE_NOT_FOUND=5,HP2_BMC_IN_PROGRESS=15,HP2_BMC_ERROR=16};
BMemcardFileSys* hp2CreateNativeMemoryCard(const char* utf8_directory);
#ifdef __cplusplus
}
#include <cstdint>
#include <string>
// Original SetFileData12E270 and actual dbFilesBase.hpp definition provenance
// are recorded individually. Data is borrowed until the deferred callback.
class cDBFilesBase : public BMemcardFileSys {
public:
    void SetFileData(const char* name,void* data,std::int32_t bytes);
protected:
    std::string name_;
    void* data_{};
    std::int32_t bytes_{};
};
#endif
#endif
