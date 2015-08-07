#include <sys/io.h>

short inpout32_enable(short PortAddress, short range, short enable) {
    return ioperm(PortAddress, range, enable);
}

short inpout32_in(short PortAddress) {
    return inb(PortAddress);
}

void inpout32_out(short PortAddress, short data) {
}    
