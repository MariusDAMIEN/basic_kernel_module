#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include "Epitech_ioctl.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Marius DAMIEN");
MODULE_DESCRIPTION("Example Module");
MODULE_VERSION("0.01");


#define DEVICE_NAME "Epitech_example"
#define EXAMPLE_MSG "Hello, World!\n"
#define USER_BUFFER_LEN 1024*1024*3 /* Your driver has the same space */ 


static struct task_struct *sleeping_task;

/* Prototypes for device functions */
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);
static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int device_mmap(struct file *filp, struct vm_area_struct *vm);
void mmap_open(struct vm_area_struct *vma);



static int synchro = 0;
static int major_num;
static int device_open_count = 0;
static char *msg_buffer;
 

/* This structure points to all of the device functions */
static struct file_operations file_ops = 
  {
   .read = device_read,
   .write = device_write,
   .open = device_open,
   .release = device_release,
   .mmap = device_mmap,
   .unlocked_ioctl = device_ioctl,
  };


/* Called when a process opens our device */
static int device_open(struct inode *inode, struct file *file) 
{
  /* If device is open, return busy */
  if (device_open_count) 
    {
      printk(KERN_ALERT " Epitech Could not Open \n");
      return -EBUSY;
    }
  else
    {
      printk(KERN_ALERT "Epitech  Open \n");
    }

  try_module_get(THIS_MODULE);
  return 0;
}



/* When a process reads from our device, this gets called. */
static ssize_t device_read(struct file *flip, char *buffer, size_t len, loff_t *offset) 
{
  printk(KERN_ALERT "Begin Read\n");

  // Verification de la taille reçus
  if (len <= 0)
    return 0;

  // Wait du write 
  sleeping_task = current;
  set_current_state(TASK_INTERRUPTIBLE);
  schedule();

  
  // Copie du  buffer sur la memoire du user
  if (copy_to_user(buffer, msg_buffer, len))
    return -EFAULT;
  
  printk(KERN_ALERT "End Read\n");  
  return len;
}

/* When a process writes from our device, this gets called. */
static ssize_t device_write(struct file *flip, const char *buffer, size_t len, loff_t *offset)
{
  printk(KERN_ALERT "Begin Write\n");

  // Verification de la taille reçus
  if (len <= 0)
    return 0;

  
  // Copie du buffer vers la mémoire
  if (copy_from_user(msg_buffer, buffer, len))
    return -EFAULT;

  wake_up_process(sleeping_task);
  printk(KERN_ALERT "End Write\n");
  return len;
}

static int device_mmap(struct file *file, struct vm_area_struct *vma)
{
  int ret;
  unsigned  long  pfn  =  virt_to_phys (( void  * ) msg_buffer ) >> PAGE_SHIFT ;

  printk(KERN_ALERT "Begin Mmap\n");

  if ((vma->vm_end - vma->vm_start) > ((unsigned long int)USER_BUFFER_LEN)) {
    printk("Buffer trop gros\n");
    return -EINVAL;
  }
  if ((ret = remap_pfn_range(vma, vma->vm_start, pfn, (vma->vm_end - vma->vm_start), vma->vm_page_prot)) < 0) {
    printk("Erreur address\n");
  }

  printk(KERN_ALERT "End Mmap\n");
  return (ret);
}





static long device_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
  // EPITECH_DRV_SYNC_BARRIER
  if (cmd == 1) {
    if (synchro == 0) {
      synchro = 1;
      sleeping_task = current;
      set_current_state(TASK_INTERRUPTIBLE);
      schedule();
    }
    else {
      synchro = 0;
      wake_up_process(sleeping_task);      
    }
      printk(KERN_ALERT "EPITECH_DRV_SYNC_BARRIER\n");
  }
  else {
    printk(KERN_ALERT "EPITECH_DRV_CLEAR_BUFFER\n");
    memset(msg_buffer, 0, USER_BUFFER_LEN);
  }
 
  return 0;
}






/* Called when a process closes our device */
static int device_release(struct inode *inode, struct file *file) 
{
  module_put(THIS_MODULE);
  return 0;
}

static int __init Epitech_example_init(void) 
{
  // Allocation de la mémoire
  if ((msg_buffer = kmalloc(USER_BUFFER_LEN, GFP_KERNEL)) == NULL)
    return -EFAULT;

  
  major_num = register_chrdev(0, DEVICE_NAME, &file_ops);

  if (major_num < 0) {
    printk(KERN_ALERT "Could not register device: %d\n", major_num);

    return major_num;

  } else {
    printk(KERN_INFO  " Hello Epitech_example module loaded with device major number %d\n", major_num);
    return 0;
  }
}

static void __exit Epitech_example_exit(void) 
{
  // Liberation de la memoire du buffer
  kfree(msg_buffer);
  unregister_chrdev(major_num, DEVICE_NAME);
  printk(KERN_INFO "Goodbye, World!\n");
}



/* Register module functions */
module_init(Epitech_example_init);

module_exit(Epitech_example_exit);
