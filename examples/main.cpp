#include <vector>
#include <snmp/ber.h>
#include <snmp/sinks.h>

using namespace std::string_literals;
using namespace std::string_view_literals;

int main()
{
    using snmp::ber_serialize;
    using snmp::sink;
    std::vector<char> buf;
    ber_serialize(nullptr, sink(buf));
    ber_serialize(true,    sink(buf));
    ber_serialize(false,   sink(buf));
    ber_serialize(42,      sink(buf));
    ber_serialize("hello there", sink(buf));
    ber_serialize("obi-wan"sv, sink(buf));
    ber_serialize("i have the high ground"s, sink(buf));
    ber_serialize(std::vector<int>{1,2,3,4,5,6}, sink(buf));
    return 0;
}