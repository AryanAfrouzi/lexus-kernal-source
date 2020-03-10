
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
    unsigned char data[DATA_SIZE];

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
    data[0] = 0xff;
    data[1] = 0x1;
    data[2] = 0x0;
    data[3] = 0x0;
    data[4] = 0xfc;
    data[5] = 0x0;
    data[6] = 0x2;
    data[7] = 0x0;
    data[8] = 0xbb;
    data[9] = 0x6d;
    data[10] = 0x89;
    data[11] = 0xbd;
    data[12] = 0x28;
    data[13] = 0x0;
    data[14] = 0x0;
    data[15] = 0x0;
    data[16] = 0x06;
    data[17] = 0x60;
    data[18] = 0x1;
    data[19] = 0x0;
    data[20] = 0xfd;
    data[21] = 0xfa;
    data[22] = 0xff;
    data[23] = 0x6d;
    data[24] = 0x91;
    data[25] = 0x1;
    data[26] = 0x20;
    data[27] = 0x0;
    data[28] = 0x0;
    data[29] = 0x0;
    data[30] = 0x1;
    data[31] = 0x0;
    data[32] = 0x2;
    data[33] = 0x0;
    data[34] = 0x0;
    data[35] = 0x8;
    data[36] = 0x0;
    data[37] = 0x0;
    data[38] = 0x0;
    data[39] = 0x0;
    data[40] = 0x0;
    data[41] = 0x0;
    data[42] = 0x4;
    data[43] = 0x0;
    data[44] = 0x6;
    data[45] = 0x0;
    data[46] = 0x0;
    data[47] = 0x0;
    data[48] = 0x1;
    data[49] = 0x0;
    data[50] = 0x0;
    data[51] = 0x0;
    data[52] = 0x0;
    data[53] = 0x0;
    data[54] = 0x0;
    data[55] = 0x0;

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
