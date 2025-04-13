#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h> //read , write, close
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>

#define P 10

int active_children = 0;

// Signal handler Ctrl+C
void handle_sigint(int sig) {
    char msg[100];  // Buffer
    int len = snprintf(msg, sizeof(msg), "\n[%d] children still searching...\n", active_children); // Bytes printed is returned
    write(1, msg, len);
}

// Count the number of times a character appears in a buffer
int buf_counter(char *buffer, char c2c) {
    int count = 0;
    for (size_t i = 0; i < strlen(buffer); i++) {
        if (buffer[i] == c2c) {
            count++;
        }
    }
    return count;
}


int main(int argc, char *argv[]) {
    // Check the arguments
    if (argc != 3) {
        perror("wrong number of args\n");
        return 1;
    }
    if (argv[2][1] != '\0') {
        perror("Wrong character input");
        return 2;
    }

    int pipefd[2];
    pipe(pipefd);  // pipe: pipefd[0] for read, pipefd[1] for write
    signal(SIGINT, handle_sigint); // Ctrl+C -> Signal Handler Parent

    // Get file size
    struct stat st;
    int stat_result = stat(argv[1], &st);
    if (stat_result == -1) {
        perror("stat failed");
        return 1;
    }
    off_t filesize = st.st_size; // Get file size 
    off_t chunk = filesize / P; // Calculate chunk size = filesize / number of children



    for (int i = 0; i < P; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            return 1;
        }

        else if (pid == 0) {
            // CHILD
            close(pipefd[0]); // child doesn't read

            int fd1 = open(argv[1], O_RDONLY); //open the input file
            if (fd1 == -1) {
                perror("Problem opening input file");
                _exit(1);
            }

            off_t start = i * chunk; // Calculate start position for each child
            off_t end;
            // Last child takes the rest of the file
            if (i == P - 1) {  
                end = filesize;
            } 
            else {
                end = start + chunk;
            }
            off_t length = end - start; // Calculate length for each child
            if (length < 0) {
                perror("Invalid length");
                close(fd1);
                _exit(1);
            }

            // Move the file pointer to the start position
            off_t lseek_result = lseek(fd1, start, SEEK_SET);
            // Check for lseek errors 
            if (lseek_result == -1) {
                perror("lseek failed");
                close(fd1);
                _exit(1);
            }
            

            char buffer[1024];
            char c2c = argv[2][0]; /* Character to search for (second parameter in command line) */
            ssize_t rfile;
            ssize_t to_read = length; // Total bytes to read for this child
            int total_count = 0;

            do {
                ssize_t bytes_to_read;
                if (to_read > sizeof(buffer)) {
                    bytes_to_read = sizeof(buffer);
                } else {
                    bytes_to_read = to_read;
                }
                rfile = read(fd1, buffer, bytes_to_read); // Read from file 
                if (rfile == -1) {
                    perror("Problem reading characters\n");
                    close(fd1);
                    return 1;
                }
                buffer[rfile] = '\0';
                total_count += buf_counter(buffer, c2c); // Count the number of times the character appears in the buffer
                to_read -= rfile;
                if (rfile == 0) {
                    break;
                }
            }
            while (to_read > 0);

            close(fd1); // close the file for reading
            write(pipefd[1], &total_count, sizeof(int));  // Write total_count to pipe
            close(pipefd[1]);
            _exit(0);
        }

        else {
            // Parent keeps track of active children
            active_children++;
        }
    }

    // PARENT continues
    close(pipefd[1]);  // parent doesn't write

    printf("Waiting... press Ctrl+C to test signal handling\n");
    
    // Δίνει 10 δευτερόλεπτα περιθώριο για Ctrl+C, χωρίς να τερματίσει το πρόγραμμα
    // This is just to test the signal handler
    for (int i = 0; i < 10; i++) {
        sleep(1);
    }  
    
    int total = 0;
    int partial;
    for (int i = 0; i < P; i++) {
        read(pipefd[0], &partial, sizeof(int));  // Read total_count from pipe 
        total += partial;  // Accumulate the partial counts
        wait(NULL);
        active_children--;  // Decrease active children after wait
    }

    close(pipefd[0]);
    printf("✅ Total count from all children: %d\n", total);

    return 0;
}

