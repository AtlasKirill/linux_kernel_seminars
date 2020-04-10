#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/slab.h>



/*  
 *  Prototypes
 */
static int myinit(void);
static void myexit(void);
static void cleanup(int device_created);

/* file operations */
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define FAIL -1
#define SUCCESS 0
#define DEVICE_NAME "chardev"
#define BUF_LEN 256		/* Max length of the message from the char device */

/* 
 * Global variables are declared as static, so are global within the file. 
 */

static int Major;		/* Major number assigned to our device driver */

static int Dev_Open = 0;
static unsigned int op_count = 0;
static size_t nr_bytes = 0;

static struct cdev mycdev;
static struct class *myclass = NULL;

static struct file_operations fops = {
	read: device_read,
	write: device_write,
	open: device_open,
	release: device_release
};

/*
 * This function is called when the module is loaded
 */
static int myinit(void)
{
    int device_created = 0;

    /* cat /proc/devices */
    if (alloc_chrdev_region(&Major, 0, 1, DEVICE_NAME "_proc") < 0)
        goto error;

    /* ls /sys/class */
    if ((myclass = class_create(THIS_MODULE, DEVICE_NAME "_sys")) == NULL)
        goto error;

    /* ls /dev/ */
    if (device_create(myclass, NULL, Major, NULL, DEVICE_NAME "_dev") == NULL)
        goto error;

    device_created = 1;

    cdev_init(&mycdev, &fops);
    if (cdev_add(&mycdev, Major, 1) == -1)
        goto error;
    return SUCCESS;
error:
    cleanup(device_created);
    return FAIL;
}


static void cleanup(int device_created)
{
    if (device_created) {
        device_destroy(myclass, Major);
        cdev_del(&mycdev);
    }
    if (myclass)
        class_destroy(myclass);
    if (Major != -1)
        unregister_chrdev_region(Major, 1);
}

/*
 * This function is called when the module is unloaded
 */
static void myexit(void)
{
    cleanup(1);
}

/*
 * Methods
 */

/* 
 * Called when a process tries to open the device file, like
 * "cat /dev/mycharfile"
 */
static int device_open(struct inode *inode, struct file *file)
{
	if (Dev_Open)
		return -EBUSY;

	Dev_Open++;

	try_module_get(THIS_MODULE); //increase number of processes are using my module
	printk(KERN_ALERT "Trying to open device file.\n");

	return SUCCESS;
	
}

/* 
 * Called when a process closes the device file.
 */
static int device_release(struct inode *inode, struct file *file)
{
	Dev_Open--;
	module_put(THIS_MODULE); //decrease number of processes are using my module
	printk(KERN_ALERT "Releasing device file.\n");
	return SUCCESS;
}

/* 
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */
static ssize_t device_read(struct file *filp,	/* see include/linux/fs.h   */
			   char *buffer,	/* buffer to fill with data */
			   size_t length,	/* length of the buffer     */
			   loff_t * offset)
{
	char op_count_str [BUF_LEN];
	sprintf(op_count_str, "%d\n", op_count);

	//signal to reader that we've already copied string
	if (nr_bytes != 0) {
		nr_bytes = 0;
		return 0;
	}

	nr_bytes = strlen(op_count_str) > length ? length : strlen(op_count_str);

	if (copy_to_user(buffer, op_count_str, nr_bytes) != 0) {
		printk(KERN_ALERT "Reading from device file was not be successful.\n");
		return -EIO;
	}

	printk(KERN_ALERT "Reading from device file was successful.(%ld bytes)\n", nr_bytes);
	return nr_bytes;
}

/*  
 * Called when a process writes to dev file: echo "hi" > /dev/hello 
 */
static ssize_t
device_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
	op_count++;
	char local_buf [BUF_LEN];

	if (len > 255) {
		printk(KERN_ALERT "Trying to write more than 255 bytes.(attempt: %u)\n", op_count);
		return 0;
	}

	if (copy_from_user(local_buf, buff, len) != 0) {
		printk(KERN_ALERT "Writing to device file was not be successful.(attempt: %u)\n", op_count);
		return 0;
	}

	local_buf[len] = '\0';
	
	printk(KERN_ALERT "Writing to device file was successful.\n%lu bytes, attempt: %u, string:%s\n", len, op_count, local_buf);
	return len;
}

module_init(myinit)
module_exit(myexit)
MODULE_LICENSE("GPL");