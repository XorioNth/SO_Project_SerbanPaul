#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
volatile sig_atomic_t running = 1;

void handle_sigint(int sig){
    running = 0;
}

void handle_sigusr1(int sig){
    printf("Monitor - New report has been added\n");
    fflush(stdout);
}
int main(){
    int f = open(".monitor_pid", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(f == -1){
        perror("Error creating .monitor.pid");
        return 1;
    }
    char pid_str[32];
    int len = snprintf(pid_str, sizeof(pid_str), "%d\n", getpid());
    write(f, pid_str, len);
    close(f);
    printf("Monitor started with PID %d\n", getpid());
    struct sigaction sa_int, sa_usr1;
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);
    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;
    sigaction(SIGUSR1, &sa_usr1, NULL);
    while(running){
        pause();
    }
    unlink(".monitor_pid");
    printf("Monitor stopped\n");
    return 0;
}