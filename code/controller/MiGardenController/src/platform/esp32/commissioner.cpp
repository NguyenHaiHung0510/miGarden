#ifdef MG_COMMISSION
#include "commissioner.hpp"
#include <ArduinoJson.h>
#include <algorithm>
#include <cerrno>
#include <dirent.h>
#include <esp_flash_encrypt.h>
#include <esp_flash.h>
#include <esp_secure_boot.h>
#include <fcntl.h>
#include <mbedtls/platform_util.h>
#include <mbedtls/sha256.h>
#include <nvs.h>
#include <esp_littlefs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <memory>

namespace mg::esp {
using namespace commission;
namespace {
constexpr char relKey[]="profile-v1";
const char* ns(bool lab){return lab?"mg-lab-v1":"mg-rel-v1";}
const char* path(bool lab){return lab?"/mg/lab-v1.journal":"/mg/release-v1.journal";}
constexpr size_t chunk=1024;
constexpr Ms idleLimit=10000,totalLimit=300000;
struct WriteScope {
    explicit WriteScope(CommissionStore store){permitCommissionWrite(store);}
    ~WriteScope(){permitCommissionWrite(CommissionStore::closed);}
};
struct Hash {
    mbedtls_sha256_context c;
    bool okay;
    Hash(){mbedtls_sha256_init(&c);okay=mbedtls_sha256_starts(&c,0)==0;}
    ~Hash(){mbedtls_sha256_free(&c);}
    bool add(const void* p,size_t n){return okay=okay&&mbedtls_sha256_update(&c,(const uint8_t*)p,n)==0;}
    void u(uint32_t n){uint8_t b[4];for(unsigned i=0;i<4;++i)b[i]=uint8_t(n>>(i*8));add(b,4);}
    void text(std::string_view s){u(s.size());add(s.data(),s.size());}
    bool finish(Digest& d){
        uint8_t b[32];if(!okay||mbedtls_sha256_finish(&c,b)!=0)return false;
        char s[65]{};for(size_t i=0;i<32;++i){s[2*i]="0123456789abcdef"[b[i]>>4];s[2*i+1]="0123456789abcdef"[b[i]&15];}
        return d.set(s);
    }
};
bool bytesHash(std::string_view s,Digest& d){Hash h;return h.add(s.data(),s.size())&&h.finish(d);}
void wipe(std::vector<uint8_t>& b){if(!b.empty())mbedtls_platform_zeroize(b.data(),b.size());}
bool fileHash(const char* path,Digest& d){
    const int fd=::open(path,O_RDONLY);if(fd<0)return false;
    Hash h;uint8_t b[1024];ssize_t n;
    while((n=::read(fd,b,sizeof b))>0)if(!h.add(b,n)){::close(fd);return false;}
    return ::close(fd)==0&&n==0&&h.finish(d);
}
bool absence(const char* path,bool& absent){
    struct stat s{};
    if(::stat(path,&s)==0){absent=false;return true;}
    absent=errno==ENOENT;return absent;
}
bool namespaceAbsent(const char* name,bool& absent){
    nvs_handle_t h;
    const auto err=nvs_open(name,NVS_READONLY,&h);
    absent=err==ESP_ERR_NVS_NOT_FOUND;
    if(err==ESP_OK)nvs_close(h);
    return err==ESP_OK||absent;
}
bool value(nvs_handle_t h,const nvs_entry_info_t& i,std::vector<uint8_t>& b){
    size_t n=0;esp_err_t e=ESP_FAIL;
    auto number=[&](auto x,auto get)->bool{
        if(get(h,i.key,&x)!=ESP_OK)return false;
        b.resize(sizeof x);std::memcpy(b.data(),&x,sizeof x);return true;
    };
    switch(i.type){
        case NVS_TYPE_U8:return number(uint8_t{},nvs_get_u8);
        case NVS_TYPE_I8:return number(int8_t{},nvs_get_i8);
        case NVS_TYPE_U16:return number(uint16_t{},nvs_get_u16);
        case NVS_TYPE_I16:return number(int16_t{},nvs_get_i16);
        case NVS_TYPE_U32:return number(uint32_t{},nvs_get_u32);
        case NVS_TYPE_I32:return number(int32_t{},nvs_get_i32);
        case NVS_TYPE_U64:return number(uint64_t{},nvs_get_u64);
        case NVS_TYPE_I64:return number(int64_t{},nvs_get_i64);
        case NVS_TYPE_STR:e=nvs_get_str(h,i.key,nullptr,&n);break;
        case NVS_TYPE_BLOB:e=nvs_get_blob(h,i.key,nullptr,&n);break;
        default:return false;
    }
    if(e!=ESP_OK||n>0x5000)return false;
    b.resize(n);
    if(i.type==NVS_TYPE_STR)e=nvs_get_str(h,i.key,(char*)b.data(),&n);
    else e=nvs_get_blob(h,i.key,b.data(),&n);
    return e==ESP_OK;
}
bool receive(uint8_t* p,size_t n){
    const Ms began=monoNow();Ms last=began;
    size_t got=0;
    while(got<n){
        const auto now=monoNow();
        if(now-began>=totalLimit||now-last>=idleLimit)return false;
        if(!Serial.available()){delay(1);continue;}
        const auto count=Serial.read(p+got,std::min(n-got,size_t(Serial.available())));
        if(count){got+=count;last=monoNow();}
    }
    return true;
}
bool readLine(char* b,size_t cap,Ms limit){
    size_t used=0;
    while(monoNow()<limit){
        if(!Serial.available()){delay(1);continue;}
        const int c=Serial.read();
        if(c=='\n'){if(used&&b[used-1]=='\r')--used;b[used]=0;return true;}
        if(c<32||c>126||used+1>=cap)return false;
        b[used++]=char(c);
    }
    return false;
}
void outcome(Result r,const Plan* p=nullptr,const Observation* o=nullptr){
    JsonDocument d;d["result"]=label(r);
    if(p)d["transaction_id"]=p->transaction.view();
    if(o){
        d["layout_verified"]=o->layout;d["namespace_absent"]=o->namespaceAbsent;d["journal_absent"]=o->journalAbsent;
        d["filesystem"]=o->fs==Fs::blank?"blank":o->fs==Fs::verified?"verified":"unknown";
        d["partition_sha256"]=o->partition.view();d["nvs_sha256"]=o->nvs.view();d["data_sha256"]=o->data.view();
        d["other_nvs_sha256"]=o->otherNvs.view();d["other_files_sha256"]=o->otherFiles.view();
        d["profile_sha256"]=o->profile.view();d["journal_sha256"]=o->journal.view();
    }
    Serial.print("COMMISSION ");serializeJson(d,Serial);Serial.println();
}
template<size_t N>bool text(JsonVariantConst v,Text<N>& d){
    if(!v.is<JsonString>())return false;const auto s=v.as<JsonString>();return d.set({s.c_str(),s.size()});
}
bool plan(JsonObjectConst o,Plan& p){
    if(o.isNull()||!text(o["transaction_id"],p.transaction)||!text(o["partition_sha256"],p.partition)||
       !text(o["nvs_sha256"],p.nvs)||!text(o["data_sha256"],p.data)||!text(o["other_nvs_sha256"],p.otherNvs)||
       !text(o["other_files_sha256"],p.otherFiles)||!text(o["profile_sha256"],p.profile)||
       !text(o["journal_sha256"],p.journal)||!text(o["target_sha256"],p.targetHash)||
       !text(o["target_build_id"],p.targetBuild)||!o["target_bytes"].is<uint32_t>())return false;
    p.targetBytes=o["target_bytes"].as<uint32_t>();
    if(o["profile"]!="lab"&&o["profile"]!="release")return false;
    p.lab=o["profile"]=="lab";
    p.approved=o["approved"].is<bool>()&&o["approved"].as<bool>();
    p.backupConfirmed=o["backup_confirmed"].is<bool>()&&o["backup_confirmed"].as<bool>();
    p.allowBlankInit=o["allow_blank_init"].is<bool>()&&o["allow_blank_init"].as<bool>();
    return true;
}
}
const esp_partition_t* CommissionPort::region(const char* name)const{
    const auto* p=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_ANY,name);
    const uint32_t offset=!strcmp(name,"nvs")?0x9000:!strcmp(name,"spiffs")?0x290000:0;
    const uint32_t size=!strcmp(name,"nvs")?0x5000:!strcmp(name,"spiffs")?0x160000:0;
    return p&&offset&&p->address==offset&&p->size==size&&!p->encrypted?p:nullptr;
}
bool CommissionPort::rawHash(const esp_partition_t* p,Digest& d,bool* ff){
    if(!p)return false;
    Hash h;uint8_t b[1024];bool blank=true;
    for(size_t at=0;at<p->size;at+=sizeof b){
        const size_t n=std::min(sizeof b,size_t(p->size-at));
        if(esp_partition_read(p,at,b,n)!=ESP_OK||!h.add(b,n))return false;
        for(size_t i=0;i<n;++i)if(b[i]!=0xff)blank=false;
        if(at%32768==0)delay(1);
    }
    if(ff)*ff=blank;
    return h.finish(d);
}
bool CommissionPort::mount(bool readOnly){
    if(mounted_&&readOnly_==readOnly)return true;
    if(mounted_&&esp_vfs_littlefs_unregister("spiffs")!=ESP_OK)return false;
    mounted_=false;
    esp_vfs_littlefs_conf_t c{};
    c.base_path="/mg";c.partition_label="spiffs";c.format_if_mount_failed=false;
    c.grow_on_mount=false;c.read_only=readOnly;
    mounted_=esp_vfs_littlefs_register(&c)==ESP_OK;readOnly_=readOnly;
    return mounted_;
}
bool CommissionPort::nvsInventory(Digest& result){
    struct Entry { std::string ns,key;uint8_t type;uint32_t bytes;Digest value; };
    std::vector<Entry> entries;
    nvs_iterator_t it=nullptr;
    auto err=nvs_entry_find("nvs",nullptr,NVS_TYPE_ANY,&it);
    bool okay=true;
    while(err==ESP_OK){
        nvs_entry_info_t i;
        if(nvs_entry_info(it,&i)!=ESP_OK){okay=false;break;}
        if(lab_||strcmp(i.namespace_name,ns(false))){
            if(entries.size()>=512){okay=false;break;}
            nvs_handle_t h;
            if(nvs_open(i.namespace_name,NVS_READONLY,&h)!=ESP_OK){okay=false;break;}
            std::vector<uint8_t> b;Digest hash;
            okay=value(h,i,b)&&bytesHash({(char*)b.data(),b.size()},hash);
            nvs_close(h);
            const auto size=b.size();wipe(b);
            if(!okay)break;
            entries.push_back({i.namespace_name,i.key,uint8_t(i.type),uint32_t(size),hash});
        }
        err=nvs_entry_next(&it);
    }
    nvs_release_iterator(it);
    if(!okay||err!=ESP_ERR_NVS_NOT_FOUND)return false;
    std::sort(entries.begin(),entries.end(),[](const Entry& a,const Entry& b){
        if(a.ns!=b.ns)return a.ns<b.ns;if(a.key!=b.key)return a.key<b.key;return a.type<b.type;
    });
    Hash h;
    for(const auto& e:entries){h.text(e.ns);h.text(e.key);h.u(e.type);h.u(e.bytes);h.text(e.value.view());}
    return h.finish(result);
}
bool CommissionPort::filesInventory(Digest& result){
    std::vector<std::string> pending{"/mg"},files;
    size_t count=0;
    while(!pending.empty()){
        const auto directory=pending.back();pending.pop_back();
        DIR* d=::opendir(directory.c_str());if(!d)return false;
        bool okay=true;
        while(auto* e=::readdir(d)){
            if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;
            const auto name=directory+"/"+e->d_name;
            if(++count>256){okay=false;break;}
            struct stat s{};
            if(::stat(name.c_str(),&s)!=0){okay=false;break;} // LittleFS has no symlinks
            if(S_ISDIR(s.st_mode))pending.push_back(name);
            else if(S_ISREG(s.st_mode)){if(name!=path(lab_))files.push_back(name);}
            else {okay=false;break;}
        }
        ::closedir(d);if(!okay)return false;
    }
    std::sort(files.begin(),files.end());Hash h;
    for(const auto& f:files){Digest d;if(!fileHash(f.c_str(),d))return false;h.text(f.substr(3));h.text(d.view());}
    return h.finish(result);
}
bool CommissionPort::observe(Observation& o){
    o={};
    const auto* a=esp_partition_find_first(ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_APP_OTA_0,"app0");
    const auto* b=esp_partition_find_first(ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_APP_OTA_1,"app1");
    if(!a||!b||a->address!=0x10000||b->address!=0x150000||a->size!=0x140000||b->size!=0x140000||
       a->encrypted||b->encrypted||esp_flash_encryption_enabled()||esp_secure_boot_enabled()||!region("nvs")||!region("spiffs"))return false;
    uint8_t table[3072];
    if(esp_flash_read(nullptr,table,0x8000,sizeof table)!=ESP_OK||
       !bytesHash({(char*)table,sizeof table},o.partition)||!namespaceAbsent(ns(lab_),o.namespaceAbsent))return false;
    o.layout=o.partition.view()=="148b959cbff1c38aa8e1d5c0ba9d612c54997b945e56a63f41223eef650653a1";
    if(!o.layout)return false;
    bool ff=false;
    if(!rawHash(region("nvs"),o.nvs)||!nvsInventory(o.otherNvs)||!rawHash(region("spiffs"),o.data,&ff))return false;
    lastData_=o.data;
    if(ff){
        o.fs=Fs::blank;o.journalAbsent=true;Hash empty;empty.finish(o.otherFiles);
    }else if(mount(true)){
        o.fs=Fs::verified;
        // A leftover replacement is evidence of an existing ledger, even if
        // the live filename was lost. It can never authorize new genesis.
        bool stagingAbsent=false;
        const auto staging=std::string(path(lab_))+".tmp";
        if(!absence(staging.c_str(),stagingAbsent)||!stagingAbsent||
           !absence(path(lab_),o.journalAbsent)||!filesInventory(o.otherFiles))return false;
        if(!o.journalAbsent&&!fileHash(path(lab_),o.journal))return false;
    }else{o.fs=Fs::unknown;return false;}
    if(!o.namespaceAbsent){
        nvs_handle_t h;size_t n=0;
        if(nvs_open(ns(lab_),NVS_READONLY,&h)!=ESP_OK)return false;
        bool okay=nvs_get_blob(h,relKey,nullptr,&n)==ESP_OK&&n<=provisionMax;
        std::vector<uint8_t> bytes(okay?n:0);
        okay=okay&&nvs_get_blob(h,relKey,bytes.data(),&n)==ESP_OK&&bytesHash({(char*)bytes.data(),n},o.profile);
        wipe(bytes);nvs_close(h);if(!okay)return false;
    }
    o.storageReadable=true;return true;
}
bool CommissionPort::formatBlank(){
    Digest actual;bool ff=false;
    if(!region("spiffs")||!rawHash(region("spiffs"),actual,&ff)||!ff||actual!=lastData_)return false;
    if(mounted_){if(esp_vfs_littlefs_unregister("spiffs")!=ESP_OK)return false;mounted_=false;}
    // Explicit mutation path only; no mount failure ever reaches here automatically.
    WriteScope writes(CommissionStore::data);
    esp_vfs_littlefs_conf_t c{};c.base_path="/mg";c.partition_label="spiffs";c.dont_mount=true;
    c.format_if_mount_failed=false;c.grow_on_mount=false;
    if(esp_vfs_littlefs_register(&c)!=ESP_OK)return false;
    const bool okay=esp_littlefs_format("spiffs")==ESP_OK;
    esp_vfs_littlefs_unregister("spiffs");
    return okay&&mount(false);
}
bool CommissionPort::createJournal(){
    WriteScope writes(CommissionStore::data);
    if(!mount(false))return false;
    bool absent=false;
    const auto staging=std::string(path(lab_))+".tmp";
    if(!absence(staging.c_str(),absent)||!absent||!absence(path(lab_),absent)||!absent)return false;
    const auto bytes=Journal::genesis(); // commissioner only; watering firmware never creates a ledger
    const int fd=::open(path(lab_),O_CREAT|O_EXCL|O_WRONLY,0600);if(fd<0)return false;
    size_t done=0;bool okay=true;
    while(done<bytes.size()){
        const auto n=::write(fd,bytes.data()+done,bytes.size()-done);
        if(n<=0){okay=false;break;}done+=n;
    }
    const bool sync=okay&&::fsync(fd)==0;
    const bool close=::close(fd)==0;
    if(!sync||!close)return false; // leave partial/existing file; no unlink/reset retry
    Digest actual,expected;
    return fileHash(path(lab_),actual)&&bytesHash({(char*)bytes.data(),bytes.size()},expected)&&actual==expected;
}
bool CommissionPort::createNamespace(std::string_view profile){
    if(lab_)return false;
    bool absent=false;if(!namespaceAbsent(ns(false),absent)||!absent)return false;
    WriteScope writes(CommissionStore::nvs);
    nvs_handle_t h;
    if(nvs_open(ns(false),NVS_READWRITE,&h)!=ESP_OK)return false;
    const bool wrote=nvs_set_blob(h,relKey,profile.data(),profile.size())==ESP_OK;
    const bool committed=wrote&&nvs_commit(h)==ESP_OK;
    nvs_close(h);
    if(!committed)return false; // never erase a partial namespace
    if(nvs_open(ns(false),NVS_READONLY,&h)!=ESP_OK)return false;
    size_t count=0;
    bool okay=nvs_get_blob(h,relKey,nullptr,&count)==ESP_OK&&count==profile.size();
    std::vector<uint8_t> b(okay?count:0);
    okay=okay&&nvs_get_blob(h,relKey,b.data(),&count)==ESP_OK&&
        !memcmp(b.data(),profile.data(),count);
    wipe(b);nvs_close(h);return okay;
}
bool CommissionPort::verifyCreated(Observation& o){return observe(o);}

void Commissioner::status(){
    snapshotOkay_=port_.observe(snapshot_);
    outcome(snapshotOkay_?Result::ok:Result::filesystem_fault,nullptr,&snapshot_);
}
Commissioner& Commissioner::instance(){static Commissioner value;return value;}
bool Commissioner::approvedTarget(size_t length,const char* hash)const{
    return verified_&&completed_.targetBytes==length&&completed_.targetHash.view()==hash;
}
bool Commissioner::mayBeginUpdate(size_t length,const char* hash)const{
    return (!transaction_.used()&&!verified_)||approvedTarget(length,hash);
}
void Commissioner::backup(const char* name){
    const auto* p=port_.region(!strcmp(name,"nvs")?"nvs":!strcmp(name,"data")?"spiffs":"");
    Digest expected;
    if(!snapshotOkay_||!p||!port_.rawHash(p,expected)||expected!=(!strcmp(name,"nvs")?snapshot_.nvs:snapshot_.data)){
        Serial.println("ERR COMM_BACKUP_SNAPSHOT");return;
    }
    Serial.printf("COMM_BACKUP_READY kind=%s length=%lu sha256=%s chunk=1024\n",name,(unsigned long)p->size,expected.c_str());
    Serial.flush();
    const auto began=monoNow();uint8_t b[chunk];char line[40];
    for(size_t at=0;at<p->size;){
        const auto n=std::min(chunk,size_t(p->size-at));
        if(monoNow()-began>=totalLimit||esp_partition_read(p,at,b,n)!=ESP_OK){
            mbedtls_platform_zeroize(b,sizeof b);Serial.println("ERR COMM_BACKUP_READ");return;
        }
        Serial.write(b,n);Serial.flush();at+=n;
        char ack[40];snprintf(ack,sizeof ack,"ACK %u",unsigned(at));
        if(!readLine(line,sizeof line,std::min(began+totalLimit,monoNow()+idleLimit))||strcmp(line,ack)){
            mbedtls_platform_zeroize(b,sizeof b);Serial.println("ERR COMM_BACKUP_ACK");return;
        }
    }
    mbedtls_platform_zeroize(b,sizeof b);
    Digest after;if(!port_.rawHash(p,after)||after!=expected){Serial.println("ERR COMM_BACKUP_CHANGED");return;}
    Serial.printf("COMM_BACKUP_OK length=%lu sha256=%s\n",(unsigned long)p->size,after.c_str());
}
void Commissioner::process(const char* data,size_t size){
    if(size>12288||memchr(data,0,size)){outcome(Result::invalid_plan);return;}
    JsonDocument doc;
    if(deserializeJson(doc,data,size,DeserializationOption::NestingLimit(4))){outcome(Result::invalid_plan);return;}
    Plan p;if(!plan(doc["plan"].as<JsonObjectConst>(),p)){outcome(Result::invalid_plan);return;}
    const auto action=doc["action"].as<std::string_view>();
    Observation o;
    if(action=="verify"){
        const auto result=transaction_.verify(p,&o);
        verified_=false;
        if(result==Result::ok){verified_=true;completed_=p;}
        outcome(result,&p,o.layout?&o:nullptr);return;
    }
    if(action!="apply"||!doc["profile_json"].is<JsonString>()){outcome(Result::invalid_plan);return;}
    if(transaction_.used()){outcome(Result::transaction_used,&p);return;}
    auto value=doc["profile_json"].as<JsonString>();std::string_view profile{value.c_str(),value.size()};
    Digest actual;
    if(!validCommissionProfile(profile.data(),profile.size(),p.lab)||!bytesHash(profile,actual)||actual!=p.profile){
        outcome(Result::invalid_plan);return;
    }
    // Reject a mismatched host genesis before any format, file or NVS call.
    const auto genesis=Journal::genesis();
    Digest expected;
    if(!bytesHash({(const char*)genesis.data(),genesis.size()},expected)||expected!=p.journal){
        outcome(Result::invalid_plan,&p);return;
    }
    verified_=false;snapshotOkay_=false;
    const auto result=transaction_.apply(p,profile,&o);
    if(result==Result::ok){verified_=true;completed_=p;}
    outcome(result,&p,o.layout?&o:nullptr);
}
void Commissioner::frame(size_t length,const char* expected){
    if(commandFailed_||length==0||length>12288){Serial.println("ERR COMM_FRAME_BLOCKED");return;}
    std::vector<uint8_t> bytes(length);
    Serial.println("COMMISSION_READY");Serial.flush();
    const auto began=monoNow();
    for(size_t at=0;at<bytes.size();){
        const auto n=std::min(size_t(1024),bytes.size()-at);
        if(monoNow()-began>=totalLimit||!receive(bytes.data()+at,n)){
            wipe(bytes);commandFailed_=true;Serial.println("ERR COMM_FRAME_UNKNOWN");return;
        }
        at+=n;Serial.printf("COMM_ACK %u\n",unsigned(at));Serial.flush();
    }
    if(monoNow()-began>=totalLimit){wipe(bytes);commandFailed_=true;Serial.println("ERR COMM_TOTAL_TIMEOUT");return;}
    if(Serial.available()){wipe(bytes);commandFailed_=true;Serial.println("ERR COMM_EXTRA_BYTES");return;}
    Digest actual;
    if(!bytesHash({(char*)bytes.data(),bytes.size()},actual)||actual.view()!=expected){
        wipe(bytes);Serial.println("ERR COMM_FRAME_SHA256");return;
    }
    process((char*)bytes.data(),bytes.size());wipe(bytes);
}
bool Commissioner::command(char* line){
    if(transaction_.used()&&strncmp(line,"COMMISSION ",11)&&strcmp(line,"PING")&&strcmp(line,"INFO")&&
       strcmp(line,"REBOOT")&&strncmp(line,"BEGIN ",6)){
        Serial.println("ERR COMMISSION_DONE_VERIFY_OR_EXIT");return true;
    }
    if(!strncmp(line,"COMM_STATUS ",12)){
        if(transaction_.used()){outcome(Result::transaction_used);return true;}
        if(strcmp(line+12,"lab")&&strcmp(line+12,"release")){outcome(Result::invalid_plan);return true;}
        port_.target(!strcmp(line+12,"lab"));status();return true;
    }
    if(!strncmp(line,"COMM_BACKUP ",12)){
        if(transaction_.used()){outcome(Result::transaction_used);return true;}
        backup(line+12);return true;
    }
    if(!strncmp(line,"COMMISSION ",11)){
        const char* p=line+11;size_t n=0;const char* start=p;
        while(*p>='0'&&*p<='9'){if(n>(12288-unsigned(*p-'0'))/10){Serial.println("ERR COMM_FRAME_LENGTH");return true;}n=n*10+(*p++-'0');}
        if(p==start||*p!=' '||strlen(p+1)!=64){Serial.println("ERR COMM_FRAME_LENGTH");return true;}
        Digest hash;hash.set(p+1);
        if(!digestValid(hash)){Serial.println("ERR COMM_FRAME_HASH");return true;}
        frame(n,hash.c_str());return true;
    }
    return false;
}
}
#endif
