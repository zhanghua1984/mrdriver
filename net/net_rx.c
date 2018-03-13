#include "net_rx.h"
#include "svr.h"

unsigned char net_rcvbuf[RECEIVE_BUF_LEVEL][RECEIVE_BUF_SIZE];
unsigned char net_rcvbuf_index = 1;
unsigned char net_ready = 0;

int thread_fn_net_rx(void *data)
{
	struct kvec vec;  
	struct msghdr msg; 
	struct TPSvrAdpater *PAD = (struct TPSvrAdpater*)data;
	struct socket *sock = PAD->client_sock;
	int ret;
	//int i;
	unsigned char net_temp_rcvbuf[1024];

	//printk(KERN_ALERT "thread_fn_rx: begin : %x\n", sock);  

	memset(net_temp_rcvbuf, 0, 1024);  
	/*receive message from client*/  
	memset(&vec,0,sizeof(vec));  
	memset(&msg,0,sizeof(msg));  
	vec.iov_base=net_temp_rcvbuf;  
	vec.iov_len=1024;  

	sock->sk->sk_allocation = GFP_NOIO;
	msleep(1000);

	msg.msg_name    = NULL;    

	msg.msg_namelen = 0;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags  = MSG_NOSIGNAL;

	printk(KERN_ALERT "server: begin recv \n");  
	net_ready = 1;
	while (!kthread_should_stop()) {
		if (sock == NULL) {
			printk(KERN_ALERT "error sock null\n");  
			while (!kthread_should_stop()) {
				printk(KERN_ALERT "thread_fn_net_rx wait exit\n");  
				msleep(2000);
			}
		}

		ret = kernel_recvmsg(sock,&msg,&vec,1,1024, 0/*MSG_WAITALL*/); /*receive message*/  

		if (ret < 0) {
			if (ret == -11) {
				printk(KERN_ALERT "rcv time out\n");  
				continue;//time out
			}

			printk(KERN_ALERT "error rcv ret : %-d\n", ret);  

			while (!kthread_should_stop()) {
				printk(KERN_ALERT "thread_fn_net_rx wait exit\n");  
				msleep(2000);
			}
		} else if (ret == 0) {
			if (sock == NULL) {
				printk(KERN_ALERT "ret len = 0\n");  
				printk(KERN_ALERT "error sock null\n");  
				while (!kthread_should_stop()) {
					printk(KERN_ALERT "thread_fn_net_rx wait exit\n");  
					msleep(2000);
				}
			}
		} else {
			memset(net_rcvbuf[net_rcvbuf_index], 0, RECEIVE_BUF_SIZE);
			memcpy(net_rcvbuf[net_rcvbuf_index], net_temp_rcvbuf, ret); 
		/*	
			printk(KERN_ALERT "net_rcvbuf_index=%d,ret=%d\r\n", net_rcvbuf_index, ret);  
			for (i=0; i<8; i++) {
				printk(KERN_ALERT "%02x ", net_rcvbuf[net_rcvbuf_index][48+i]);  
			}	
			printk(KERN_ALERT "\r\n");  
		*/
			
			net_rcvbuf_index++; 
		}

	}

	return 0;
}





