all:
	gcc -g io_uring-test.c -o io_uring-test -luring
	gcc -g send_msg.c -o send_msg -luring
