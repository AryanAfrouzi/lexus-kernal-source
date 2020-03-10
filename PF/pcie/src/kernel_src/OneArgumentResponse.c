
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>		/* open */
#include <unistd.h>		/* exit */
#include <sys/ioctl.h>	        /* ioctl */
#include <sys/poll.h>
#include <string.h>

#define DEV_NAME	"/dev/triton"
#define DATA_SIZE       56
#define DATA_PATTERN    0xAAAA1116

int main(int argc, char *argv[])
{
    int fd = 0;
    int i = 0;
    int iReadReturn = 0;
    int iWriteReturn = 0;
    unsigned int data[DATA_SIZE/4];

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

   /* sleep (2);
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
*/
    data[0] = 0x1ff;
    data[1] = 0x200fc;
    data[2] = 0x8002;
    data[3] = 0x28;
    data[4] = 0x16006;
    data[5] = 0x0;
    data[6] = 0x200183;
    data[7] = 0x1;
    data[8] = 0x2;
    data[9] = 0x0;
    data[10] = 0x4ffff;
    data[11] = 0x7;
    data[12] = 0xa;
    data[13] = 0x0;

    printf("\nWrite response\n");
    if ((iWriteReturn = write(fd, data, DATA_SIZE)) == -1)
    {
        printf("write failed %d : %s\n", errno, strerror(errno));
        return -1;
    }
    printf("\nWrite return %d\n",iWriteReturn);
  //  sleep(2);

  /*  memset(data, 0, sizeof(data));
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
*/
	sleep(1);
    close(fd);
    return 0;
}
