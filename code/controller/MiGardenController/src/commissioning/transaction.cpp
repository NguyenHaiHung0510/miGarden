#include "transaction.hpp"
namespace mg::commission {
bool digestValid(const Digest& d) {
    if(d.size()!=64)return false;
    for(char c:d.view())if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;
    return true;
}
namespace {
bool valid(const Plan& p) {
    return digestValid(p.partition)&&digestValid(p.nvs)&&digestValid(p.data)&&digestValid(p.otherNvs)&&
        digestValid(p.otherFiles)&&digestValid(p.profile)&&digestValid(p.journal)&&digestValid(p.targetHash)&&
        !p.transaction.empty()&&!p.targetBuild.empty()&&p.targetBytes>=32&&p.targetBytes<=0x140000;
}
}
Result Transaction::apply(const Plan& p,std::string_view profile,Observation* post) {
    if(used_)return Result::transaction_used;
    if(!p.approved||!valid(p)||profile.empty())return Result::invalid_plan;
    if(!p.backupConfirmed)return Result::backup_required;
    port_.target(p.lab);
    Observation o;
    if(!port_.observe(o)||!o.storageReadable)return Result::filesystem_fault;
    if(!o.layout||o.partition!=p.partition)return Result::wrong_layout;
    if(!o.journalAbsent||(!p.lab&&!o.namespaceAbsent))return Result::target_exists;
    if(p.lab&&(o.namespaceAbsent||o.profile!=p.profile))return Result::preimage_mismatch;
    if(o.nvs!=p.nvs||o.data!=p.data||o.otherNvs!=p.otherNvs||o.otherFiles!=p.otherFiles)
        return Result::preimage_mismatch;
    if(o.fs==Fs::unknown)return Result::filesystem_fault;
    if(o.fs==Fs::blank&&!p.allowBlankInit)return Result::init_not_approved;
    // No retry after the first possible mutation in this boot. On restart,
    // existing/partial artifacts are conflicts; verify is strictly read-only.
    used_=true;
    if(o.fs==Fs::blank&&!port_.formatBlank())return Result::filesystem_fault;
    if(!port_.createJournal())return Result::partial_journal;
    if(!p.lab&&!port_.createNamespace(profile))return Result::partial_namespace;
    return after(p,post);
}
Result Transaction::after(const Plan& p,Observation* post) {
    Observation o;
    if(!port_.verifyCreated(o)||!o.layout||!o.storageReadable||o.fs!=Fs::verified||
       o.partition!=p.partition||o.profile!=p.profile||o.journal!=p.journal||
       o.namespaceAbsent||o.journalAbsent)return Result::verification_fault;
    if(post)*post=o;
    if(o.otherNvs!=p.otherNvs||o.otherFiles!=p.otherFiles)return Result::preserve_fault;
    if(p.lab&&o.nvs!=p.nvs)return Result::preserve_fault;
    return Result::ok;
}
Result Transaction::verify(const Plan& p,Observation* post) {
    if(!p.backupConfirmed||!valid(p))return Result::invalid_plan;
    port_.target(p.lab);
    return after(p,post);
}
}
