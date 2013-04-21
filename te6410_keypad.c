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
	int irq;
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

static int __init te6410_keypad_init(void){
	int retval = 0;
	te6410_keypad = kmalloc(sizeof(struct keypad),GFP_KERNEL);
	if(te6410_keypad == NULL){
		printk(KERN_ERR "[te6410_keypad/te6410_keypad_init]Can not malloc\n");
		return -ENOMEM;
	}
	/*initialize hardware resources*/
	if(!request_mem_region(KEYBASE,8,"KEYPADPIN")){
		printk(KERN_ERR "[te6410_keypad/te6410_keypad_init]Can not request memory region\n");
		retval =  -ENOMEM;
		goto failed1;
	}
	if(!request_mem_region(KEYIFBASE,5,"KEYPADREG")){
		printk(KERN_ERR "[te6410_keypad/te6410_keypad_init]Can not request memory region\n");
		retval =  -ENOMEM;
		goto failed2;
	}
	te6410_keypad->gpkcon1 = (volatile unsigned long*)ioremap_nocache(GPKCON1,1);
	te6410_keypad->gpkpud = (volatile unsigned long*)ioremap_nocache(GPKPUD,1);
	te6410_keypad->gplcon0 = (volatile unsigned long*)ioremap_nocache(GPLCON0,1);
	te6410_keypad->gplpud = (volatile unsigned long*)ioremap_nocache(GPLPUD,1);
	do{
		//directly use virtual address to setup register or you can use writel function
		/*setup gpk as keypad row_in*/
		te6410_keypad->gpkcon1 = (0x33333333);
		/*setup as no pull*/
		te6410_keypad->gpkpud = (0x00000000);
		/*setup gpl as keypad col_out(output)*/
		te6410_keypad->gplcon0 = (0x33333333);
		/*setup as no pull*/
		te6410_keypad->gplpud = (0x00000000);

		//setup keypad interface register

	}while(0);
failed2:
	release_mem_region(KEYBASE,8);
failed1:
	kfree(te6410_keypad);
	return retval;
}
static void __exit te6410_keypad_exit(void){
		
}

module_init();
module_exit();

MODULE_LICENSE("GPL");
MODULE_AUTHOR("zengweitotty");
MODULE_DESCRIPTION("te6410 keypad driver");
