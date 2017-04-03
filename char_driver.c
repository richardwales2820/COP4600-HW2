#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#define DEVICE_NAME "fjr"
#define BUFFER_SIZE 1024

// Kernel driver info things
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Farza, Jacob, Richie");
MODULE_DESCRIPTION("Linux character device driver");
MODULE_VERSION("9001");

typedef struct charq
{
    char c;
    struct charq *next;
} charq;

// Major number to be assigned to the device
static int major_number;

// Head of the character queue to be printed in FIFO order
charq *head = NULL;
int letters_available = 0;

static int dev_open(struct inode *, struct file *);
static int dev_rls(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops =
{
    .read = dev_read,
    .open = dev_open,
    .write = dev_write,
    .release = dev_rls,
};

// Make a queue node for the char c
charq *make_c(char c)
{
    charq *node = kmalloc(sizeof(charq), GFP_KERNEL);
    node->c = c;
    node->next = NULL;

    return node;
}

// Free the whole queue
charq *freeq(charq *node)
{
    if (node == NULL)
        return NULL;

    freeq(node->next);
    kfree(node);

    return NULL;
}

int init_module(void)
{
    // register_chrdev returns a major number when 0 is passed to it
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    letters_available = 0;

    if (major_number < 0)
    {
        printk(KERN_ALERT "FJR could not register a major number :(\n");
        return major_number;
    }

    printk(KERN_INFO "FJR: assigned major number %d\n", major_number);

    return 0;
}

// Free the queue and unregister the device
void cleanup_module(void)
{
    freeq(head);
    unregister_chrdev(major_number, DEVICE_NAME);
}

// Log that the device was opened
static int dev_open(struct inode *inod, struct file *fil)
{
    printk(KERN_ALERT "FJR opened");
    return 0;
}

static ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *off)
{
    int count = 0;
    charq *temp;

    printk(KERN_ALERT "FJR read from\n");
    printk(KERN_ALERT "FJR: %d bytes available to read\n", letters_available);

    // Read from the list, going from head to the tail
    // Free each node as you read it
    while (head != NULL)
    {
        put_user(head->c, buff++);

        temp = head;
        count++;
        head = head->next;

        kfree(temp);
        // Decrement how many letters are left
        letters_available--;
    }

    return count;
}

// Write to the queue
static ssize_t dev_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
    charq *last_node = head;
    int i = 0;

    printk(KERN_ALERT "FJR written to\n");
    printk(KERN_ALERT "FJR: %d bytes available to write\n", BUFFER_SIZE - letters_available);

    // Get the tail node
    while (last_node != NULL && last_node->next != NULL)
    {
        last_node = last_node->next;
    }

    // Write and append to the list
    while (i < len && letters_available < BUFFER_SIZE)
    {
        charq *new_node = make_c(buff[i]);
        // Set the head if it isn't set yet
        if (head == NULL)
        {
            head = new_node;
            last_node = head;
        }
        else
        {
            last_node->next = new_node;
            last_node = last_node->next;
        }


        i++;
        letters_available++;
    }

    return i;
}

static int dev_rls(struct inode *inod, struct file *fil)
{
    printk(KERN_ALERT "FJR closed\n");
    return 0;
}
