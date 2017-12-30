#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "scullc.h"

int main()
{
	int fd;

	fd = open("/dev/scullc0", O_RDONLY);
	if (fd < 0) {
		perror("open()");
		return 1;
	}

	int quantum;
	int ret = ioctl(fd, SCULLC_IOCGQUANTUM, &quantum);
	if (ret < 0) {
		perror("ioctl()");
		return 1;
	}
	printf("quantum: %d\n", quantum);

	int qset = ioctl(fd, SCULLC_IOCQQSET);
	if (qset < 0) {
		perror("ioctl()");
		return 1;
	}
	printf("qset: %d\n", qset);

	return 0;
}
