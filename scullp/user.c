#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "scullp.h"

int main()
{
	int fd;

	fd = open("/dev/scull0", O_RDONLY);
	if (fd < 0) {
		perror("open()");
		return 1;
	}

	int quantum;
	int ret = ioctl(fd, SCULLP_IOCGORDER, &quantum);
	if (ret < 0) {
		perror("ioctl()");
		return 1;
	}
	printf("quantum: %d\n", quantum);

	int qset = ioctl(fd, SCULLP_IOCQQSET);
	if (qset < 0) {
		perror("ioctl()");
		return 1;
	}
	printf("qset: %d\n", qset);

	return 0;
}
