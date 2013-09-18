#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/param.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#define DEVICE_NAME "gmem"
#define MEMORY_BUFFER_SIZE 256

struct Gmem_dev {
	struct cdev cdev;
	char name[20];
	char memory_buffer[MEMORY_BUFFER_SIZE];  /* memory buffer of size MEMORY_BUFFER_SIZE */
	size_t size;
} *gmem_devp;

static dev_t my_dev_number;      /* Allotted device number */
struct class *my_dev_class;          /* Tie with the device model */

/*
 * Driver Open
 */
int My_driver_open(struct inode *inode, struct file *file)
{
	struct Gmem_dev *gmem_devp;
	
	/* find the location for gmem_devp */
	gmem_devp = container_of(inode->i_cdev, struct Gmem_dev, cdev);

	/* make it accessible to other calls */
	file->private_data = gmem_devp;
	
	file->f_pos = gmem_devp->size;
	
	gmem_devp->size = file->f_pos;
	printk("\n%s opened\n", gmem_devp->name);
	return 0;
}

/*
 * Release My driver
 */
int My_driver_release(struct inode *inode, struct file *file)
{
	struct Gmem_dev *gmem_devp = file->private_data;
	printk("\n%s closed.\n",gmem_devp->name);
	return 0;
}

/*
 * Write to gmem driver
 */
ssize_t My_driver_write(struct file *file, const char *buf,
           size_t count, loff_t *ppos)
{
	struct Gmem_dev* gmem_devp;
	char *kern_buffer;
	int i,res = 0;
	gmem_devp = file->private_data;
	
	kern_buffer = kmalloc(sizeof(char) * (count+1), GFP_KERNEL);
	
	if (copy_from_user(kern_buffer,buf,count))
	{
		res = -EFAULT;
		kfree(kern_buffer);
		return res;
	}
	res = count;
	for (i=0;i<count;i++)
	{
		gmem_devp->memory_buffer[(*ppos + i)%MEMORY_BUFFER_SIZE] = kern_buffer[i];
	}
	
	gmem_devp->memory_buffer[(*ppos + count)%MEMORY_BUFFER_SIZE] = '\0';
	kern_buffer[count] = '\0';
	*ppos = (*ppos + count)%MEMORY_BUFFER_SIZE;
	gmem_devp->size = (gmem_devp->size+count) > MEMORY_BUFFER_SIZE ? MEMORY_BUFFER_SIZE : gmem_devp->size+count;
	
	printk ("\n%s kernel buffer %d\n",gmem_devp->memory_buffer,*ppos);
	kfree(kern_buffer);
	return res;
}


/*
 * Read to g driver
 */
ssize_t My_driver_read(struct file *file, char *buf,
           size_t count, loff_t *ppos)
{
	int res;
	size_t total_data = 0, buffer_size;
	struct Gmem_dev* gmem_devp;
	
	/* Accessing the device specific struct stored in file->private_data */
	gmem_devp = file->private_data;
	
	buffer_size = gmem_devp->size;
	
	total_data = (count > buffer_size && count != 0) ? buffer_size : count;
	
	/* copying data to user space buffer. */
	if(copy_to_user(buf, gmem_devp->memory_buffer,total_data))
	{
		res = -EFAULT;
		return res;
	}
	res = total_data;
	printk("\n%d characters copied.\n",total_data);
	printk("\n %s \n",gmem_devp->memory_buffer);
	return res;
}
/* File operations structure. Defined in linux/fs.h */
static struct file_operations My_fops = {
    .owner = THIS_MODULE,           /* Owner */
    .open = My_driver_open,              /* Open method */
    .release = My_driver_release,        /* Release method */
    .write = My_driver_write,            /* Write method */
	.read = My_driver_read,              /* Read method */
};
/*
 * Driver Initialization
 */
static int __init hello_init(void)
{
	int ret;
	long jiffies_passed = 0,time_passed = 0;

	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&my_dev_number, 0, 1, DEVICE_NAME) < 0) {
		printk(KERN_DEBUG "Can't register device\n"); return -1;
	}

	/* Populate sysfs entries */
	my_dev_class = class_create(THIS_MODULE, DEVICE_NAME);

	
	/* Allocate memory for the per-device structure */
	gmem_devp = kmalloc(sizeof(struct Gmem_dev), GFP_KERNEL);
	
	if (!gmem_devp) {
		printk("Bad Kmalloc\n"); return -ENOMEM;
	}

	/* Request I/O region */
	sprintf(gmem_devp->name, DEVICE_NAME); 


	/* Connect the file operations with the cdev */
	cdev_init(&gmem_devp->cdev, &My_fops);
	gmem_devp->cdev.owner = THIS_MODULE;

	/* Connect the major/minor number to the cdev */
	ret = cdev_add(&gmem_devp->cdev, (my_dev_number), 1);

	if (ret) {
		printk("Bad cdev\n");
		return ret;
	}

	/* Send uevents to udev, so it'll create /dev nodes */
	device_create(my_dev_class, NULL, MKDEV(MAJOR(my_dev_number), 0), NULL, DEVICE_NAME);		
	
	/* Getting time since kernel start */
	jiffies_passed = get_jiffies_64();
	time_passed = jiffies_passed/HZ;
	
	/* initialize the data and position */
	gmem_devp->size = sprintf(gmem_devp->memory_buffer, "Hello World! This is Vedang and this machine has worked for %ld seconds.",time_passed);
	
	
	printk("My Driver Initialized.\n");
	return 0;
}

/*
 * Driver Exit
 */
static void __exit hello_exit(void)
{
	
	/* Release the major number */
	unregister_chrdev_region((my_dev_number), 1);

	/* Destroy device */
	device_destroy (my_dev_class, MKDEV(MAJOR(my_dev_number), 0));
	cdev_del(&gmem_devp->cdev);
	kfree(gmem_devp);
	
	/* Destroy driver_class */
	class_destroy(my_dev_class);

	printk("My Driver removed.\n"); 
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL v2");
