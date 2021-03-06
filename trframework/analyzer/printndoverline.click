//------------Declarations-----------

//+++++DPDK0 Interface(Inside)+++++
//DPDK0_IN :: FromDPDKDevice(0, PROMISC true);
DPDK0_OUT ::  ToDPDKDevice(0);

//+++++DPDK1 Interface(Outside)+++++
DPDK1_IN :: FromDPDKDevice(1, PROMISC true);
//DPDK1_OUT :: ToDPDKDevice(1);


//--------Program Start----------
InfiniteSource(LENGTH 64, LIMIT 100, STOP true)
        -> UDPIP6Encap(2001:2001:2001:2001::1, 1234, 64:ff9b::192.168.2.5, 1234)
        -> EtherEncap(0x86DD, 00:04:23:D0:93:63, 00:17:cb:0d:f8:db)
        -> Print(out, CONTENTS true, MAXLENGTH 1500)
        -> DPDK0_OUT;

DPDK1_IN
        -> Print(in, CONTENTS true, MAXLENGTH 1500)
        -> counter :: AverageCounter()
        -> Discard;
//--------Program End----------
DriverManager(wait,read counter.count)

