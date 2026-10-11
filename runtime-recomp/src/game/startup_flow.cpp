#include "startup_flow.hpp"
#include "revision_addresses.hpp"
#include "recomp.h"

#include <cstdio>

extern "C" void dkr_skip_intro_logos(std::uint8_t* rdram, recomp_context* ctx) {
    if (!rdram || !ctx || !dkr::runtime::startup::skip_logos()) return;
    const auto timer = dkr::runtime::revision_addresses::selected_revision() ==
        dkr::runtime::rom::Revision::UsV80 ? 0x801269F0U : 0x80126450U;
    // Float zero. Let retail disable its logo viewport, initialize EEPROM
    // unlocks/language/subtitles and construct the title screen normally.
    MEM_W(0, static_cast<gpr>(static_cast<std::int32_t>(timer))) = 0;
}

extern "C" void dkr_skip_intro_title(std::uint8_t* rdram, recomp_context* ctx) {
    if (!rdram || !ctx || !dkr::runtime::startup::consume_title_skip()) return;
    const auto base = static_cast<std::uint32_t>(ctx->r2);
    if (base < 0x80000000U || base > 0x807FFFFAU) return;
    const auto pointer = static_cast<gpr>(static_cast<std::int32_t>(base));
    if (!dkr::runtime::startup::valid_title_demo(
            base, MEM_B(3, pointer), MEM_B(4, pointer))) return;
    // The constructor has ALREADY stored the original table pointer, but has
    // not yet read its first entry. Select the second retail demo, not an
    // invented scene or a synthetic Start button. Input/music stay retail.
    ctx->r2 = static_cast<gpr>(static_cast<std::int32_t>(base + 3U));
    MEM_W(0, static_cast<gpr>(static_cast<std::int32_t>(
        dkr::runtime::revision_addresses::TitleDemoIndex))) = 3;
    MEM_W(0, static_cast<gpr>(static_cast<std::int32_t>(
        dkr::runtime::revision_addresses::TitleRevealTimer))) = 32;
    std::fprintf(stderr, "[boot][startup] skipped intro; normal Start Game title ready\n");
}
