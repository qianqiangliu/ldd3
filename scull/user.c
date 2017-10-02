#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "scull.h"

int main()
{
	int fd;

	fd = open("/dev/scull0", O_RDONLY);
	if (fd < 0) {
		perror("open()");
		return 1;
	}

	int quantum;
	int ret = ioctl(fd, SCULL_IOCGQUANTUM, &quantum);
	if (ret < 0) {
		perror("ioctl()");
		return 1;
	}
	printf("quantum: %d\n", quantum);

	int qset = ioctl(fd, SCULL_IOCQQSET);
	if (qset < 0) {
		perror("ioctl()");
		return 1;
	}
	printf("qset: %d\n", qset);

	return 0;
}
