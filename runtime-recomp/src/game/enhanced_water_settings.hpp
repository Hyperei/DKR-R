#pragma once
#include "enhanced_water_policy.hpp"
#include <atomic>

namespace dkr::runtime::water::enhanced {
// Presentation preferences are not guest data/checkpoint inputs.
inline std::atomic<Quality> requested_quality{Quality::Original};
inline std::atomic<Reflection> requested_reflection{Reflection::ScreenSpace};
inline std::atomic<bool> wakes_enabled{true};
inline std::atomic<bool> spray_enabled{true};
// -1 until a game renderer has reported the actual enabled device features.
// This describes capability, not a promise that every scene fits its budget.
inline std::atomic<int> hardware_ray_capability{-1};
inline void set_quality(int value) {
    requested_quality.store(static_cast<Quality>(std::clamp(value,0,2)),std::memory_order_relaxed);
}
inline void set_reflection(int value) {
    requested_reflection.store(static_cast<Reflection>(std::clamp(value,0,2)),std::memory_order_relaxed);
}
}
