/*
 * mandel.c
 *
 * A program to draw the Mandelbrot Set on a 256-color xterm.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/mman.h>

/*TODO header file for m(un)map*/

#include "mandel-lib.h"

#define MANDEL_MAX_ITERATION 100000

int NPROCS;

void sigint_handler(int sig) {
        reset_xterm_color(1);
        printf("\n");
        exit(1);
}

int safe_atoi(char *c, int *val) {
        long l;
        char *args;

        l = strtol(c, &args, 10);
        if(c!=args && *args=='\0') {
                *val = l;
                return 0;
        } else
                return -1;
}

sem_t *sem;

/***************************
 * Compile-time parameters *
 ***************************/

/*
 * Output at the terminal is is x_chars wide by y_chars long
*/
int y_chars = 50;
int x_chars = 90;

/*
 * The part of the complex plane to be drawn:
 * upper left corner is (xmin, ymax), lower right corner is (xmax, ymin)
*/
double xmin = -1.8, xmax = 1.0;
double ymin = -1.0, ymax = 1.0;

/*
 * Every character in the final output is
 * xstep x ystep units wide on the complex plane.
 */
double xstep;
double ystep;

/*
 * This function computes a line of output
 * as an array of x_char color values.
 */
void compute_mandel_line(int line, int color_val[])
{
        /*
         * x and y traverse the complex plane.
         */
        double x, y;

        int n;
        int val;

        /* Find out the y value corresponding to this line */
        y = ymax - ystep * line;

        /* and iterate for all points on this line */
        for (x = xmin, n = 0; n < x_chars; x+= xstep, n++) {

                /* Compute the point's color value */
                val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
                if (val > 255)
                        val = 255;

                /* And store it in the color_val[] array */
                val = xterm_color(val);
                color_val[n] = val;
        }
}

/*
 * This function outputs an array of x_char color values
 * to a 256-color xterm.
 */
void output_mandel_line(int fd, int color_val[])
{
        int i;

        char point ='@';
        char newline='\n';

        for (i = 0; i < x_chars; i++) {
                /* Set the current color, then output the point */
                set_xterm_color(fd, color_val[i]);
                if (write(fd, &point, 1) != 1) {
                        perror("compute_and_output_mandel_line: write point");
                        exit(1);
                }
        }

        /* Now that the line is done, output a newline character */
        if (write(fd, &newline, 1) != 1) {
                perror("compute_and_output_mandel_line: write newline");
                exit(1);
        }
}

/*Edited to get no. of process and compute line and wait for its turn*/
void compute_and_output_mandel_line(int i_PROC){

        int color_val[x_chars];

        for(int i = i_PROC; i<y_chars; i+=NPROCS) {
                compute_mandel_line(i, color_val);
                if(sem_wait(&sem[i_PROC])<0) {
                        perror("sem wait");
                        exit(1);
                }

                output_mandel_line(1, color_val);

                if(sem_post(&sem[(i_PROC+1)%NPROCS])<0) {
                        perror("sem post");
                        exit(1);
                }
        }

//        return NULL;
}

/*
 * Create a shared memory area, usable by all descendants of the calling
 * process.
 */
void *create_shared_memory_area(unsigned int numbytes)
{
        int pages;
        void *addr;

        if (numbytes == 0) {
                fprintf(stderr, "%s: internal error: called for numbytes == 0\n", __func__);
                exit(1);
        }

        /*
         * Determine the number of pages needed, round up the requested number of
         * pages
         */
        pages = (numbytes - 1) / sysconf(_SC_PAGE_SIZE) + 1;

        /* Create a shared, anonymous mapping for this number of pages */
        /* TODO:
                addr = mmap(...)
        */

        addr = mmap(NULL, pages * sysconf(_SC_PAGE_SIZE), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);

        if(addr == MAP_FAILED) {
                perror("mmap");
                exit(1);
        }
        return addr;
}

void destroy_shared_memory_area(void *addr, unsigned int numbytes) {
        int pages;

        if (numbytes == 0) {
                fprintf(stderr, "%s: internal error: called for numbytes == 0\n", __func__);
                exit(1);
        }

        /*
         * Determine the number of pages needed, round up the requested number of
         * pages
         */
        pages = (numbytes - 1) / sysconf(_SC_PAGE_SIZE) + 1;

        if (munmap(addr, pages * sysconf(_SC_PAGE_SIZE)) == -1) {
                perror("destroy_shared_memory_area: munmap failed");
                exit(1);
        }
}

int main(int argc, char *argv[]){


        xstep = (xmax - xmin) / x_chars;
        ystep = (ymax - ymin) / y_chars;


        /*Error handling*/
        if(argc!=2) {
                printf("Wrong number of arguments\n");
                return 1;
        }

        if(safe_atoi(argv[1], &NPROCS)<0) {
                printf("You must input number of processes\n");
                return 1;
        }

        if(NPROCS<0) {
                printf("Number of processes  needs to be a positive number\n");
                return 1;
        }

        if(signal(SIGINT, sigint_handler) == SIG_ERR) {
                perror("signal");
                exit(1);
        }

        /*Shared memory to hold semaphores*/
        sem = create_shared_memory_area(NPROCS * sizeof(sem_t));

        // Initialize the first semaphore to 1 (to allow the first process to start)
        if(sem_init(&sem[0], 1, 1)<0) {
                perror("sem init");
                exit(1);
        }
        
        // Initialize semaphores except for the first one
        for(int j=1; j<NPROCS; j++) {
                if(sem_init(&sem[j], 1, 0)<0) {
                        perror("sem init");
                        exit(1);
                }
        }

        pid_t p[NPROCS];

        for(int i=0; i<NPROCS; i++) {
                p[i] = fork();

                if(p[i]<0) {
                        perror("fork failed");
                        exit(1);
                } else if(p[i]==0) {
                        /* Child process */
                        compute_and_output_mandel_line(i);
                        exit(0);
                }
        }

        /* Parent process */
        for(int i=0; i<NPROCS; i++)
                wait(NULL);

        // Cleanup semaphores
        for(int i=0; i<NPROCS; i++) {
                if(sem_destroy(&sem[i])<0) {
                        perror("sem destroy");
                        exit(1);
                }
        }

        // Destroy shared memory area for semaphores
        destroy_shared_memory_area(sem, NPROCS * sizeof(sem_t));

        reset_xterm_color(1);
        return 0;
}