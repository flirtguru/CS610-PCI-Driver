#include<crypter.h>
#include<sys/mman.h>
/*Function template to create handle for the CryptoCard device.
On success it returns the device handle as an integer*/
struct data
{       int type;
	ADDR_PTR s;
	uint64_t length;
	KEY_COMP a;
	KEY_COMP b;
        uint8_t ismmap;
        config_t typ;
	uint8_t set;
}*d2;
//type 1 keyset
//type 2 encrypt
//type 3 decrypt
//type 4 config
DEV_HANDLE create_handle()
{


d2=(struct data *)malloc(sizeof(struct data));//to handle multiprocess information
d2->typ=0;
d2->set=0;
DEV_HANDLE fd=open("/dev/cs614_device",O_RDWR);
  return fd;
}

/*Function template to close device handle.
Takes an already opened device handle as an arguments*/
void close_handle(DEV_HANDLE cdev)
{
close(cdev);

}

/*Function template to encrypt a message using MMIO/DMA/Memory-mapped.
Takes four arguments
  cdev: opened device handle
  addr: data address on which encryption has to be performed
  length: size of data to be encrypt
  isMapped: TRUE if addr is memory-mapped address otherwise FALSE
*/
int encrypt(DEV_HANDLE cdev, ADDR_PTR addr, uint64_t length, uint8_t isMapped)
{
//write free logic
int ret;
struct data *d1=(struct data *)malloc(sizeof(struct data));
d1->type=2;
d1->s=addr;
d1->length=length;
d1->a=d2->a;
d1->b=d2->b;
d1->typ=d2->typ;
d1->set=d2->set;
//include the null character in length as strlen is used.
if(*(char *)(addr+d1->length)=='\0')
{
d1->length++;
}

d1->ismmap=isMapped;
int left=d1->length;

//sending data in 128*4096 size chunks
while(left>128*4096)
{ 
  d1->length=128*4096;	
	ret=write(cdev,d1,sizeof(struct data));
        left-=128*4096;
//	d1->length-=128*4096;
	d1->s=d1->s+128*4096;
}
d1->length=left;
if(left>0)
 ret = write(cdev, d1, sizeof(struct data));
  return ret;
}

/*Function template to decrypt a message using MMIO/DMA/Memory-mapped.
Takes four arguments
  cdev: opened device handle
  addr: data address on which decryption has to be performed
  length: size of data to be decrypt
  isMapped: TRUE if addr is memory-mapped address otherwise FALSE
*/
int decrypt(DEV_HANDLE cdev, ADDR_PTR addr, uint64_t length, uint8_t isMapped)
{
	int ret;
 struct data *d1=(struct data *)malloc(sizeof(struct data));
d1->type=3;
d1->s=addr;
d1->length=length;
d1->a=d2->a;
d1->b=d2->b;
d1->typ=d2->typ;
d1->set=d2->set;
if(*(char *)(addr+d1->length)=='\0')
{
d1->length++;
}
d1->ismmap=isMapped;

int left=d1->length;
while(left>4096*128)
{        
	d1->length=128*4096;
	ret=write(cdev,d1,sizeof(struct data));
        left-=128*4096;
//	d1->length-=128*4096;
	d1->s=d1->s+128*4096;


}

d1->length=left;



if(left>0)
ret = write(cdev, d1, sizeof(struct data));
return ret;
 
}

/*Function template to set the key pair.
Takes three arguments
  cdev: opened device handle
  a: value of key component a
  b: value of key component b
Return 0 in case of key is set successfully*/
int set_key(DEV_HANDLE cdev, KEY_COMP a, KEY_COMP b)
{

struct data *d1=(struct data *)malloc(sizeof(struct data));
d1->type=1;
d1->a=a;
d1->b=b;

d2->a=a;
d2->b=b;
int ret = write(cdev, d1, sizeof(struct data));
  return ret;
}

/*Function template to set configuration of the device to operate.
Takes three arguments
  cdev: opened device handle
  type: type of configuration, i.e. set/unset DMA operation, interrupt
  value: SET/UNSET to enable or disable configuration as described in type
Return 0 in case of key is set successfully*/
int set_config(DEV_HANDLE cdev, config_t type, uint8_t value)
{

 struct data *d1=(struct data *)malloc(sizeof(struct data));
 d1->type=4;
d1->typ=type;
d1->set=value;
d2->typ=type;
d2->set=value;


//printf("Interrupt is %d",d1->set);
 int ret = write(cdev, d1, sizeof(struct data));
 	
  return ret;
}

/*Function template to device input/output memory into user space.
Takes three arguments
  cdev: opened device handle
  size: amount of memory-mapped into user-space (not more than 1MB strict check)
Return virtual address of the mapped memory*/
ADDR_PTR map_card(DEV_HANDLE cdev, uint64_t size)
{
       	if(size<=1024*1024)size=1024*1024;
            else return NULL;

      unsigned long int ret=mmap(NULL,size,PROT_READ|PROT_WRITE,MAP_SHARED,cdev,0);      
 ret+=0xa8;

 return (void *)ret;
}

/*Function template to device input/output memory into user space.
Takes three arguments
  cdev: opened device handle
  addr: memory-mapped address to unmap from user-space*/
void unmap_card(DEV_HANDLE cdev, ADDR_PTR addr)
{
      munmap(addr-0xa8,1024*1024);
	return ;
}
