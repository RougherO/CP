#include "cp/sys/input_source.hpp"
#include "cp/sys/output_sink.hpp"
#include <catch2/catch_test_macros.hpp>
#include "fd_helpers.hpp"
#include <string>

using namespace test;  // NOLINT(google-build-using-namespace): test helpers

namespace {

auto pattern(std::size_t n) -> std::string {
    std::string s(n, '\0');
    for (std::size_t i = 0; i < n; ++i) s[i] = static_cast<char>('a' + i % 26);
    return s;
}

}  // namespace

TEST_CASE("files are mapped only when the last page has room for the padding", "[io][source]") {
    using cp::detail::input_padding;
    using cp::detail::mapping_has_padding;
    STATIC_REQUIRE_FALSE(mapping_has_padding(0, 4096));
    STATIC_REQUIRE(mapping_has_padding(1, 4096));
    STATIC_REQUIRE(mapping_has_padding(4096 - input_padding, 4096));
    STATIC_REQUIRE_FALSE(mapping_has_padding(4096 - input_padding + 1, 4096));
    STATIC_REQUIRE_FALSE(mapping_has_padding(4096, 4096));
    STATIC_REQUIRE_FALSE(mapping_has_padding(std::size_t{3} * 16384, 16384));
    STATIC_REQUIRE(mapping_has_padding(4096 + 1, 4096));
}

// Sizes around page boundaries: an exact multiple of the page would fault past the end if mapped, and a last page
// with fewer than input_padding spare bytes cannot hold the padding.
TEST_CASE("regular files are in memory, followed by zero padding", "[io][source]") {
    auto const page = page_size();
    for (std::size_t size : {std::size_t{0}, std::size_t{1}, std::size_t{100}, page - cp::detail::input_padding,
                             page - cp::detail::input_padding + 1, page - 1, page, page + 1, 2 * page}) {
        CAPTURE(size);
        auto content = pattern(size);
        temp_file file(content);
        cp::detail::input_source src(file.fd());
        REQUIRE(src.data() != nullptr);
        REQUIRE_FALSE(src.may_block());
        REQUIRE(std::string(src.data(), src.size()) == content);
        for (std::size_t i = 0; i < cp::detail::input_padding; ++i) REQUIRE(src.data()[size + i] == '\0');
        char c = 0;
        REQUIRE(src.read_some(&c, 1) == 0);
    }
}

TEST_CASE("a file read from the middle yields only the rest", "[io][source]") {
    temp_file file("skip|rest");
    REQUIRE(seek(file.fd(), 5) == 5);
    cp::detail::input_source src(file.fd());
    REQUIRE(std::string(src.data(), src.size()) == "rest");
    REQUIRE(src.data()[4] == '\0');
}

// An interactive judge writes a line and waits for the answer: read_some must return what has arrived.
TEST_CASE("a pipe returns available bytes without waiting for a full buffer", "[io][source]") {
    test_pipe pipe;
    cp::detail::input_source src(pipe.read_end());
    REQUIRE(src.data() == nullptr);
    REQUIRE(src.may_block());

    pipe.write("12 34\n");
    std::string buf(1 << 16, '\0');
    REQUIRE(src.read_some(buf.data(), buf.size()) == 6);
    REQUIRE(buf.compare(0, 6, "12 34\n") == 0);

    pipe.close_write();
    REQUIRE(src.read_some(buf.data(), buf.size()) == 0);
}

TEST_CASE("output_sink writes every byte", "[io][source]") {
    auto content = pattern(3 << 20);
    temp_file file("");
    cp::detail::output_sink(file.fd()).write_all(content.data(), content.size());
    REQUIRE(seek(file.fd(), 0) == 0);
    cp::detail::input_source src(file.fd());
    REQUIRE(std::string(src.data(), src.size()) == content);
}
