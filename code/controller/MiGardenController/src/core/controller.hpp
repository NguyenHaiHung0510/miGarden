#pragma once
#include "journal.hpp"

namespace mg {
class ClockPort {
public:
    virtual ~ClockPort() = default;
    virtual Sample now() = 0;
};
struct Drive {
    bool on = false, stopHeld = false, usable = true;
    uint32_t stops = 0;
    Ms onAt = none, offAt = none;
    Reason offReason = Reason::none;
};
// The output adapter owns the deadline and raw STOP path even when
// controller/persistence/network work is delayed. open() checks its fence and
// absolute latest-start before energizing; a false result means never energized.
class OutputPort {
public:
    virtual ~OutputPort() = default;
    virtual Drive read() = 0;
    virtual bool open(Ms duration, Ms latestStartExclusive, uint32_t observedStops, Drive& actual) = 0;
    virtual Drive close(Reason) = 0;
};

class Controller {
public:
    Controller(ClockPort& clock, OutputPort& output, Journal& journal, BootId boot);
    void boot();
    // Called by one executor. STOP is processed first, then local edge,
    // scheduled edge, and authenticated remote handoff. A local edge captured
    // during a busy period is passed as busyEdge, never deferred until idle.
    void tick(bool localStart = false, bool busyEdge = false,
              const Exchange* inbound = nullptr, const std::vector<EventId>& sent = {});
    Snapshot snapshot();
    std::vector<Event> reports();
    bool enterMaintenance();
    void leaveMaintenance();
    bool idle() const { return !running_; }
    bool maintenance() const { return maintenance_; }
    size_t queued() const { return journal_.queued(); }
    uint32_t logBytes() const { return journal_.bytes(); }
    uint64_t change() const { return change_; }
    uint64_t evidenceGaps() const { return gaps_; }
    const Ledger& ledger() const { return state_; }
    const BootId& bootId() const { return bootId_; }
private:
    static constexpr size_t privilegedRoom = 6; // reserve critical report slots at admission
    static constexpr Ms dispatchBound = 2 * second;
    ClockPort& clock_;
    OutputPort& output_;
    Journal& journal_;
    BootId bootId_;
    Ledger state_;
    bool running_ = false, broken_ = false, possible_ = false, maintenance_ = false;
    uint32_t epoch_ = 0, observedStops_ = 0;
    uint64_t localCounter_ = 0, change_ = 1, gaps_ = 0, offeredSequence_ = 0;
    Sample prior_, anchor_;
    Ms lastMonotonic_ = none, lastProbe_ = none, monoStart_ = none, deadline_ = none;
    Ms pendingExpires_ = none;
    std::vector<uint64_t> startSeen_, staleStops_;
    std::vector<Id> stopSeen_;
    bool remoteStopCapacity_=false;
    Id lastRejectedConfig_;
    Reason lastConfigReason_ = Reason::none;

    Ms accounting(Sample) const;
    bool observe(Sample);
    Reason gate(Sample, Drive, bool ignoreRunning = false) const;
    Reason remote(const Intent&, Sample) const;
    Event make(Ledger&, Kind, Sample, Source = Source::none, Reason = Reason::none);
    Event fromSession(Ledger&, Kind, Sample, const Session&);
    Event occurrence(Ledger&, Sample, Ms at, Outcome, Reason, const Id& request = {}, const Id& session = {});
    Id newId(const char*);
    bool save(Ledger&&, const std::vector<Event>& = {}, bool critical = false);
    void breakStorage();
    void handleStop(Sample, Source, const Id&, bool stale = false);
    void flushStopReceipts();
    void finish(Ledger&, std::vector<Event>&, Sample, Drive, Reason);
    void rejected(Sample, Source, Reason, const Id&, const Id& config = {});
    void cancelPending(Ledger&, std::vector<Event>&, Sample, Reason);
    void apply(const Exchange&, Sample);
    void scheduled(Sample, bool crossed, Reason simultaneousBlock);
    void local(Sample, Reason simultaneousBlock);
    void candidate(const Intent&, Sample, Reason simultaneousBlock);
    void grant(const Grant&, Sample, Reason simultaneousBlock);
    void start(Source, Id request, int32_t duration, Ms dueAt, const Intent* = nullptr);
    void noteDiscontinuity(Sample, Sample);
};
}
