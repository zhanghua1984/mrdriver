#ifndef __LOGFILE_H
#define __LOGFILE_H

extern struct file *log_file_open(void);
extern void log_file_write(struct file *fp, char* data, unsigned int len);
extern void log_file_close(struct file *fp);
void test(void);


#endif
