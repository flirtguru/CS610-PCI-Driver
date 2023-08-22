#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include<linux/slab.h>                 //kmalloc()
#include<linux/uaccess.h>              //copy_to/from_user()
#include<linux/sysfs.h> 
#include<linux/kobject.h> 
#include <linux/err.h>
#include<linux/module.h>
#include<linux/pci.h>
#include<linux/delay.h>
#include<linux/wait.h>
#include<linux/mman.h>
#define DEVNAME "cs614_device"
#define CRYPTOCARD_VENDOR_ID 0X1234
#define CRYPTOCARD_DEVICE_ID 0Xdeba  
dma_addr_t dma_handle;
void *cpu_addr;
MODULE_LICENSE("GPL");
MODULE_AUTHOR("SAMARTH");
MODULE_DESCRIPTION("DRIVER FOR CRYPTOCARD");
typedef void* ADDR_PTR;
typedef int DEV_HANDLE;
typedef unsigned char KEY_COMP;
static int major;
static int it=0;
atomic_t  device_opened;
static struct class *demo_class;
struct device *demo_device;
static char *d_buf = NULL;
//static int      __init cs614_driver_init(void);
//static void     __exit cs614_driver_exit(void);
//static int enabled=0;
static int bar; 
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);
unsigned long mmio_start, mmio_len;
DEFINE_SPINLOCK(lock);
u8 __iomem *hwmem;
typedef enum {INTERRUPT, DMA} config_t;
struct pci_dev *gdev;
DECLARE_WAIT_QUEUE_HEAD(wq);
struct data
{       int type;
	ADDR_PTR s;
	uint64_t length;
	KEY_COMP a;
	KEY_COMP b;
        uint8_t ismmap;
        config_t typ;
	uint8_t set;
};
//type 1 keyset
//type 2 encrypt
//type 3 decrypt
//type 4 config

uint8_t isintr,isdma;
char* lyrics[] = {
"Listen up, let me tell you 'bout encryption,",
"It's like a secret code, it's our protection,",
"We keep our data safe and secure,",
"Only those with the key can access it for sure.",
"",
"Chali chali phir chali chali,",
"Chali encryption ki hawa chali,",
"Chali chali phir chali chali,",
"Chali encryption ki hawa chali.",
"",
"Just like the love between you and me,",
"Encryption keeps our information free,",
"From prying eyes that try to steal,",
"We encrypt our data with a seal.",
"",
"Koyi hacker koyi spy,",
"Sabko data ki talash hai,",
"Chali chali phir chali chali,",
"Chali encryption ki hawa chali,",
"Chali chali phir chali chali,",
"Chali encryption ki hawa chali.",
"",
"Encryption is our shield and sword,",
"It keeps our secrets safe on board,",
"From banking to social media,",
"Encryption keeps our data criteria.",
"",
"Say ooh ooh say la la,",
"Say ooh ooh say la la,",
"Say ooh la la la la la la la la,",
"Tara ramp pam pam tara,",
"Tara ramp pam pam tara.",
"",
"Encryption is like a love affair,",
"We keep it close, we handle with care,",
"It keeps us safe from cyber threats,",
"Encryption is the game we'll never regret.",
"",
"Chali chali phir chali chali,",
"Chali encryption ki hawa chali,",
"Chali chali phir chali chali,",
"Chali encryption ki hawa chali.",
"",
"So let's encrypt our data today,",
"And keep it safe in every way,",
"Let's spread the word about encryption,",
"And make the world a safer encryption."
};






//type 1 keyset
//type 2 encrypt
//type 3 decrypt

static struct pci_device_id cryptocard_ids[]=
{
	{PCI_DEVICE(CRYPTOCARD_VENDOR_ID,CRYPTOCARD_DEVICE_ID)},
{ }

};

MODULE_DEVICE_TABLE(pci,cryptocard_ids);

static irqreturn_t irq_handler(int irq, void *cookie)
{
  // (void) cookie;
   u32 val;
   printk("Handle IRQ #%d\n", irq);


   val=ioread32(hwmem+0x24);
if((struct pci_dev *)cookie!=gdev)
{
printk("Nakli interrupt\n");
return IRQ_NONE;
}

   iowrite32(val,hwmem+0x64);	
   wake_up_interruptible(&wq);
   return IRQ_HANDLED;
}


int set_interrupts(struct pci_dev *dev)

{


int ret=request_irq(dev->irq,irq_handler,IRQF_SHARED,"Cryptocard Interrupts",dev);




return ret;
}

static int cryptocard_probe(struct pci_dev *dev,const struct pci_device_id *id)
{
u32 idno;
u32 livcheck=4+32+16,newval;
bar = pci_select_bars(dev, IORESOURCE_MEM);
if(pci_enable_device(dev))
{
	printk("Error while enabling PCI device");
return -1;
}
printk("Entered probe function\n"); 
//if(pci_request_region(dev, bar, "My PCI driver"))
//{
//printk("Request failed\n");
//return -1;
//}
//unsigned long bar0= pci_resource_start(dev, 0);
//pci_request_region(pdev, bar, "My PCI driver");
mmio_start = pci_resource_start(dev, 0);
mmio_len = pci_resource_len(dev, 0);
hwmem = ioremap(mmio_start, mmio_len);
gdev=dev;
if(pci_read_config_dword(dev,0x0,&idno))
{
printk("Error reading\n");
	return -1;
}
printk("Identification no. is like 0x%x\n",ioread32(hwmem));
printk("mmio length is %lu\n",(unsigned long)mmio_len);
iowrite32(livcheck,hwmem+4);
newval=ioread32(hwmem+4);
printk("Old val %lu and new val %lu\n",(unsigned long)livcheck,(unsigned long)newval);

if(set_interrupts(dev))
{

	printk("Set interrupt failed\n");
return -1;
}

cpu_addr = dma_alloc_coherent(&(dev->dev),62*1024, &dma_handle, GFP_KERNEL);//https://www.kernel.org/doc/Documentation/DMA-API-HOWTO.txt
return 0;

}


static void  cryptocard_remove(struct pci_dev *dev )
{
printk("Inside remove function");

dma_free_coherent(&(dev->dev), 64*1024, cpu_addr, dma_handle);
free_irq(dev->irq,dev);
pci_disable_device(dev);


}


static struct pci_driver cryptocard_driver=
{
.name="cryptocard",
.id_table=cryptocard_ids,
.probe=cryptocard_probe,
.remove=cryptocard_remove,

};

void simple_vma_open(struct vm_area_struct *vma)
{
printk(KERN_NOTICE "Simple VMA open, virt %lx, phys %lx\n",
vma->vm_start, vma->vm_pgoff << PAGE_SHIFT);
}
void simple_vma_close(struct vm_area_struct *vma)
{
printk(KERN_NOTICE "Simple VMA close.\n");
}
static struct vm_operations_struct simple_remap_vm_ops = {
.open = simple_vma_open,
.close = simple_vma_close,
};








static int device_open(struct inode *inode, struct file *file)
{


        atomic_inc(&device_opened);
        try_module_get(THIS_MODULE);
	isintr=0;
	isdma=0;
  //	cpu_addr = dma_alloc_coherent(dev, size, &dma_handle, GFP_ATOMIC);//https://www.kernel.org/doc/Documentation/DMA-API-HOWTO.txt
  	printk(KERN_INFO "Device opened successfully\n");
        return 0;

}




static int device_release(struct inode *inode, struct file *file)
{
        atomic_dec(&device_opened);
        module_put(THIS_MODULE);
        printk(KERN_INFO "Device closed successfully\n");
        return 0;

}


static ssize_t device_read(struct file *filp,char *ubuf,size_t length,loff_t * offset)
{      

	if(copy_to_user(ubuf, d_buf, strlen(d_buf)))
		return -EINVAL;
       
         return strlen(d_buf);

}




static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{

unsigned int decryption,encryption,msk,newlen,ig=0,statusreg=0x20;

u32 keys=0,ka,kb;
void *address;
struct data *d1;
spin_lock(&lock);
d1=(struct data *)kmalloc(sizeof(struct data),GFP_KERNEL);

decryption=2;
msk=0;
encryption=1;
newlen=0;
address=(hwmem+0xa8);
ig=0;
       if(copy_from_user(d1,buff,len))
       { 
	      
	      printk("golmaal h bhai sab golmaal h\n");
	       return -EINVAL;

       }    
       	printk(KERN_INFO "In write\n");


if(d1->type==4)
{
if(d1->typ==INTERRUPT)
{

	isintr=d1->set;
}
else if(d1->typ==DMA)
{
isdma=d1->set;
}
}


if(isdma)
{
address=cpu_addr;

}

if(d1->type==1)//KEY assigned
{
	 ka=d1->a;
	kb=d1->b;
       	keys=((ka<<8)|(kb));
iowrite32(keys,hwmem+0x08);
       //	iowrite8(d1->a,hwmem+0x08+2);
//	iowrite8(d1->b,hwmem+0x08+3);
printk("Keys are %u ,%u \n",keys>>8,keys&((1<<8)-1));
       
}      

if(d1->type==2)//encrypt
{
//multiprocess logic
	 ka=d1->a;
	kb=d1->b;
       	keys=((ka<<8)|(kb));
iowrite32(keys,hwmem+0x08);

//
printk("INTR IS set %d\n",isintr);

        if(!d1->ismmap)
	{
	if(copy_from_user(d_buf,d1->s,d1->length))
	{
	printk("String copy from user space failed\n");
	return -EINVAL;
	}
	}
//printk(" %s\n",lyrics[(it++)%50]);
	
if(!isdma)
{	
while((ioread32(hwmem+statusreg)&(u32)1)==1)
{
//printk(" %s\n",lyrics[(it++)%50]);

}
}

else
{
while((readq(hwmem+0xa0)&1)==1)
{
//printk(" %s\n",lyrics[(it++)%50]);

}

}

printk("%lu",( unsigned long int)d1->length);
if(!d1->ismmap)
{
for(int i=0;i<d1->length;i++)
{
iowrite8(d_buf[i],address+i);
}
}
if(!isdma)
{
iowrite32((u32)d1->length,hwmem+0x08+4);//length of data to be encrypted
iowrite32(0|(isintr<<7),hwmem+0x20);//set encryption bit
writeq((u64)0xa8,hwmem+0x80);
}
else
{
writeq((u64)d1->length,hwmem+0x98);
writeq((u64)dma_handle,hwmem+0x90);
printk("Adresses are %lu %lu\n",(long unsigned int )cpu_addr,(long unsigned int)readq(hwmem+0x90));
writeq(1|(isintr<<2),hwmem+0xa0);//trigger


}
//printk("check string %s\n",(char *)address);


//write to data address register to trigger encryption
//copy to dbuf
//
if(!isintr)
{
if(!isdma)
{	
while((ioread32(hwmem+statusreg)&(u32)1)==1)
{
//printk(" %s\n",lyrics[(it++)%50]);

}
}

else
{
while((readq(hwmem+0xa0)&1)==1)
{
//printk(" %s\n",lyrics[(it++)%50]);

}

}
}
else
{
if(!isdma)
wait_event_interruptible(wq,((ioread32(hwmem+statusreg))&1)==0);
else
wait_event_interruptible(wq,(readq(hwmem+0xa0)&1)==0);

}
//printk("Kuch to hua encrypt\n");
//whats the length after encryption
//newlen=ioread32(hwmem+0x08+4);
//printk(" %s\n",lyrics[(it++)%50]);
if(!d1->ismmap)
memcpy(d_buf,address,d1->length);
//printk(" %s\n",lyrics[(it++)%50]);
//sprintf(d_buf,lbuff);
// printk(" %s\n",lyrics[(it++)%50]);
 printk("check string after enc %s\n",(char *)address);

if(!d1->ismmap)
{
if(copy_to_user(d1->s, d_buf, strlen(d_buf)))
		return -EINVAL;
 

}
}





if(d1->type==3)//decrypt

{
//multiprocess logic
	 ka=d1->a;
	kb=d1->b;
       	keys=((ka<<8)|(kb));
iowrite32(keys,hwmem+0x08);
//






        if(!d1->ismmap)
	{

	if(copy_from_user(d_buf,d1->s,d1->length))
	{
	printk("String copy from user space failed\n");
	return -EINVAL;
	}
	}
//printk(" %s\n",lyrics[(it++)%50]);

it=0;	
if(!isdma)
{	
while((ioread32(hwmem+statusreg)&(u32)1)==1)
{
//printk(" %s\n",lyrics[(it++)%50]);

}
}

else
{
while((readq(hwmem+0xa0)&1)==1)
{
//printk(" %s\n",lyrics[(it++)%50]);

}

}
//printk("Horha\n");
//printk("%lu",( unsigned long int)d1->length);
//memcpy(address,lbuff,d1->length);
if(!d1->ismmap){
for(int i=0;i<d1->length;i++)
{
iowrite8(d_buf[i],address+i);
}
}

if(!isdma)
{
iowrite32((u32)d1->length,hwmem+0x08+4);//length of data to be encrypted
iowrite32(2|(isintr<<7),hwmem+0x20);//set encryption bit
//printk(" %s\n",lyrics[(it++)%50]);
writeq((u64)0xa8,hwmem+0x80);

}
else
{
printk("3 value changes\n");
writeq((u64)d1->length,hwmem+0x98);
writeq((u64)dma_handle,hwmem+0x90);
writeq(3|(isintr<<2),hwmem+0xa0);//trigger

}

//write to data address register to trigger encryption
//copy to dbuf
if(!isintr){
if(!isdma)
{	
while((ioread32(hwmem+statusreg)&(u32)1)==1)
{
//printk(" %s\n",lyrics[(it++)%50]);

}
}

else
{
while((readq(hwmem+0xa0)&1)==1)
{
printk("DMA karrha h kuch\n");

}

}
}
else
{
printk("Wait mai ghusne se pehle\n");
if(!isdma)
wait_event_interruptible(wq,((ioread32(hwmem+statusreg))&1)==0);
else
wait_event_interruptible(wq,(readq(hwmem+0xa0)&1)==0);
}
//whats the length after encryption
//newlen=ioread32(hwmem+0x08+4);
//printk(" %s\n",lyrics[(it++)%50]);

//printk(" Decrypted%.12s\n",(char *)address);
if(!d1->ismmap)
memcpy(d_buf,address,d1->length);
//printk(" %s\n",lyrics[(it++)%50]);

//sprintf(d_buf,lbuff);
//printk(" %s\n",lyrics[(it++)%50]);

if(!d1->ismmap)
{
if(copy_to_user(d1->s, d_buf, strlen(d_buf)))
		return -EINVAL;
}


}

kfree(d1);
printk("DMA is set to %d\n ",isdma);
spin_unlock(&lock);

return 7;
}



static char *demo_devnode(struct device *dev, umode_t *mode)
{
        if (mode && dev->devt == MKDEV(major, 0))
                *mode = 0666;
        return NULL;
}


static int simple_remap_mmap(struct file *filp, struct vm_area_struct *vma)
{
	printk("Andarwala mmap called\n");
if (remap_pfn_range(vma, vma->vm_start, (mmio_start>>PAGE_SHIFT),
vma->vm_end - vma->vm_start,
vma->vm_page_prot)){

	printk("Andarwala mmap failed\n");
	return -EAGAIN;

}
vma->vm_ops = &simple_remap_vm_ops;
simple_vma_open(vma);
return 0;
}



static struct file_operations fops = {
        .read = device_read,
        .write = device_write,
        .open = device_open,
        .release = device_release,
	.mmap=simple_remap_mmap,
};




static int __init my_init(void){
int err;
printk("Registering cryptocard device.\n");


	printk(KERN_INFO "Hello kernel\n");
            
        major = register_chrdev(0, DEVNAME, &fops);
        err = major;
        if (err < 0) {      
             printk(KERN_ALERT "Registering char device failed with %d\n", major);   
             goto error_regdev;
        } 

        demo_class = class_create(THIS_MODULE, DEVNAME);
        err = PTR_ERR(demo_class);
        if (IS_ERR(demo_class))
                goto error_class;

        demo_class->devnode = demo_devnode;

        demo_device = device_create(demo_class, NULL,
                                        MKDEV(major, 0),
                                        NULL, DEVNAME);
        err = PTR_ERR(demo_device);
        if (IS_ERR(demo_device))
                goto error_device;
 
        d_buf = kzalloc(2*4096, GFP_KERNEL);
        printk(KERN_INFO "I was assigned major number %d. To talk to\n", major);                                                              
        atomic_set(&device_opened, 0);
       

        pr_info("Device Driver Insert...Done!!!\n");


 return pci_register_driver(&cryptocard_driver);

error_device:
         class_destroy(demo_class);
error_class:
        unregister_chrdev(major, DEVNAME);
error_regdev:
        return  err;    

}

static void  __exit my_exit(void)
{
printk(".Removing PCI Device\n");
pci_unregister_driver(&cryptocard_driver);
kfree(d_buf);
device_destroy(demo_class, MKDEV(major, 0));
class_destroy(demo_class);
unregister_chrdev(major, DEVNAME);
pr_info("Device Driver Remove...Done!!!\n");
printk(KERN_INFO "Goodbye kernel\n");
}

module_init(my_init);
module_exit(my_exit);




















