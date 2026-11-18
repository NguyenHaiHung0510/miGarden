#pragma once
#include "../core/types.hpp"
#include <string>
namespace mg::commission {
using Digest=Text<64>;
enum class Fs : uint8_t { unknown, blank, verified };
enum class Result : uint8_t {
    ok, invalid_plan, wrong_layout, preimage_mismatch, target_exists, backup_required, init_not_approved,
    filesystem_fault, partial_journal, partial_namespace, preserve_fault, verification_fault, transaction_used
};
struct Observation {
    bool layout=false, namespaceAbsent=false, journalAbsent=false, storageReadable=false;
    Fs fs=Fs::unknown;
    Digest partition, nvs, data, otherNvs, otherFiles, profile, journal;
};
struct Plan {
    Text<64> transaction;
    Digest partition, nvs, data, otherNvs, otherFiles, profile, journal, targetHash;
    Text<96> targetBuild;
    uint32_t targetBytes=0;
    bool lab=false;
    bool backupConfirmed=false, allowBlankInit=false, approved=false;
};
struct Port {
    virtual ~Port()=default;
    virtual bool observe(Observation&)=0; // read only, no autoformat or implicit file creation
    virtual void target(bool lab)=0;
    virtual bool formatBlank()=0;        // full FF + exact layout rechecked by the actual adapter
    virtual bool createJournal()=0;      // O_EXCL; keep any partial artifact, never overwrite/delete
    virtual bool createNamespace(std::string_view)=0; // NVS API; existing namespace is always conflict
    virtual bool verifyCreated(Observation&)=0;
};
inline const char* label(Result r) {
    constexpr const char* x[]={"OK","INVALID_PLAN","LAYOUT_MISMATCH","PREIMAGE_MISMATCH","TARGET_EXISTS",
        "PRIVATE_BACKUP_REQUIRED","BLANK_INIT_NOT_APPROVED","FILESYSTEM_FAULT","PARTIAL_JOURNAL",
        "PARTIAL_NAMESPACE","PRESERVE_FAULT","VERIFICATION_FAULT","TRANSACTION_USED"};
    return x[unsigned(r)];
}
class Transaction {
public:
    explicit Transaction(Port& p):port_(p){}
    Result apply(const Plan&,std::string_view,Observation* post=nullptr);
    Result verify(const Plan&,Observation* post=nullptr);
    bool used() const{return used_;}
private:
    Port& port_;
    bool used_=false;
    Result after(const Plan&,Observation*);
};
bool digestValid(const Digest&);
}
