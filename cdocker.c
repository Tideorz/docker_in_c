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

int container_main(void* arg)
{
    printf("Container Process Id: [%d] - inside the container!\n", getpid());

    // CLONE_NEWUTS
    // set hostname of container 
    sethostname("container", 10);

    // CLONE_NEWNS
    //remount "/proc" to make sure the "top" and "ps" show container's information
    if (mount("proc", "rootfs/proc2", "proc", 0, NULL) !=0 ) {
        perror("proc");
    }
    /*
     * 模仿Docker的从外向容器里mount相关的配置文件
     * 你可以查看：/var/lib/docker/containers/<container_id>/目录，
     * 你会看到docker的这些文件的。
     */
    if (mount("conf/hosts", "rootfs/etc/hosts", "none", MS_BIND, NULL) != 0 ||
          mount("conf/hostname", "rootfs/etc/hostname", "none", MS_BIND, NULL) != 0 ||
          mount("conf/resolv.conf", "rootfs/etc/resolv.conf", "none", MS_BIND, NULL) != 0 ) {
        perror("conf");
    }
    /* chroot 隔离目录 */
    if ( chdir("./rootfs") != 0 || chroot("./") != 0 ){
        perror("chdir/chroot");
    }

    /* 直接执行一个shell，以便我们观察这个进程空间里的资源是否被隔离了 */
    execv(container_args[0], container_args); 
    printf("Something's wrong!\n");
    return 1;
}
 
int main()
{
    printf("Parent - start a container!\n");
    /* 调用clone函数，其中传出一个函数，还有一个栈空间的（为什么传尾指针，因为栈是反着的） */
    int container_pid = clone(container_main, container_stack+STACK_SIZE,
	     CLONE_NEWNS | CLONE_NEWIPC | CLONE_NEWUTS | SIGCHLD, NULL);
    if (container_pid < 0) {
        printf("ERROR: Unable to create the child process.\n");
        exit(EXIT_FAILURE);
    }

    /* 等待子进程结束 */
    waitpid(container_pid, NULL, 0);
    printf("Parent - container stopped!\n");
    return 0;
}
