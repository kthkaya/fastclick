//------------Declarations-----------

//Required if run without DPDK interfaces
//Idle -> EnsureDPDKBuffer -> Discard;

OUT ::  Queue(1000000) -> td :: ToDevice(VETH-SRC_00);
//IN  ::  FromDevice(VETH-SNK_06, METHOD LINUX);
IN  ::  FromDevice(VETH-SNK_$snkintf);



//+++++DPDK0 Interface(Inside)+++++
//OUT ::  ToDPDKDevice(0);

//+++++DPDK1 Interface(Outside)+++++
//IN :: FromDPDKDevice(1, PROMISC true);


//--------Program Start----------
InfiniteSource(LENGTH $packetlen, LIMIT $packetcount, STOP true)
//rs :: RatedSource(LENGTH 64, RATE 18000, LIMIT 380000, STOP true)
        -> NumberPacket(OFFSET 0)
        -> udpEncap :: UDPIP6Encap(2001:2001:2001:2001::1, 1234, 64:ff9b::192.168.2.5, 1234)
        -> EtherEncap(0x86DD, 00:04:23:D0:93:63, 00:17:cb:0d:f8:db)
        -> RatedUnqueue(RATE $packetrate)
        -> record :: RecordTimestamp()
        -> OUT;

IN
        -> chnum :: CheckNumberPacket(OFFSET 62, COUNT $packetcount)
        -> diff :: TimestampDiff(OFFSET 62, N $packetcount, MAXDELAY 2000, RECORDER record)
        -> counter :: AverageCounter()
        -> Discard;
//--------Program End----------

scr :: Script( TYPE ACTIVE,
        init x 0,
        init waitTime 1,
        set origIP $(udpEncap.src),
        set waitTime $(div 1 $flowspersecond),
        label reset,
        set x 0,
        write udpEncap.src $origIP,
        label loop,
        set x $(add $x 1),
        wait $waitTime s,
        write udpEncap.incr_saddr,
        goto reset $(eq $x $flowspersecond),
        goto loop );

StaticThreadSched(scr 1);

DriverManager(wait, wait 2s, append "$(diff.perc00), $(diff.perc01), $(diff.perc05), $(diff.perc10), $(diff.perc25), $(diff.median), $(diff.perc75), $(diff.perc95), $(diff.perc99), $(diff.perc100), $(counter.count), $(counter.rate)" /usr/src/fastclick/trframework/analyzer/results/chlen$chlen/$filename$packetlen.log )

