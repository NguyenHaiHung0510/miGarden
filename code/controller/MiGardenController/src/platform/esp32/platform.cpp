#include "platform.hpp"
#include <ArduinoJson.h>
#include <atomic>
#include <driver/gpio.h>
#include <esp_core_dump.h>
#include <esp_littlefs.h>
#include <esp_partition.h>
#include <esp_random.h>
#include <esp_timer.h>
#include <fcntl.h>
#include <mbedtls/platform_util.h>
#include <mbedtls/x509_crt.h>
#include <nvs.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef MG_COMMISSION
namespace {
std::atomic<uint8_t> commissionWrite{0};
bool blockedWrite(const esp_partition_t* p) {
    using mg::esp::CommissionStore;
    if (!p || p->type!=ESP_PARTITION_TYPE_DATA) return false;
    const auto allow=static_cast<CommissionStore>(commissionWrite.load());
    if (p->address==0x9000 || p->subtype==ESP_PARTITION_SUBTYPE_DATA_NVS)
        return allow!=CommissionStore::nvs || p->address!=0x9000 || p->size!=0x5000 || strcmp(p->label,"nvs");
    if (p->address==0x290000 || !strcmp(p->label,"spiffs"))
        return allow!=CommissionStore::data || p->address!=0x290000 || p->size!=0x160000 || strcmp(p->label,"spiffs");
    return false; // OTA app/otadata are handled by the retained updater
}
}
void mg::esp::permitCommissionWrite(CommissionStore s) { commissionWrite.store(uint8_t(s)); }
extern "C" esp_err_t __real_esp_partition_write(const esp_partition_t*,size_t,const void*,size_t);
extern "C" esp_err_t __wrap_esp_partition_write(const esp_partition_t* p,size_t at,const void* b,size_t n) {
    return blockedWrite(p)?ESP_ERR_INVALID_STATE:__real_esp_partition_write(p,at,b,n);
}
extern "C" esp_err_t __real_esp_partition_write_raw(const esp_partition_t*,size_t,const void*,size_t);
extern "C" esp_err_t __wrap_esp_partition_write_raw(const esp_partition_t* p,size_t at,const void* b,size_t n) {
    return blockedWrite(p)?ESP_ERR_INVALID_STATE:__real_esp_partition_write_raw(p,at,b,n);
}
#endif

// Arduino-ESP32 3.3.12 initArduino otherwise erases the complete default NVS
// partition on NO_FREE_PAGES or NEW_VERSION_FOUND. Allow ordinary per-page GC.
extern "C" esp_err_t __real_esp_partition_erase_range(const esp_partition_t*,size_t,size_t);
extern "C" esp_err_t __wrap_esp_partition_erase_range(const esp_partition_t* p,size_t at,size_t n) {
#ifdef MG_COMMISSION
    if (blockedWrite(p)) return ESP_ERR_INVALID_STATE;
#endif
    if (p && p->type==ESP_PARTITION_TYPE_DATA && p->subtype==ESP_PARTITION_SUBTYPE_DATA_NVS &&
        at==0 && n>=p->size) return ESP_ERR_INVALID_STATE;
    return __real_esp_partition_erase_range(p,at,n);
}
// The stock panic writer can capture task stacks containing a provision frame
// or authorization header. Leave the existing coredump partition untouched.
extern "C" void IRAM_ATTR __wrap_esp_core_dump_write(panic_info_t*) {}

namespace mg::esp {
namespace {
std::atomic<uint32_t> lastSync{0};
std::atomic<bool> online{false};
constexpr gpio_num_t startPin=GPIO_NUM_33, stopPin=GPIO_NUM_32, valvePin=GPIO_NUM_27;
constexpr Ms debounce=35, controlLease=2*second;
constexpr char mountAt[]="/mg";
constexpr char key[]="profile-v1";
std::string livePath() { return std::string(mountAt)+"/"+journalName; }
std::string tempPath() { return livePath()+".tmp"; }
bool writeAll(int fd,const uint8_t* p,size_t n) {
    while (n) {
        const auto count=::write(fd,p,n);
        if (count<=0) return false;
        p+=count; n-=count;
    }
    return true;
}
bool syncAndClose(int fd) {
    const bool durable=::fsync(fd)==0;
    return ::close(fd)==0 && durable;
}
struct SecretBytes {
    std::vector<char> b;
    ~SecretBytes() { if (!b.empty()) mbedtls_platform_zeroize(b.data(),b.size()); }
};
template<size_t N> bool readText(JsonVariantConst v,Text<N>& into) {
    if (!v.is<JsonString>()) return false;
    const auto s=v.as<JsonString>();
    return into.set({s.c_str(),s.size()});
}
bool origin(std::string_view uri) {
    constexpr std::string_view ssl="https://";
    if (uri.substr(0,ssl.size())!=ssl) return false;
    uri.remove_prefix(ssl.size());
    if (!uri.empty() && uri.back()=='/') uri.remove_suffix(1);
    if (uri.empty()) return false;
    const auto sep=uri.find(':');
    const auto host=uri.substr(0,sep);
    if (host.empty() || host.front()=='.' || host.back()=='.') return false;
    for (const char c:host)
        if (!((c>='0'&&c<='9') || (c>='A'&&c<='Z') || (c>='a'&&c<='z') || c=='.' || c=='-')) return false;
    if (sep!=uri.npos) {
        unsigned port=0;
        auto s=uri.substr(sep+1);
        if (s.empty() || s.size()>5) return false;
        for (char c:s) { if (c<'0'||c>'9') return false; port=port*10+(c-'0'); }
        if (!port || port>65535) return false;
    }
    return true;
}
bool tokenValid(std::string_view s) {
    if (s.size()<32) return false;
    for (const char c:s)
        if (!((c>='0'&&c<='9') || (c>='A'&&c<='Z') || (c>='a'&&c<='z') ||
              c=='-' || c=='_' || c=='.' || c=='~' || c=='+' || c=='/' || c=='=')) return false;
    return true;
}
bool decodeProvision(const char* data,size_t size,Provision& p) {
    if (!size || size>provisionMax || std::memchr(data,0,size)) return false;
    // Local framing is exact-length JSON. An ArduinoJson zero-copy char*
    // overload is not used: all received text is bounded and verified here.
    JsonDocument doc;
    if (deserializeJson(doc,data,size,DeserializationOption::NestingLimit(2))) return false;
    auto o=doc.as<JsonObjectConst>();
    if (o.isNull() || !readText(o["device_id"],p.device) || !visible(p.device.view()) ||
        !decodeWifiProfile(o,p) ||
        !readText(o["base_url"],p.baseUrl) ||
        !origin(p.baseUrl.view()) || !readText(o["device_token"],p.token) || !tokenValid(p.token.view()) ||
        !o["ca_pem"].is<JsonString>()) return false;
    const auto ca=o["ca_pem"].as<JsonString>();
    p.ca.assign(ca.c_str(),ca.size());
    if (p.ca.size()>6144 || p.ca.find('\0')!=p.ca.npos ||
        p.ca.find("-----BEGIN CERTIFICATE-----")==p.ca.npos ||
        p.ca.find("PRIVATE KEY")!=p.ca.npos) return false;
    mbedtls_x509_crt root;
    mbedtls_x509_crt_init(&root);
    const bool cert=mbedtls_x509_crt_parse(&root,reinterpret_cast<const uint8_t*>(p.ca.c_str()),p.ca.size()+1)==0 &&
        root.MBEDTLS_PRIVATE(ca_istrue);
    mbedtls_x509_crt_free(&root);
    if (!cert) return false;
#ifdef MG_LAB
    p.fixtureVerified=true; // virtual load only; no mapped output pin
#else
    p.fixtureVerified=o["fixture_verified"].is<bool>() && o["fixture_verified"].as<bool>();
#endif
    return true;
}
ProvisionStatus readProfile(nvs_handle_t h,Provision& p) {
    SecretBytes bytes;
    size_t size=0;
    const auto ok=nvs_get_blob(h,key,nullptr,&size);
    if (ok==ESP_ERR_NVS_NOT_FOUND) return ProvisionStatus::missing;
    if (ok!=ESP_OK || size==0 || size>provisionMax) return ProvisionStatus::storage_error;
    bytes.b.resize(size);
    if (nvs_get_blob(h,key,bytes.b.data(),&size)!=ESP_OK) return ProvisionStatus::storage_error;
    return decodeProvision(bytes.b.data(),size,p) ? ProvisionStatus::ok : ProvisionStatus::invalid;
}
}
Ms monoNow() { return esp_timer_get_time()/1000; }
void WallClock::synchronized(struct timeval*) { lastSync.store(uint32_t(monoNow()/second)+1); }
void WallClock::connected(bool state) { online.store(state); }
Sample WallClock::now() {
    const Ms mono=monoNow();
    const auto when=lastSync.load();
    if (!when) return {mono,none,Quality::unknown};
    timeval t;
    if (gettimeofday(&t,nullptr)!=0 || t.tv_sec<1704067200LL || t.tv_sec>4102444800LL)
        return {mono,none,Quality::unknown}; // plausible RTC/TLS preflight, 2024..2100
    const bool recent=uint32_t(mono/second)+1-when<=3700;
    return {mono,t.tv_sec*second+t.tv_usec/1000,online.load()&&recent ? Quality::synced : Quality::holdover};
}
void Output::initialize() {
#ifndef MG_LAB
    // Preload LOW before enabling output; no code in lab touches these pins.
    gpio_set_level(valvePin,0);
    gpio_config_t out{};
    out.pin_bit_mask=1ULL<<valvePin; out.mode=GPIO_MODE_OUTPUT;
    const bool outOkay=gpio_config(&out)==ESP_OK;
    gpio_config_t in{};
    in.pin_bit_mask=(1ULL<<startPin)|(1ULL<<stopPin); in.mode=GPIO_MODE_INPUT;
    in.pull_up_en=GPIO_PULLUP_ENABLE;
    const bool inOkay=gpio_config(&in)==ESP_OK;
    drive_.usable=outOkay && inOkay;
    stopLocked(gpio_get_level(stopPin)==0,monoNow());
#endif
}
void Output::writePin(bool on) {
#ifndef MG_LAB
    gpio_set_level(valvePin,on ? 1 : 0);
#else
    (void)on;
#endif
}
void Output::enable(bool yes) {
    portENTER_CRITICAL(&lock_);
    enabled_=yes;
    if (!enabled_) closeLocked(Reason::interlock,monoNow());
    portEXIT_CRITICAL(&lock_);
}
void Output::closeLocked(Reason why,Ms now) {
    writePin(false);
    if (drive_.on) {
        drive_.on=false;
        drive_.offAt=now;
        drive_.offReason=why;
    }
    deadline_=none;
}
void Output::stopLocked(bool low,Ms now) {
    if (low) {
        stopRelease_=none;
        if (!drive_.stopHeld) { ++drive_.stops; drive_.stopHeld=true; }
        closeLocked(Reason::stopped,now);
    } else if (drive_.stopHeld) {
        if (stopRelease_==none) stopRelease_=now;
        if (now-stopRelease_>=debounce) drive_.stopHeld=false;
    }
}
void Output::service() {
    portENTER_CRITICAL(&lock_);
    const auto now=monoNow();
    timingOkay_=true;
#ifndef MG_LAB
    stopLocked(gpio_get_level(stopPin)==0,now); // first LOW closes; only release is debounced
    const bool low=gpio_get_level(startPin)==0;
    if (low) {
        // Capture busy at the *first* sampled LOW, not when debounce completes.
        if (startSince_==none) {
            startSince_=now; startSent_=false; pressBlocked_=!enabled_ || drive_.on || drive_.stopHeld; startStop_=drive_.stops;
        }
        if (startArmed_ && !startSent_ && !edge_ && now-startSince_>=debounce) {
            startSent_=true; startArmed_=false; edge_=true;
            busyEdge_=pressBlocked_; edgeStop_=startStop_;
        }
    } else { startSince_=none; startSent_=false; }
    // Release is debounced separately; held START at boot cannot open.
    static Ms released=none;
    if (low) released=none;
    else { if (released==none) released=now; if (now-released>=debounce) startArmed_=true; }
#endif
    if (drive_.on && now>=deadline_) closeLocked(Reason::deadline,now);
    else if (drive_.on && (beat_==none || now-beat_>controlLease)) {
        closeLocked(Reason::storage_fault,now);
    }
    portEXIT_CRITICAL(&lock_);
}
void Output::touch() {
    portENTER_CRITICAL(&lock_); beat_=monoNow(); portEXIT_CRITICAL(&lock_);
}
bool Output::startEdge(bool& busy) {
    portENTER_CRITICAL(&lock_);
    const bool edge=edge_; busy=edge && (busyEdge_ || edgeStop_!=drive_.stops); edge_=false;
    portEXIT_CRITICAL(&lock_);
    return edge;
}
Drive Output::read() {
    portENTER_CRITICAL(&lock_);
    auto state=drive_;
    state.usable=state.usable && enabled_ && timingOkay_;
    portEXIT_CRITICAL(&lock_);
    return state;
}
bool Output::open(Ms duration,Ms until,uint32_t stops,Drive& actual) {
    portENTER_CRITICAL(&lock_);
    const auto now=monoNow();
#ifndef MG_LAB
    stopLocked(gpio_get_level(stopPin)==0,now);
#endif
    const bool allowed=duration>0 && duration<=windowSeconds*second && !drive_.on &&
        enabled_ && drive_.usable && timingOkay_ && !drive_.stopHeld && stops==drive_.stops &&
        now<until && beat_!=none && now-beat_<=controlLease;
    if (allowed) {
        deadline_=now+duration;
        drive_.on=true; drive_.onAt=now; drive_.offAt=none; drive_.offReason=Reason::none;
        writePin(true);
        actual=drive_;
    }
    portEXIT_CRITICAL(&lock_);
    return allowed;
}
Drive Output::close(Reason why) {
    portENTER_CRITICAL(&lock_);
    closeLocked(why,monoNow());
    auto actual=drive_;
    portEXIT_CRITICAL(&lock_);
    return actual;
}

bool LocalFile::mount() {
    const auto* part=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_SPIFFS,"spiffs");
    if (!part || part->address!=0x290000 || part->size!=0x160000 || part->encrypted) return false;
    esp_vfs_littlefs_conf_t cfg{};
    cfg.base_path=mountAt; cfg.partition_label="spiffs";
    // Explicitly forbid both autoformat and automatic growth.
    cfg.format_if_mount_failed=false; cfg.grow_on_mount=false;
    mounted_=esp_vfs_littlefs_register(&cfg)==ESP_OK;
    return mounted_;
}
#ifdef MG_LAB
const char* LocalFile::checkLayout() {
    if (mounted_) return size()>0 ? "LITTLEFS_JOURNAL_PRESENT" : "LITTLEFS_JOURNAL_MISSING";
    const auto* part=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_SPIFFS,"spiffs");
    if (!part || part->address!=0x290000 || part->size!=0x160000 || part->encrypted) return "LAYOUT_UNVERIFIED";
    uint8_t block[1024];
    for (size_t offset=0;offset<part->size;offset+=sizeof block) {
        if (esp_partition_read(part,offset,block,sizeof block)!=ESP_OK) return "READ_ERROR";
        for (const auto byte:block) if (byte!=0xff) return "UNMOUNTABLE_HAS_DATA";
        if (offset%32768==0) delay(1);
    }
    return "BLANK_FF";
}
#endif
int64_t LocalFile::size() {
    struct stat s{};
    return mounted_ && ::stat(livePath().c_str(),&s)==0 && S_ISREG(s.st_mode) ? s.st_size : -1;
}
bool LocalFile::read(uint32_t offset,uint8_t* out,size_t n) {
    if (!mounted_) return false;
    const int fd=::open(livePath().c_str(),O_RDONLY);
    if (fd<0) return false;
    bool ok=::lseek(fd,offset,SEEK_SET)==static_cast<off_t>(offset);
    while (ok && n) {
        const auto k=::read(fd,out,n);
        if (k<=0) ok=false; else { n-=k; out+=k; }
    }
    return ::close(fd)==0 && ok;
}
bool LocalFile::append(const uint8_t* p,size_t n) {
    if (!mounted_) return false;
    const int fd=::open(livePath().c_str(),O_WRONLY|O_APPEND);
    if (fd<0) return false;
    const bool wrote=writeAll(fd,p,n);
    return syncAndClose(fd) && wrote;
}
bool LocalFile::beginReplacement() {
    if (!mounted_ || replacement_>=0) return false;
    replacement_=::open(tempPath().c_str(),O_WRONLY|O_CREAT|O_TRUNC,0600);
    return replacement_>=0;
}
bool LocalFile::writeReplacement(const uint8_t* p,size_t n) {
    return replacement_>=0 && writeAll(replacement_,p,n);
}
bool LocalFile::publishReplacement() {
    if (replacement_<0) return false;
    const bool ok=syncAndClose(replacement_);
    replacement_=-1;
    return ok && ::rename(tempPath().c_str(),livePath().c_str())==0;
}
void LocalFile::discardReplacement() {
    if (replacement_>=0) { ::close(replacement_); replacement_=-1; }
    if (mounted_) ::unlink(tempPath().c_str());
}

ProvisionStatus loadProvision(Provision& p) {
    nvs_handle_t n;
    const auto code=nvs_open(nvsName,NVS_READONLY,&n);
    if (code==ESP_ERR_NVS_NOT_FOUND) return ProvisionStatus::missing;
    if (code!=ESP_OK) return ProvisionStatus::storage_error;
    const auto status=readProfile(n,p);
    nvs_close(n);
    return status;
}
#ifdef MG_COMMISSION
bool validCommissionProfile(const char* bytes,size_t n,bool lab) {
    Provision p;
    if(!decodeProvision(bytes,n,p))return false;
    JsonDocument doc;
    if(deserializeJson(doc,bytes,n,DeserializationOption::NestingLimit(2)))return false;
    const auto o=doc.as<JsonObjectConst>();
    if((!lab&&(!o["fixture_verified"].is<bool>()||!o["fixture_verified"].as<bool>()))||
       p.device.empty()||p.token.size()!=43)return false;
    for(size_t i=0;i<p.device.size();++i){
        const char c=p.device.data[i];
        const bool alpha=(c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9');
        if(!alpha&&(i==0||(c!='_'&&c!='.'&&c!=':'&&c!='-')))return false;
    }
    for(char c:p.token.view())
        if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return false;
    return true;
}
#endif
#ifdef MG_LAB
ProvisionStatus saveProvision(const char* data,size_t size) {
    Provision next;
    if (!decodeProvision(data,size,next)) return ProvisionStatus::invalid;
    Provision old;
    const auto existing=loadProvision(old);
    if (existing==ProvisionStatus::ok &&
        (old.device!=next.device || old.baseUrl!=next.baseUrl || old.ca!=next.ca))
        return ProvisionStatus::device_change;
    if (existing!=ProvisionStatus::ok && existing!=ProvisionStatus::missing) return existing;
    nvs_handle_t n;
    if (nvs_open(nvsName,NVS_READWRITE,&n)!=ESP_OK) return ProvisionStatus::storage_error;
    const auto set=nvs_set_blob(n,key,data,size);
    const auto committed=set==ESP_OK ? nvs_commit(n) : set;
    nvs_close(n);
    Provision checked;
    return committed==ESP_OK && loadProvision(checked)==ProvisionStatus::ok &&
        checked.device==next.device && checked.baseUrl==next.baseUrl && checked.ca==next.ca &&
        checked.token==next.token && checked.ssid==next.ssid && checked.password==next.password &&
        checked.hasFallback==next.hasFallback && checked.fallbackSsid==next.fallbackSsid &&
        checked.fallbackPassword==next.fallbackPassword ?
        ProvisionStatus::ok : ProvisionStatus::storage_error;
}
#endif
BootId newBootId() {
    uint32_t random[4];
    esp_fill_random(random,sizeof random);
    char b[40];
    snprintf(b,sizeof b,"b-%08lx%08lx%08lx%08lx",static_cast<unsigned long>(random[0]),
        static_cast<unsigned long>(random[1]),static_cast<unsigned long>(random[2]),static_cast<unsigned long>(random[3]));
    BootId id; id.set(b); return id;
}
}
