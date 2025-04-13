#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

    
int main() {
    int x = 15;

    pid_t pid = fork();
    
    if (pid < 0) {
        // Fork failed
        perror("fork failed");
        return 1;
    
    } else if (pid == 0) {
        // Child process
        x = 10;
        printf("Hello World. This is the child proccess.\n PID: %d\n", getpid());
        printf("Parent PID: %d\n", getppid());
        printf(x);
    } else {
        // Parent process
        x = 20;
        printf("This is the parent process. Child PID: %d\n", pid);
        wait(NULL);
        printf(x);
    }

    return 0;
}