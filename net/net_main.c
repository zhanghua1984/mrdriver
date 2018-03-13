#include "net_main.h"
#include "net_rx.h"
#include "net_tx.h"
#include "svr.h"

#define TPCAST_SVR_PORT_NUMBER 3240 

int tcp_keepalive(struct socket *sock)
{
	int keepidle = 5; /* send a probe 'keepidle' secs after last data */
	int keepcnt = 1; /* number of unack'ed probes before declaring dead */
	int keepalive = 1;
	int ret = 0;
	int opt_value; 
	int opt_len;

	ret = kernel_setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE,
			(char *)&keepalive, sizeof(keepalive));
	if (ret < 0)
		goto fail;

	ret = kernel_setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT,
			(char *)&keepcnt, sizeof(keepcnt));
	if (ret < 0)
		goto fail;

	ret = kernel_setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE,
			(char *)&keepidle, sizeof(keepidle));
	if (ret < 0)
		goto fail;

	/* KEEPINTVL is the interval between successive probes. We follow
	 *      ¦* the model in xs_tcp_finish_connecting() and re-use keepidle.
	 *           ¦*/
	ret = kernel_setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL,
			(char *)&keepidle, sizeof(keepidle));

	opt_len = sizeof(opt_value);
	ret = kernel_getsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, (char*)&opt_value, &opt_len);
	if (ret) {
		printk(KERN_ALERT "server:socket_create error!\n");  
		return ret;
	}
	else
	{
		printk(KERN_ALERT "server: keepidle : %d\n", opt_value);
	}


fail:
	return ret;
}

int tcp_reuse_addr(struct socket *sock)
{
	int ret ;

	int opt_val = 1;
	ret = kernel_setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt_val, sizeof(opt_val));
	if (ret < 0)
	{
		printk(KERN_ALERT "sock set reuse err\n");
		return ret;
	}
	return 0;
}

int tcp_set_nodelay(struct socket *sock)
{
	int ret ;

	int opt_val = 1;
	ret = kernel_setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&opt_val, sizeof(opt_val));
	if (ret < 0)
	{
		printk(KERN_ALERT "sock set nodelay err\n");
		return ret;
	}
	return 0;
}

int init_tcp(struct TPSvrAdpater *PAD)
{
	struct socket *sock;  
	struct sockaddr_in s_addr;  
	int ret=0;  
	unsigned short portnum = TPCAST_SVR_PORT_NUMBER;  

	memset(&s_addr,0,sizeof(s_addr));  
	s_addr.sin_family=AF_INET;  
	s_addr.sin_port=htons(portnum);  
	s_addr.sin_addr.s_addr=htonl(INADDR_ANY);  
	sock = (struct socket *)kmalloc(sizeof(struct socket),GFP_KERNEL);  

	/*create a socket*/  
	ret = sock_create_kern(&init_net, AF_INET, SOCK_STREAM,0,&sock);  
	if (ret) {  
		printk(KERN_ALERT "server:socket_create error!\n");  
	}  
	printk(KERN_ALERT "server:socket_create ok!\n");  
	PAD->sock = sock;

	/*set tcp options*/
	tcp_keepalive(sock);
	tcp_reuse_addr(sock);
	tcp_set_nodelay(sock);

	/*bind the socket*/  
	ret = sock->ops->bind(sock,(struct sockaddr *)&s_addr,sizeof(struct sockaddr_in));  
	if (ret < 0) {  
		printk(KERN_ALERT "server: bind error\n");  
		return ret;  
	}  
	printk(KERN_ALERT "server:bind ok!\n");  

	/*listen*/  
	ret = sock->ops->listen(sock,10);  
	if (ret < 0) {  
		printk(KERN_ALERT "server: listen error\n");  
		return ret;  
	}  
	printk(KERN_ALERT "server:listen ok!\n");  

	return 0;  
}

int init_net_tx_rx_threads(struct TPSvrAdpater *PAD)      
{
	PAD->tcp_rx =  kthread_run(thread_fn_net_rx, PAD, "svr rx thread" );
	PAD->tcp_tx =  kthread_run(thread_fn_net_tx, PAD, "svr tx thread" );

	return 0;
}



int thread_fn_net_main(void *data)
{
	int ret;
	struct socket *sock,*client_sock;  
	struct TPSvrAdpater *PAD = (struct TPSvrAdpater*)data;
	struct timeval timeout;

	printk(KERN_ALERT "thread_fn_main: begin\n");

	sock = PAD->sock;
	//printk(KERN_ALERT "thread_fn_main: sock: %x\n", sock);
	do //while (1)
	{
		client_sock = (struct socket *)kmalloc(sizeof(struct socket),GFP_KERNEL); 
		ret = sock_create_kern(&init_net, AF_INET, SOCK_STREAM,0,&client_sock);
		PAD->client_sock = client_sock;

		timeout.tv_sec=3;
		timeout.tv_usec=0;
#if 0
		ret = kernel_setsockopt(PAD->sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(timeout));
		if (ret < 0){  
			printk(KERN_ALERT "kernel_setsockopt server socket SO_RCVTIMEO error!\n");  
			return ret;  
		}  

		while (!kthread_should_stop()) {
			ret=sock->ops->accept(sock, client_sock, 10);
			if (ret < 0) {  
				printk(KERN_ALERT "server:accept ret=%-d\n", ret);  
			}  
		}
		if (kthread_should_stop()) {
			return 0; 
		}
#endif
		ret=sock->ops->accept(sock, client_sock, 10);
		if (ret < 0) {  
			printk(KERN_ALERT "server:accept ret=%-d\n", ret);  
		}  

		printk(KERN_ALERT "server: accept ok, Connection Established\n");  

		ret = kernel_setsockopt(PAD->client_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,sizeof(timeout));
		if (ret < 0){  
			printk(KERN_ALERT "kernel_setsockopt client SO_RCVTIMEO error!\n");  
			return ret;  
		}  

		init_net_tx_rx_threads(PAD);

		break;
	} while(0);

	while (!kthread_should_stop()) {
		msleep(2000);
	}

	printk(KERN_ALERT "thread_fn_main: exitn\n");


	return 0; 
}



int creat_net_main_thread(void *data)
{
	struct TPSvrAdpater *PAD = (struct TPSvrAdpater*)data;
	int ret;

	ret = init_tcp(PAD);
	if (ret == -1)
		if (ret != 0) {  
			printk(KERN_ALERT "init tcp error!\n");  
			return -1;  
		}      

	PAD->tcp_main = kthread_run(thread_fn_net_main, (void*)PAD, "svr main thread" ); 

	return 0;
}


