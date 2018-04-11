NAT64  :: StatefulTranslator64();

FromDPDKDevice(0, PROMISC true, VERBOSE 1)
                -> Strip(14)
                -> NAT64
                -> EtherEncap(0x0800, 00:17:cb:02:c8:d1, 00:04:23:d1:5a:54)
//              -> Print(CONTENTS true, MAXLENGTH 1500)
                -> ToDPDKDevice(1);

FromDPDKDevice(1, PROMISC true, VERBOSE 1)
                -> Strip(14)
                -> [1]NAT64[1]
                -> EtherEncap(0x86DD, 00:17:cb:02:c8:d1, 00:04:23:d1:5a:54)
                -> ToDPDKDevice(0);

