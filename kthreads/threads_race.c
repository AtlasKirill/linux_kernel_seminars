#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/completion.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>



/*  
 *  Prototypes
 */
static int myinit(void);
static void myexit(void);
static int func_thread(void* arg);

#define FAIL -1
#define SUCCESS 0
#define THREAD_NUM 4


struct thread_data {
    int id;
    struct completion *comp;
    struct task_struct *struct_thread;
};

DEFINE_SPINLOCK(my_lock);

static int counter = 0;

static int func_thread(void *arg) {
    int i;
    struct thread_data *data = (struct thread_data*)arg;
    
    for (i = 0; i < 5000; i++)
    {
        spin_lock(&my_lock);
        counter++;
        spin_unlock(&my_lock);
    }

    complete(data->comp);

    return 0;
}

/*
 * This function is called when the module is loaded
 */
static int myinit(void)
{
    int i;
    struct thread_data *data[THREAD_NUM];
    memset(data, 0, sizeof(data));
    //data = (struct thread_data *)kzalloc(sizeof(struct thread_data) * THREAD_NUM, GFP_KERNEL);
    printk(KERN_ALERT "We are inside myinit\n");
    for (i = 0; i < THREAD_NUM; i++) 
    {
        init_completion(data[i]->comp);
        data[i]->id = i;
        data[i]->struct_thread = kthread_run(func_thread, (void*) data[i], "my_thread%d", i);
        if (data[i]->struct_thread) {
            printk(KERN_ALERT "Kthread1 Created Successfully...\n");
        } else {
            printk(KERN_ALERT "Cannot create kthread\n");
             goto thread_err;
        }
    }

    for (i = 0; i < THREAD_NUM; i++)
    {
        wait_for_completion(data[i]->comp);
    }

    kfree(data);
    return SUCCESS;

thread_err:
    for (i = 0; i < THREAD_NUM; i++)
    {
        if (data[i]->struct_thread)
            kthread_stop(data[i]->struct_thread);
    }
    return FAIL;    
}


/*
 * This function is called when the module is unloaded
 */
static void myexit(void)
{
    printk(KERN_INFO "Goodbye world 1.\n");
}

module_init(myinit)
module_exit(myexit)
MODULE_LICENSE("GPL");