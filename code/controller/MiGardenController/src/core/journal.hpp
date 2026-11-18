#pragma once
#include "types.hpp"
#include <string>

namespace mg {
// Single writer. append() returns true only after the complete frame is durable.
// Replacement is written to a distinct file and published by atomic rename.
// Missing/invalid live file is a fault, never permission to create an empty ledger.
class LogFile {
public:
    virtual ~LogFile() = default;
    virtual int64_t size() = 0;
    virtual bool read(uint32_t offset, uint8_t* out, size_t count) = 0;
    virtual bool append(const uint8_t*, size_t) = 0;
    virtual bool beginReplacement() = 0;
    virtual bool writeReplacement(const uint8_t*, size_t) = 0;
    virtual bool publishReplacement() = 0;
    virtual void discardReplacement() = 0;
};

class Journal {
public:
    explicit Journal(LogFile& file) : file_(file) {}
    bool restore(Ledger&);
    bool commit(const Ledger* state, const std::vector<Event>& add = {},
                const std::vector<EventId>& ack = {});
    bool prepare(size_t frames); // idle only: reserve log bytes or compact
    bool verifyTail();
    std::vector<Event> pending(uint64_t prioritySequence = 0);
    bool hasStopReceipt(const BootId&, const Id&); // exact RAM index, no storage before CLOSE
    bool healthy() const { return okay_; }
    size_t queued() const { return index_.size(); }
    uint32_t bytes() const { return used_; }
    static std::vector<uint8_t> genesis(); // offline tool and separate commissioner; never normal runtime
    static constexpr uint32_t maxFrame = 24 * 1024, maxLog = 448 * 1024;
private:
    struct Index { uint64_t sequence; uint32_t offset, crc; uint16_t length; };
    struct Receipt { uint64_t sequence; std::string key; };
    using Bytes = std::vector<uint8_t>;
    LogFile& file_;
    bool okay_ = false;
    uint32_t used_ = 0, lastFrame_ = 0, lastCrc_ = 0;
    uint64_t sequence_ = 0;
    std::vector<Index> index_;
    std::vector<Receipt> stops_;
    Bytes stateBytes_;
    bool compact();
    bool indexFrame(const Bytes&, uint32_t offset, Ledger* restored = nullptr);
    bool readFrame(uint32_t offset, Bytes&);
    bool readEvent(const Index&, Event&);
};
}
