#include <vector>
#include <snmp/ber.h>
#include <snmp/sinks.h>

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
    return 0;
}