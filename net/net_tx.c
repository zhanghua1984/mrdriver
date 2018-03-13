#include "net_tx.h"
#include "svr.h"
#include "../file/logfile.h"

unsigned char net_data01_sndbuf[DATA01_SEND_BUF_LEVEL][DATA01_SEND_BUF_SIZE];
unsigned int net_data01_sndbuf_index = 1;
unsigned long long dbg_net_data01_send_count = 0;
unsigned long long dbg_usb_data01_in_count = 0;

unsigned char net_data02_sndbuf[DATA02_SEND_BUF_LEVEL][DATA02_SEND_BUF_SIZE];
unsigned int net_data02_sndbuf_index = 1;
unsigned long long dbg_net_data02_send_count = 0;
unsigned long long dbg_usb_data02_in_count = 0;

unsigned char net_ctrl_sndbuf[CTRL_SEND_BUF_LEVEL][CTRL_SEND_BUF_SIZE];
unsigned int net_ctrl_sndbuf_index = 1;
unsigned long long dbg_net_ctrl_send_count = 0;
unsigned long long dbg_usb_ctrl_in_count = 0;

#if 0 //FRAME_TEST
extern unsigned char frame_buf[6000000];
extern unsigned long frame_offset;
extern unsigned long frame_finish;
int net_send_data(unsigned char* buf, unsigned int len);
int thread_fn_net_tx(void *data)
{
	int times = 0;
	int i = 0;
	struct file *fp; 

	fp = log_file_open();
	
	while (!kthread_should_stop()) {
		if (frame_finish == 1) {
			times = frame_offset/1024;
			times += 1;
			printk(KERN_ALERT "send times = %d\n", times);  
			for (i=0; i<times; i++) {
				log_file_write(fp, &frame_buf[i*1024], 1024);
				if (net_send_data(&frame_buf[i*1024], 1024) < 0) {
					while (!kthread_should_stop());
				}
			}
			log_file_close(fp);
			while (!kthread_should_stop());
		}
	}

	printk(KERN_ALERT "thread_fn_tx: exit\n");  

	return 0;
}
#endif 

#if 1
int thread_fn_net_tx(void *data)
{
	struct kvec vec;  
	struct msghdr msg; 
	struct TPSvrAdpater *PAD = (struct TPSvrAdpater*)data;
	struct socket *sock = PAD->client_sock;
	int ret;
	unsigned int len = 0;
	static unsigned int crnt01 = 0;
	static unsigned int crnt02 = 0;
	static unsigned int crnt03 = 0;
	unsigned int last = 0;
	//struct file *fp; 

	//fp = log_file_open();
	

	printk(KERN_ALERT "thread_fn_tx: begin\n");  

	/*receive message from client*/  
	memset(&vec,0,sizeof(vec));  
	memset(&msg,0,sizeof(msg));  

	while (!kthread_should_stop()) {
		//send data 01------------------------------------------------------------
		if (net_data01_sndbuf_index == 0) {
			last = DATA01_SEND_BUF_LEVEL-1;
		} else {
			last = net_data01_sndbuf_index-1;
		}
		if ( crnt01 != last) {
			if (crnt01 < (DATA01_SEND_BUF_LEVEL-1)) {
				crnt01++;
			} else {
				crnt01 = 0;
			}
			//do send data
			memset(&vec,0,sizeof(vec));  
			memset(&msg,0,sizeof(msg));  
			vec.iov_base = &net_data01_sndbuf[crnt01];  
			len = (0x00FF&net_data01_sndbuf[crnt01][8+1])<<8;
			len |= (0x00FF&net_data01_sndbuf[crnt01][8+0]);
			len += 8+2+38;
			vec.iov_len = len;  
			//log_file_write(fp, &net_sndbuf[crnt][2], len);
			ret = kernel_sendmsg(sock, &msg, &vec, 1, len);
			dbg_net_data01_send_count++;
			if (ret != len) {
				printk(KERN_ALERT "send data01 error ret=%-d,len=%d\n", ret, len);
				break;
			}
		} else {
			if ((dbg_usb_data01_in_count - dbg_net_data01_send_count) > (DATA01_SEND_BUF_LEVEL-1)) {
				printk(KERN_ALERT "running buf out error,\
					dbg_net_data01_send_count=%lld,\
					dbg_usb_data01_in_count=%lld\r\n",
					dbg_net_data01_send_count, dbg_usb_data01_in_count);
				
				//break;

			}
		}

		//send data 02------------------------------------------------------------
		if (net_data02_sndbuf_index == 0) {
			last = DATA02_SEND_BUF_LEVEL-1;
		} else {
			last = net_data02_sndbuf_index-1;
		}
		if ( crnt02 != last) {
			if (crnt02 < (DATA02_SEND_BUF_LEVEL-1)) {
				crnt02++;
			} else {
				crnt02 = 0;
			}
			//do send data
			memset(&vec,0,sizeof(vec));  
			memset(&msg,0,sizeof(msg));  
			vec.iov_base = &net_data02_sndbuf[crnt02];  
			len = (0x00FF&net_data02_sndbuf[crnt02][8+1])<<8;
			len |= (0x00FF&net_data02_sndbuf[crnt02][8+0]);
			len += 8+2+38;
			vec.iov_len = len;  
			
			//log_file_write(fp, &net_sndbuf[crnt][2], len);
			ret = kernel_sendmsg(sock, &msg, &vec, 1, len);
			dbg_net_data02_send_count++;
			if (ret != len) {
				printk(KERN_ALERT "send data02 error ret=%-d,len=%d\n", ret, len);
				break;
			}
		} else {
			if ((dbg_usb_data02_in_count - dbg_net_data02_send_count) > (DATA02_SEND_BUF_LEVEL-1)) {
				printk(KERN_ALERT "running buf out error,\
					dbg_net_data02_send_count=%lld,\
					dbg_usb_data02_in_count=%lld\r\n", 
					dbg_net_data02_send_count, dbg_usb_data02_in_count);
				
				break;

			}
		}
		
		//send ctrl ------------------------------------------------------------
		if (net_ctrl_sndbuf_index == 0) {
			last = CTRL_SEND_BUF_LEVEL-1;
		} else {
			last = net_ctrl_sndbuf_index-1;
		}
		if ( crnt03 != last) {
			if (crnt03 < (CTRL_SEND_BUF_LEVEL-1)) {
				crnt03++;
			} else {
				crnt03 = 0;
			}
			//do send data
			memset(&vec,0,sizeof(vec));  
			memset(&msg,0,sizeof(msg));  
			vec.iov_base = &net_ctrl_sndbuf[crnt03];  
			len = (0x00FF&net_ctrl_sndbuf[crnt03][26])<<8;
			len |= (0x00FF&net_ctrl_sndbuf[crnt03][27]);
			len += 48;
			vec.iov_len = len;  
			
			//log_file_write(fp, &net_sndbuf[crnt][2], len);
			ret = kernel_sendmsg(sock, &msg, &vec, 1, len);
			dbg_net_ctrl_send_count++;
			if (ret != len) {
				printk(KERN_ALERT "send ctrl error ret=%-d,len=%d\n", ret, len);
				break;
			}
		} else {
			if ((dbg_usb_ctrl_in_count - dbg_net_ctrl_send_count) > (CTRL_SEND_BUF_LEVEL-1)) {
				printk(KERN_ALERT "running buf out error,\
					dbg_net_ctrl_send_count=%lld,\
					dbg_usb_ctrl_in_count=%lld\r\n", 
					dbg_net_ctrl_send_count, dbg_usb_ctrl_in_count);
				
				break;

			}
		}


		//msleep(0);
	}

	//log_file_close(fp);
	while (!kthread_should_stop()) {
		//printk(KERN_ALERT "thread_fn_tx wait exit\n");  
		msleep(2000);
	}

	printk(KERN_ALERT "thread_fn_tx: exit\n");  

	return 0;
}
#endif

int net_send_data(unsigned char* buf, unsigned int len)
{
	struct kvec vec;  
	struct msghdr msg; 
	struct socket *sock = TAD.client_sock;
	int ret;

	//do send data
	memset(&vec,0,sizeof(vec));  
	memset(&msg,0,sizeof(msg));  
	vec.iov_base = buf;  
	vec.iov_len = len;  

	ret = kernel_sendmsg(sock, &msg, &vec, 1, len);

	if (ret != len) {
		printk(KERN_ALERT "send error ret=%-d,len=%d\n", ret, len);
		return -1;
	}

	return 0;
}
