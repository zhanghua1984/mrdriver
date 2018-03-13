#include <linux/in.h>  
#include <linux/inet.h>  
#include <linux/socket.h>  
#include <net/sock.h>  

#include <linux/init.h>  
#include <net/tcp.h>
#include "svr.h"
#include "net_main.h"

struct TPSvrAdpater TAD; 

int tpcast_server(void)
{  
	struct TPSvrAdpater *PAD = &TAD;

	memset((void*)PAD, 0, sizeof(TAD));
	
	creat_net_main_thread(PAD);

	return 0;  
}  

int server_init(void)
{
	return (tpcast_server());      
}  

void server_exit(void)
{  
	if (TAD.tcp_tx) {
		printk(KERN_ALERT "stop tcp tx thread\n");
		kthread_stop(TAD.tcp_tx);
	}

	if (TAD.tcp_rx) {
		printk(KERN_ALERT "stop tcp rx thread\n");
		kthread_stop(TAD.tcp_rx);
	} 

	if (TAD.tcp_main) {
		printk(KERN_ALERT "stop tcp main thread\n");
		kthread_stop(TAD.tcp_main);
	}


	if (TAD.client_sock) {
		printk(KERN_ALERT "release client sock\n");

		sock_release(TAD.client_sock);
		kfree(TAD.client_sock);
	}

	if (TAD.sock) {
		printk(KERN_ALERT "release svr sock\n");
		
		sock_release(TAD.sock);
		kfree(TAD.sock);
	}
	
	printk(KERN_ALERT "good bye\n");  
}  
