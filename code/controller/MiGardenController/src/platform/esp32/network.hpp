#pragma once
#include "platform.hpp"
#include "../../core/sync_policy.hpp"
#include <atomic>
#include <freertos/queue.h>

namespace mg::esp {
class Network {
public:
    bool begin(Provision&&);
    // All calls below except the worker are on the control executor.
    struct Reply {
        Exchange exchange;
        std::vector<EventId> sent;
        bool accepted=false;
        uint32_t generation=0;
    };
    Reply* take();
    void after(const Reply&,Controller&);
    void submit(Controller&);
    void fence() { ++generation_; }
    const char* status() const;
private:
    struct Job;
    Provision provision_;
    QueueHandle_t outbound_=nullptr, inbound_=nullptr;
    std::atomic<bool> wifi_{false};
    std::atomic<unsigned> status_{0};
    bool inFlight_=false;
    uint32_t generation_=0;
    uint64_t lastSubmitted_=0;
    SyncPolicy policy_;
    static void run(void*);
    void work();
};
}
