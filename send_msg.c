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

#define ENTRIES_LENGTH 2

/*
        msg->user_data = READ_ONCE(sqe->off);
        msg->len = READ_ONCE(sqe->len);
        msg->cmd = READ_ONCE(sqe->addr);
        msg->src_fd = READ_ONCE(sqe->addr3);
        msg->dst_fd = READ_ONCE(sqe->file_index);
        msg->flags = READ_ONCE(sqe->msg_ring_flags);

 */
void create_sqes(struct io_uring *ring, int fd)
{
	struct io_uring_sqe *sqe;
	int data = 0xdeadbeef;

	sqe = io_uring_get_sqe(ring);
	sqe->opcode = IORING_OP_MSG_RING;
	/* addr is subcmd */
	sqe->addr = IORING_MSG_DATA;
	sqe->fd = fd;
	io_uring_sqe_set_data64(sqe, 0x1024);
}

int main()
{
	struct io_uring ring1, ring2;
	struct io_uring_cqe *cqe;

	int fd1 = io_uring_queue_init(ENTRIES_LENGTH, &ring1, 0);
	int fd2 = io_uring_queue_init(ENTRIES_LENGTH, &ring2, 0);

	// Send msg from ring1 to ring2
	create_sqes(&ring1, fd2);
	int pending = io_uring_submit(&ring1);
	printf("Pending = %x\n", pending);

	printf("Waiting on ring 1\n");
	int ret1 = io_uring_wait_cqe(&ring1, &cqe);
	printf(" ret1 = %d\n", ret1);
	printf(" data = %x\n", cqe->user_data);
	printf(" flags = %x\n", cqe->flags);

	printf("Waiting on ring 2\n");
	int ret2 = io_uring_wait_cqe(&ring2, &cqe);
	printf("ret = %d\n", ret2);
	printf("data = %x\n", cqe->user_data);



        io_uring_queue_exit(&ring1);
        io_uring_queue_exit(&ring2);

	return 0;
}
