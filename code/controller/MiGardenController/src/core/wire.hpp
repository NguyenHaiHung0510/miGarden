#pragma once
#include "types.hpp"
#include <string>

namespace mg {
inline constexpr size_t requestBytes = 24576, responseBytes = 16384;
enum class WireError { none, syntax, shape, bounds, protocol_version };
WireError decodeExchange(std::string_view, Exchange&);
struct Health {
    const char* build="unknown";
    uint64_t uptime=0, freeHeap=0, minimumHeap=0, gaps=0;
    unsigned resetReason=0, pending=0;
    const char* wifi="READY";
    const char* sync="UNKNOWN";
};
bool encodeExchange(const Id& device, const BootId&, const Snapshot&, const std::vector<Event>&, std::string&, const Health* health=nullptr);
std::string encodeSnapshot(const Snapshot&);
}
