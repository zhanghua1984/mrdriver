#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <linux/hid.h>
#include <linux/kthread.h>
#include "net/svr.h"
#include "file/logfile.h"
#include <linux/time.h>  
#include <linux/timer.h>  

/*
 * Version Information
 */
#define DRIVER_VERSION ""
#define DRIVER_AUTHOR "qqq"
#define DRIVER_DESC "mr driver"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

struct file *fp; 

struct usb_mr {
	struct usb_device *usbdev;
	struct urb *urb_data01;
	struct urb *urb_data02;
	struct urb *urb_data03;
	struct urb *urb_ctrl;

	unsigned char *data01_in_buf;
	unsigned char *data02_in_buf;
	unsigned char *data03_out_buf;
	struct usb_ctrlrequest *cr;
	unsigned char *ctrl_buf;
	dma_addr_t data01_in_buf_dma;
	dma_addr_t data02_in_buf_dma;
	dma_addr_t data03_out_buf_dma;
	dma_addr_t ctrl_buf_dma;
};

struct usb_mr *mr;
int  mr_data01_pkg_size;
int  mr_data02_pkg_size;
int  mr_data03_pkg_size;
unsigned char send_command_step = 0;
static struct task_struct * thread_usb_handle = NULL;

static void usb_mr_ctrl_callback(struct urb *urb);
static void usb_mr_data03_out_callback(struct urb *urb);

unsigned int package_index0 = 0;
unsigned char package_devnum = 0;
unsigned char usb_busy = 0;

struct timespec tv1;  
struct timespec tv2;  


void send_command_get_name(struct usb_mr *mr)
{
	printk(KERN_ALERT "send_command_get_name\r\n"); 

	mr->cr->bRequestType = 0x80;
	mr->cr->bRequest = 0x06;
	mr->cr->wValue = cpu_to_le16(0x0302);
	//mr->cr->wIndex = cpu_to_le16(interface->desc.bInterfaceNumber);
	mr->cr->wIndex = cpu_to_le16(0x0409);
	mr->cr->wLength = cpu_to_le16(0x64);

	usb_fill_control_urb(mr->urb_ctrl, mr->usbdev, usb_rcvctrlpipe(mr->usbdev, 0),
			(void *) mr->cr, mr->ctrl_buf, 0x64,
			usb_mr_ctrl_callback, mr);
	mr->urb_ctrl->transfer_dma = mr->ctrl_buf_dma;
	mr->urb_ctrl->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	if (usb_submit_urb(mr->urb_ctrl, GFP_ATOMIC))
		printk(KERN_ALERT "usb_submit_urb(ctrl) failed\n");

	printk(KERN_ALERT "usb_submit_urb control\r\n"); 

}

//010001 01 15160500 0000000000000000000000650900009000000060e31600000000
//010001 02 07b20100 0000000000000000000000650900009000000060e31600000000

//000001 01 15160500 0000000000000000000000650900009000000060e31600000000
//000001 02 07b20100 0000000000000000000000650900009000000060e31600000000
#define FPS90
void send_command_01(struct usb_mr *mr)
{
#ifdef FPS90
	unsigned char buf[0x22] = {
		0x01,0x00,0x01,0x02,0x07,0xb2,0x01,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x65,0x09,0x00,0x00,0x90,
		0x00,0x00,0x00,0x60,0xe3,0x16,0x00,0x00,0x00 ,0x00
	};
#else
	unsigned char buf[0x22] = {
		0x01,0x00,0x01,0x01,0x15,0x16,0x05,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x65,0x09,0x00,0x00,0x90,
		0x00,0x00,0x00,0x60,0xe3,0x16,0x00,0x00,0x00,0x00
	};
#endif

	printk(KERN_ALERT "send_command_01\r\n"); 

	mr->cr->bRequestType = 0x21;
	mr->cr->bRequest = 0x01;
	mr->cr->wValue = cpu_to_le16(0x0100);
	mr->cr->wIndex = cpu_to_le16(0x0001);
	mr->cr->wLength = cpu_to_le16(0x0022);
	memcpy(mr->ctrl_buf, buf, 0x22);

	usb_fill_control_urb(mr->urb_ctrl, mr->usbdev, usb_sndctrlpipe(mr->usbdev, 0),
			(void *) mr->cr, mr->ctrl_buf, 0x22,
			usb_mr_ctrl_callback, mr);
	mr->urb_ctrl->transfer_dma = mr->ctrl_buf_dma;
	mr->urb_ctrl->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	if (usb_submit_urb(mr->urb_ctrl, GFP_ATOMIC))
		printk(KERN_ALERT "usb_submit_urb(ctrl) failed\n");

	printk(KERN_ALERT "usb_submit_urb control\r\n"); 

}

void send_command_02(struct usb_mr *mr)
{
#ifdef FPS90
	unsigned char buf[0x22] = {
		0x00,0x00,0x01,0x02,0x07,0xb2,0x01,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x65,0x09,0x00,0x00,0x90,
		0x00,0x00,0x00,0x60,0xe3,0x16,0x00,0x00,0x00 ,0x00
	};
#else
	unsigned char buf[0x22] = {
		0x00,0x00,0x01,0x01,0x15,0x16,0x05,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x65,0x09,0x00,0x00,0x90,
		0x00,0x00,0x00,0x60,0xe3,0x16,0x00,0x00,0x00,0x00
	};
#endif

	printk(KERN_ALERT "send_command_02\r\n"); 

	mr->cr->bRequestType = 0x21;
	mr->cr->bRequest = 0x01;
	mr->cr->wValue = cpu_to_le16(0x0200);
	mr->cr->wIndex = cpu_to_le16(0x0001);
	mr->cr->wLength = cpu_to_le16(0x0022);
	memcpy(mr->ctrl_buf, buf, 0x22);

	usb_fill_control_urb(mr->urb_ctrl, mr->usbdev, usb_sndctrlpipe(mr->usbdev, 0),
			(void *) mr->cr, mr->ctrl_buf, 0x22,
			usb_mr_ctrl_callback, mr);
	mr->urb_ctrl->transfer_dma = mr->ctrl_buf_dma;
	mr->urb_ctrl->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	if (usb_submit_urb(mr->urb_ctrl, GFP_ATOMIC))
		printk(KERN_ALERT "usb_submit_urb(ctrl) failed\n");

	printk(KERN_ALERT "usb_submit_urb control\r\n"); 
}

void send_command_hid_01(struct usb_mr *mr)
{
	unsigned char buf[0x40] = {
		0x02,0x0b,0x00,0x00,0x00,0x00,0x00,0x00,
		0x1d,0xce,0x24,0x57,0xf8,0x7f,0x00,0x00,
		0x70,0x32,0x6d,0xfd,0x5c,0x02,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0xe0,0x30,0xf7,0x4b,0xf8,0x7f,0x00,0x00,
		0xd0,0x48,0xf5,0x4b,0xf8,0x7f,0x00,0x00,
		0x0d,0x31,0xce,0xce,0xb1,0x0d,0x00,0x00,
		0x17,0x19,0xea,0x37,0xf8,0x7f,0x00,0x00
	};

	printk(KERN_ALERT "send_command_hid_01\r\n"); 

	//2109020202004000
	mr->cr->bRequestType = 0x21;
	mr->cr->bRequest = 0x09;
	mr->cr->wValue = cpu_to_le16(0x0202);
	mr->cr->wIndex = cpu_to_le16(0x0002);
	mr->cr->wLength = cpu_to_le16(0x0040);
	memcpy(mr->ctrl_buf, buf, 0x40);

	usb_fill_control_urb(mr->urb_ctrl, mr->usbdev, usb_sndctrlpipe(mr->usbdev, 0),
			(void *) mr->cr, mr->ctrl_buf, 0x40,
			usb_mr_ctrl_callback, mr);
	mr->urb_ctrl->transfer_dma = mr->ctrl_buf_dma;
	mr->urb_ctrl->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	if (usb_submit_urb(mr->urb_ctrl, GFP_ATOMIC))
		printk(KERN_ALERT "usb_submit_urb(ctrl) failed\n");

	printk(KERN_ALERT "usb_submit_urb control\r\n"); 
}

void send_command_hid_02(struct usb_mr *mr)
{
	unsigned char buf[0x40] = {
		0x02,0x06,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
	};

	printk(KERN_ALERT "send_command_hid_02\r\n"); 

	//2109020202004000
	mr->cr->bRequestType = 0x21;
	mr->cr->bRequest = 0x09;
	mr->cr->wValue = cpu_to_le16(0x0202);
	mr->cr->wIndex = cpu_to_le16(0x0002);
	mr->cr->wLength = cpu_to_le16(0x0040);
	memcpy(mr->ctrl_buf, buf, 0x40);

	usb_fill_control_urb(mr->urb_ctrl, mr->usbdev, usb_sndctrlpipe(mr->usbdev, 0),
			(void *) mr->cr, mr->ctrl_buf, 0x40,
			usb_mr_ctrl_callback, mr);
	mr->urb_ctrl->transfer_dma = mr->ctrl_buf_dma;
	mr->urb_ctrl->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	if (usb_submit_urb(mr->urb_ctrl, GFP_ATOMIC))
		printk(KERN_ALERT "usb_submit_urb(ctrl) failed\n");

	printk(KERN_ALERT "usb_submit_urb control\r\n"); 
}

void send_command_hid_03(struct usb_mr *mr)
{
	unsigned char buf[0x40] = {
		0x02,0x08,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
	};

	printk(KERN_ALERT "send_command_hid_03\r\n"); 

	//2109020202004000
	mr->cr->bRequestType = 0x21;
	mr->cr->bRequest = 0x09;
	mr->cr->wValue = cpu_to_le16(0x0202);
	mr->cr->wIndex = cpu_to_le16(0x0002);
	mr->cr->wLength = cpu_to_le16(0x0040);
	memcpy(mr->ctrl_buf, buf, 0x40);

	usb_fill_control_urb(mr->urb_ctrl, mr->usbdev, usb_sndctrlpipe(mr->usbdev, 0),
			(void *) mr->cr, mr->ctrl_buf, 0x40,
			usb_mr_ctrl_callback, mr);
	mr->urb_ctrl->transfer_dma = mr->ctrl_buf_dma;
	mr->urb_ctrl->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	if (usb_submit_urb(mr->urb_ctrl, GFP_ATOMIC))
		printk(KERN_ALERT "usb_submit_urb(ctrl) failed\n");

	printk(KERN_ALERT "usb_submit_urb control\r\n"); 
}

void send_ctrl_in_command(struct usb_mr *mr,
		unsigned char bmRequestType,
		unsigned char bRequest,
		unsigned int wValue,
		unsigned int wIndex,
		unsigned int nLength,
		unsigned char *pbuf
		)
{
	//printk(KERN_ALERT "send_ctrl_in_command\r\n"); 

	mr->cr->bRequestType = bmRequestType;
	mr->cr->bRequest = bRequest;
	mr->cr->wValue = cpu_to_le16(wValue);
	mr->cr->wIndex = cpu_to_le16(wIndex);
	mr->cr->wLength = cpu_to_le16(nLength);
	if (pbuf != NULL) {
		memcpy(mr->ctrl_buf, pbuf, nLength);
	}

	usb_fill_control_urb(mr->urb_ctrl, mr->usbdev, usb_rcvctrlpipe(mr->usbdev, 0),
			(void *) mr->cr, mr->ctrl_buf, nLength,
			usb_mr_ctrl_callback, mr);
	mr->urb_ctrl->transfer_dma = mr->ctrl_buf_dma;
	mr->urb_ctrl->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	if (usb_submit_urb(mr->urb_ctrl, GFP_ATOMIC))
		printk(KERN_ALERT "usb_submit_urb(ctrl) failed\n");

	//printk(KERN_ALERT "usb_submit_urb control\r\n"); 
}

void send_ctrl_out_command(struct usb_mr *mr,
		unsigned char bmRequestType,
		unsigned char bRequest,
		unsigned int wValue,
		unsigned int wIndex,
		unsigned int nLength,
		unsigned char *pbuf
		)
{
	//printk(KERN_ALERT "send_ctrl_out_command\r\n"); 

	mr->cr->bRequestType = bmRequestType;
	mr->cr->bRequest = bRequest;
	mr->cr->wValue = cpu_to_le16(wValue);
	mr->cr->wIndex = cpu_to_le16(wIndex);
	mr->cr->wLength = cpu_to_le16(nLength);
	if (pbuf != NULL) {
		memcpy(mr->ctrl_buf, pbuf, nLength);
	}

	usb_fill_control_urb(mr->urb_ctrl, mr->usbdev, usb_sndctrlpipe(mr->usbdev, 0),
			(void *) mr->cr, mr->ctrl_buf, nLength,
			usb_mr_ctrl_callback, mr);
	mr->urb_ctrl->transfer_dma = mr->ctrl_buf_dma;
	mr->urb_ctrl->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	if (usb_submit_urb(mr->urb_ctrl, GFP_ATOMIC))
		printk(KERN_ALERT "usb_submit_urb(ctrl) failed\n");

	//printk(KERN_ALERT "usb_submit_urb control\r\n"); 
}

void send_vendor_out_data(struct usb_mr *mr,
		unsigned int nLength,
		unsigned char *pbuf
		)
{
	//printk(KERN_ALERT "send_vendor_out_data\n"); 

	if (pbuf != NULL) {
		memcpy(mr->data03_out_buf, pbuf, nLength);
	}

	usb_fill_bulk_urb(mr->urb_data03, mr->usbdev, usb_sndbulkpipe(mr->usbdev, 0x05),
			mr->data03_out_buf, nLength,
			usb_mr_data03_out_callback, mr);
	mr->urb_data03->transfer_dma = mr->data03_out_buf_dma;
	mr->urb_data03->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	if (usb_submit_urb(mr->urb_data03, GFP_ATOMIC))
		printk(KERN_ALERT "usb_submit_urb(urb_data03) failed\n");

	//printk(KERN_ALERT "usb_submit_urb urb_data03\r\n"); 
}

#define DBG_PRINT 0
void usb_send_task(struct usb_mr *mr)
{
	static unsigned char crnt = 0;
	unsigned char last = 0;
	unsigned char *pbuf;
	unsigned char usb_data_bmRequestType;
	unsigned char usb_data_bRequest;
	unsigned char usb_data_wValue_L;
	unsigned char usb_data_wValue_H;
	unsigned char usb_data_wIndex_L;
	unsigned char usb_data_wIndex_H;
	unsigned char usb_data_length;
#if DBG_PRINT
	unsigned int offset;
	unsigned int i;
	unsigned char str[100];
#endif
/*
	printk(KERN_ALERT "--------------------------------\r\n");
	printk(KERN_ALERT "---usb_busy=%d\r\n", usb_busy);
	printk(KERN_ALERT "---crnt=%d\r\n", crnt);
	printk(KERN_ALERT "---net_rcvbuf_index=%d\r\n", net_rcvbuf_index);
*/
	if (usb_busy == 0) {
		last = net_rcvbuf_index-1;
		if ( crnt != last) {
			crnt++;
			//do receive data
			//printk(KERN_ALERT "-%02x---", net_rcvbuf[crnt][11]);

			if (net_rcvbuf[crnt][19] == 0x00) {
				/*
				for (i=0; i<8; i++) {
					printk(KERN_ALERT "%02x ", net_rcvbuf[crnt][40+i]);
				}
				printk(KERN_ALERT "\r\n");
				*/

				//get package index
				package_index0 = 0;
				package_index0 |= net_rcvbuf[crnt][4];
				package_index0 <<= 8;
				package_index0 |= net_rcvbuf[crnt][5];
				package_index0 <<= 8;
				package_index0 |= net_rcvbuf[crnt][6];
				package_index0 <<= 8;
				package_index0 |= net_rcvbuf[crnt][7];

				package_devnum = net_rcvbuf[crnt][11];

				if (net_rcvbuf[crnt][11] == 0x80) { //get report
					usb_data_bmRequestType = net_rcvbuf[crnt][48];
					usb_data_bRequest = net_rcvbuf[crnt][48+1];
					usb_data_wValue_L = net_rcvbuf[crnt][48+2];
					usb_data_wValue_H = net_rcvbuf[crnt][48+3];
					usb_data_wIndex_L = net_rcvbuf[crnt][48+4];
					usb_data_wIndex_H = net_rcvbuf[crnt][48+5];
					usb_data_length = net_rcvbuf[crnt][48+6];   
					if (usb_data_length > 0) {
						pbuf = &net_rcvbuf[crnt][48+8];
					} else {
						pbuf = NULL;
					}

					usb_busy = 1;
#if DBG_PRINT
					offset = 0;
					for (i=0; i<8; i++)
					{
						offset += sprintf(&str[offset], "%02X ", net_rcvbuf[crnt][48+i]);
					}
					printk(KERN_ALERT "%s\r\n", str);
#endif
					send_ctrl_in_command(mr, 
							usb_data_bmRequestType,
							usb_data_bRequest,
							usb_data_wValue_L + usb_data_wValue_H * 256,
							usb_data_wIndex_L + usb_data_wIndex_H * 256,
							usb_data_length,
							pbuf);

				} else if (net_rcvbuf[crnt][11] == 0x00) { //set report
					usb_data_bmRequestType = net_rcvbuf[crnt][48];
					usb_data_bRequest = net_rcvbuf[crnt][48+1];
					usb_data_wValue_L = net_rcvbuf[crnt][48+2];
					usb_data_wValue_H = net_rcvbuf[crnt][48+3];
					usb_data_wIndex_L = net_rcvbuf[crnt][48+4];
					usb_data_wIndex_H = net_rcvbuf[crnt][48+5];
					usb_data_length = net_rcvbuf[crnt][48+6];   
					if (usb_data_length > 0) {
						pbuf = &net_rcvbuf[crnt][48+8];
					} else {
						pbuf = NULL;
					}

					usb_busy = 1;
#if DBG_PRINT
					offset = 0;
					for (i=0; i<8; i++)
					{
						offset += sprintf(&str[offset], "%02X ", net_rcvbuf[crnt][48+i]);
					}
					printk(KERN_ALERT "%s\r\n", str);
#endif
					
					send_ctrl_out_command(mr, 
							usb_data_bmRequestType,
							usb_data_bRequest,
							usb_data_wValue_L + usb_data_wValue_H * 256,
							usb_data_wIndex_L + usb_data_wIndex_H * 256,
							usb_data_length,
							pbuf);

				} else if (net_rcvbuf[crnt][11] == 0x05) { //vendor out
					usb_data_length = 18;   
					pbuf = &net_rcvbuf[crnt][48];

					send_vendor_out_data(mr, usb_data_length, pbuf);
				}

				msleep(3);
			}
		}

	}

}

static int thread_usb(void *data)
{
	struct usb_mr *mr = data;

	printk(KERN_ALERT "thread_usb\r\n"); 

	send_command_step = 0xF1;

	while(!kthread_should_stop()) {
		switch (send_command_step) {
			case 0x00:
				msleep(100);
				break;
			case 0x01:
				send_command_step = 0x81;
				send_command_01(mr);
				msleep(100);
				break;
			case 0x81://wait
				break;
			case 0x02:
				send_command_step = 0x82;
				send_command_02(mr);
				msleep(100);
				break;
			case 0x82://wait
				break;
			case 0x03:
				send_command_step = 0x83;
				send_command_hid_01(mr);
				msleep(100);
				break;
			case 0x83://wait
				break;
			case 0x04:
				send_command_step = 0x84;
				send_command_hid_02(mr);
				msleep(100);
				break;
			case 0x84://wait
				break;
			case 0x05:
				send_command_step = 0x85;
				send_command_hid_03(mr);
				//send_command_get_name(mr);
				msleep(100);
				break;
			case 0x85://wait
				break;
			case 0x06:
				break;
			case 0x86://wait
				break;

			case 0xF0:
				usb_send_task(mr);
				break;

			case 0xF1:
				if (net_ready == 1) {
					tv1 = current_kernel_time();  
					usb_submit_urb(mr->urb_data01, GFP_KERNEL);
					send_command_step = 0xF0;
					//send_command_step = 0x01;
				}
				msleep(100);
				break;
			default:
				break;
		}
	}



	return 0;
}

unsigned char frame_buf[6000000] = {0};
unsigned long frame_offset = 0;
unsigned char frame_finish = 0;
static void usb_mr_data01_in_callback(struct urb *urb)
{
	struct usb_mr *mr = urb->context;
	int i;

	switch (urb->status) {
		case 0:			/* success */
			break;
		case -ECONNRESET:	/* unlink */
		case -ENOENT:
		case -ESHUTDOWN:
			return;
			/* -EPIPE:  should clear the halt */
		default:		/* error */
			goto resubmit;
	}

#if 0
	printk(KERN_ALERT "---p1---\r\n"); 
	printk(KERN_ALERT "len=%d", urb->actual_length); 
	for (i=0; i<4; i++) {
		printk(KERN_ALERT "%02x ", mr->data01_in_buf[i]); 
	}
	printk(KERN_ALERT "\r\n"); 
#endif 

	/*
	   memset(net_sndbuf, 0, 1024); 
	   memcpy(net_sndbuf, mr->data01_in_buf, 1024); 
	   net_sndlen = 1024;
	 */

	memset(net_data01_sndbuf[net_data01_sndbuf_index], 0, DATA01_SEND_BUF_SIZE);
	memcpy(net_data01_sndbuf[net_data01_sndbuf_index], "tpcast01", 8);
	net_data01_sndbuf[net_data01_sndbuf_index][8+0] = (0x00FF&(urb->actual_length));
	net_data01_sndbuf[net_data01_sndbuf_index][8+1] = (0xFF00&(urb->actual_length))>>8;
	memcpy(&net_data01_sndbuf[net_data01_sndbuf_index][8+2+38], mr->data01_in_buf, urb->actual_length); 
//-test-----
//	net_data01_sndbuf[net_data01_sndbuf_index][8+2+urb->actual_length-2] = 0x5A; 
//	net_data01_sndbuf[net_data01_sndbuf_index][8+2+urb->actual_length-1] = 0xA5; 
//----------
	if (net_data01_sndbuf_index < (DATA01_SEND_BUF_LEVEL-1)) {
		net_data01_sndbuf_index++; 
	} else {
		net_data01_sndbuf_index = 0;
	}
	dbg_usb_data01_in_count++;

#if 0 //FRAME_TEST
	//printk(KERN_ALERT "len=%d\r\n", urb->actual_length); 
	memcpy(&frame_buf[frame_offset], mr->data01_in_buf, urb->actual_length); 
	frame_offset += urb->actual_length;
	//memcpy(&frame_buf[frame_offset], mr->data01_in_buf, 1024); 
	//frame_offset += 1024;
	if (frame_offset > 4925440) {
		tv2 = current_kernel_time();  
		printk(KERN_ALERT "time1=%40i.%09i\n", (int) tv1.tv_sec, (int) tv1.tv_nsec);  
		printk(KERN_ALERT "time2=%40i.%09i\n", (int) tv2.tv_sec, (int) tv2.tv_nsec);  
		printk(KERN_ALERT "---%ld---\r\n", frame_offset);
		frame_finish = 1;
		return;
	}
#endif 		
	/*
	   frame_offset++;
	   if (mr->data01_in_buf[0] == 0x22) {
	   if (mr->data01_in_buf[1] == 0x9e) {
	   printk(KERN_ALERT "---%ld---\r\n", frame_offset);
	   }

	   }
	 */
	//return;
resubmit:
	i = usb_submit_urb (urb, GFP_ATOMIC);
	if (i)
		hid_err(urb->dev, "can't resubmit intr, %s-%s, status %d",
				mr->usbdev->bus->bus_name,
				mr->usbdev->devpath, i);
}


static void usb_mr_data02_in_callback(struct urb *urb)
{
	struct usb_mr *mr = urb->context;
	int i;

	switch (urb->status) {
		case 0:			/* success */
			break;
		case -ECONNRESET:	/* unlink */
		case -ENOENT:
		case -ESHUTDOWN:
			return;
			/* -EPIPE:  should clear the halt */
		default:		/* error */
			goto resubmit;
	}

#if 0
	printk(KERN_ALERT "---p2---\r\n"); 
	printk(KERN_ALERT "len=%d", urb->actual_length); 
	for (i=0; i<4; i++) {
		printk(KERN_ALERT "%02x ", mr->data02_in_buf[i]); 
	}
	printk(KERN_ALERT "\r\n"); 
#endif 

	memset(net_data02_sndbuf[net_data02_sndbuf_index], 0, CTRL_SEND_BUF_SIZE);
	memcpy(net_data02_sndbuf[net_data02_sndbuf_index], "tpcast02", 8);
	net_data02_sndbuf[net_data02_sndbuf_index][8+0] = (0x00FF&(urb->actual_length));
	net_data02_sndbuf[net_data02_sndbuf_index][8+1] = (0xFF00&(urb->actual_length))>>8;
	memcpy(&net_data02_sndbuf[net_data02_sndbuf_index][8+2+38], mr->data02_in_buf, urb->actual_length); 
	if (net_data02_sndbuf_index < (DATA02_SEND_BUF_LEVEL-1)) {
		net_data02_sndbuf_index++; 
	} else {
		net_data02_sndbuf_index = 0;
	}
	dbg_usb_data02_in_count++;

	//return;
resubmit:
	i = usb_submit_urb (urb, GFP_ATOMIC);
	if (i)
		hid_err(urb->dev, "can't resubmit intr, %s-%s, status %d",
				mr->usbdev->bus->bus_name,
				mr->usbdev->devpath, i);
}

static void usb_mr_data03_out_callback(struct urb *urb)
{
	switch (urb->status) {
		case 0:			/* success */
			break;
		case -ECONNRESET:	/* unlink */
		case -ENOENT:
		case -ESHUTDOWN:
			return;
			/* -EPIPE:  should clear the halt */
		default:		/* error */
			goto resubmit;
	}

	//do something here
	printk(KERN_ALERT "urb->status:%08x\r\n", urb->status); 



resubmit:
	;

}

static void usb_mr_ctrl_callback(struct urb *urb)
{
	//struct usb_mr *mr = urb->context;
	//unsigned char strbuf[100];
	//unsigned int i = 0;
	//unsigned int j = 0;
	int pipe;

	if (urb->status)
		hid_warn(urb->dev, "urb status %d received\n",
				urb->status);

	switch (send_command_step) {
		case 0x00:
			break;
		case 0x81:
			send_command_step = 0x02;
			break;
		case 0x82:
			send_command_step = 0xF0;
			break;
		case 0x83:
			send_command_step = 0x04;
			break;
		case 0x84:
			send_command_step = 0x05;
			break;
		case 0x85:
			send_command_step = 0xF0;//start usb send task
			break;
		default:
			break;
	}
#if 0
	if (send_command_step == 0x83) {
		memset(strbuf, 0, 64);
		j = 0;
		printk(KERN_ALERT "rcv [0]:0x%02x, [1]:0x%02x, len:%d\r\n", mr->ctrl_buf[0], mr->ctrl_buf[1], urb->actual_length); 
		for (i=2; i<urb->actual_length; i+=2) {
			strbuf[j] = mr->ctrl_buf[i];
			j++;
		}
		printk(KERN_ALERT "name:%s\r\n", strbuf); 
	}
#endif

	//\x00\x00\x00\x03\x00\x00\x00\x22\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00

	pipe = usb_rcvctrlpipe(mr->usbdev, 0);
	//printk(KERN_ALERT "-------pipe:%02x\r\n", pipe); 
	//printk(KERN_ALERT "-------urb->pipe:%02x\r\n", urb->pipe); 
	if (pipe == urb->pipe) {
		memset(net_ctrl_sndbuf[net_ctrl_sndbuf_index], 0, DATA02_SEND_BUF_SIZE);
		//memcpy(net_ctrl_sndbuf[net_ctrl_sndbuf_index], "tpcast00", 8);
		//net_ctrl_sndbuf[net_ctrl_sndbuf_index][8+0] = (0x00FF&(urb->actual_length+48));
		//net_ctrl_sndbuf[net_ctrl_sndbuf_index][8+1] = (0xFF00&(urb->actual_length+48))>>8;
		memcpy(&net_ctrl_sndbuf[net_ctrl_sndbuf_index][0], "\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 48);
		net_ctrl_sndbuf[net_ctrl_sndbuf_index][4] = (0x000000FF)&(package_index0>>24);
		net_ctrl_sndbuf[net_ctrl_sndbuf_index][5] = (0x000000FF)&(package_index0>>16);
		net_ctrl_sndbuf[net_ctrl_sndbuf_index][6] = (0x000000FF)&(package_index0>>8);
		net_ctrl_sndbuf[net_ctrl_sndbuf_index][7] = (0x000000FF)&package_index0;
		net_ctrl_sndbuf[net_ctrl_sndbuf_index][11] = package_devnum;
		if (urb->status < 0) {
			net_ctrl_sndbuf[net_ctrl_sndbuf_index][20] = 0xFF;
			net_ctrl_sndbuf[net_ctrl_sndbuf_index][21] = 0xFF;
			net_ctrl_sndbuf[net_ctrl_sndbuf_index][22] = 0xFF;
			net_ctrl_sndbuf[net_ctrl_sndbuf_index][23] = 0xE0;
		}     
		net_ctrl_sndbuf[net_ctrl_sndbuf_index][26] = (0xFF00&urb->actual_length)>>8;
		net_ctrl_sndbuf[net_ctrl_sndbuf_index][27] = (0x00FF&urb->actual_length);

		memcpy(&net_ctrl_sndbuf[net_ctrl_sndbuf_index][48], mr->ctrl_buf, urb->actual_length); 
		if (net_ctrl_sndbuf_index < (DATA02_SEND_BUF_LEVEL-1)) {
			net_ctrl_sndbuf_index++; 
		} else {
			net_ctrl_sndbuf_index = 0;
		}
		dbg_usb_ctrl_in_count++;

		usb_busy = 0;
	} else {
		memset(net_ctrl_sndbuf[net_ctrl_sndbuf_index], 0, DATA02_SEND_BUF_SIZE);
		memcpy(&net_ctrl_sndbuf[net_ctrl_sndbuf_index][0], "\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 48);
		net_ctrl_sndbuf[net_ctrl_sndbuf_index][4] = (0x000000FF)&(package_index0>>24);
		net_ctrl_sndbuf[net_ctrl_sndbuf_index][5] = (0x000000FF)&(package_index0>>16);
		net_ctrl_sndbuf[net_ctrl_sndbuf_index][6] = (0x000000FF)&(package_index0>>8);
		net_ctrl_sndbuf[net_ctrl_sndbuf_index][7] = (0x000000FF)&package_index0;
		net_ctrl_sndbuf[net_ctrl_sndbuf_index][11] = package_devnum;
		if (urb->status < 0) {
			net_ctrl_sndbuf[net_ctrl_sndbuf_index][20] = 0xFF;
			net_ctrl_sndbuf[net_ctrl_sndbuf_index][21] = 0xFF;
			net_ctrl_sndbuf[net_ctrl_sndbuf_index][22] = 0xFF;
			net_ctrl_sndbuf[net_ctrl_sndbuf_index][23] = 0xE0;
		}     
		//net_ctrl_sndbuf[net_ctrl_sndbuf_index][26] = (0xFF00&0)>>8;
		//net_ctrl_sndbuf[net_ctrl_sndbuf_index][27] = (0x00FF&0);
		net_ctrl_sndbuf[net_ctrl_sndbuf_index][26] = 0;
		net_ctrl_sndbuf[net_ctrl_sndbuf_index][27] = 0;
		
		if (net_ctrl_sndbuf_index < (DATA02_SEND_BUF_LEVEL-1)) {
			net_ctrl_sndbuf_index++; 
		} else {
			net_ctrl_sndbuf_index = 0;
		}
		dbg_usb_ctrl_in_count++;

		usb_busy = 0;
	}

	//printk(KERN_ALERT "urb->status:%08x\r\n", urb->status); 
}


static void usb_mr_free_mem(struct usb_device *dev, struct usb_mr *mr)
{
	usb_free_urb(mr->urb_data01);
	usb_free_urb(mr->urb_data02);
	usb_free_urb(mr->urb_ctrl);
	usb_free_coherent(dev, mr_data01_pkg_size, mr->data01_in_buf, mr->data01_in_buf_dma);
	usb_free_coherent(dev, mr_data02_pkg_size, mr->data02_in_buf, mr->data02_in_buf_dma);
	usb_free_coherent(dev, mr_data03_pkg_size, mr->data03_out_buf, mr->data03_out_buf_dma);
	kfree(mr->cr);
	usb_free_coherent(dev, 255, mr->ctrl_buf, mr->ctrl_buf_dma);
}

static int usb_mr_probe(struct usb_interface *iface,
		const struct usb_device_id *id)
{
	struct usb_device *dev = NULL;
	struct usb_host_interface *interface;
	struct usb_endpoint_descriptor *endpoint;
	int  pipe;
	int error = -ENOMEM;



	//printk(KERN_ALERT "usb_mr_probe send_command_step:%02x\r\n", send_command_step); 

	interface = iface->cur_altsetting;

	dev = interface_to_usbdev(iface);

	printk(KERN_ALERT "probe\r\n"); 
	printk(KERN_ALERT "---interface info\r\n"); 
	printk(KERN_ALERT "interface->desc.bLength=%02x\r\n", interface->desc.bLength); 
	printk(KERN_ALERT "interface->desc.bDescriptorType=%02x\r\n", interface->desc.bDescriptorType); 
	printk(KERN_ALERT "interface->desc.bInterfaceNumber=%02x\r\n", interface->desc.bInterfaceNumber); 
	printk(KERN_ALERT "interface->desc.bAlternateSetting=%02x\r\n", interface->desc.bAlternateSetting); 
	printk(KERN_ALERT "interface->desc.bNumEndpoints=%02x\r\n", interface->desc.bNumEndpoints); 
	printk(KERN_ALERT "interface->desc.bInterfaceClass=%02x\r\n", interface->desc.bInterfaceClass); 
	printk(KERN_ALERT "interface->desc.bInterfaceSubClass=%02x\r\n", interface->desc.bInterfaceSubClass); 
	printk(KERN_ALERT "interface->desc.bInterfaceProtocol=%02x\r\n", interface->desc.bInterfaceProtocol); 
	printk(KERN_ALERT "interface->desc.iInterface=%02x\r\n", interface->desc.iInterface); 

	if (interface->desc.bInterfaceNumber == 0x01) {
		net_ready = 0;

		endpoint = &interface->endpoint[0].desc;

		printk(KERN_ALERT "---used interface\r\n"); 
		printk(KERN_ALERT "endpoint->bLength=%02x\r\n", endpoint->bLength); 
		printk(KERN_ALERT "endpoint->bDescriptorType=%02x\r\n", endpoint->bDescriptorType); 
		printk(KERN_ALERT "endpoint->bEndpointAddress=%02x\r\n", endpoint->bEndpointAddress); 
		printk(KERN_ALERT "endpoint->bmAttributes=%02x\r\n", endpoint->bmAttributes); 
		printk(KERN_ALERT "endpoint->bInterval=%02x\r\n", endpoint->bInterval); 
		printk(KERN_ALERT "endpoint->bRefresh=%02x\r\n", endpoint->bRefresh); 
		printk(KERN_ALERT "endpoint->bSynchAddress=%02x\r\n", endpoint->bSynchAddress); 

		mr_data01_pkg_size=endpoint->wMaxPacketSize*36;
		pipe = usb_rcvbulkpipe(dev, endpoint->bEndpointAddress);

		mr = kzalloc(sizeof(struct usb_mr), GFP_KERNEL);
		if (!mr)
			goto fail1;

		if (!(mr->urb_data01 = usb_alloc_urb(0, GFP_KERNEL)))
			goto fail2;

		if (!(mr->urb_ctrl = usb_alloc_urb(0, GFP_KERNEL)))
			goto fail2;

		if (!(mr->data01_in_buf = usb_alloc_coherent(dev, mr_data01_pkg_size, GFP_ATOMIC, &mr->data01_in_buf_dma)))
			goto fail2;

		if (!(mr->cr = kmalloc(sizeof(struct usb_ctrlrequest), GFP_KERNEL)))
			goto fail2;

		if (!(mr->ctrl_buf = usb_alloc_coherent(dev, 255, GFP_ATOMIC, &mr->ctrl_buf_dma)))
			goto fail2;

		mr->usbdev = dev;

		usb_fill_bulk_urb(mr->urb_data01, dev, pipe,
				mr->data01_in_buf, mr_data01_pkg_size,
				usb_mr_data01_in_callback, mr);
		mr->urb_data01->transfer_dma = mr->data01_in_buf_dma;
		mr->urb_data01->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

		//usb_submit_urb(mr->urb_data01, GFP_KERNEL);

		printk(KERN_ALERT "start thread_usb\r\n"); 
		thread_usb_handle = kthread_run(thread_usb, mr, "thread_usb");

		printk(KERN_ALERT "start net server\r\n"); 
		server_init();

		usb_set_intfdata(iface, mr);
		device_set_wakeup_enable(&dev->dev, 1);

		return 0;
	} else if (interface->desc.bInterfaceNumber == 0x02) {

		endpoint = &interface->endpoint[0].desc;

		printk(KERN_ALERT "---used interface\r\n"); 
		printk(KERN_ALERT "endpoint->bLength=%02x\r\n", endpoint->bLength); 
		printk(KERN_ALERT "endpoint->bDescriptorType=%02x\r\n", endpoint->bDescriptorType); 
		printk(KERN_ALERT "endpoint->bEndpointAddress=%02x\r\n", endpoint->bEndpointAddress); 
		printk(KERN_ALERT "endpoint->bmAttributes=%02x\r\n", endpoint->bmAttributes); 
		printk(KERN_ALERT "endpoint->bInterval=%02x\r\n", endpoint->bInterval); 
		printk(KERN_ALERT "endpoint->bRefresh=%02x\r\n", endpoint->bRefresh); 
		printk(KERN_ALERT "endpoint->bSynchAddress=%02x\r\n", endpoint->bSynchAddress); 

		mr_data02_pkg_size=endpoint->wMaxPacketSize;
		pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress);

		if (!(mr->urb_data02 = usb_alloc_urb(0, GFP_KERNEL)))
			goto fail2;

		if (!(mr->data02_in_buf = usb_alloc_coherent(dev, mr_data02_pkg_size, GFP_ATOMIC, &mr->data02_in_buf_dma)))
			goto fail2;

		usb_fill_int_urb(mr->urb_data02, dev, pipe,
				mr->data02_in_buf, mr_data02_pkg_size,
				usb_mr_data02_in_callback, mr, endpoint->bInterval);
		mr->urb_data02->transfer_dma = mr->data02_in_buf_dma;
		mr->urb_data02->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

		usb_submit_urb(mr->urb_data02, GFP_KERNEL);

		return 0;
	} else if (interface->desc.bInterfaceNumber == 0x03) {

		endpoint = &interface->endpoint[0].desc;

		printk(KERN_ALERT "---used interface\r\n"); 
		printk(KERN_ALERT "endpoint->bLength=%02x\r\n", endpoint->bLength); 
		printk(KERN_ALERT "endpoint->bDescriptorType=%02x\r\n", endpoint->bDescriptorType); 
		printk(KERN_ALERT "endpoint->bEndpointAddress=%02x\r\n", endpoint->bEndpointAddress); 
		printk(KERN_ALERT "endpoint->bmAttributes=%02x\r\n", endpoint->bmAttributes); 
		printk(KERN_ALERT "endpoint->bInterval=%02x\r\n", endpoint->bInterval); 
		printk(KERN_ALERT "endpoint->bRefresh=%02x\r\n", endpoint->bRefresh); 
		printk(KERN_ALERT "endpoint->bSynchAddress=%02x\r\n", endpoint->bSynchAddress); 

		mr_data03_pkg_size=endpoint->wMaxPacketSize;
		pipe = usb_sndbulkpipe(dev, endpoint->bEndpointAddress);

		if (!(mr->urb_data03 = usb_alloc_urb(0, GFP_KERNEL)))
			goto fail2;

		if (!(mr->data03_out_buf = usb_alloc_coherent(dev, mr_data03_pkg_size, GFP_ATOMIC, &mr->data03_out_buf_dma)))
			goto fail2;

		return 0;
	} else {
		printk(KERN_ALERT "---unused interface\r\n"); 
		return 0;
	}


fail2:	
	usb_mr_free_mem(dev, mr);
fail1:	
	printk(KERN_ALERT "free mr mem\r\n"); 
	kfree(mr);
	return error;
}

static void usb_mr_disconnect(struct usb_interface *intf)
{
	struct usb_mr *mr = usb_get_intfdata (intf);
	struct usb_host_interface *interface;

	interface = intf->cur_altsetting;

	if (interface->desc.bInterfaceNumber != 0x00) {
		printk(KERN_ALERT "disconnect interface %d return\r\n", interface->desc.bInterfaceNumber); 
		return;
	}

	printk(KERN_ALERT "disconnect interface %d , free all\r\n", interface->desc.bInterfaceNumber); 

	usb_set_intfdata(intf, NULL);
	if (mr) {
		usb_kill_urb(mr->urb_data01);
		usb_kill_urb(mr->urb_data02);
		usb_kill_urb(mr->urb_data03);
		usb_kill_urb(mr->urb_ctrl);
		usb_mr_free_mem(interface_to_usbdev(intf), mr);
		printk(KERN_ALERT "free mr mem\r\n"); 
		kfree(mr);
	}

	if(thread_usb_handle) {
		printk(KERN_ALERT "stop thread_usb\n");
		kthread_stop(thread_usb_handle);
	}

	printk(KERN_ALERT "start net server\r\n"); 
	server_exit();
}

static struct usb_device_id usb_mr_id_table [] = {
	//{ USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT, USB_INTERFACE_PROTOCOL_KEYBOARD) },
	//{ USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT, USB_INTERFACE_PROTOCOL_MOUSE) },
	//{USB_DEVICE(0x2833, 0x0031)}, // vend:厂家id prod :设备id
	//{USB_DEVICE(0x28DE, 0x2000)}, // vend:厂家id prod :设备id
	{USB_DEVICE(0x045E, 0x0659)}, // vend:厂家id prod :设备id
	{ }						/* Terminating entry */
};

MODULE_DEVICE_TABLE (usb, usb_mr_id_table);

static struct usb_driver usb_mr_driver = {
	.name =		"mr",
	.probe =	usb_mr_probe,
	.disconnect =	usb_mr_disconnect,
	.id_table =	usb_mr_id_table,
};

module_usb_driver(usb_mr_driver);
