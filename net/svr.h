#include <linux/compiler.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/net.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/sched.h>	//wake_up_process()
#include <linux/kthread.h>	//kthread_create()、kthread_run()
#include <linux/err.h>		//IS_ERR()、PTR_ERR()
#include <linux/socket.h>   
#include <net/sock.h> 
#include <linux/miscdevice.h>
#include <net/tcp.h>

struct TPSvrAdpater
{
    struct socket *sock;
    struct socket *client_sock;

    struct task_struct *tcp_main;
    struct task_struct *tcp_rx;
    struct task_struct *tcp_tx;
    struct task_struct *usb_main;
};

extern struct TPSvrAdpater TAD; 


#define RECEIVE_BUF_LEVEL 256
#define RECEIVE_BUF_SIZE 256

#define DATA01_SEND_BUF_LEVEL 2000
#define DATA01_SEND_BUF_SIZE 24572+38

#define DATA02_SEND_BUF_LEVEL 300
#define DATA02_SEND_BUF_SIZE 507+38

#define CTRL_SEND_BUF_LEVEL 300
#define CTRL_SEND_BUF_SIZE 255

extern unsigned char net_rcvbuf[RECEIVE_BUF_LEVEL][RECEIVE_BUF_SIZE];
extern unsigned char net_rcvbuf_index;

extern unsigned char net_data01_sndbuf[DATA01_SEND_BUF_LEVEL][DATA01_SEND_BUF_SIZE];
extern unsigned int net_data01_sndbuf_index;
extern unsigned long long dbg_net_data01_send_count;
extern unsigned long long dbg_usb_data01_in_count;

extern unsigned char net_data02_sndbuf[DATA02_SEND_BUF_LEVEL][DATA02_SEND_BUF_SIZE];
extern unsigned int net_data02_sndbuf_index;
extern unsigned long long dbg_net_data02_send_count;
extern unsigned long long dbg_usb_data02_in_count;

extern unsigned char net_ctrl_sndbuf[CTRL_SEND_BUF_LEVEL][CTRL_SEND_BUF_SIZE];
extern unsigned int net_ctrl_sndbuf_index;
extern unsigned long long dbg_net_ctrl_send_count;
extern unsigned long long dbg_usb_ctrl_in_count;

extern unsigned char net_ready;


extern int server_init(void);
extern void server_exit(void);
