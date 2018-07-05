#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/module.h>

#include<linux/fs.h>
#include<linux/cdev.h>

#include<linux/kthread.h>
#include<linux/device.h>
#include<linux/pid.h>
#include<linux/mm_types.h>
#include<linux/vmalloc.h>
#include<linux/types.h>
#include<linux/moduleparam.h>
#include<linux/slab.h>
#include<linux/pci.h>
#include<asm/unistd.h>
#include<asm/uaccess.h>
#include<linux/sched.h>

#include<linux/mm.h>
#include<linux/pagemap.h>
#include<linux/security.h>

#include<linux/highmem.h>

#include<linux/wait.h>

#define HACK_MAJOR 256
#define HACK_MINOR 0
#define HACK_DEV_CNT 1
#define HACK_DEV_NAME "hack123"

#define AREA_SIZE 100

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jun W<junv1003@outlook.com>");
MODULE_DESCRIPTION("hack caipiao123");

/**
 * 描述地址结构
 * off表示业内偏移，不超过4096
 * seg表示所在段，0代码段，1数据段，2堆段，3堆栈段
 * page表示页索引
*/
union OFFSET{
    struct{
        unsigned int off:12;
        unsigned int seg:4;
        unsigned int page:16;
    };
    unsigned int ad;
};

struct HACK{
    unsigned char sync;
    unsigned char cmd;
    unsigned char end0;

    union{
        unsigned int pid;
        unsigned char pch[4];
    };
};

static unsigned int upid;
module_param(upid, uint, S_IRUGO);
static char known[] = {18, 5, 26, 9};
static struct cdev hackdev;

static int __init hack_init(void);
static void __exit hack_exit(void);
static int hack_open(struct inode *inode,struct file *filp);
static int hack_release(struct inode *inode,struct file *filp);
static ssize_t hack_read(struct file *filp, char __user *buf, size_t count, loff_t *pos);
static ssize_t hack_write(struct file *filp, const char __user *buf, size_t count, loff_t *pos);
int search(void *argc);


static struct file_operations hack_ops = {
    .owner = THIS_MODULE,
    .open = hack_open,
    .release = hack_release,
    .read = hack_read,
    .write = hack_write,
};

module_init(hack_init);
module_exit(hack_exit);