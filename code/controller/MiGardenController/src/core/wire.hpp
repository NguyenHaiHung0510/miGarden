#pragma once
#include "types.hpp"
#include <string>

namespace mg {
inline constexpr size_t requestBytes = 24576, responseBytes = 16384;
enum class WireError { none, syntax, shape, bounds, protocol_version };
WireError decodeExchange(std::string_view, Exchange&);
bool encodeExchange(const Id& device, const BootId&, const Snapshot&, const std::vector<Event>&, std::string&);
std::string encodeSnapshot(const Snapshot&);
}
