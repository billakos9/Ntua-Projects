#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/select.h>

#define MAX_CMD_LEN 256

int dispatcher_pid; // pid dispatcher
int command_pipe_fd; //= cmd_pipe[1] = fd for writing commands to the dispatcher


// Function to send commands to the dispatcher
void send_command(const char *cmd) {
    ssize_t bytes_written;
    bytes_written = write(command_pipe_fd, cmd, strlen(cmd));
    if (bytes_written == -1) {
        perror("[FRONTEND] Failed to send command");
    }
    // Send newline to indicate end of command
    write(command_pipe_fd, "\n", 1);
    //debug
    write(STDERR_FILENO, "[FRONTEND] Sent command: ", 25);
    write(STDERR_FILENO, cmd, strlen(cmd));
    write(STDERR_FILENO, "\n", 1);
}

// Function to read responses from the dispatcher
// This function reads from the response pipe and prints the response
void handle_response_pipe(int fd) {
    char buffer[256];
    ssize_t bytes;
    bytes = read(fd, buffer, sizeof(buffer) - 1);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("%s", buffer);
    }
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        perror("[FRONTEND] Wrong number of arguments");
        return 1;
    }

    if (argv[2][1] != '\0') {
        perror("[FRONTEND] Wrong character input at position 2");
        return 2;
    }

    const char *input_file = argv[1];
    const char *character = argv[2];

    // Create pipes for communication with the dispatcher
    int cmd_pipe[2]; //send commands to dispatcher
    int response_pipe[2]; // receive responses from dispatcher

    if (pipe(cmd_pipe) == -1 || pipe(response_pipe) == -1) {
        perror("[FRONTEND] Pipe creation failed");
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("[FRONTEND] Fork failed");
        return 1;
    }

    if (pid == 0) {
        // CHILD (Dispatcher)
        dup2(cmd_pipe[0], STDIN_FILENO); // Read commands from the command pipe
        dup2(response_pipe[1], STDOUT_FILENO); // Write responses to the response pipe
        // Close unused ends of pipes
        close(cmd_pipe[1]);
        close(response_pipe[0]);
        
        char response_fd_str[16];
        snprintf(response_fd_str, sizeof(response_fd_str), "%d", response_pipe[1]);

        execl("./dispatcher", "dispatcher", input_file, character, response_fd_str, NULL);
        perror("[FRONTEND] Exec dispatcher failed");
        exit(1);
    } 
    else {
        // PARENT (Frontend)
        dispatcher_pid = pid; // Store the dispatcher PID
        command_pipe_fd = cmd_pipe[1]; // Save the command pipe file descriptor
        // Close unused ends of pipes
        close(cmd_pipe[0]);
        close(response_pipe[1]);

        printf("\n[FRONTEND] Ready. Available commands: add, remove, status, progress, quit\n");
        
        char command[MAX_CMD_LEN];
        fd_set readfds;
        fprintf(stderr, "[FRONTEND] Entered main loop\n");

        while (1) {
            FD_ZERO(&readfds); //clean the set
            FD_SET(STDIN_FILENO, &readfds); // add stdin to the set
            FD_SET(response_pipe[0], &readfds); // add response pipe to the set


            // Check which file descriptor is the highest for select
            int maxfd;
            if (response_pipe[0] > STDIN_FILENO) {
                maxfd = response_pipe[0];
            } 
            else {
                maxfd = STDIN_FILENO;
            }


            // Wait for input on stdin or response pipe
            // The select function will block until one of the file descriptors is ready
            if (select(maxfd + 1, &readfds, NULL, NULL, NULL) == -1) {
                perror("[FRONTEND] Select failed");
                if (errno == EINTR) {
                    continue; // Restart select if interrupted by a signal
                } 
                else {
                    break; // Exit loop on other errors
                }
            }


            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                // Get input from stdin
                if (fgets(command, MAX_CMD_LEN, stdin) != NULL) {
                    command[strcspn(command, "\n")] = '\0';

                    // Quit sends SIGTERM to the dispatcher and exits
                    // Other commands are sent to the dispatcher
                    if (strcmp(command, "quit") == 0) {
                        kill(dispatcher_pid, SIGTERM);
                        break;
                    } 
                    else {
                        send_command(command);
                    }
                }
                else {
                    // fgets failed: just continue
                    clearerr(stdin); // Καθαρίζει το EOF ή error στο stdin
                    continue;
                }    
            }

            // Check if there is a response from the dispatcher and handle it
            if (FD_ISSET(response_pipe[0], &readfds)) {
                handle_response_pipe(response_pipe[0]);
            }
        }

        close(command_pipe_fd);
        close(response_pipe[0]);
        waitpid(dispatcher_pid, NULL, 0);

        printf("[FRONTEND] Exiting cleanly.\n");
    }

    return 0;
}

