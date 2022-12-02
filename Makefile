all:
	gcc -g io_uring-test.c -o io_uring-test -luring
	gcc -g io_uring-test-withflag.c -o io_uring-test-withflag -luring
