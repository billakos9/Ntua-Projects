#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>

int buf_counter(char *buffer, char c2c) {
    int count = 0;
    for (size_t i = 0; i < strlen(buffer); i++) {
        if (buffer[i] == c2c) {
            count++;
        }
    }
    return count;
}

void itoa_sys(int num, char *buffer) {
    int i = 0;
    int is_negative = 0;

    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    do {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    } while (num > 0);

    if (is_negative) {
        buffer[i++] = '-';
    }

    for (int j = 0; j < i / 2; j++) {
        char temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }

    buffer[i] = '\0';
}

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
    
    char c2c;
    int fd1, fd2;
    int oflags, mode;
    char buffer[1024];

    //open the files
    fd1 = open(argv[1], O_RDONLY);
    oflags = O_CREAT | O_WRONLY | O_TRUNC;
    mode = S_IRUSR | S_IWUSR;
    fd2 = open(argv[2], oflags, mode);

    //check for errors on open
    if (fd1 == -1) {
        perror("Problem opening file to read\n");
        return 1;
    }
    if (fd2 == -1) {
        perror("Problem opening file to write\n");
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;

    } else if (pid == 0) {
        c2c = argv[3][0]; /* Character to search for (third parameter in command line) */
        size_t count_bytes = sizeof(buffer) - 1;
        ssize_t rfile;
        int total_count = 0;
        char total_count_str[12] = {0}; // To conver total_count to string and write it in the output file || Important to initialize.

        do {
            rfile = read(fd1, buffer, count_bytes);
            if (rfile == -1) {
                perror("Problem reading characters\n");
                close(fd1);
                return 1;
            }
            buffer[rfile] = '\0';
            total_count += buf_counter(buffer, c2c); // Count the number of times the character appears in the buffer
            if (rfile == 0) {
                break;
            }
        }
        while (rfile != 0);

        printf("Total count: %d\n", total_count); // Print the total count before conversion
        itoa_sys(total_count, total_count_str);
        printf("Total count as string: %s\n", total_count_str); // Check if the conversion is correct

        _exit(0);

    } else {
        // Parent waits and close files
        wait(NULL);
        close(fd1);
        close(fd2);
    }

    return 0;
}
