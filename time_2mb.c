#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "cmd.h"
#define FILE_NAME "/mnt/hugetlbfs/a"

void call_ioctl()
{
	int fd;

	if ((fd = open("/dev/mydevice0", O_RDWR)) < 0) perror("open");

	if(ioctl(fd, HUGE_TIME2MB, NULL) < 0) perror("ioctl_set");

	if(close(fd) != 0) perror("close");
	return 0;
}

int main(void)
{

	call_ioctl();
	
	return 0;
}
