#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>		/* open */
#include <unistd.h>		/* exit */
#include <sys/ioctl.h>	        /* ioctl */
#include <sys/poll.h>
#include <string.h>
#include <sys/mman.h>
#include <string.h>

#define DEV_NAME	"/dev/Triton"
#define DATA_SIZE       40
#define DATA_PATTERN    0xAAAA1116

int main(int argc, char *argv[])
{
    int fd = 0;
    int i = 0;
    int iReadReturn = 0;
    int iWriteReturn = 0;
    unsigned int data[DATA_SIZE];
    void *mmap_addr;
    //char *str = "123";
    unsigned int data1[2];
    unsigned int data2[2];
    data1[0] = DATA_PATTERN;
    data1[1] = DATA_PATTERN+1;

    /*system("echo 1 > /sys/bus/pci/devices/0000\:00\:18.0/remove");
    system("echo 1 > /sys/bus/pci/rescan");
    system("lspci -v");
    sleep(1);*/

    /*system("rmmod triton_ep");
    system("rm -f /dev/triton");
    system("insmod /usr/tmp/triton_ep.ko");
    system("mknod /dev/triton c 248 0");
    sleep(1);*/

    fd = open(DEV_NAME, O_RDWR);
    if(fd <0)
    {
        printf("open error %d : %s\n", errno, strerror(errno));
        return -1;
    }

    sleep (3);

    mmap_addr = mmap(NULL, 8*1024*1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mmap_addr != (void *) -1)
    {
    	printf("Address = %x \n", mmap_addr);
    	memset(mmap_addr, 2, (8*1024*1024));
    	memcpy(mmap_addr, (const void *) &data1, (2*sizeof(int)));
    	data2[0] = *(unsigned int *)mmap_addr;
    	data2[1] = *(unsigned int *)mmap_addr+1;
    	printf("kd - 0x%08X   0x%08X \n", data2[0], data2[1]);
    }
    else
    {
    	printf("mmap failed\n");
    	close(fd);
    	return -1;
    }

#if 0
    if ((iReadReturn = read(fd, data, sizeof(data))) == -1)
        {
            printf("read failed %d : %s\n", errno, strerror(errno));
            return -1;
        }
    	printf("\nInitial Read return %d\n",iReadReturn);
        for(i = 0; i < DATA_SIZE; i++)
        {
            if((i % 4) == 0)
            {
                printf("\n");
            }
            printf("B 0x%08X ", data[i]);
    	// printf("B 0x%x", data[i]);
       }
        printf("\n");
    sleep (2);

    memset(data, 0, sizeof(data));
    for(i = 0; i < DATA_SIZE; i++)
    {
       data[i] = DATA_PATTERN + i;
    }

    if ((iWriteReturn = write(fd, data, sizeof(data))) == -1)
    {
        printf("write failed %d : %s\n", errno, strerror(errno));
        return -1;
    }
    printf("\nWrite return %d\n",iWriteReturn);
    sleep(2);

    memset(data, 0, sizeof(data));
    if ((iReadReturn = read(fd, data, sizeof(data))) == -1)
        {
            printf("read failed %d : %s\n", errno, strerror(errno));
            return -1;
        }
    printf("\nRead return %d\n",iReadReturn);
        for(i = 0; i < DATA_SIZE; i++)
        {
            if((i % 4) == 0)
            {
                printf("\n");
            }
            printf("B %d = 0x%08X \t",i, data[i]);
    	// printf("B 0x%x", data[i]);
       }

   // memset(data, 0, sizeof(data));
#endif

	sleep(1);

	if (munmap(mmap_addr, 8*1024*1024) == -1)
	{
		printf("munmap failed\n");
	}
    close(fd);
    return 0;
}
