//------------Declarations-----------

//Required if run without DPDK interfaces
//Idle -> EnsureDPDKBuffer -> Discard;

OUT ::  Queue(500000) -> td :: ToDevice(VETH-SRC_00);
//IN  ::  FromDevice(VETH-SNK_06, METHOD LINUX);
IN  ::  FromDevice(VETH-SNK_00);



//+++++DPDK0 Interface(Inside)+++++
//OUT ::  ToDPDKDevice(0);

//+++++DPDK1 Interface(Outside)+++++
//IN :: FromDPDKDevice(1, PROMISC true);


//--------Program Start----------
InfiniteSource(LENGTH 64, LIMIT 100000, STOP true)
//rs :: RatedSource(LENGTH 64, RATE 19000, LIMIT 380000, STOP true)
        -> NumberPacket(OFFSET 0)
//      -> replay :: ReplayUnqueue(STOP 1, ACTIVE true, QUICK_CLONE 1)
//      -> UDPIP6Encap(2001:2001:2001:2001::1, 1234, 2001:2001:2001:2001::2, 1234)
        -> UDPIP6Encap(2001:2001:2001:2001::1, 1234, 64:ff9b::192.168.2.5, 1234)
        -> EtherEncap(0x86DD, 00:04:23:D0:93:63, 00:17:cb:0d:f8:db)
//	-> Queue(19007)
	-> RatedUnqueue(RATE 10000)
        -> record :: RecordTimestamp()

	-> counter3 :: AverageCounter()
        -> OUT;

IN
	-> counter2 :: AverageCounter()
        -> chnum :: CheckNumberPacket(OFFSET 62, COUNT 380000)
        -> diff :: TimestampDiff(OFFSET 62, N 380000, RECORDER record)
        -> counter :: AverageCounter()
        -> Discard;
//--------Program End----------
//StaticThreadSched(rs 1);
//StaticThreadSched(diff 2);
//StaticThreadSched(chnum 2);
//StaticThreadSched(counter 2);

DriverManager(wait, wait 2s,read diff.average,read counter2.count,  read counter.count, read counter.rate, read counter3.count, read td.count)
