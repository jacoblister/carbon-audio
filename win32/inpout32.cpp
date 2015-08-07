short _stdcall Inp32(short PortAddress);
void _stdcall Out32(short PortAddress, short data);

extern "C" {

short inpout32_enable(short PortAddress, short range, short enable) {
    return 1;
}
    
short inpout32_in(short PortAddress) {
    return Inp32(PortAddress);
}

void inpout32_out(short PortAddress, short data) {
}    
    
}