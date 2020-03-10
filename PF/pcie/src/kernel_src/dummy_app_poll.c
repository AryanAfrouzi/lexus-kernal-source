
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>		/* open */
#include <unistd.h>		/* exit */
#include <sys/ioctl.h>	        /* ioctl */
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <poll.h>


#define DEV_NAME	"/dev/Triton"
#define DATA_SIZE       6
#define DATA_PATTERN    0xAAAA1116

#define MAJOR_NUM 248

#define GET_SHM_INFO 	_IOR(MAJOR_NUM, 1, int)
#define SHM_CREATE	 	_IOR(MAJOR_NUM, 2, int)
#define SHM_RELEASE	 	_IOR(MAJOR_NUM, 3, int)
#define SHM_ATTACH	 	_IOR(MAJOR_NUM, 4, int)
#define SHM_DETACH	 	_IOR(MAJOR_NUM, 5, int)
#define SHM_LOCK_RPC 	_IOR(MAJOR_NUM, 6, int)
#define SHM_UNLOCK_RPC 	_IOR(MAJOR_NUM, 7, int)
#define SHM_READ	 	_IOR(MAJOR_NUM, 8, int)
#define SHM_WRITE	 	_IOR(MAJOR_NUM, 9, int)
#define SHM_GET_LEN	 	_IOR(MAJOR_NUM, 10, int)

int main(int argc, char *argv[])
{
    int fd = 0;
    int i = 0, rc = 0, max_sd;
    int iReadReturn = 0;
    int iWriteReturn = 0;
    unsigned int data[DATA_SIZE];
    struct timeval       timeout;
    fd_set        master_set, working_set;
    int ret_val;
    int len = 0;


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

    //sleep (20);

#if 1
    struct pollfd pollfds;
    pollfds.fd = fd;
    pollfds.events = POLLIN;    /* Wait for input */
    timeout.tv_sec  = 30 * 60;
    timeout.tv_usec = 0;

    printf("Waiting on poll()...\n");

   	rc = poll (&pollfds, 1, -1);

   	printf("After poll()...Something is readable\n");
#endif

   	ret_val =ioctl(fd, SHM_GET_LEN, &len);
   	printf("Lenght of data recieved is %d and ret_val = %d\n", len, ret_val);

   	memset(data, 0, sizeof(data));
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

#if 0
    FD_ZERO(&master_set);
    max_sd = fd;
    FD_SET(fd, &master_set);

    //************************************************************/
    // Initialize the timeval struct to 3 minutes.  If no        */
    // activity after 3 minutes this program will end.           */
    //************************************************************/
    timeout.tv_sec  = 30 * 60;
    timeout.tv_usec = 0;

    memcpy(&working_set, &master_set, sizeof(master_set));

    printf("Waiting on select()...\n");
    rc = select(max_sd + 1, &working_set, NULL, NULL, &timeout);

    //**********************************************************
    // Check to see if the select call failed.                *
    //*********************************************************
    if (rc < 0)
    {
       perror("  select() failed");
       //break;
    }

    printf("Done with select, something readable..\n");
#endif

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

	sleep(1);
#endif

    close(fd);
    return 0;
}
