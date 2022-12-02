// Example of catting a file using libiouring


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


struct iovec *get_iovecs(int fd)
{
	struct iovec *iovecs;
	void *buf;


	iovecs = calloc(ENTRIES_LENGTH, sizeof(struct iovec));

	for (int i = 0 ; i < ENTRIES_LENGTH; i++) {
		// Let's just  read the first 4k
		if (posix_memalign(&buf, 4096, 4096))
			return NULL;

		iovecs[i].iov_base = buf;
		printf("%d %p\n", i, iovecs[i].iov_base);
		printf("X -> %s\n", (char *) iovecs[i].iov_base);
		iovecs[i].iov_len = 4096;
		buf = 0x0;
	}

	return iovecs;
}

void create_sqes(struct io_uring *ring, int fd, struct iovec *iovecs)
{
	struct io_uring_sqe *sqe;
	int offset = 0;

	for (int i = 0 ; i < ENTRIES_LENGTH; i++) {
		sqe = io_uring_get_sqe(ring);
		printf("sqe value = %p\n", sqe);
		int z = i + 42;
		io_uring_prep_readv(sqe, fd, &iovecs[i], 1, offset);
		offset += iovecs[i].iov_len;
		printf("seq->opcode = %d\n", sqe->opcode);
		printf("seq->flags = %d\n", sqe->flags);
		io_uring_sqe_set_data(sqe, &z);
		offset += iovecs[i].iov_len;
	}
}


void parse_cqes(struct io_uring *ring, int pending)
{
	int ret;
	struct io_uring_cqe *cqe;

	for (int i = 0; i < pending; i++) {
		ret = io_uring_wait_cqe(ring, &cqe);
		printf("Waiting %d returned with %d\n", i, ret);
		printf("cqe->res = %d\n", cqe->res);
		io_uring_cqe_get_data(cqe);
		printf("cqe->user_data = %llx %ld\n", cqe->user_data, *(long int *)(cqe->user_data));
		printf("cqe->flags = %d\n", cqe->flags);
		io_uring_cqe_seen(ring, cqe);
	}
}

void consume_iovecs(int pending, struct iovec *iovecs)
{
	for (int i = 0; i < pending; i++) {
		printf("%d: len = %zu\n", i, iovecs[i].iov_len);
		printf("%d: data = %s\n", i, (char *) iovecs[i].iov_base);
	}
}

int main()
{
	struct io_uring ring;
	int fd, pending;
	struct iovec *iovecs;


	int ret = io_uring_queue_init(ENTRIES_LENGTH, &ring, IORING_SETUP_SQE128 | IORING_SETUP_CQE32);
	if (ret){
		perror("Failed to initialize iouring");
		exit(-1);
	}

	printf("io_uring_queue_init ret = %d\n", ret);

	fd  = open("LICENSE", O_RDONLY | O_DIRECT);
	if (fd < 0) {
		perror("Error opening the file\n");
		return -1;
	}

	iovecs = get_iovecs(fd);

	if (!iovecs) {
		printf("Not able to alloc iovecs\n");
		return -1;
	}

	create_sqes(&ring, fd, iovecs);

	pending = io_uring_submit(&ring);
	printf("pending = %d\n", pending);
	parse_cqes(&ring, pending);
	consume_iovecs(pending, iovecs);


	// Wrapping up
	close(fd);
        io_uring_queue_exit(&ring);

	return 0;
}
