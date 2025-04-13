#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h> //read , write, close
#include <string.h>
#include <stdio.h>



int main(int argc, char *argv[]) {
    // Check the arguments
    if (argc != 4) {
        perror("wrong number of args\n");
        return 1;
    }
    if (argv[3][1] != '\0') {
        perror("Wrong character input");
        return 2;
    }

    pid_t pid = fork();

    if (pid == 0) {
        // Child proccess → execv 1.1
        char *args[] = { "../1.1/afe11-1", argv[1], argv[2], argv[3], NULL };
        execv("../1.1/afe11-1", args);
        perror("execv failed");
        _exit(1);

    } else {
        // Parent waits
        wait(NULL);
        printf("Parent: child process finished.\n");
    }

    return 0;
}
