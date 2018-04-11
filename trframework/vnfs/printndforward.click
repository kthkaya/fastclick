FromDPDKDevice(0, PROMISC true, VERBOSE 1) -> Print(CONTENTS true, MAXLENGTH 1500) -> ToDPDKDevice(1);
FromDPDKDevice(1, PROMISC true, VERBOSE 1) -> Print(CONTENTS true, MAXLENGTH 1500) -> ToDPDKDevice(0);

