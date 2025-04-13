#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

    
int main() {
    pid_t pid = fork();
    
    if (pid < 0) {
        // Fork failed
        perror("fork failed");
        return 1;
    
    } else if (pid == 0) {
        // Child process
        printf("Hello World.\n PID: %d\n", getpid());
        printf("Parent PID: %d\n", getppid());
    } else {
        // Parent process
        printf("This is the parent process. Child PID: %d\n", pid);
        wait(NULL);
    }

    return 0;
}