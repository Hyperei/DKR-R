#pragma once
#include <cstdint>

namespace dkr::mods::online {
// Rolling credits require a matching sender and receiver. Also included in
// lobby admission identity so old Beta 15 peers are rejected before transfer.
inline constexpr std::uint8_t WireVersion=2;
}
