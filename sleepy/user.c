#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main()
{
	int fd;

	fd = open("/dev/sleepy", O_RDWR);
	if (fd < 0) {
		perror("open()");
		return 1;
	}

	int ret;
	char buf[4096];

	ret = read(fd, buf, sizeof(buf));
	if (ret < 0) {
		perror("read()");
		return 1;
	}

	return 0;
}
