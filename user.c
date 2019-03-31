#define _GNU_SOURCE
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <signal.h>
#include <unistd.h>
 
/* 定义一个给 clone 用的栈，栈大小1M */
#define STACK_SIZE (1024 * 1024)
static char container_stack[STACK_SIZE];
 
char* const container_args[] = {
    "/bin/bash",
    NULL
};

// Pipe for make user CLONE_NEWUID, in parent process,
// it creats map pid file before child process exec bash
// pipefd[0] for reading, pipefd[1] for writing
int pipefd[2];

void set_map(char* file, int inside_id, int outside_id, int len) {
    FILE* mapfd = fopen(file, "w");
    if (NULL == mapfd) {
        perror("open file error");
        return;
    }
    fprintf(mapfd, "%d %d %d", inside_id, outside_id, len);
    fclose(mapfd);
}

void set_uid_map(pid_t pid, int inside_id, int outside_id, int len) {
    char file[256];
    sprintf(file, "/proc/%d/uid_map", pid);
    set_map(file, inside_id, outside_id, len);
}

void set_gid_map(pid_t pid, int inside_id, int outside_id, int len) {
    char file[256];
    sprintf(file, "/proc/%d/gid_map", pid);
    set_map(file, inside_id, outside_id, len);
}
 
int container_main(void* arg)
{
    printf("Container Process Id: [%d] - inside the container!\n", getpid());
    printf("Container: eUID = %ld;  eGID = %ld, UID=%ld, GID=%ld\n",
            (long) geteuid(), (long) getegid(), (long) getuid(), (long) getgid());
 
    // close writing end of pipe, as we only need reading end
    close(pipefd[1]);
    // waiting for parent process's notification
    char ch;
    read(pipefd[0], &ch, 1);
    close(pipefd[0]);

    // CLONE_NEWUTS
    // set hostname of container 
    sethostname("container", 10);

    /* 直接执行一个shell，以便我们观察这个进程空间里的资源是否被隔离了 */
    execv(container_args[0], container_args); 
    printf("Something's wrong!\n");
    return 1;
}
 
int main()
{
    const int gid=getgid(), uid=getuid();
    printf("Parent Process: eUID = %ld;  eGID = %ld, UID=%ld, GID=%ld\n",
        (long) geteuid(), (long) getegid(), (long) getuid(), (long) getgid());

    pipe(pipefd);
    // close reading end of pipe, as we only need reading end.
    close(pipefd[0]);

    printf("Parent - start a container!\n");
    /* 调用clone函数，其中传出一个函数，还有一个栈空间的（为什么传尾指针，因为栈是反着的） */
    int container_pid = clone(container_main, container_stack+STACK_SIZE,
	CLONE_NEWUSER | SIGCHLD, NULL);
    if (container_pid < 0) {
        printf("ERROR: Unable to create the child process.\n");
        exit(EXIT_FAILURE);
    }

    //To map the uid/gid,
    //   we need edit the /proc/PID/uid_map (or /proc/PID/gid_map) in parent
    //The file format is
    //   ID-inside-ns   ID-outside-ns   length
    //if no mapping,
    //   the uid will be taken from /proc/sys/kernel/overflowuid
    //   the gid will be taken from /proc/sys/kernel/overflowgid
    set_uid_map(container_pid, 0, uid, 1);
    set_gid_map(container_pid, 0, gid, 1);
    printf("Parent [%5d] - user/group mapping done!\n", getpid());

    // notify child process we have finished pid mapping in parent process.
    close(pipefd[1]);

    /* 等待子进程结束 */
    waitpid(container_pid, NULL, 0);
    printf("Parent - container stopped!\n");
    return 0;
}
