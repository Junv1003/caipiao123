#include"hack.h"

static int hack_open(struct inode *inode, struct file *filp){
    try_module_get(THIS_MODULE);
    printk("device open success\n");
    return 0;
}

static int hack_release(struct inode *inode, struct file *filp){
    module_put(THIS_MODULE);
    printk("device release success\n");
    return 0;
}

static ssize_t hack_read(struct file *filp, char __user *buf, size_t count, loff_t *pos){
    return count;
}

static ssize_t hack_write(struct file *filp, const char __user *buf, size_t count, loff_t *pos){
    //kernel_thread(search, NULL, CLONE_KERNEL);

    kthread_run(search, NULL, "search_thread%d", 1);
    return count;
}

/**
 * 搜索
*/
int search(void *argc){
    printk("search start\n");
    //daemonize("ty_thd1");
    struct pid *kpid;
    printk("进程号%d\n", upid);
    printk("已知数字");
    int i;
    for(i = 0; i < 4; i++){
        printk("%d ",known[i]);
    }
    printk("\n");
    kpid = find_get_pid((unsigned int)upid);
    if(kpid == NULL){
        printk("find_get_pid error\n");
        goto ferr;
    }

    struct task_struct *t_str;
    t_str = pid_task(kpid, PIDTYPE_PID);//获取进程控制块
    if(t_str == NULL){
        printk("pid_task error\n");
        goto ferr;
    }
    struct mm_struct *mm_str;
    mm_str = get_task_mm(t_str);//获取内存描述符
    if(mm_str == NULL){
        printk("get_task_mm error\n");
        goto ferr;
    }

    unsigned int seg[4];//4个段基地址
    unsigned int t_len;//代码段长度
    unsigned int d_len;//数据段长度

    seg[0] = mm_str->start_code;//代码段
    seg[1] = mm_str->start_data;//数据段
    seg[2] = mm_str->start_brk;//分配的堆段
    seg[3] = mm_str->start_stack;//堆栈段
    t_len = mm_str->end_code - mm_str->start_code;;
    d_len = mm_str->end_data - mm_str->start_data;
    printk("total pages=%lx stack pages=%lx\n", mm_str->total_vm, mm_str->stack_vm);
    
    /*vm_area_struct描述的是一段连续的、具有相同访问属性的虚存空间，该虚存空间的大小为物理内存页面的整数倍*/
    struct vm_area_struct *vadr;
    vadr = mm_str->mmap;
    unsigned int adr;
    int k, j, m, n;
    int len, ret;
    i=0;m=0;n=0;
    char *v;
    v = NULL;
    struct page **pages;
    unsigned int total_pages;
    char *c;
    int suc=0;
    do{
        adr = vadr->vm_start;//虚存空间首地址
        len = vma_pages(vadr);
        if(v != NULL){
            vfree(v);
            v = NULL;
        }
        v = (char*)vmalloc(sizeof(void*)*(len + 1));//vmalloc使得连续的内核虚拟地址对应不连续的物理地址
        pages = (struct page **)v;
        if(pages == NULL){
            printk("vmalloc error\n");
            goto ferr;
        }
        memset((void*)pages, 0, sizeof(void*)*(len + 1));
        down_read(&mm_str->mmap_sem);//设置为读权限
        ret = get_user_pages(t_str, mm_str, adr, len, 0, 0, pages, NULL);
        if(ret <= 0){
            printk("ret<0\n");
            up_read(&mm_str->mmap_sem);
            goto ferr;
        }
        //TODO
        
        total_pages = ret;
        for(k = 0; k < ret; k++){
            if(pages[k] == NULL){
                printk("search finished\n");
                break;
            }
            if(k > 60000){
                printk("pages counts too large\n");
                break;
            }
            
            c = (char*)kmap(pages[k]);
            for(j = 0; j < 4093; j++){
                if(c[j] == known[0]){
                    if(c[j+1] == known[1]){
                        if(c[j+2] == known[2]){
                            if(c[j+3] == known[3]){
                                printk("found it\n");
                                //printk("next nums:%d\n",c[j+4]);
								/*程序比较粗糙，打印内存信息的时候没有处理跨页的情况（跨页处理也不复杂，先判断到达页边界，然后打印下一页的数据就行），
								只是证明思路的可行性*/
                                printk("next nums:%d %d %d %d %d %d\n",c[j+4],c[j+5],c[j+6],c[j+7],c[j+8],c[j+9]);   
                                suc = 1;
                                break;                           
                            }
                        }
                    }
                }
            }
            page_cache_release(pages[k]);
            if(suc == 1){
                break;
            }
        }
        up_read(&mm_str->mmap_sem);
        pages = NULL;
        printk("seg finished\n");
        vadr = vadr->vm_next;
        i++;
        if(vadr == NULL){
            break;
        }
    }while(i < 8);

    printk("search end\n");

    return 0;

ferr:
    if(v != NULL)
        vfree(v);
    printk("kernel_thread finished\n");
    return 0;
}

static int __init hack_init(void){
    int ret;
    dev_t dev;

    dev = MKDEV(HACK_MAJOR,HACK_MINOR);//将主设备号和次设备号合并
    ret = register_chrdev_region(dev, HACK_DEV_CNT, HACK_DEV_NAME);//注册字符设备
    if(ret){
        printk("<1>register char dev error\n");
        goto reg_err;
    }

    cdev_init(&hackdev, &hack_ops);//初始化hackdev中的部分成员，将ops指针指向hack_ops
    hackdev.owner = THIS_MODULE;

    ret = cdev_add(&hackdev, dev, HACK_DEV_CNT);//添加到内核的cdev_map散列表
    if(ret){
        printk("add error\n");
        goto add_err;
    }
    printk("hack module init\n");
    return 0;

add_err:
    unregister_chrdev_region(dev, HACK_DEV_CNT);
reg_err:
    return ret;
}

static void __exit hack_exit(void){
    dev_t dev;
    dev = MKDEV(HACK_MAJOR, HACK_MINOR);

    cdev_del(&hackdev);
    unregister_chrdev_region(dev, HACK_DEV_CNT);
    printk("hack module exit\n");
}