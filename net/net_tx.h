#ifndef __NET_TX__H
#define __NET_TX__H

int thread_fn_net_tx(void *data);
int net_send_data(unsigned char* buf, unsigned int len);

#endif
