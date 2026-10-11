#pragma once

#include <atomic>
#include <cstdint>
#include <string_view>

namespace dkr::runtime::startup {

// Launcher-owned, one-shot preference. Never part of a checkpoint or lobby
// settings: online startup follows the normal shared retail sequence.
inline std::atomic<bool> skip_intro_pending{false};

inline bool parse_skip_intro(std::string_view value) {
    // Settings copied from Windows retain CRLF when read on Linux/Android.
    if (!value.empty() && value.back() == '\r') value.remove_suffix(1);
    return value == "1";
}

inline void prepare_launch(bool requested, bool online, bool direct_track_boot) {
    skip_intro_pending.store(requested && !online && !direct_track_boot,
                            std::memory_order_release);
}

inline bool skip_logos() {
    return skip_intro_pending.load(std::memory_order_acquire);
}

inline bool consume_title_skip() {
    return skip_intro_pending.exchange(false, std::memory_order_acq_rel);
}

// The demo table has three signed bytes per entry (level, players, cutscene).
// Keep the table's base pointer intact; only the constructor's local read moves
// to the next retail demo. Refuse stale pointers or an absent next entry.
inline bool valid_title_demo(std::uint32_t address, int next_level, int players) {
    return address >= 0x80000000U && address <= 0x807FFFFAU &&
           next_level >= 0 && next_level < 128 &&
           players >= -2 && players <= 4;
}

} // namespace dkr::runtime::startup
