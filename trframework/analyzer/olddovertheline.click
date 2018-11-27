//------------Declarations-----------

//Required if run without DPDK interfaces
//Idle -> EnsureDPDKBuffer -> Discard;

OUT ::  Queue(1000000) -> td :: ToDevice(VETH-SRC_00);
//IN  ::  FromDevice(VETH-SNK_06, METHOD LINUX);
IN  ::  FromDevice(VETH-SNK_06);



//+++++DPDK0 Interface(Inside)+++++
//OUT ::  ToDPDKDevice(0);

//+++++DPDK1 Interface(Outside)+++++
//IN :: FromDPDKDevice(1, PROMISC true);


//--------Program Start----------
InfiniteSource(LENGTH $packetlen, LIMIT 1900000, STOP true)
//rs :: RatedSource(LENGTH 64, RATE 18000, LIMIT 380000, STOP true)
        -> NumberPacket(OFFSET 0)
        -> UDPIP6Encap(2001:2001:2001:2001::1, 1234, 64:ff9b::192.168.2.5, 1234)
        -> EtherEncap(0x86DD, 00:04:23:D0:93:63, 00:17:cb:0d:f8:db)
	-> RatedUnqueue(RATE 19000)
        -> record :: RecordTimestamp()
        -> OUT;

IN
        -> chnum :: CheckNumberPacket(OFFSET 62, COUNT 1900000)
        -> diff :: TimestampDiff(OFFSET 62, N 1900000, RECORDER record)
        -> counter :: AverageCounter()
        -> Discard;
//--------Program End----------
//StaticThreadSched(rs 1);
//StaticThreadSched(diff 2);
//StaticThreadSched(chnum 2);
//StaticThreadSched(counter 2);

//DriverManager(wait, wait 2s,read diff.average,read counter2.count,  read counter.count, read counter.rate)

//DriverManager(wait, wait 2s, read diff.perc00, read diff.perc01, read diff.perc05, read diff.perc10, read diff.perc25, read diff.median, read diff.perc75,  read diff.perc95,  read diff.perc99,  read diff.perc100, read counter.count, read counter.rate)

DriverManager(wait, wait 2s, append "$(diff.perc00), $(diff.perc01), $(diff.perc05), $(diff.perc10), $(diff.perc25), $(diff.median), $(diff.perc75), $(diff.perc95), $(diff.perc99), $(diff.perc100), $(counter.count), $(counter.rate)" /usr/src/fastclick/trframework/analyzer/results/vxlan$packetlen.log )


