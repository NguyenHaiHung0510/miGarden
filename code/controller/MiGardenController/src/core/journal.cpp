#include "journal.hpp"
#include <algorithm>
#include <utility>

namespace mg {
namespace {
using Bytes = std::vector<uint8_t>;
constexpr uint32_t magic = 0x314a474d; // MGJ1, explicitly little endian
constexpr size_t headerSize = 8, trailerSize = 4;

class Writer {
public:
    Bytes b;
    void u(uint64_t n, unsigned bytes) {
        for (unsigned i = 0; i < bytes; ++i) b.push_back(uint8_t(n >> (8*i)));
    }
    void time(Ms n) { u(uint64_t(n), 8); } // -1 is the only negative persisted value
    void text(std::string_view s) {
        u(s.size(), 1);
        if (!s.empty()) b.insert(b.end(), s.begin(), s.end());
    }
    void add(const Bytes& other) { b.insert(b.end(), other.begin(), other.end()); }
};
class Reader {
public:
    Reader(const uint8_t* data, size_t n) : p(data), end(data+n) {}
    const uint8_t* p;
    const uint8_t* end;
    bool okay = true;
    size_t left() const { return size_t(end-p); }
    uint64_t u(unsigned bytes) {
        if (bytes > left()) { okay = false; return 0; }
        uint64_t n = 0;
        for (unsigned i = 0; i < bytes; ++i) n |= uint64_t(*p++) << (8*i);
        return n;
    }
    Ms time() {
        const auto n = u(8);
        if (n == UINT64_MAX) return none;
        if (n > uint64_t(maxUtc)) { okay = false; return none; }
        return Ms(n);
    }
    template<class T> T en(T last) {
        const auto x = u(1);
        if (x > uint64_t(last)) { okay = false; return T{}; }
        return T(x);
    }
    bool boolean() {
        const auto val = u(1);
        if (val > 1) okay = false;
        return val == 1;
    }
    template<size_t N> void text(Text<N>& out) {
        const auto n = u(1);
        if (n > left() || !out.set({reinterpret_cast<const char*>(p), size_t(n)}) ||
            (!out.empty() && !visible(out.view()))) { okay = false; return; }
        p += n;
    }
};
uint32_t checksum(const uint8_t* data, size_t n) {
    uint32_t c = ~uint32_t{0};
    for (size_t i = 0; i < n; ++i) {
        c ^= data[i];
        for (int k = 0; k < 8; ++k) c = (c >> 1) ^ ((c & 1) ? 0xedb88320U : 0);
    }
    return ~c;
}
uint32_t word(const uint8_t* data) { Reader r(data,4); return uint32_t(r.u(4)); }
Bytes encodeEvent(const Event&);
bool decodeEvent(const uint8_t*, size_t, Event&, bool deferred = false);

void writeConfig(Writer& w, const Config& c) {
    w.text(c.ref.view());
    w.u(c.policy.manual,4); w.u(c.policy.session,4); w.u(c.policy.window,4); w.u(c.policy.cumulative,4);
    const auto& s = c.schedule;
    w.u(s.enabled,1); w.u(s.duration,4); w.time(s.from); w.time(s.until);
    w.u(uint8_t(s.type),1); w.u(s.localMinute,2); w.u(s.weekdays,1);
    w.time(s.at); w.u(s.intervalMinutes,4);
}
void readConfig(Reader& r, Config& c) {
    r.text(c.ref);
    c.policy.manual=int32_t(r.u(4)); c.policy.session=int32_t(r.u(4));
    c.policy.window=int32_t(r.u(4)); c.policy.cumulative=int32_t(r.u(4));
    auto& s = c.schedule;
    s.enabled=r.boolean(); s.duration=int32_t(r.u(4)); s.from=r.time(); s.until=r.time();
    s.type=r.en(Recurrence::interval); s.localMinute=uint16_t(r.u(2)); s.weekdays=uint8_t(r.u(1));
    s.at=r.time(); s.intervalMinutes=uint32_t(r.u(4));
}
void writeIntent(Writer& w, const Intent& i) {
    w.text(i.request.view()); w.text(i.config.view()); w.text(i.boot.view()); w.u(i.epoch,4);
    w.time(i.confirmed); w.time(i.expires); w.u(i.duration,4); w.u(i.target,1);
}
void readIntent(Reader& r, Intent& i) {
    r.text(i.request); r.text(i.config); r.text(i.boot); i.epoch=uint32_t(r.u(4));
    i.confirmed=r.time(); i.expires=r.time(); i.duration=int32_t(r.u(4)); i.target=r.boolean();
}
Bytes encodeState(const Ledger& d) {
    Writer w;
    w.u(1,1);
    w.u(d.sequence,8); w.time(d.scheduleFloor); w.time(d.accountingFloor); w.u(d.unknown,1);
    writeConfig(w, d.applied);
    const auto& s=d.session;
    w.u(uint8_t(s.phase),1); w.text(s.id.view()); w.text(s.request.view()); w.text(s.config.view());
    w.u(uint8_t(s.source),1); w.u(s.duration,4); w.time(s.occurrence); w.time(s.started); w.time(s.accountedAt);
    w.u(d.pending.receivedSequence,8);
    writeIntent(w,d.pending.intent);
    w.u(d.history.size(),2);
    for (const auto& i : d.history) { w.time(i.start); w.time(i.end); }
    w.u(d.configRefs.size(),2);
    for (auto h : d.configRefs) w.u(h,8);
    if (!d.deferredStops.empty()) {
        w.u(0x53,1); w.u(d.deferredStops.size(),1);
        for (const auto& e : d.deferredStops) {
            const auto data=encodeEvent(e);
            w.u(data.size(),2); w.add(data);
        }
    }
    return std::move(w.b);
}
bool decodeState(const uint8_t* data, size_t bytes, Ledger& d) {
    Reader r(data,bytes);
    if (r.u(1)!=1) return false;
    d.sequence=r.u(8); d.scheduleFloor=r.time(); d.accountingFloor=r.time(); d.unknown=r.boolean();
    readConfig(r,d.applied);
    if (!d.applied.ref.empty() && !valid(d.applied)) return false;
    auto& s = d.session;
    s.phase=r.en(Session::Phase::unresolved); r.text(s.id); r.text(s.request); r.text(s.config);
    s.source=r.en(Source::remote); s.duration=int32_t(r.u(4)); s.occurrence=r.time();
    s.started=r.time(); s.accountedAt=r.time();
    if (s.phase != Session::Phase::idle && (s.id.empty() || s.request.empty() || s.config.empty() ||
        s.source == Source::none || !durationValid(s.duration) || s.duration > windowSeconds ||
        s.accountedAt < 0 || d.accountingFloor < s.accountedAt ||
        (s.phase == Session::Phase::running && s.started < 0) ||
        (s.phase == Session::Phase::unresolved && !d.unknown))) return false;
    d.pending.receivedSequence=r.u(8);
    readIntent(r,d.pending.intent);
    const auto& i=d.pending.intent;
    if (d.pending.receivedSequence && (d.pending.receivedSequence > d.sequence || i.request.empty() ||
        i.config.empty() || i.boot.empty() || i.confirmed < 0 ||
        i.expires-i.confirmed != ttlSeconds*second || !i.target || !durationValid(i.duration))) return false;
    const size_t intervals = r.u(2);
    if (!r.okay || intervals > intervalMax) return false;
    d.history.clear(); d.history.reserve(intervals);
    for (size_t n = 0; n < intervals; ++n) {
        const Range x{r.time(),r.time()};
        if (x.start < 0 || x.end <= x.start || x.end > d.accountingFloor ||
            (n && x.start < d.history.back().end)) return false;
        d.history.push_back(x);
    }
    if ((s.phase == Session::Phase::running || s.phase == Session::Phase::armed) &&
        (!d.history.empty() && d.history.back().end > s.accountedAt)) return false;
    const size_t refs = r.u(2);
    if (!r.okay || refs > configHistoryMax) return false;
    d.configRefs.clear(); d.configRefs.reserve(refs);
    for (size_t n=0; n<refs; ++n) d.configRefs.push_back(r.u(8));
    if ((!d.applied.ref.empty() && (d.configRefs.empty() || d.configRefs.back() != idFingerprint(d.applied.ref.view()))) ||
        (d.scheduleFloor != none && d.scheduleFloor % minute)) return false;
    d.deferredStops.clear();
    if (r.left()) {
        if (r.u(1)!=0x53) return false;
        const auto count=r.u(1);
        if (!count || count>deferredStopMax) return false;
        for (size_t n=0;n<count;++n) {
            const size_t len=r.u(2);
            Event e;
            if (len>r.left() || !decodeEvent(r.p,len,e,true) || e.sequence || e.kind!=Kind::stop_processed ||
                e.source!=Source::remote || e.request.empty() || !e.session.empty() ||
                (e.outcome!=Outcome::close_requested && e.outcome!=Outcome::already_off &&
                 e.outcome!=Outcome::unconfirmed)) return false;
            for (const auto& old : d.deferredStops)
                if (old.boot==e.boot && old.request==e.request) return false;
            d.deferredStops.push_back(e);
            r.p+=len;
        }
    }
    return r.okay && !r.left();
}
Bytes encodeEvent(const Event& e) {
    Writer w;
    w.text(e.boot.view()); w.u(e.sequence,8); w.u(uint8_t(e.kind),1); w.time(e.occurred);
    w.u(uint8_t(e.quality),1); w.text(e.config.view()); w.text(e.request.view()); w.text(e.session.view());
    w.u(uint8_t(e.source),1); w.u(uint8_t(e.reason),1); w.u(uint8_t(e.outcome),1);
    w.time(e.started); w.time(e.ended); w.time(e.occurrence); w.u(e.plannedSeconds,4); w.time(e.outputMs);
    return std::move(w.b);
}
bool decodeEvent(const uint8_t* data, size_t bytes, Event& e, bool deferred) {
    Reader r(data,bytes);
    r.text(e.boot); e.sequence=r.u(8); e.kind=r.en(Kind::occurrence_decided); e.occurred=r.time();
    e.quality=r.en(Quality::holdover); r.text(e.config); r.text(e.request); r.text(e.session);
    e.source=r.en(Source::remote); e.reason=r.en(Reason::unsafe_output); e.outcome=r.en(Outcome::rejected);
    e.started=r.time(); e.ended=r.time(); e.occurrence=r.time(); e.plannedSeconds=int32_t(r.u(4)); e.outputMs=r.time();
    return r.okay && !r.left() && (e.sequence || deferred) && e.sequence <= uint64_t(INT64_MAX) &&
        !e.boot.empty() && ((e.occurred == none) == (e.quality == Quality::unknown));
}
Bytes frame(const Bytes& state, const std::vector<Bytes>& reports, const std::vector<uint64_t>& acks) {
    Writer w;
    w.u(magic,4); w.u(0,4);
    w.u(state.size(),2); w.u(reports.size(),1); w.u(acks.size(),1); w.add(state);
    for (const auto& p : reports) { w.u(p.size(),2); w.add(p); }
    for (auto seq : acks) w.u(seq,8);
    const auto length = uint32_t(w.b.size() + trailerSize);
    for (unsigned i=0; i<4; ++i) w.b[4+i]=uint8_t(length>>(8*i));
    w.u(checksum(w.b.data(),w.b.size()),4);
    return std::move(w.b);
}
bool ackSequence(const EventId& id, uint64_t& seq) {
    const auto v=id.view();
    const auto at=v.rfind('.');
    if (at==v.npos || at+1==v.size()) return false;
    seq=0;
    for (size_t i=at+1;i<v.size();++i) {
        const char c=v[i];
        if (c<'0' || c>'9' || seq > (uint64_t(INT64_MAX)-unsigned(c-'0'))/10) return false;
        seq=seq*10+unsigned(c-'0');
    }
    return seq != 0;
}
}

std::vector<uint8_t> Journal::genesis() { return frame(encodeState(Ledger{}),{},{}); }
bool Journal::readFrame(uint32_t at, Bytes& b) {
    uint8_t head[headerSize];
    if (at > used_ || used_-at < headerSize || !file_.read(at,head,sizeof head) || word(head)!=magic)
        return false;
    const uint32_t n=word(head+4);
    if (n > maxFrame || n < headerSize+4+trailerSize || n > used_-at) return false;
    b.resize(n);
    if (!file_.read(at,b.data(),n) || word(b.data())!=magic || word(b.data()+4)!=n) return false;
    return word(b.data()+n-trailerSize)==checksum(b.data(),n-trailerSize);
}

bool Journal::indexFrame(const Bytes& b, uint32_t at, Ledger* restored) {
    Reader r(b.data()+headerSize,b.size()-headerSize-trailerSize);
    const size_t stateLen=r.u(2), events=r.u(1), acks=r.u(1);
    if (!r.okay || stateLen > r.left() || events > batchMax || acks > batchMax || (!at && !stateLen))
        return false;
    if (stateLen) {
        if (restored) {
            if (!decodeState(r.p,stateLen,*restored) || restored->sequence<sequence_ ||
                restored->sequence>uint64_t(INT64_MAX)) return false;
            sequence_=restored->sequence;
        }
        stateBytes_.assign(r.p,r.p+stateLen);
        r.p+=stateLen;
    }
    for (size_t n=0; n<events; ++n) {
        const size_t len=r.u(2);
        Event e;
        if (!len || len>r.left() || !decodeEvent(r.p,len,e) || e.sequence>sequence_ ||
            (!index_.empty() && e.sequence<=index_.back().sequence) || index_.size()>=backlogMax)
            return false;
        index_.push_back({e.sequence,at+uint32_t(r.p-b.data()),checksum(r.p,len),uint16_t(len),e.kind});
        r.p+=len;
    }
    for (size_t n=0; n<acks; ++n) {
        const auto seq=r.u(8);
        auto i=std::lower_bound(index_.begin(),index_.end(),seq,[](const Index& x,uint64_t s){return x.sequence<s;});
        if (i==index_.end() || i->sequence!=seq) return false;
        index_.erase(i);
    }
    if (!r.okay || r.left()) return false;
    lastFrame_=at;
    lastCrc_=word(b.data()+b.size()-trailerSize);
    return true;
}

bool Journal::restore(Ledger& out) {
    okay_=false; used_=0; sequence_=0; index_.clear(); stateBytes_.clear();
    const auto n=file_.size();
    if (n<=0 || n>maxLog) return false;
    used_=uint32_t(n);
    index_.reserve(backlogMax);
    Bytes b;
    Ledger read;
    for (uint32_t at=0; at<used_;) {
        if (!readFrame(at,b) || !indexFrame(b,at,&read)) { index_.clear(); return false; }
        at+=uint32_t(b.size());
    }
    out=std::move(read);
    return okay_=true;
}

bool Journal::verifyTail() {
    if (!okay_) return false;
    Bytes b;
    return okay_=(file_.size()==used_ && readFrame(lastFrame_,b) && word(b.data()+b.size()-trailerSize)==lastCrc_ &&
                  lastFrame_+b.size()==used_);
}

bool Journal::readEvent(const Index& i, Event& e) {
    Bytes b(i.length);
    return file_.read(i.offset,b.data(),b.size()) && checksum(b.data(),b.size())==i.crc &&
        decodeEvent(b.data(),b.size(),e) && e.sequence==i.sequence;
}

std::vector<Event> Journal::pending(uint64_t priority) {
    std::vector<Event> out;
    if (!okay_) return out;
    out.reserve(std::min(batchMax,index_.size()));
    auto add=[&](const Index& i) {
        Event e;
        if (!readEvent(i,e)) { okay_=false; out.clear(); return false; }
        out.push_back(e);
        return true;
    };
    // Keep chronology whenever possible. If candidate receipt is outside this
    // batch, send it as the last entry rather than delaying grant by 32 polls.
    const auto first=std::min(batchMax,index_.size());
    auto tail=std::find_if(index_.begin()+first,index_.end(),[&](const Index& i){return i.sequence==priority;});
    const size_t count=tail==index_.end() ? first : first-1;
    for (size_t i=0; i<count; ++i) if (!add(index_[i])) return out;
    if (tail!=index_.end()) add(*tail);
    return out;
}
bool Journal::hasStopReceipt(const BootId& boot, const Id& request) {
    if (!okay_) return false;
    for (const auto& i : index_) {
        if (i.kind!=Kind::stop_processed) continue;
        Event e;
        if (!readEvent(i,e)) { okay_=false; return false; }
        if (e.boot==boot && e.source==Source::remote && e.request==request) return true;
    }
    return false;
}

bool Journal::commit(const Ledger* state, const std::vector<Event>& add, const std::vector<EventId>& ack) {
    if (!okay_ || add.size()>batchMax || ack.size()>batchMax || index_.size()+add.size()>backlogMax) return false;
    const uint64_t next=state ? state->sequence : sequence_;
    if (next>uint64_t(INT64_MAX) || next!=sequence_+add.size()) return okay_=false;
    std::vector<Bytes> blobs;
    for (const auto& e : add) {
        if (e.sequence!=sequence_+blobs.size()+1) return okay_=false;
        blobs.emplace_back(encodeEvent(e));
    }
    std::vector<uint64_t> acks;
    for (const auto& id : ack) {
        uint64_t seq;
        if (!ackSequence(id,seq) || std::find(acks.begin(),acks.end(),seq)!=acks.end()) continue;
        auto i=std::lower_bound(index_.begin(),index_.end(),seq,[](const Index& x,uint64_t s){return x.sequence<s;});
        if (i==index_.end() || i->sequence!=seq) continue;
        Event e;
        if (!readEvent(*i,e)) return okay_=false;
        if (eventId(e)==id) acks.push_back(seq);
    }
    if (!state && blobs.empty() && acks.empty()) return true;
    auto data=frame(state ? encodeState(*state) : Bytes{},blobs,acks);
    if (data.size()>maxFrame || data.size()>maxLog-used_ || !file_.append(data.data(),data.size())) return okay_=false;
    sequence_=next;
    if (!indexFrame(data,used_)) return okay_=false;
    used_+=uint32_t(data.size());
    return true;
}

bool Journal::prepare(size_t frames) {
    if (!okay_ || frames>maxLog/maxFrame) return false;
    if (maxLog-used_>=frames*maxFrame) return true;
    return compact() && maxLog-used_>=frames*maxFrame;
}

bool Journal::compact() {
    if (!file_.beginReplacement()) return okay_=false;
    Bytes data=frame(stateBytes_,{},{});
    bool wrote=file_.writeReplacement(data.data(),data.size());
    uint32_t bytes=uint32_t(data.size());
    for (size_t i=0; wrote && i<index_.size();) {
        std::vector<Bytes> group;
        for (size_t j=0; j<4 && i<index_.size(); ++j,++i) {
            Event e;
            if (!readEvent(index_[i],e)) { wrote=false; break; }
            group.push_back(encodeEvent(e));
        }
        if (!wrote) break;
        data=frame({},group,{});
        bytes+=uint32_t(data.size());
        wrote=bytes<=maxLog && file_.writeReplacement(data.data(),data.size());
    }
    if (!wrote || !file_.publishReplacement()) {
        file_.discardReplacement();
        return okay_=false;
    }
    Ledger d;
    return restore(d); // crc/length/indices verified after rename before next admission
}
}
