#include <linux/kernel.h> 
#include <linux/fs.h> 
#include <asm/uaccess.h> 
#include <linux/mm.h> 

struct file *log_file_open(void)
{
	struct file *fp;

	fp = filp_open("/root/mr-log", O_RDWR | O_CREAT, 0644);
	if (IS_ERR(fp)) {
		printk("open error...\n"); 
		return NULL;
	}   

	return fp;
}

void log_file_write(struct file *fp, char* data, unsigned int len)
{
	mm_segment_t fs;

	fs = get_fs();
	set_fs(KERNEL_DS);
	vfs_write(fp, data, len, &fp->f_pos);
	set_fs(fs);
}

void log_file_close(struct file *fp)
{
	filp_close(fp,NULL);
}

void test(void) 
{ 
	struct file *fp; 

	fp = log_file_open();
	log_file_write(fp, "\x11\x22\x33\x44", 4);
	log_file_close(fp);
}
