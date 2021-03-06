// SPDX-License-Identifier: GPL-3.0
/*
 * Copyright(C) 2020 Yuki Kimura+Ryuichi Ueda.All right reserved.
*/


#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/delay.h>

MODULE_AUTHOR("Yuki Kimura&Ryuichi Ueda");
MODULE_DESCRIPTION("driver for LED control");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.1");

static dev_t dev;
static struct cdev cdv;
static struct class *cls = NULL;
static volatile u32 *gpio_base = NULL;
static int num[2] = {25, 26};

static ssize_t led_write(struct file* filp, const char* buf, size_t count, loff_t* pos)
{
	char c;
	int j;
	if(copy_from_user(&c,buf,sizeof(char)))
		return -EFAULT;
//out_put ON/OFF////////////////
	if(c == '0')
		gpio_base[7] = 1 << 25;
	else if(c == '1')
		gpio_base[10] = 1 << 25;
	else if(c == '2')
		gpio_base[7] = 1 << 26;
	else if(c == '3')
		gpio_base[10] = 1 << 26;
	else if(c == '4'){
		gpio_base[10] = 1 << 25;
		gpio_base[10] = 1 << 26;
		for(j=0;j<10;j++){
			gpio_base[7] = 1 << 25;
			msleep(200);
			gpio_base[10] = 1 << 25;
			gpio_base[7] = 1 << 26;
			msleep(200);
			gpio_base[10] = 1 << 26;
			msleep(200);
		}
	}	
//////////////////////////////
	printk(KERN_INFO "receive %c\n",c);
	return 1;
}

static struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.write = led_write,
};
static int __init init_mod(void)
{
	int retval,i;
	gpio_base = ioremap_nocache(0x3f200000, 0xA0);
//GPIO 25 out_put mode ON////////////////////////////////////////////////////
	for(i = 0;i<2;i++){
		const u32 led = num[i];
		const u32 index = led/10;
		const u32 shift = (led%10)*3;
		const u32 mask = ~(0x7 << shift); //0x7 is 111 
		gpio_base[index] = (gpio_base[index] & mask) | (0x1 << shift); //0x1 is 1
	}
/////////////////////////////////////////////////////////////////////////////	
	retval = alloc_chrdev_region(&dev, 0, 1, "myled");
	if(retval < 0){
		printk(KERN_ERR "alloc_chrdev_region failed.\n");
		return retval;
	}
	printk(KERN_INFO "%s is loaded. major:%d\n",__FILE__,MAJOR(dev));
	cdev_init(&cdv, &led_fops);
	retval = cdev_add(&cdv, dev, 1);

	cls = class_create(THIS_MODULE, "myled");
	if(IS_ERR(cls)){
		printk(KERN_ERR"class_create failed.");
		return PTR_ERR(cls);
	}
	if(retval < 0){
		printk(KERN_ERR "cdev_add failed. major:%d, minor:%d",MAJOR(dev),MINOR(dev));
		return retval;
	}
	device_create(cls, NULL, dev, NULL , "myled%d", MINOR(dev));
	return 0;
}

static void __exit cleanup_mod(void)
{
	cdev_del(&cdv);
	device_destroy(cls, dev);
	class_destroy(cls);
	unregister_chrdev_region(dev, 1);
	printk(KERN_INFO "%s is uniloaded. major:%d\n",__FILE__,MAJOR(dev));
}

module_init(init_mod);
module_exit(cleanup_mod);
