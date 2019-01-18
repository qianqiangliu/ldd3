#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "%s <num>", argv[0]);
		return 1;
	}

	int fd = open("/dev/short", O_RDWR);
	if (fd < 0) {
		perror("open()");
		return 1;
	}

	unsigned char data = atoi(argv[1]);
	write(fd, &data, 1);
	close(fd);
	return 0;
}
