#include "network.hpp"
#include <WiFi.h>
#include <esp_http_client.h>
#include <esp_log.h>
#include <esp_random.h>
#include <esp_sntp.h>
#include <esp_tls.h>
#include <memory>
#include <new>

namespace mg::esp {
namespace {
enum State { unprovisioned, wifi_pending, clock_pending, idle, sending, transient_error, auth_error,
             tls_error, wire_error, serial_conflict, queue_error };
struct Result {
    std::string body;
    int32_t retry=-1;
    int status=0;
    Ms deadline=0;
    bool overflow=false, tls=false;
};
esp_err_t event(esp_http_client_event_t* ev) {
    auto& r=*static_cast<Result*>(ev->user_data);
    if (monoNow()>=r.deadline) { r.overflow=true; return ESP_FAIL; }
    if (ev->event_id==HTTP_EVENT_ON_DATA && ev->data_len>0) {
        if (size_t(ev->data_len)>responseBytes-r.body.size()) {
            r.overflow=true;
            return ESP_FAIL;
        }
        r.body.append(static_cast<const char*>(ev->data),ev->data_len);
    }
    if (ev->event_id==HTTP_EVENT_ON_HEADER && ev->header_key && ev->header_value &&
        !strcasecmp(ev->header_key,"Retry-After")) {
        int32_t n=0;
        const char* p=ev->header_value;
        const char* begin=p;
        for (; *p && *p>='0' && *p<='9'; ++p) {
            if (n>(INT32_MAX-(*p-'0'))/10) { n=INT32_MAX; while (*p>='0'&&*p<='9') ++p; break; }
            n=n*10+(*p-'0');
        }
        if (p>begin && !*p) r.retry=n;
        else if (*begin) {
            tm parsed{};
            const char* end=strptime(begin,"%a, %d %b %Y %H:%M:%S GMT",&parsed);
            if (end && !*end) {
                char utc[32]{};
                if (strftime(utc,sizeof utc,"%Y-%m-%dT%H:%M:%SZ",&parsed)) {
                    Ms at;
                    const auto now=time(nullptr);
                    if (parseUtc(utc,at) && at/second>now)
                        r.retry=int32_t(std::min<int64_t>(INT32_MAX,at/second-now));
                }
            }
        }
    }
    if (ev->event_id==HTTP_EVENT_DISCONNECTED) {
        int esp=0, tls=0, flags=0;
        if (ev->data) esp=esp_tls_get_and_clear_last_error(static_cast<esp_tls_error_handle_t>(ev->data),&tls,&flags);
        if (flags || esp==ESP_ERR_MBEDTLS_X509_CRT_PARSE_FAILED) r.tls=true;
    }
    return ESP_OK;
}
Result exchangeHttps(const Provision& profile,const std::string& payload) {
    auto url=std::string(profile.baseUrl.view());
    if (url.back()=='/') url.pop_back();
    url+=exchangePath;
    Result r;
    r.deadline=monoNow()+8*second;
    r.body.reserve(2048);
    esp_http_client_config_t cfg{};
    cfg.url=url.c_str(); cfg.method=HTTP_METHOD_POST; cfg.timeout_ms=3500;
    cfg.disable_auto_redirect=true; cfg.max_authorization_retries=-1;
    cfg.cert_pem=profile.ca.c_str(); cfg.cert_len=profile.ca.size()+1;
    cfg.skip_cert_common_name_check=false;
    cfg.buffer_size=2048; cfg.buffer_size_tx=2048;
    cfg.event_handler=event; cfg.user_data=&r;
    auto h=esp_http_client_init(&cfg);
    if (!h) return r;
    std::string auth="Bearer ";
    auth+=profile.token.view();
    const bool configured=esp_http_client_set_header(h,"Authorization",auth.c_str())==ESP_OK &&
        esp_http_client_set_header(h,"Content-Type","application/json")==ESP_OK &&
        esp_http_client_set_header(h,"Accept","application/json")==ESP_OK &&
        esp_http_client_set_header(h,"Connection","close")==ESP_OK &&
        esp_http_client_set_post_field(h,payload.data(),payload.size())==ESP_OK;
    const auto code=configured?esp_http_client_perform(h):ESP_FAIL;
    int tls=0, flags=0;
    const int transport=esp_http_client_get_and_clear_last_tls_error(h,&tls,&flags);
    if (flags || transport==ESP_ERR_MBEDTLS_X509_CRT_PARSE_FAILED) r.tls=true;
    if (code==ESP_OK && !r.overflow && monoNow()<r.deadline &&
        esp_http_client_is_complete_data_received(h)) r.status=esp_http_client_get_status_code(h);
    esp_http_client_cleanup(h);
    std::fill(auth.begin(),auth.end(),'\0');
    return r;
}
}
struct Network::Job {
    std::unique_ptr<Reply> reply;
    std::string request;
};
bool Network::begin(Provision&& p) {
    provision_=std::move(p);
    outbound_=xQueueCreate(1,sizeof(Job*)); inbound_=xQueueCreate(1,sizeof(Reply*));
    const bool ok=outbound_ && inbound_ && xTaskCreatePinnedToCore(run,"mg-net",11264,this,1,nullptr,0)==pdPASS;
    status_=ok?wifi_pending:queue_error;
    return ok;
}
void Network::run(void* arg) { static_cast<Network*>(arg)->work(); }
void Network::work() {
    // IDF's resolver can print the private hostname on an error. Emit only
    // the redacted adapter status (AUTH_REJECTED, TLS_VERIFY, RETRY, ...).
    for (const char* tag:{"esp-tls","esp-tls-mbedtls","transport_base","HTTP_CLIENT","wifi"})
        esp_log_level_set(tag,ESP_LOG_NONE);
    WiFi.persistent(false);
    WiFi.onEvent([this](arduino_event_id_t event, arduino_event_info_t info) {
        if (event==ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
            disconnects_.fetch_add(1);
#ifdef MG_LAB
            disconnectReason_.store(info.wifi_sta_disconnected.reason);
#else
            (void)info;
#endif
        }
    },ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    WiFi.disconnect(false,false); // no erase; all station config stays RAM-only
    sntp_set_time_sync_notification_cb(WallClock::synchronized);
    WifiPolicy wifiPolicy(provision_.hasFallback);
    uint32_t observedDisconnects=disconnects_.load();
    bool ntp=false, link=false;
    for (;;) {
        const Ms now=monoNow();
        bool connected=WiFi.status()==WL_CONNECTED;
        const auto disconnects=disconnects_.load();
        const auto action=wifiPolicy.step(now,connected,disconnects!=observedDisconnects);
        observedDisconnects=disconnects;
#ifdef MG_LAB
        wifiStatus_.store(int(WiFi.status()));
#endif
        if(action!=WifiPolicy::Action::none) {
            WiFi.disconnect(false,false); // async cancel, never erase or rewrite profile
            if(action==WifiPolicy::Action::preferred)
                WiFi.begin(provision_.ssid.c_str(),provision_.password.c_str());
            else if(action==WifiPolicy::Action::fallback)
                WiFi.begin(provision_.fallbackSsid.c_str(),provision_.fallbackPassword.c_str());
            connected=false;
        }
        connected=connected&&wifiPolicy.phase()==WifiPolicy::Phase::connected;
        wifiSlot_=wifiPolicy.slot();wifiPhase_=wifiPolicy.phase();
        if (connected && !ntp) {
            configTime(0,0,"pool.ntp.org","time.google.com","time.cloudflare.com");
            ntp=true;
        }
        WallClock::connected(connected);
        wifi_=connected;
        if (!connected && link && status_<auth_error) status_=wifi_pending;
        link=connected;
        Job* raw=nullptr;
        if (xQueueReceive(outbound_,&raw,pdMS_TO_TICKS(100))!=pdPASS) continue;
        std::unique_ptr<Job> job(raw);
        auto reply=std::move(job->reply);
        if (connected) {
            auto r=exchangeHttps(provision_,job->request);
            reply->exchange.retryAfter=r.retry;
            if (r.status==200) {
                reply->accepted=decodeExchange(r.body,reply->exchange)==WireError::none;
                if (r.retry>=0) reply->exchange.retryAfter=std::max(r.retry,reply->exchange.retryAfter);
                status_=reply->accepted?idle:wire_error;
            } else if (r.status==401 || r.status==403) status_=auth_error;
            else if (r.status==409) status_=serial_conflict;
            else if (r.tls) status_=tls_error;
            else if ((r.status>=300 && r.status<400) || (r.status>=400 && r.status<500 && r.status!=429 &&
                    r.status!=408)) status_=wire_error;
            else status_=transient_error;
        } else status_=wifi_pending;
        auto* p=reply.get();
        if (xQueueSend(inbound_,&p,0)==pdPASS) reply.release(); else status_=queue_error;
    }
}
Network::Reply* Network::take() {
    Reply* p=nullptr;
    if (!inbound_ || xQueueReceive(inbound_,&p,0)!=pdPASS) return nullptr;
    inFlight_=false;
    if (p->generation!=generation_) { delete p; return nullptr; }
    return p;
}
void Network::after(const Reply& r,Controller& c) {
    if (r.accepted) {
        // If old backlog remains or a candidate just caused a durable receipt,
        // follow immediately; a stable duplicate/no-ACK reply uses normal cadence.
        policy_.ok(monoNow(),!c.idle(),(!r.exchange.acked.empty() && c.queued()) ||
                   (!r.exchange.candidates.empty() && c.change()!=lastSubmitted_),r.exchange.retryAfter);
    } else if (status_>=auth_error) policy_.permanent();
    else policy_.transient(monoNow(),esp_random(),r.exchange.retryAfter);
}
void Network::submit(Controller& c) {
    if (!outbound_ || inFlight_ || c.maintenance()) return;
    const auto s=c.snapshot();
    if (!s.time.usable()) {
        if (wifi_ && status_<auth_error) status_=clock_pending;
        return; // no TLS until trustworthy date
    }
    if (!policy_.due(s.time.mono,wifi_,c.change())) return;
    auto job=std::unique_ptr<Job>(new(std::nothrow) Job);
    if (job) job->reply.reset(new(std::nothrow) Reply);
    if (!job || !job->reply) { status_=queue_error; policy_.permanent(); return; }
    const auto records=c.reports();
    if (!encodeExchange(provision_.device,c.bootId(),c.snapshot(),records,job->request)) {
        status_=wire_error; policy_.permanent(); return;
    }
    for (const auto& e:records) job->reply->sent.push_back(eventId(e));
    job->reply->generation=generation_;
    auto* p=job.get();
    if (xQueueSend(outbound_,&p,0)!=pdPASS) { status_=queue_error; policy_.permanent(); return; }
    job.release();
    status_=sending;
    lastSubmitted_=c.change();
    policy_.sent(lastSubmitted_);
    inFlight_=true;
}
const char* Network::status() const {
    constexpr const char* labels[]={"UNPROVISIONED","WIFI_PENDING","CLOCK_PENDING","OK","SENDING","RETRY",
        "AUTH_REJECTED","TLS_VERIFY","WIRE_REJECTED","REPORT_CONFLICT","QUEUE_FAULT"};
    return labels[status_.load()];
}
}
