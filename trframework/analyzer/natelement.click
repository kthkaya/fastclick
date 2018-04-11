//------------Declarations-----------
Idle -> EnsureDPDKBuffer -> Discard;
//+++++DPDK0 Interface(Inside)+++++
//DPDK0_IN :: FromDPDKDevice(0, PROMISC true);
//DPDK0_OUT ::  ToDPDKDevice(0);

//+++++DPDK1 Interface(Outside)+++++
//DPDK1_IN :: FromDPDKDevice(1, PROMISC true);
//DPDK1_OUT :: ToDPDKDevice(1);
NAT64 :: StatefulTranslator64();

//--------Program Start----------
InfiniteSource(LENGTH 24, LIMIT 250000, STOP true)
        -> NumberPacket(OFFSET 0)
//      -> replay :: ReplayUnqueue(STOP 1, ACTIVE true, QUICK_CLONE 1)
        -> UDPIP6Encap(2001:2001:2001:2001::1, 1234, 64:ff9b::192.168.2.5, 1234)
        -> NAT64
        -> EtherEncap(0x0800, 00:04:23:D0:93:63, 00:17:cb:0d:f8:db)
//      -> IP6Print(CONTENTS true)
        -> record :: RecordTimestamp()
        -> CheckNumberPacket(OFFSET 42, COUNT 250000)
        -> diff :: TimestampDiff(OFFSET 42, RECORDER record)
        -> counter :: AverageCounter()
        -> Discard;
//--------Program End----------

Idle -> [1]NAT64[1] ->Discard;

DriverManager(wait,read diff.average, read counter.count, read counter.rate)

