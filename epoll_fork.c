#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <errno.h>


static void run_child(int epoll_fd)
{
	struct epoll_event event;
	int fd[2];
	int i, ret;

	pipe(fd);

	memset(&event, 0, sizeof(event));

	event.events = EPOLLIN|EPOLLERR;
	event.data.u32 = getpid();

	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd[0], &event);

	for (i=0;i<100000;i++) {
		char c = 0;
		write(fd[1], &c, 1);
		ret = epoll_wait(epoll_fd, &event, 1, 0);
		if (ret <= 0) {
			continue;
		}
		if (event.data.u32 != getpid()) {
			printf("Wrong pid! should be %u but got %u\n", 
			       getpid(), event.data.u32);
		}
	}
	exit(0);
}


int main(void) 
{
	int epoll_fd;
	pid_t child1, child2;

	epoll_fd = epoll_create(64);

	child1 = fork();
	if (child1 == 0) {
		run_child(epoll_fd);
	}
	child2 = fork();
	if (child2 == 0) {
		run_child(epoll_fd);
	}

	waitpid(child1, NULL, 0);
	waitpid(child2, NULL, 0);

	return 0;
}
