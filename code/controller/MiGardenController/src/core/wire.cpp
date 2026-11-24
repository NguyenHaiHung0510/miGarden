#include "wire.hpp"
#include <ArduinoJson.h>

namespace mg {
namespace {
using Obj = JsonObjectConst;
using Var = JsonVariantConst;
template<class Destination> void putText(Destination dst,std::string_view s) {
    if (s.empty()) dst.set(nullptr); else dst.set(s);
}
template<class Destination> void putUtc(Destination dst, Ms t) {
    char b[32]; formatUtc(t,b);
    if (b[0]) dst.set(std::string_view(b)); else dst.set(nullptr);
}
bool str(Var v,std::string_view& out) {
    if (!v.is<JsonString>()) return false;
    const auto s=v.as<JsonString>();
    out={s.c_str(),s.size()}; return out.find('\0')==out.npos;
}
bool equal(Var v,std::string_view value) {
    std::string_view s; return str(v,s) && s==value;
}
template<size_t N> bool id(Var v,Text<N>& out) {
    std::string_view s; return str(v,s) && visible(s) && out.set(s);
}
bool utc(Var v,Ms& t) { std::string_view s; return str(v,s) && parseUtc(s,t); }
bool match(Obj o) { return equal(o["zone_id"],zone) && equal(o["binding_ref"],binding); }
bool hhmm(Var v,uint16_t& out) {
    std::string_view s;
    if (!str(v,s) || s.size()!=5 || s[2]!=':') return false;
    for (size_t i:{0,1,3,4}) if (s[i]<'0'||s[i]>'9') return false;
    const int hh=(s[0]-'0')*10+s[1]-'0', mm=(s[3]-'0')*10+s[4]-'0';
    if (hh>23 || mm>59) return false;
    out=uint16_t(hh*60+mm);
    return true;
}
template<class T> bool integer(Var v,T& out) {
    if (!v.is<T>()) return false;
    out=v.as<T>();
    return true;
}
bool config(Obj o,Config& c) {
    if (!id(o["config_ref"],c.ref) || !match(o)) return false;
    const Obj p=o["policy"], s=o["schedule"], r=s["recurrence"];
    auto& pol=c.policy; auto& sc=c.schedule;
    if (!integer(p["manual_duration_seconds"],pol.manual) || !integer(p["max_session_seconds"],pol.session) ||
        !integer(p["window_seconds"],pol.window) || !integer(p["max_window_seconds"],pol.cumulative) ||
        !s["enabled"].is<bool>() || !integer(s["duration_seconds"],sc.duration) ||
        !utc(s["effective_from"],sc.from) || !s["effective_until"].is<JsonVariantConst>() ||
        (!s["effective_until"].isNull() && !utc(s["effective_until"],sc.until))) return false;
    sc.enabled=s["enabled"].as<bool>();
    if (equal(r["kind"],"once")) {
        sc.type=Recurrence::once; sc.localMinute=0;
        if (!utc(r["at"],sc.at)) return false;
    } else if (equal(r["kind"],"interval")) {
        sc.type=Recurrence::interval; sc.localMinute=0;
        if (!utc(r["anchor_at"],sc.at) || !integer(r["interval_minutes"],sc.intervalMinutes)) return false;
    } else if (equal(r["kind"],"weekly")) {
        sc.type=Recurrence::weekly;
        const auto days=r["days_of_week"].as<JsonArrayConst>();
        if (days.isNull() || days.size()==0 || days.size()>7 || !hhmm(r["at_local_time"],sc.localMinute)) return false;
        for (Var d:days) {
            int32_t n;
            if (!integer(d,n) || n<1 || n>7 || (sc.weekdays & (1U<<(n-1)))) return false;
            sc.weekdays|=uint8_t(1U<<(n-1));
        }
    } else if (equal(r["kind"],"daily")) {
        if (!hhmm(r["at_local_time"],sc.localMinute)) return false;
    } else return false;
    return valid(c);
}
bool intent(Obj o,Intent& i) {
    i.target=match(o);
    return equal(o["kind"],"start") && id(o["request_id"],i.request) && id(o["config_ref"],i.config) &&
        id(o["boot_id"],i.boot) && integer(o["control_epoch"],i.epoch) &&
        utc(o["confirmed_at"],i.confirmed) && utc(o["expires_at"],i.expires) &&
        integer(o["duration_seconds"],i.duration);
}
bool stop(Obj o,Stop& d) {
    d.target=match(o);
    return equal(o["kind"],"stop") && id(o["request_id"],d.request) && id(o["boot_id"],d.boot) &&
        utc(o["confirmed_at"],d.confirmed);
}
void snapshot(JsonObject o,const Snapshot& s) {
    putUtc(o["observed_at"],s.time.usable() ? s.time.wall : none);
    o["time_quality"]=label(s.time.usable() ? s.time.quality : Quality::unknown);
    o["output"]=s.on?"on":"off";
    if (s.active.phase==Session::Phase::running) {
        auto a=o["active_session"].to<JsonObject>();
        a["session_id"]=s.active.id.view();
        a["request_id"]=s.active.request.view();
        a["trigger_source"]=label(s.active.source);
        putUtc(a["started_at"],s.active.started);
        a["duration_seconds"]=s.active.duration;
        a["elapsed_seconds"]=s.elapsedSeconds;
        a["config_ref"]=s.active.config.view();
    } else o["active_session"]=nullptr;
    putText(o["applied_config_ref"],s.applied.view());
    o["ready"]=s.ready;
    putText(o["blocked_reason"],label(s.blocked));
}
void event(JsonObject o,const Event& e) {
    o["event_id"]=eventId(e).view();
    o["boot_id"]=e.boot.view(); o["sequence"]=e.sequence;
    o["kind"]=label(e.kind);
    putUtc(o["occurred_at"],e.occurred); o["time_quality"]=label(e.quality);
    o["zone_id"]=zone; o["binding_ref"]=binding; putText(o["config_ref"],e.config.view());
    if (!e.request.empty()) o["request_id"]=e.request.view();
    if (!e.session.empty()) o["session_id"]=e.session.view();
    if (e.source!=Source::none) o["trigger_source"]=label(e.source);
    auto p=o["payload"].to<JsonObject>();
    if (e.reason!=Reason::none) p["reason"]=label(e.reason);
    switch (e.kind) {
        case Kind::session_started:
            putUtc(p["started_at"],e.started); p["duration_seconds"]=e.plannedSeconds; break;
        case Kind::session_ended:
            putUtc(p["started_at"],e.started); putUtc(p["ended_at"],e.ended);
            if (e.outputMs==none) p["output_duration_seconds"]=nullptr;
            // Wire history uses whole seconds; journal keeps millisecond overlap.
            else p["output_duration_seconds"]=e.outputMs/second;
            p["outcome"]=label(e.outcome); break;
        case Kind::stop_processed: p["outcome"]=label(e.outcome); break;
        case Kind::occurrence_decided:
            p["occurrence_id"]=occurrenceId(e.occurrence).view();
            putUtc(p["scheduled_at"],e.occurrence); p["disposition"]=label(e.outcome); break;
        default: break;
    }
}
}

WireError decodeExchange(std::string_view body,Exchange& result) {
    if (body.empty() || body.size()>responseBytes) return WireError::bounds;
    // Reject an embedded NUL and anything after the final JSON object. ArduinoJson
    // expects exactly one value via its stream API, so count what it consumed.
    struct Cursor {
        std::string_view input;
        size_t pos=0;
        int read() { return pos<input.size() ? uint8_t(input[pos++]) : -1; }
        int peek() { return pos<input.size() ? uint8_t(input[pos]) : -1; }
        size_t readBytes(char* dst,size_t n) {
            if (n>input.size()-pos) n=input.size()-pos;
            std::memcpy(dst,input.data()+pos,n); pos+=n; return n;
        }
    } cursor{body};
    if (body.find('\0')!=body.npos) return WireError::syntax;
    JsonDocument doc;
    const auto error=deserializeJson(doc,cursor,DeserializationOption::NestingLimit(8));
    if (error) return error==DeserializationError::NoMemory ? WireError::bounds : WireError::syntax;
    for (size_t i=cursor.pos;i<body.size();++i)
        if (body[i]!=' ' && body[i]!='\t' && body[i]!='\r' && body[i]!='\n') return WireError::syntax;
    const auto root=doc.as<Obj>();
    if (root.isNull()) return WireError::shape;
    if (equal(root["protocol"],"migarden.wait.v1")) {
        if (root.size()!=2 || !root["retry_after_seconds"].is<int>() ||
            root["retry_after_seconds"].as<int>()!=5) return WireError::shape;
        Exchange x; x.waiting=true; x.retryAfter=5;
        result=std::move(x); return WireError::none;
    }
    if (!equal(root["protocol"],protocol)) return WireError::protocol_version;
    for (auto key:{"desired_config","retry_after_seconds"}) if (!root[key].is<JsonVariantConst>()) return WireError::shape;
    const auto cs=root["candidates"].as<JsonArrayConst>(), gs=root["grants"].as<JsonArrayConst>(),
               ss=root["stops"].as<JsonArrayConst>(), ack=root["acked_event_ids"].as<JsonArrayConst>();
    if (cs.isNull() || gs.isNull() || ss.isNull() || ack.isNull()) return WireError::shape;
    if (cs.size()>remoteBatchMax || gs.size()>remoteBatchMax || ss.size()>remoteBatchMax || ack.size()>batchMax)
        return WireError::bounds;
    Exchange x;
    x.quietHealth=equal(root["extension"],"quiet-health-v1");
    const bool timing=utc(root["server_time"],x.serverTime);
    // A valid STOP can still close even when a different action or desired bundle
    // is malformed. Malformed OPEN data has no path into the arbiter.
    for (Var v:ss) { Stop item; if (stop(v.as<Obj>(),item)) x.stops.push_back(item); }
    if (timing) {
        for (Var v:cs) { Intent item; if (intent(v.as<Obj>(),item)) x.candidates.push_back(item); }
        for (Var v:gs) {
            Grant item;
            if (intent(v.as<Obj>(),item.start) && id(v["grant_id"],item.id)) x.grants.push_back(item);
        }
    }
    for (Var v:ack) { EventId item; if (id(v,item)) x.acked.push_back(item); }
    if (!root["desired_config"].isNull()) {
        x.hasConfig=true;
        x.invalidConfig=!config(root["desired_config"].as<Obj>(),x.config);
    }
    if (!root["retry_after_seconds"].isNull() &&
        (!integer(root["retry_after_seconds"],x.retryAfter) || x.retryAfter<0)) x.retryAfter=-1;
    result=std::move(x);
    return WireError::none;
}

bool encodeExchange(const Id& device,const BootId& boot,const Snapshot& s,const std::vector<Event>& reports,std::string& out,const Health* health) {
    if (!visible(device.view()) || !visible(boot.view()) || reports.size()>batchMax) return false;
    JsonDocument doc;
    doc["protocol"]=protocol; doc["device_id"]=device.view(); doc["boot_id"]=boot.view();
    doc["control_epoch"]=s.epoch;
    snapshot(doc["snapshot"].to<JsonObject>(),s);
    auto a=doc["reports"].to<JsonArray>();
    for (auto& e:reports) event(a.add<JsonObject>(),e);
    if (health) {
        auto h=doc["health"].to<JsonObject>();
        h["build_id"]=health->build; h["boot_id"]=boot.view();
        putUtc(h["sampled_at"],s.time.usable()?s.time.wall:none);
        h["time_quality"]=label(s.time.usable()?s.time.quality:Quality::unknown);
        h["uptime_seconds"]=health->uptime;
        h["reset_reason"]=health->resetReason;
        h["free_heap"]=health->freeHeap; h["minimum_heap"]=health->minimumHeap;
        h["wifi_phase"]=health->wifi; h["sync_status"]=health->sync;
        h["pending_reports"]=health->pending; h["evidence_gaps"]=health->gaps;
    }
    const size_t n=measureJson(doc);
    if (doc.overflowed() || n>requestBytes) return false;
    out.clear(); out.reserve(n);
    return serializeJson(doc,out)==n;
}

std::string encodeSnapshot(const Snapshot& s) {
    JsonDocument doc;
    snapshot(doc.to<JsonObject>(),s);
    std::string out;
    serializeJson(doc,out);
    return out;
}
}
