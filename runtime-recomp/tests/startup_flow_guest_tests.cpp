#include "startup_flow.hpp"
#include "revision_addresses.hpp"
#include "recomp.h"

#include <cassert>
#include <cstdio>
#include <vector>

extern "C" void dkr_skip_intro_logos(std::uint8_t*, recomp_context*);
extern "C" void dkr_skip_intro_title(std::uint8_t*, recomp_context*);

int main() {
    using namespace dkr::runtime;
    std::vector<std::uint8_t> memory(8U * 1024U * 1024U);
    auto* rdram = memory.data();
    recomp_context registers{};
    auto* ctx = &registers;
    for (const auto revision : {rom::Revision::UsV77, rom::Revision::UsV80}) {
        assert(revision_addresses::select(revision));
        const gpr timer = static_cast<std::int32_t>(revision == rom::Revision::UsV80
            ? 0x801269F0U : 0x80126450U);
        const gpr index = static_cast<std::int32_t>(revision_addresses::TitleDemoIndex);
        const gpr reveal = static_cast<std::int32_t>(revision_addresses::TitleRevealTimer);
        const gpr base = static_cast<std::int32_t>(0x80010000U);
        for (const bool online : {false, true}) {
            MEM_W(0, timer) = 0x41800000; // retail 16 seconds
            MEM_W(0, index) = 0;
            MEM_W(0, reveal) = 0;
            MEM_B(3, base) = 5;
            MEM_B(4, base) = -2;
            MEM_B(5, base) = 7;
            ctx->r2 = base;
            ctx->r4 = 0x1234;
            const auto table_before = MEM_W(0, base);
            startup::prepare_launch(true, online, false);
            dkr_skip_intro_logos(rdram, ctx);
            assert(MEM_W(0, timer) == (online ? 0x41800000 : 0));
            dkr_skip_intro_title(rdram, ctx);
            assert(ctx->r2 == (online ? base : base + 3));
            assert(ctx->r4 == 0x1234);
            assert(MEM_W(0, index) == (online ? 0 : 3));
            assert(MEM_W(0, reveal) == (online ? 0 : 32));
            assert(MEM_W(0, base) == table_before); // never rewrite the retail table
            ctx->r2 = base;
            dkr_skip_intro_title(rdram, ctx);
            assert(ctx->r2 == base); // one shot per offline launch
        }
        startup::prepare_launch(true, false, false);
        MEM_B(3, base) = -1; // terminator, no usable second demo
        ctx->r2 = base;
        dkr_skip_intro_title(rdram, ctx);
        assert(ctx->r2 == base);
        assert(!startup::skip_logos());
        startup::prepare_launch(true, false, false);
        ctx->r2 = static_cast<std::int32_t>(0x80800000U);
        dkr_skip_intro_title(rdram, ctx);
        assert(static_cast<std::uint32_t>(ctx->r2) == 0x80800000U);
    }
    std::puts("Both retail revisions: title hooks, invalid assets, table preservation and online no-op passed.");
}
