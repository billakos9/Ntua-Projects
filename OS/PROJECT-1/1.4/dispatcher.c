#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/select.h>

#define MAX_WORKERS 100
#define MAX_WORK_POOL 10000
#define CHUNK_SIZE 4096

// Worker structure, PID, FD, alive status
typedef struct {
    pid_t pid;
    int to_worker_fd;
    int from_worker_fd;
    int alive;
    int assigned_chunks; // Track the work load of each worker
} Worker;

// Work structure, offset, length, assigned status, done status, assigned worker
typedef struct {
    long offset;
    int length;
    int assigned;
    int done;
    int assigned_worker;
} Work;

//Global variables
// Worker array, work pool, total file size, total characters found, processed bytes
Worker workers[MAX_WORKERS];
int worker_count = 0;
Work work_pool[MAX_WORK_POOL];
int work_count = 0;

off_t total_file_size = 0;
off_t processed_bytes = 0;
int total_characters_found = 0;
const char *input_file;
const char *character;
int response_fd = -1; // για να γράφουμε την απάντηση του dispatcher

// Function Prototypes
void spawn_worker(); // Δημιουργεί έναν worker
void remove_worker(); // Αφαιρεί έναν worker
void assign_work(); // Αναθέτει δουλειά στους workers
void collect_one_result(int i); // Συλλέγει το αποτέλεσμα από έναν worker
void check_dead_workers(); // Ελέγχει αν υπάρχουν νεκροί workers
void show_pstree(pid_t p); // Εμφανίζει το δέντρο διεργασιών του dispatcher
void make_nonblocking(int fd); // Κάνει ένα file descriptor μη μπλοκαρισμένο
void handle_sigterm(int sig); // Χειριστής σήματος για SIGTERM
void handle_sigusr1(int sig); // Χειριστής σήματος για SIGUSR1 - Progress
void create_work_pool(); // Δημιουργεί το work pool
void spawn_worker_at(int index); // Δημιουργεί έναν worker σε συγκεκριμένο index

// Function to show the process tree of the dispatcher
void show_pstree(pid_t p) {
    int ret;
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "echo; echo; pstree -a -G -c -p %ld; echo; echo", (long)p);
    cmd[sizeof(cmd)-1] = '\0';
    ret = system(cmd);
    if (ret < 0) {
        perror("system");
        exit(104);
    }

    fprintf(stderr, "=== Worker assignments ===\n");
    for (int i = 0; i < worker_count; i++) {
        if (workers[i].alive) {
            fprintf(stderr, "[WORKER %d] PID %d, assigned_chunks = %d\n", i, workers[i].pid, workers[i].assigned_chunks);
            for (int j = 0; j < work_count; j++) {
                if (work_pool[j].assigned_worker == i && work_pool[j].done == 0) {
                    fprintf(stderr, "    - Chunk offset: %ld, length: %d\n",
                        work_pool[j].offset, work_pool[j].length);
                }
            }
        }
    }
    fprintf(stderr, "===========================\n");
}

// Quit handler
void handle_sigterm(int sig) {
    (void)sig;  // Για να μην πετάει warning unused
    for (int i = 0; i < worker_count; i++) {
        if (workers[i].alive) {
            kill(workers[i].pid, SIGTERM);
            waitpid(workers[i].pid, NULL, 0);
        }
    }
    exit(0);
}

// Signal handler for SIGUSR1 
// It sends the progress and characters found to the response_fd
void handle_sigusr1(int sig) {
    (void)sig;
    char buffer[256];
    int len = snprintf(buffer, sizeof(buffer),"[DISPATCHER] Progress: %.2f%%, Characters found so far: %d\n", (processed_bytes * 100.0) / total_file_size, total_characters_found);
    if (response_fd != -1) {
        if (write(response_fd, buffer, len) == -1) {
            perror("[DISPATCHER] Failed to write progress");
        }
    }
}

// Function to make a file descriptor non-blocking
// This is used to prevent blocking on read/write operations
void make_nonblocking(int fd) {
    // Get the current flags of the file descriptor
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("[DISPATCHER] fcntl get failed");
        exit(1);
    }
    // Add O_NONBLOCK flag to the file descriptor
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("[DISPATCHER] fcntl set failed");
        exit(1);
    }
}

// Function to spawn a worker process
// It creates pipes for communication and forks a new process
// The child process executes the worker program
void spawn_worker_at(int index) {
    int to_worker[2];
    int from_worker[2];

    if (pipe(to_worker) == -1 || pipe(from_worker) == -1) {
        perror("pipe creation failed");
        exit(1);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0) {
        dup2(to_worker[0], STDIN_FILENO);
        dup2(from_worker[1], STDOUT_FILENO);
        close(to_worker[1]);
        close(from_worker[0]);
        execl("./worker", "worker", input_file, character, NULL);
        perror("exec worker failed");
        exit(1);
    } 
    else {
        close(to_worker[0]);
        close(from_worker[1]);
        workers[index].pid = pid;
        // Pipes for communication
        workers[index].to_worker_fd = to_worker[1];
        workers[index].from_worker_fd = from_worker[0];
        workers[index].alive = 1; // Active worker
        workers[index].assigned_chunks = 0; // Initialize assigned chunks
        make_nonblocking(workers[index].from_worker_fd);
        fprintf(stderr, "[DISPATCHER] New worker spawned (PID: %d)\n", pid);
    }
}

// Function to spawn a worker process
void spawn_worker() {
    if (worker_count >= MAX_WORKERS) {
        printf("[DISPATCHER] Maximum number of workers reached\n");
        return;
    }
    spawn_worker_at(worker_count); // Also writes feedback
    worker_count++;
}

// Function to remove a worker process
void remove_worker() {
    if (worker_count > 0) {
        int i = worker_count - 1; // Remove the last worker
        pid_t pid = workers[i].pid;

        // Check if the worker has a result
        fd_set set;
        FD_ZERO(&set);
        FD_SET(workers[i].from_worker_fd, &set);
        struct timeval timeout = {0, 0}; // No waiting
        if (select(workers[i].from_worker_fd + 1, &set, NULL, NULL, &timeout) > 0) {
            if (FD_ISSET(workers[i].from_worker_fd, &set)) {
                collect_one_result(i);
            }
        }
        
        //kill
        kill(pid, SIGTERM);
        waitpid(pid, NULL, 0);
        workers[i].alive = 0;

        // Make available the work assigned to this worker
        for (int j = 0; j < work_count; j++) {
            if (work_pool[j].assigned_worker == i && work_pool[j].done == 0) {
                work_pool[j].assigned = 0;
                work_pool[j].assigned_worker = -1;
            }
        }

        worker_count--;
        fprintf(stderr, "[DISPATCHER] Worker (PID: %d) removed\n", pid);

        // Reassign leftover work
        assign_work();
    } 
    else {
        fprintf(stderr, "[DISPATCHER] No workers to remove\n");
    }
}

// Function to create the work pool
// It divides the total file size into chunks and assigns them to the work pool
void create_work_pool() {
    for (off_t offset = 0; offset < total_file_size; offset += CHUNK_SIZE) {
        if (offset + CHUNK_SIZE > total_file_size) {
            work_pool[work_count].offset = offset;
            work_pool[work_count].length = total_file_size - offset;
            work_pool[work_count].assigned = 0;
            work_pool[work_count].done = 0;
            work_pool[work_count].assigned_worker = -1;
            work_count++;
        } 
        else {
            work_pool[work_count].offset = offset;
            work_pool[work_count].length = CHUNK_SIZE;
            work_pool[work_count].assigned = 0;
            work_pool[work_count].done = 0;
            work_pool[work_count].assigned_worker = -1;
            work_count++;
        }
        
    }
}

// Function to assign work to workers
// It checks for unassigned work and assigns it to available workers
void assign_work() {
    for (int j = 0; j < worker_count; j++) {
        if (!workers[j].alive) continue; // Only alive workers

        if (workers[j].assigned_chunks == 0) { // Free worker
            // Find next unassigned work
            for (int i = 0; i < work_count; i++) {
                if (work_pool[i].assigned == 0 && work_pool[i].done == 0) {
                    // Assign this work to the free worker
                    char msg[128];
                    snprintf(msg, sizeof(msg), "%ld %d\n", work_pool[i].offset, work_pool[i].length);
                    write(workers[j].to_worker_fd, msg, strlen(msg));
                    
                    work_pool[i].assigned = 1;
                    work_pool[i].assigned_worker = j;
                    workers[j].assigned_chunks++;

                    break; // assign one job at a time to each free worker
                }
            }
        }
    }
}


// Function to collect results from a specific worker
void collect_one_result(int i) {
    char buffer[128];
    ssize_t n = read(workers[i].from_worker_fd, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        // Check if the last character is a newline
        // This indicates that I collect only full results
        if (buffer[n-1] == '\n') {  
            int found = atoi(buffer);
            total_characters_found += found;

            for (int j = 0; j < work_count; j++) {
                if (work_pool[j].assigned_worker == i && work_pool[j].done == 0) {
                    work_pool[j].done = 1;
                    processed_bytes += work_pool[j].length;
                    workers[i].assigned_chunks--;
                    break;
                }
            }
        }
    }
    else if (n == 0) {
        workers[i].alive = 0; 
    }
    else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("[DISPATCHER] Error reading from worker");
            workers[i].alive = 0;
        }
    }
}

// Function to check for dead workers
// It waits for any dead workers and restarts them
void check_dead_workers() {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < worker_count; i++) {
            if (workers[i].pid == pid) {
                workers[i].alive = 0;
                printf("[DISPATCHER] Worker (PID: %d) died, restarting...\n", pid);
                spawn_worker_at(i);
                // Make available the work assigned to this worker
                for (int j = 0; j < work_count; j++) {
                    if (work_pool[j].assigned_worker == i && work_pool[j].done == 0) {
                        work_pool[j].assigned = 0;
                        work_pool[j].assigned_worker = -1;
                    }
                }
                break;
            }
        }
    }
}


int main(int argc, char *argv[]) {
    
    input_file = argv[1];
    character = argv[2];
    response_fd = atoi(argv[3]);

    signal(SIGTERM, handle_sigterm);
    signal(SIGUSR1, handle_sigusr1);

    int fd = open(input_file, O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        exit(1);
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat failed");
        close(fd);
        exit(1);
    }
    close(fd); // Κλείνουμε το αρχείο αφού πάρουμε το μέγεθος
    total_file_size = st.st_size;

    // --- Δημιουργία του work pool ---
    create_work_pool();

    char command[256];
    fd_set readfds;
    fprintf(stderr, "[DISPATCHER] Waiting for command...\n");


    // --- Main loop ---
    // Εδώ περιμένουμε εντολές από το frontend
    while (1) {
        // 1. Βήμα: δώσε δουλειά σε ελεύθερους workers
        assign_work();

        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        int maxfd = STDIN_FILENO;
    
        for (int i = 0; i < worker_count; i++) {
            if (workers[i].alive) {
                FD_SET(workers[i].from_worker_fd, &readfds);
                if (workers[i].from_worker_fd > maxfd) {
                    maxfd = workers[i].from_worker_fd;
                }
            }
        }
    
        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) == -1) {
            perror("select failed");
            continue;
        }
    
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            // Διαβάζεις εντολή
            if (fgets(command, sizeof(command), stdin)) {
                command[strcspn(command, "\n")] = '\0'; // Remove newline character
                // Εδώ ελέγχεις την εντολή
                if (strcmp(command, "add") == 0) spawn_worker();
                else if (strcmp(command, "remove") == 0) remove_worker();
                else if (strcmp(command, "status") == 0) show_pstree(getpid());
                else if (strcmp(command, "progress") == 0) kill(getpid(), SIGUSR1);
                else if (strcmp(command, "quit") == 0) handle_sigterm(SIGTERM);
                else fprintf(stderr, "[DISPATCHER] Unknown command\n");
            }
        }
    
        for (int i = 0; i < worker_count; i++) {
            if (workers[i].alive && FD_ISSET(workers[i].from_worker_fd, &readfds)) {
                // collect from that worker!
                collect_one_result(i);
                // reassign work!
                assign_work();
            }
        }
        assign_work();
        check_dead_workers();
    }

    // Καθάρισμα - Κλείσιμο Dispatcher 
    printf("[DISPATCHER] Shutting down...\n");
    for (int i = 0; i < worker_count; i++) {
        if (workers[i].alive) {
            kill(workers[i].pid, SIGTERM);
            waitpid(workers[i].pid, NULL, 0);
        }
    }

    return 0;
}





