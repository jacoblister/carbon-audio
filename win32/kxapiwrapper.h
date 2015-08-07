void *kxapi_open(void);
int kxapi_connect_microcode(void *kx_interface, int pgm1, int src, int pgm2, int dst);
int kxapi_disconnect_microcode(void *kx_interface, int pgm1, int src);
int kxapi_get_efx_register(void *kx_interface,int pgm,int id, long *val);
int kxapi_close(void *kx_interface);