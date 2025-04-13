#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define BUFFER_SIZE 1024

int buf_counter(char *buffer, ssize_t len, char c2c) {
    int count = 0;
    for (ssize_t i = 0; i < len; i++) {
        if (buffer[i] == c2c) {
            count++;
        }
    }
    return count;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        perror("[WORKER] Wrong number of arguments");
        return 1;
    }
    if (argv[2][1] != '\0') {
        perror("[WORKER] I need a single character input");
        return 2;
    }

    const char *input_file = argv[1];
    char search_char = argv[2][0];

    // Open the file once
    int fd = open(input_file, O_RDONLY);
    if (fd == -1) {
        perror("[WORKER] Failed to open input file");
        return 1;
    }

    // Get file information using fstat
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("[WORKER] fstat failed");
        close(fd);
        return 1;
    }
    close(fd); // Close the file descriptor after getting the size

    off_t filesize = st.st_size; // Total file size (can be useful later)

    char msg[256];
    snprintf(msg, sizeof(msg), "[WORKER %d] Ready to work (file size: %ld bytes)\n", getpid(), (long)filesize);
    write(STDERR_FILENO, msg, strlen(msg));

    fd_set readfds;
    
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, NULL) > 0) {
            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                char buffer_cmd[128];
                ssize_t bytes_read = read(STDIN_FILENO, buffer_cmd, sizeof(buffer_cmd) - 1);
                if (bytes_read <= 0) {
                    break;
                }
                buffer_cmd[bytes_read] = '\0';

                long offset;
                int length;
                if (sscanf(buffer_cmd, "%ld %d", &offset, &length) != 2) {
                    write(STDERR_FILENO, "[WORKER] Invalid work command format\n", 38);
                    continue;
                }

                // Open the input file for the job
                int job_fd = open(input_file, O_RDONLY);
                if (job_fd == -1) {
                    write(STDERR_FILENO, "[WORKER] Failed to open input file for job\n", 44);
                    continue;
                }

                // Move the file pointer to the correct offset
                if (lseek(job_fd, offset, SEEK_SET) == (off_t)-1) {
                    write(STDERR_FILENO, "[WORKER] lseek failed\n", 23);
                    close(job_fd);
                    continue;
                }


                int total_count = 0;
                int to_read = length;
                char buffer[BUFFER_SIZE];
                ssize_t rfile;

                do {
                    int chunk = (to_read > BUFFER_SIZE) ? BUFFER_SIZE : to_read;
                    rfile = read(job_fd, buffer, chunk);
                    if (rfile == -1) {
                        perror("[WORKER] Problem reading characters\n");
                        close(job_fd);
                        return 1;
                    }
                    
                    buffer[rfile] = '\0'; // Null-terminate the buffer
                    total_count += buf_counter(buffer, rfile, search_char);
                    to_read -= rfile;
                    
                    if (rfile == 0) {
                        break; // End of file
                    }
                }
                while (to_read > 0 && rfile != 0);

                close(job_fd);
                
                // Simulate some processing time
                srand(time(NULL) ^ getpid()); // Seed randomness per worker
                sleep(rand() % 3 + 10); // Random sleep between 10 and 12 seconds
                
                // Send the result back to the dispatcher
                char result[64];
                int len = snprintf(result, sizeof(result), "%d\n", total_count);
                // Error handling
                if (len > 0 && len < (int)sizeof(result)) {
                    ssize_t written = write(STDOUT_FILENO, result, len);
                    if (written != len) {
                        write(STDERR_FILENO, "[WORKER] Failed to write full result\n", 36);
                    }
                } 
                else {
                    write(STDERR_FILENO, "[WORKER] Failed to format result\n", 33);
                }
            }
        }
        
    }
    
    return 0;
}



