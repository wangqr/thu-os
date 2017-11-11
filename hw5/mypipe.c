#include <linux/fs.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>

#include <asm/uaccess.h>

#define KBUF_SIZE 4096

static char kbuf[KBUF_SIZE];
struct mutex m_read, m_write, m_empty, m_full;
static char *pstart, *pend;

static ssize_t mypipe_read (struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
    char *spend;
    int errno;
	if (count == 0)
		return 0;
    if (mutex_lock_interruptible(&m_read))
        return -ERESTARTSYS;
    if ((errno = mutex_lock_interruptible(&m_empty))) {
        mutex_unlock(&m_read);
        return -ERESTARTSYS;
    }
    spend = pend;
    if (spend <= pstart) {  // used kbuf wraped, read from unwraped part only.
        if (count > KBUF_SIZE - (pstart - kbuf))
            count = KBUF_SIZE - (pstart - kbuf);
    }
    else if (count > spend - pstart)
        count = spend - pstart;
    if (copy_to_user(buf, pstart, count)) {
        mutex_unlock(&m_empty);
        mutex_unlock(&m_read);
		return -EINVAL;
    }
    pstart += count;
    if (pstart - kbuf == KBUF_SIZE)
        pstart = kbuf;
    if (pstart != spend)
        mutex_unlock(&m_empty);
    mutex_unlock(&m_full);
    mutex_unlock(&m_read);
	return count;
}

static ssize_t mypipe_write (struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    char *spstart;
    int errno;
	if (count == 0)
		return 0;
    if (mutex_lock_interruptible(&m_write))
        return -ERESTARTSYS;
    if ((errno = mutex_lock_interruptible(&m_full))) {
        mutex_unlock(&m_write);
        return -ERESTARTSYS;
    }
    spstart = pstart;
    if (spstart <= pend) {
        if (count > KBUF_SIZE - (pend - kbuf))
            count = KBUF_SIZE - (pend - kbuf);
    }
    else if (count > spstart - pend)
        count = spstart - pend;
    if (copy_from_user(pend, buf, count)) {
        mutex_unlock(&m_full);
        mutex_unlock(&m_write);
		return -EINVAL;
    }
    pend += count;
    if (pend - kbuf == KBUF_SIZE)
        pend = kbuf;
    if (pend != spstart)
        mutex_unlock(&m_full);
    mutex_unlock(&m_empty);
    mutex_unlock(&m_write);
	return count;
}

static const struct file_operations mypipe_fops = {
	.owner		= THIS_MODULE,
	.read		= mypipe_read,
    .write      = mypipe_write,
};

static struct miscdevice mypipe = {
	MISC_DYNAMIC_MINOR,
	"mypipe",
	&mypipe_fops
};

static int __init mypipe_init(void) {
	int ret;
    mutex_init(&m_read);
    mutex_init(&m_write);
    mutex_init(&m_empty);
    mutex_lock_interruptible(&m_empty);
    mutex_init(&m_full);
    pstart = kbuf;
    pend = kbuf;
	ret = misc_register(&mypipe);
	if (ret)
		printk(KERN_ERR "mypipe: Unable to register \"mypipe\" misc device\n");
    else {
        printk("mypipe: Module \"mypipe\" init success.\n");
        printk(KERN_WARNING "mypipe: YOU ARE LOADING A HOMEWORK-PURPOSE-ONLY MODULE.\n");
        printk(KERN_WARNING "mypipe: IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY.\n");
    }
	return ret;
}

static void __exit mypipe_exit(void) {
	misc_deregister(&mypipe);
    printk("mypipe: Module \"mypipe\" exit success.\n");
}

module_init(mypipe_init);
module_exit(mypipe_exit);

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("wangqr");
MODULE_DESCRIPTION("Simple pipe module [Homework Purpose Only]");
MODULE_VERSION("1.0.0");
