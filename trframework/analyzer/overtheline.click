//------------Declarations-----------

//Required if run without DPDK interfaces
Idle -> EnsureDPDKBuffer -> Discard;

OUT ::  Queue(500000) -> ToDevice(VETH-SRC_00);
IN  ::  FromDevice(VETH-SNK_00, METHOD LINUX);


//+++++DPDK0 Interface(Inside)+++++
//OUT ::  ToDPDKDevice(0);

//+++++DPDK1 Interface(Outside)+++++
//IN :: FromDPDKDevice(1, PROMISC true);


//--------Program Start----------
//InfiniteSource(LENGTH 64, LIMIT 150000, STOP true)
RatedSource(LENGTH 64, RATE 70000, LIMIT 1500000, STOP true)
        -> NumberPacket(OFFSET 0)
//      -> replay :: ReplayUnqueue(STOP 1, ACTIVE true, QUICK_CLONE 1)
//      -> UDPIP6Encap(2001:2001:2001:2001::1, 1234, 2001:2001:2001:2001::2, 1234)
        -> UDPIP6Encap(2001:2001:2001:2001::1, 1234, 64:ff9b::192.168.2.5, 1234)
        -> EtherEncap(0x86DD, 00:04:23:D0:93:63, 00:17:cb:0d:f8:db)
        -> record :: RecordTimestamp()
        -> OUT;

IN
        -> CheckNumberPacket(OFFSET 62, COUNT 1500000)
        -> diff :: TimestampDiff(OFFSET 62, RECORDER record)
        -> counter :: AverageCounter()
        -> Discard;
//--------Program End----------

DriverManager(wait,read diff.average, read counter.count, read counter.rate)
