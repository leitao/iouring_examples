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

void create_sqes(struct io_uring *ring, int fd)
{
	struct io_uring_sqe *sqe;
	int data = 0; //0xdeadbeef;

	sqe = io_uring_get_sqe(ring);

	sqe->opcode = IORING_OP_MSG_RING;
	sqe->fd = fd;
	io_uring_sqe_set_data(sqe, &data);
}

int main()
{
	struct io_uring ring1, ring2;
	struct io_uring_cqe *cqe;

	int fd1 = io_uring_queue_init(ENTRIES_LENGTH, &ring1, IORING_SETUP_SQE128 | IORING_SETUP_CQE32);
	int fd2 = io_uring_queue_init(ENTRIES_LENGTH, &ring2, IORING_SETUP_SQE128 | IORING_SETUP_CQE32);

	// Send msg from ring1 to ring2
	create_sqes(&ring1, fd2);
	int pending = io_uring_submit(&ring1);
	printf("Pending = %x\n", pending);

	int ret = io_uring_wait_cqe(&ring2, &cqe);
	printf("ret = %d\n", ret);
	printf("data = %x\n", cqe->user_data);

        io_uring_queue_exit(&ring1);
        io_uring_queue_exit(&ring2);

	return 0;
}
