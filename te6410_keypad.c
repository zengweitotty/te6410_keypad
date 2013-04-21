/*
    file Name:      te6410_keypad.c
	Author:         zengweitotty
    version:        V1.0   
    Data:           2013/04/21
    Email:          zengweitotty@gmail.com
    Description     te6410 keypad driver
*/
#include <linux/init.h>	//using for function module_init module_exit
#include <linux/module.h>
#include <linux/kernel.h>	//using for function printk
#include <linux/slab.h>	//using for function kmalloc kfree
#include <linux/errno.h>	//using for error number
#include <linux/interrupt.h>
#include <linux/sched.h>

#define KEY_COL	5
#define KEY_ROW	5

#define KEYBASE	0x7F008800
#define GPKCON0	0x7F008800
#define GPKCON1	0x7F008804
#define GPKPUD	0x7F00880C
#define GPLCON0	0x7F008810
#define GPLCON1	0x7F008814
#define GPLPUD	0x7F00881C

#define KEYIFBASE	0x7E00A000
#define KEYIFCON	0x7E00A000
#define KEYIFSTSCLR	0x7E00A004
#define KEYIFCOL	0x7E00A008
#define KEYIFROW	0x7E00A00C
#define KEYIFFC	0x7E00A010

struct keypad{
	unsigned int irq;
	//struct input_dev* key_input;	//TODO
	struct tasklet_struct task;
	volatile unsigned char col_value;
	volatile unsigned char row_value;
	volatile unsigned long* gpkcon1;
	volatile unsigned long* gpkpud;
	volatile unsigned long* gplcon0;
	volatile unsigned long* gplpud;
	volatile unsigned long* keyifcon;
	volatile unsigned long* keyifstsclr;
	volatile unsigned long* keyifcol;
	volatile unsigned long* keyifrow;
	volatile unsigned long* keyiffc;
};

struct keypad* te6410_keypad;

static int scan_keypad(void){
	int i = 0;
	int j = 0;
	volatile unsigned long temp = 0;
	disable_irq(te6410_keypad->irq);
	/*detect if is pressed*/
	te6410_keypad->colvalue = readl(te6410_keypad->keyifstsclr) & 0x000000ff;
	for(i = 0;i < KEYCOL;i++){
		temp = ~(0x1 << i) & (0x000000FF);
		/*write barrier*/
		do{
			writel(temp,te6410_keypad->keyifcol)
			mdelay(1);
		}
		while(0);
		temp = readl(te6410_keypad->keyifrow);
		if(temp & 0x000000ff){
			//te6410_keypad->colvalue = i;
			for(j = 0;j < KEYROW;j++){
				if(temp >> j == 0){
					te6410_keypad->rowvalue = j;
				}
			}
			printf(KERN_INFO "[te6410_keypad/scan_keypad]The col value is %1li,The row value is %1li.\n",te6410_keypad->colvalue,te6410_keypad->rowvalue);
			continue;
		}
	}	
	/*detect if is released*/
	//TODO
}

static void buttomTasklet(unsigned long data){
	scan_keypad();
	//update input event
}

irqreturn_t keypad_interrupt(int irq,void *dev_id){
	tasklet_schedule(&te6410_keypad->task);
	return IRQ_HANDLED;	
}

static int __init te6410_keypad_init(void){
	int retval = 0;
	te6410_keypad = kmalloc(sizeof(struct keypad),GFP_KERNEL);
	if(te6410_keypad == NULL){
		printk(KERN_ERR "[te6410_keypad/te6410_keypad_init]Can not malloc\n");
		return -ENOMEM;
	}
	te6410_keypad->irq = ;	//irq number
	/*initialize hardware resources*/
	if(!request_mem_region(KEYBASE,8,"KEYPADPIN")){
		printk(KERN_ERR "[te6410_keypad/te6410_keypad_init]Can not request memory region1\n");
		retval = -ENOMEM;
		goto failed1;
	}
	if(!request_mem_region(KEYIFBASE,5,"KEYPADREG")){
		printk(KERN_ERR "[te6410_keypad/te6410_keypad_init]Can not request memory region2\n");
		retval = -ENOMEM;
		goto failed2;
	}
	te6410_keypad->gpkcon1 = (volatile unsigned long*)ioremap_nocache(GPKCON1,1);
	te6410_keypad->gpkpud = (volatile unsigned long*)ioremap_nocache(GPKPUD,1);
	te6410_keypad->gplcon0 = (volatile unsigned long*)ioremap_nocache(GPLCON0,1);
	te6410_keypad->gplpud = (volatile unsigned long*)ioremap_nocache(GPLPUD,1);
	te6410_keypad->keyifcon = (volatile unsigned long*)ioremap_nocache(KEYIFCON,1);
	te6410_keypad->keyifstsclr = (volatile unsigned long*)ioremap_nocache(KEYIFSTSCLR,1);
	te6410_keypad->keyifcol = (volatile unsigned long*)ioremap_nocache(KEYIFCOL,1);
	te6410_keypad->keyifrow = (volatile unsigned long*)ioremap_nocache(KEYIFROW,1);
	te6410_keypad->keyiffc = (volatile unsigned long*)ioremap_nocache(KEYIFFC,1); 
	do{
		//directly use virtual address to setup register or you can use writel function
		/*setup gpk as keypad row_in*/
		te6410_keypad->gpkcon1 = (0x33333333);
		/*setup as pull up*/
		te6410_keypad->gpkpud = (0xAAAA0000);
		/*setup gpl as keypad col_out(output)*/
		te6410_keypad->gplcon0 = (0x33333333);
		/*setup as no pull*/
		te6410_keypad->gplpud = (0x00000000);

		//setup keypad interface register
		te6410_keypad->keyifcon = (0x1 << 3 |	//use division counter
								   0x1 << 2 |	//KEYPAD input port debouncing filter
								   0x1	//key_pressed interrupt
									);
		te6410_keypad->keyifstsclr = 0xffffffff;	//clear pressed interrupt
		te6410_keypad->keyiffc = 0x3ff;	//FCLK_freq = 6Khz
	}while(0);
	retval = request_irq(te6410_keypad->irq,keypad_interrupt,0,"te6410_keypad",NULL);
	if(retval){
		printk(KERN_ERR "[te6410_keypad/te6410_keypad_init]Can not request irq number\n");
		goto failed3;
	}
	DECLARED_TASKLET(te6410_keypad->task,buttomTasklet,0);
	printk(KERN_INFO "[te6410_keypad/te6410_keypad_init]Success to register te6410 keypad driver\n");
failed3:
	iounmap((void*)te6410_keypad->gpkcon1);
	iounmap((void*)te6410_keypad->gpkpud);
	iounmap((void*)te6410_keypad->gplcon0);
	iounmap((void*)te6410_keypad->gplpud);
	iounmap((void*)te6410_keypad->keyifcon);
	iounmap((void*)te6410_keypad->keyifstsclr);
	iounmap((void*)te6410_keypad->keyifcol);
	iounmap((void*)te6410_keypad->keyifrow);
	iounmap((void*)te6410_keypad->keyiffc);
	release_mem_region(KEYIFBASE,5);
failed2:
	release_mem_region(KEYBASE,8);
failed1:
	kfree(te6410_keypad);
	return retval;
}
static void __exit te6410_keypad_exit(void){
    iounmap((void*)te6410_keypad->gpkcon1);
    iounmap((void*)te6410_keypad->gpkpud);
    iounmap((void*)te6410_keypad->gplcon0);
    iounmap((void*)te6410_keypad->gplpud);
    iounmap((void*)te6410_keypad->keyifcon);
    iounmap((void*)te6410_keypad->keyifstsclr);
    iounmap((void*)te6410_keypad->keyifcol);
    iounmap((void*)te6410_keypad->keyifrow);
    iounmap((void*)te6410_keypad->keyiffc);
    release_mem_region(KEYIFBASE,5);
    release_mem_region(KEYBASE,8);
	kfree(te6410_keypad);
}

module_init(te6410_keypad_init);
module_exit(te6410_keypad_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zengweitotty");
MODULE_DESCRIPTION("te6410 keypad driver");
