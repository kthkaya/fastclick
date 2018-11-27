//------------Declarations-----------
//Idle -> EnsureDPDKBuffer -> Discard;
//+++++DPDK0 Interface(Inside)+++++
//DPDK0_IN :: FromDPDKDevice(0, PROMISC true);
//DPDK0_OUT ::  ToDPDKDevice(0);

//+++++DPDK1 Interface(Outside)+++++
//DPDK1_IN :: FromDPDKDevice(1, PROMISC true);
//DPDK1_OUT :: ToDPDKDevice(1);


//--------Program Start----------
InfiniteSource(LENGTH 64, LIMIT 110000, STOP true)
//rs :: RatedSource(LENGTH 64, RATE 25000, LIMIT 500000, STOP true)
        -> NumberPacket(OFFSET 0)
//      -> replay :: ReplayUnqueue(STOP 1, ACTIVE true, QUICK_CLONE 1)
        -> UDPIP6Encap(2001:2001:2001:2001::1, 1234, 2001:2001:2001:2001::2, 1234)
        -> EtherEncap(0x86DD, 00:04:23:D0:93:63, 00:17:cb:0d:f8:db)
//      -> IP6Print(CONTENTS true)
//      -> replay :: ReplayUnqueue(STOP 10, ACTIVE true, QUICK_CLONE 1)
	-> Queue(10000)
	-> RatedUnqueue (RATE 9000)
        -> record :: RecordTimestamp()
        -> CheckNumberPacket(OFFSET 62, COUNT 100000)
        -> diff :: TimestampDiff(OFFSET 62, N 100000, RECORDER record)
//      -> counter :: AverageCounter()
        -> counter :: Counter()
        -> Discard;
//--------Program End----------

DriverManager(wait,read diff.average, read counter.count, read counter.rate)

