#define _GNU_SOURCE


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <liburing.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <liburing/io_uring.h>

#define ENTRIES_LENGTH 4

int main()
{
	struct io_uring ring;
	struct io_uring_sqe *sqe;
	struct io_uring_cqe *cqe;

	void *buf;

	int ret = io_uring_queue_init(ENTRIES_LENGTH, &ring, IORING_SETUP_SQE128 | IORING_SETUP_CQE32);

	printf("io_uring_queue_init ret = %d\n", ret);


	int fd  = open("README", O_RDONLY | O_DIRECT);
	if (fd < 0) {
		perror("Error opening the file\n");
		return 1;
	}

	struct iovec *iovecs;
	iovecs = calloc(ENTRIES_LENGTH, sizeof(struct iovec));

	int i;
	for (i = 0 ; i < ENTRIES_LENGTH; i++) {
		// Let's just  read the first 4k
		if (posix_memalign(&buf, 4096, 4096))
			return 1;

		iovecs[i].iov_base = buf;
		printf("%d %p\n", i, iovecs[i].iov_base);
		printf("X -> %s\n", (char *) iovecs[i].iov_base);
		iovecs[i].iov_len = 4096;
		buf = 0x0;
	}



	int offset = 0;

	for (i = 0 ; i < ENTRIES_LENGTH; i++) {
		sqe = io_uring_get_sqe(&ring);
		printf("sqe value = %p\n", sqe);
		int z = i + 42;
		io_uring_prep_readv(sqe, fd, &iovecs[i], 1, offset);
		offset += iovecs[i].iov_len;
		printf("seq->opcode = %d\n", sqe->opcode);
		printf("seq->flags = %d\n", sqe->flags);
		io_uring_sqe_set_data(sqe, &z);
		offset += iovecs[i].iov_len;
	}

	int pending = io_uring_submit(&ring);
	printf("pending = %d\n", pending);

	for (i = 0; i < pending; i++) {
		ret = io_uring_wait_cqe(&ring, &cqe);
		printf("Waiting %d returned with %d\n", i, ret);
		printf("cqe->res = %d\n", cqe->res);
		io_uring_cqe_get_data(cqe);
		printf("cqe->user_data = %p %ld\n", cqe->user_data, *(long int *)(cqe->user_data));
		printf("cqe->flags = %d\n", cqe->flags);
		io_uring_cqe_seen(&ring, cqe);
	}

	for (i = 0; i < pending; i++) {
		printf("i = %d\n", i);
		printf("%d: output = %d\n", i, iovecs[i].iov_len);
		printf("%d: output = %p\n", i, (char *) iovecs[i].iov_base);
		printf("%d: output = %s\n", i, (char *) iovecs[i].iov_base);
		printf("%d: output = %s\n", i, (char *) (iovecs[i].iov_base + 5));
	}


	close(fd);
        io_uring_queue_exit(&ring);

	return 0;

}

