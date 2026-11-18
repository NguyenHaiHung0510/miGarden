#ifdef MG_LAB
#include "lab_usb.hpp"
#include "commissioner.hpp"
#include <WiFi.h>
#include <esp_flash_encrypt.h>
#include <esp_secure_boot.h>
#include <mbedtls/platform_util.h>
#include <mbedtls/sha256.h>
#include <memory>
#include <new>

#ifdef CONFIG_APP_ROLLBACK_ENABLE
// Do not let Arduino's early default acceptance mark this app as healthy.
extern "C" bool verifyRollbackLater() { return true; }
#endif

namespace mg::esp {
namespace {
constexpr size_t slotBytes=0x140000, block=1024;
constexpr Ms idleTime=10000,totalTime=300000,quietTime=1500;
bool same(const esp_partition_t* a,const esp_partition_t* b) {
    return a && b && a->address==b->address && a->size==b->size;
}
const char* runningLabel() {
    const auto* running=esp_ota_get_running_partition();
    return running ? running->label : "UNKNOWN";
}
bool decimal(const char*& p,size_t cap,size_t& n) {
    n=0; const auto* from=p;
    for (; *p>='0'&&*p<='9'; ++p) {
        const unsigned digit=*p-'0';
        if (n>(cap-digit)/10) return false;
        n=n*10+digit;
    }
    return p!=from && n<=cap;
}
int hexDigit(char c) {
    if (c>='0'&&c<='9') return c-'0';
    if (c>='a'&&c<='f') return c-'a'+10;
    if (c>='A'&&c<='F') return c-'A'+10;
    return -1;
}
struct Hash {
    mbedtls_sha256_context context;
    Hash() { mbedtls_sha256_init(&context); }
    ~Hash() { mbedtls_sha256_free(&context); }
    bool begin() { return mbedtls_sha256_starts(&context,0)==0; }
    bool add(const uint8_t* p,size_t n) { return mbedtls_sha256_update(&context,p,n)==0; }
    bool matches(const uint8_t* expected) {
        uint8_t hash[32];
        return mbedtls_sha256_finish(&context,hash)==0 && memcmp(hash,expected,sizeof hash)==0;
    }
};
bool receive(uint8_t* dst,size_t n,Ms since,Ms& last) {
    size_t received=0;
    while (received<n) {
        const auto now=monoNow();
        if (now-since>=totalTime || now-last>=idleTime) return false;
        const auto available=Serial.available();
        if (!available) { delay(1); continue; }
        const size_t count=Serial.read(dst+received,std::min(n-received,size_t(available)));
        if (count) { received+=count; last=monoNow(); }
    }
    return true;
}
bool markerPresent(const uint8_t* p,size_t n,size_t& cursor) {
    constexpr char marker[]="MG-LAB-USB-PROTOCOL-1-N01";
    for (size_t i=0;i<n;++i) {
        if (p[i]==uint8_t(marker[cursor])) {
            if (++cursor==sizeof marker-1) return true;
        } else cursor=p[i]==uint8_t(marker[0]) ? 1 : 0;
    }
    return false;
}
alignas(4) volatile const char compatibilityMarker[]="MG-LAB-USB-PROTOCOL-1-N01";
}
const esp_partition_t* LabUsb::target() const {
    if (!healthy_ || uncertain_ || esp_flash_encryption_enabled() || esp_secure_boot_enabled()) return nullptr;
    const auto* a=esp_partition_find_first(ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_APP_OTA_0,"app0");
    const auto* b=esp_partition_find_first(ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_APP_OTA_1,"app1");
    const auto* active=esp_ota_get_running_partition();
    const auto* next=esp_ota_get_next_update_partition(active);
    if (!a || !b || a->address!=0x10000 || b->address!=0x150000 ||
        a->size!=slotBytes || b->size!=slotBytes || a->encrypted || b->encrypted ||
        !same(active,esp_ota_get_boot_partition())) return nullptr;
    esp_ota_img_states_t state;
    const auto err=esp_ota_get_state_partition(active,&state);
    if (err!=ESP_OK && err!=ESP_ERR_NOT_FOUND) return nullptr;
    if (err==ESP_OK && (state==ESP_OTA_IMG_NEW || state==ESP_OTA_IMG_PENDING_VERIFY ||
                         state==ESP_OTA_IMG_INVALID || state==ESP_OTA_IMG_ABORTED)) return nullptr;
    if ((same(active,a) && same(next,b)) || (same(active,b) && same(next,a))) return next;
    return nullptr;
}
void LabUsb::heartbeat() {
    if (compatibilityMarker[0]!='M') return;
    Serial.printf("BOOTSTRAP_USB %s uptime_ms=%llu running=%s phase=%s\n",buildId,
        static_cast<unsigned long long>(monoNow()),runningLabel(),resync_?"RESYNC":(uncertain_?"UNKNOWN":"IDLE"));
    lastBeat_=monoNow();
}
void LabUsb::begin() {
    Serial.setRxBufferSize(4096);
    Serial.begin(115200);
    booted_=monoNow();
    heartbeat();
}
bool LabUsb::maintenance() {
#ifndef MG_COMMISSION
    controller_.tick();
#endif
    if (!controller_.enterMaintenance()) { Serial.println("ERR CONTROLLER_BUSY"); return false; }
    network_.fence(); // any previous exchange reply, including a grant, becomes stale
    return true;
}
void LabUsb::fail(const char* code,size_t n) {
    Serial.printf("ERR %s received=%u\n",code,static_cast<unsigned>(n));
    resync_=true;
    lastInput_=monoNow();
}
void LabUsb::update(size_t length,const uint8_t* expected,const char* hex) {
#ifdef MG_COMMISSION
    if(!Commissioner::instance().mayBeginUpdate(length,hex)){Serial.println("ERR PINNED_TARGET_NOT_VERIFIED");return;}
#endif
    const auto* active=esp_ota_get_running_partition();
    const auto* next=target();
    if (!next) { Serial.println("ERR PARTITION_LAYOUT_OR_BOOT_PENDING"); return; }
    if (!maintenance()) return;
    const Ms start=monoNow();
    esp_ota_handle_t h;
    if (esp_ota_begin(next,length,&h)!=ESP_OK) { fail("BEGIN",0); return; }
    Serial.printf("READY chunk=%u length=%u target=%s idle_ms=10000 total_ms=300000\n",
        unsigned(block),unsigned(length),next->label);
    Serial.flush();
    size_t received=0, markerIndex=0;
    Ms last=monoNow();
    Hash input;
    const char* error=input.begin()?nullptr:"HASH_INIT";
    uint8_t buffer[block];
    while (!error && received<length) {
        const size_t n=std::min(block,length-received);
        if (!receive(buffer,n,start,last)) { error="RECEIVE_TIMEOUT"; break; }
        if (received==0 && buffer[0]!=0xe9) { error="IMAGE_MAGIC"; break; }
        if (!input.add(buffer,n) || esp_ota_write(h,buffer,n)!=ESP_OK) { error="WRITE"; break; }
        received+=n;
        Serial.printf("ACK %u\n",unsigned(received));
    }
    if (!error && !input.matches(expected)) error="SHA256";
    if (!error && Serial.available()) error="EXTRA_BYTES";
    if (error) {
        esp_ota_abort(h);
        fail(error,received);
        return;
    }
    if (esp_ota_end(h)!=ESP_OK) { fail("IMAGE_VALIDATE",received); return; } // does not select boot slot

    Hash readback;
    if (!readback.begin()) { fail("FLASH_HASH_INIT",received); return; }
    bool compatible=false;
    for (size_t at=0;at<length;) {
        const auto n=std::min(block,length-at);
        if (monoNow()-start>=totalTime || esp_partition_read(next,at,buffer,n)!=ESP_OK || !readback.add(buffer,n)) {
            fail("FLASH_READBACK",received); return;
        }
        if (!compatible) compatible=markerPresent(buffer,n,markerIndex);
        at+=n;
        delay(1);
    }
    if (!readback.matches(expected)) { fail("FLASH_SHA256",received); return; }
    if (!compatible) {
#ifdef MG_COMMISSION
        if(!Commissioner::instance().approvedTarget(length,hex)){fail("PINNED_TARGET_NOT_VERIFIED",received);return;}
#else
        fail("LAB_PROFILE_MARKER_REQUIRED",received); return;
#endif
    }
    if (Serial.available()) { fail("EXTRA_BYTES",received); return; }
    if (!same(target(),next) || !same(esp_ota_get_running_partition(),active)) {
        fail("PARTITION_CHANGED",received); return;
    }
    if (esp_ota_set_boot_partition(next)!=ESP_OK) {
        uncertain_=true; fail("ACTIVATION_UNKNOWN",received); return;
    }
    Serial.printf("OK length=%u sha256=%s from=%s target=%s restarting=1\n",unsigned(length),hex,active->label,next->label);
    Serial.flush(); delay(200); ESP.restart();
}
void LabUsb::provision(size_t length) {
    if (!maintenance()) return;
    std::unique_ptr<uint8_t[]> data(new(std::nothrow) uint8_t[length]);
    if (!data) { controller_.leaveMaintenance(); Serial.println("ERR PROVISION_RESOURCE"); return; }
    Serial.println("PROVISION_READY"); Serial.flush();
    const Ms start=monoNow(); Ms last=start;
    if (!receive(data.get(),length,start,last) || Serial.available()) {
        mbedtls_platform_zeroize(data.get(),length);
        fail("PROVISION_FRAME",0);
        return;
    }
    const auto result=saveProvision(reinterpret_cast<const char*>(data.get()),length);
    mbedtls_platform_zeroize(data.get(),length);
    if (result==ProvisionStatus::ok) {
        // Keep OPEN blocked until reboot loads the new profile. UART still
        // serves PING, INFO and REBOOT. No credentials in the receipt.
        Serial.println("PROVISION_OK reboot_required=1");
    } else {
        controller_.leaveMaintenance();
        Serial.println(result==ProvisionStatus::device_change?"ERR PROVISION_BINDING_CHANGE":
            result==ProvisionStatus::invalid?"ERR PROVISION_INVALID":"ERR PROVISION_STORAGE");
    }
}
void LabUsb::command() {
#ifdef MG_COMMISSION
    if(Commissioner::instance().command(line_))return;
    if(!strncmp(line_,"PROVISION ",10)){Serial.println("ERR COMMISSIONER_USE_PINNED_PLAN");return;}
#endif
    if (!strcmp(line_,"PING")) { Serial.printf("PONG BOOTSTRAP_USB %s\n",buildId); return; }
    if (!strcmp(line_,"NETINFO")) {
        if (WiFi.status()!=WL_CONNECTED) Serial.println("NETINFO LINK_PENDING");
        else {
            const auto ip=WiFi.localIP();
            if (ip==IPAddress()) Serial.println("NETINFO LINK_PENDING");
            else Serial.printf("NETINFO %u.%u.%u.%u\n",unsigned(ip[0]),unsigned(ip[1]),unsigned(ip[2]),unsigned(ip[3]));
        }
        return;
    }
    if (!strcmp(line_,"STORAGE")) {
        if (!controller_.idle() || controller_.snapshot().on) { Serial.println("ERR CONTROLLER_BUSY"); return; }
        // Same control executor, no active session and no START while reading.
        Serial.printf("STORAGE %s\n",file_.checkLayout());
        return;
    }
    if (!strcmp(line_,"INFO")) {
        const auto* p=target();
        Serial.printf("INFO BOOTSTRAP_USB %s protocol=1 baud=115200 running=%s target=%s slot_bytes=%u chunk=%u idle_ms=10000 total_ms=300000\n",
            buildId,runningLabel(),p?p->label:"BLOCKED",unsigned(slotBytes),unsigned(block));
        return;
    }
    if (!strcmp(line_,"DIAG")) {
        const auto s=controller_.snapshot();
        Serial.printf("DIAG profile=lab build=%s phase=%s ready=%u output=%s reason=%s time=%s queue=%u journal=%lu gaps=%llu network=%s heap=%u min_heap=%u wifi_status=%d disconnect_reason=%u\n",
            buildId,uncertain_?"UNKNOWN":controller_.maintenance()?"MAINTENANCE":"IDLE",s.ready,
            s.on?"virtual_on":"off",s.blocked==Reason::none?"NONE":label(s.blocked),label(s.time.quality),
            unsigned(controller_.queued()),static_cast<unsigned long>(controller_.logBytes()),
            static_cast<unsigned long long>(controller_.evidenceGaps()),network_.status(),
            unsigned(ESP.getFreeHeap()),unsigned(ESP.getMinFreeHeap()),network_.wifiStatus(),network_.disconnectReason());
        return;
    }
    if (uncertain_) { Serial.println("ERR ACTIVATION_UNKNOWN"); return; }
    if (!strcmp(line_,"REBOOT")) {
        if (!controller_.maintenance() && !maintenance()) return;
        Serial.printf("REBOOTING BOOTSTRAP_USB %s\n",buildId);
        Serial.flush(); delay(200); ESP.restart();
        return;
    }
    size_t n;
    if (!strncmp(line_,"PROVISION ",10)) {
        const char* p=line_+10;
        if (decimal(p,provisionMax,n) && n && !*p) provision(n);
        else Serial.println("ERR PROVISION_LENGTH");
        return;
    }
    if (!strncmp(line_,"BEGIN ",6)) {
        const char* p=line_+6;
        if (!decimal(p,slotBytes,n) || n<32 || *p!=' ' || strlen(++p)!=64) {
            Serial.println("ERR COMMAND_OR_BEGIN"); return;
        }
        char hex[65]{};
        uint8_t sha[32];
        for (size_t i=0;i<32;++i) {
            const int hi=hexDigit(p[i*2]),lo=hexDigit(p[i*2+1]);
            if (hi<0 || lo<0) { Serial.println("ERR COMMAND_OR_BEGIN"); return; }
            sha[i]=uint8_t(16*hi+lo);
            hex[i*2]="0123456789abcdef"[hi]; hex[i*2+1]="0123456789abcdef"[lo];
        }
        update(n,sha,hex);
        return;
    }
    Serial.println("ERR COMMAND_OR_BEGIN");
}
void LabUsb::visit() {
    const auto now=monoNow();
    if (!healthy_ && now-booted_>=1500) {
        esp_ota_img_states_t state;
        const auto code=esp_ota_get_state_partition(esp_ota_get_running_partition(),&state);
        healthy_=(code==ESP_ERR_NOT_FOUND || (code==ESP_OK && (state!=ESP_OTA_IMG_PENDING_VERIFY ||
            esp_ota_mark_app_valid_cancel_rollback()==ESP_OK)));
    }
    if (resync_) {
        for (int i=0;i<4096 && Serial.available();++i) { Serial.read(); lastInput_=monoNow(); }
        if (now-lastInput_>=quietTime && !Serial.available()) {
            resync_=false; length_=0; discard_=false;
            if (!uncertain_) controller_.leaveMaintenance();
            Serial.println("RESYNC READY");
        }
    } else {
        for (int i=0;i<128 && Serial.available() && !resync_;++i) {
            const auto byte=Serial.read();
            lastInput_=monoNow();
            if (byte=='\n') {
                if (discard_) Serial.println("ERR COMMAND_LINE");
                else {
                    if (length_ && line_[length_-1]=='\r') --length_;
                    line_[length_]=0; if (length_) command();
                }
                length_=0; discard_=false;
            } else if (!discard_) {
                if ((byte<32 && byte!='\r') || byte>126 || length_+1>=sizeof line_) discard_=true;
                else line_[length_++]=char(byte);
            }
        }
        if ((length_ || discard_) && monoNow()-lastInput_>=idleTime) {
            length_=0; discard_=false; Serial.println("ERR COMMAND_TIMEOUT");
        }
    }
    if (monoNow()-lastBeat_>=second) heartbeat();
}
}
#endif
