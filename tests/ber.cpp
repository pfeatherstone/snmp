#include "doctest.h"
#include <sstream>
#include <snmp/ber.h>
#include <snmp/sinks.h>
#include "utils.h"

using namespace snmp;
using namespace snmp_tests;

const auto same_byte = [](auto lhs, auto rhs)
{
    return static_cast<unsigned char>(lhs) == static_cast<unsigned char>(rhs);
};

TEST_SUITE("[BER]")
{
    TEST_CASE("basic")
    {
        std::mt19937 eng(std::random_device{}());

        for (int repeat = 0 ; repeat < 100000 ; ++repeat)
        {
            const bool          a = std::uniform_int_distribution<int>{0,1}(eng);
            const uint8_t       b = random_int<uint8_t>(eng);
            const int8_t        c = random_int<int8_t>(eng);
            const uint16_t      d = random_int<uint16_t>(eng);
            const int16_t       e = random_int<int16_t>(eng);
            const uint32_t      f = random_int<uint32_t>(eng);
            const int32_t       g = random_int<int32_t>(eng);
            const uint64_t      h = random_int<uint64_t>(eng);
            const int64_t       i = random_int<int64_t>(eng);
            const char*         j = "Hello There!";
            std::string_view    k = "Obi-Wan ? ";
            std::string         l = "You lack conviction";
            std::vector<int>    m = {255, 1265653235, -2353325, 0, 127, 128, -127, -128};

            // Check serialising too different buffer types yields same wire memory
            const auto run = [&](auto& buf)
            {
                ber_serialize(a, sink(buf));
                ber_serialize(b, sink(buf));
                ber_serialize(c, sink(buf));
                ber_serialize(d, sink(buf));
                ber_serialize(e, sink(buf));
                ber_serialize(f, sink(buf));
                ber_serialize(g, sink(buf));
                ber_serialize(h, sink(buf));
                ber_serialize(i, sink(buf));
                ber_serialize(j, sink(buf));
                ber_serialize(k, sink(buf));
                ber_serialize(l, sink(buf));
                ber_serialize(m, sink(buf));
            };

            std::vector<char>    buf0;
            std::vector<int8_t>  buf1;
            std::vector<uint8_t> buf2;
            std::string          buf3;
            std::stringstream    buf4;
            run(buf0);
            run(buf1);
            run(buf2);
            run(buf3);
            run(buf4);
            auto buf4_str = buf4.str();
            REQUIRE(buf0.size() == buf1.size());
            REQUIRE(buf0.size() == buf2.size());
            REQUIRE(buf0.size() == buf3.size());
            REQUIRE(buf0.size() == buf4_str.size());
            CHECK(std::equal(begin(buf0), end(buf0), begin(buf1), same_byte));
            CHECK(std::equal(begin(buf0), end(buf0), begin(buf2), same_byte));
            CHECK(std::equal(begin(buf0), end(buf0), begin(buf3), same_byte));
            CHECK(std::equal(begin(buf0), end(buf0), begin(buf4_str), same_byte));

            // Test deserialization
            bool                a_{};
            uint8_t             b_{};
            int8_t              c_{};
            uint16_t            d_{};
            int16_t             e_{};
            uint32_t            f_{};
            int32_t             g_{};
            uint64_t            h_{};
            int64_t             i_{};
            std::string         j_{};
            std::string         k_{};
            std::string         l_{};
            std::vector<int>    m_{};
            auto src = source(buf0);
            ber_deserialize(a_, src);
            ber_deserialize(b_, src);
            ber_deserialize(c_, src);
            ber_deserialize(d_, src);
            ber_deserialize(e_, src);
            ber_deserialize(f_, src);
            ber_deserialize(g_, src);
            ber_deserialize(h_, src);
            ber_deserialize(i_, src);
            ber_deserialize(j_, src);
            ber_deserialize(k_, src);
            ber_deserialize(l_, src);
            ber_deserialize(m_, src);
            CHECK(a == a_);
            CHECK(b == b_);
            CHECK(c == c_);
            CHECK(d == d_);
            CHECK(e == e_);
            CHECK(f == f_);
            CHECK(g == g_);
            CHECK(h == h_);
            CHECK(i == i_);
            CHECK(std::string_view{j} == j_);
            CHECK(k == k_);
            CHECK(l == l_);
            CHECK(m == m_);
        }
    }

    TEST_CASE("tuple")
    {
        std::mt19937 eng(std::random_device{}());
        std::tuple<int, bool, std::string> a(1, true, "example");
        std::tuple<int, bool, std::string> b{};

        std::vector<char> buf;
        ber_serialize(a, sink(buf));
        ber_deserialize(b, source(buf));
        CHECK(a == b);
    }
}