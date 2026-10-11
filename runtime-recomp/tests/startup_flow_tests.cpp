#include "startup_flow.hpp"
#include <cassert>
#include <cstdio>
#include <initializer_list>

int main() {
    using namespace dkr::runtime::startup;
    assert(!skip_logos());
    for (bool requested : {false, true}) {
        for (bool online : {false, true}) {
            for (bool direct : {false, true}) {
                prepare_launch(requested, online, direct);
                const bool expected = requested && !online && !direct;
                assert(skip_logos() == expected);
                assert(consume_title_skip() == expected);
                assert(!skip_logos());
                assert(!consume_title_skip()); // returning to the title never re-skips
            }
        }
    }
    prepare_launch(true, false, false);
    prepare_launch(false, true, false); // next online launch cancels stale request
    assert(!consume_title_skip());
    assert(parse_skip_intro("1"));
    assert(parse_skip_intro("1\r"));
    for (const auto text : {"", "0", "true", "2", "-1", "1junk"})
        assert(!parse_skip_intro(text));
    assert(valid_title_demo(0x80000000, 5, -2));
    assert(valid_title_demo(0x807FFFFA, 5, 4));
    assert(!valid_title_demo(0x807FFFFB, 5, 1));
    assert(!valid_title_demo(0x7FFFFFFF, 5, 1));
    assert(!valid_title_demo(0x80000000, -1, 1));
    assert(!valid_title_demo(0x80000000, 5, -3));
    assert(!valid_title_demo(0x80000000, 5, 5));
    std::puts("Offline title skip, settings parsing, one-shot and online isolation passed.");
}
