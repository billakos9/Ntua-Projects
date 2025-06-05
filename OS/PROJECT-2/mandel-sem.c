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
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

#include "mandel-lib.h"

#define MANDEL_MAX_ITERATION 100000
#define NTHREADS 3


/* Signal handler for cleaning up before exit */
void cleanup_handler(int signum) {
    /* Reset terminal colors */
    reset_xterm_color(1);
    
    /* Output a newline for clean display */
    write(1, "\n", 1);
    
    /* Exit with appropriate status */
    exit(128 + signum);
}

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

/* Thread synchronization */
sem_t output_sem;  // Semaphore for output synchronization
sem_t *line_sems;  // Array of semaphores for line ordering

/* Thread argument structure */
struct thread_args {
    int thread_id;
    int fd;
};

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

void compute_and_output_mandel_line(int fd, int line)
{
    /*
     * A temporary array, used to hold color values for the line being drawn
     */
    int color_val[x_chars];

    compute_mandel_line(line, color_val);
    
    /* Wait for our turn to output */
    sem_wait(&line_sems[line]);
    
    /* Protect the output with a semaphore */
    sem_wait(&output_sem);
    output_mandel_line(fd, color_val);
    sem_post(&output_sem);
    
    /* Signal the next line can be output */
    if (line + 1 < y_chars) {
        sem_post(&line_sems[line + 1]);
    }
}

/* Thread function */
void *thread_function(void *arg)
{
    struct thread_args *args = (struct thread_args *)arg;
    int thread_id = args->thread_id;
    int fd = args->fd;
    
    /* Each thread handles rows: thread_id, thread_id + NTHREADS, thread_id + 2*NTHREADS, ... */
    for (int line = thread_id; line < y_chars; line += NTHREADS) {
        compute_and_output_mandel_line(fd, line);
    }
    
    return NULL;
}

int main(void)
{
        /* Set up signal handler for cleanup */
        signal(SIGINT, cleanup_handler);
        signal(SIGTERM, cleanup_handler);
        signal(SIGHUP, cleanup_handler);

        pthread_t threads[NTHREADS];
        struct thread_args thread_args[NTHREADS];
        int i;

        xstep = (xmax - xmin) / x_chars;
        ystep = (ymax - ymin) / y_chars;

        /* Initialize semaphores */
        sem_init(&output_sem, 0, 1);  // Binary semaphore for output protection
        
        /* Initialize line ordering semaphores */
        line_sems = malloc(y_chars * sizeof(sem_t));
        if (line_sems == NULL) {
            perror("malloc");
            exit(1);
        }
        
        /* Initialize all line semaphores to 0 except the first one */
        for (i = 0; i < y_chars; i++) {
            sem_init(&line_sems[i], 0, i == 0 ? 1 : 0);
        }

        /* Create threads */
        for (i = 0; i < NTHREADS; i++) {
            thread_args[i].thread_id = i;
            thread_args[i].fd = 1;  // stdout
            if (pthread_create(&threads[i], NULL, thread_function, &thread_args[i]) != 0) {
                perror("pthread_create");
                exit(1);
            }
        }

        /* Wait for all threads to complete */
        for (i = 0; i < NTHREADS; i++) {
            if (pthread_join(threads[i], NULL) != 0) {
                perror("pthread_join");
                exit(1);
            }
        }

        /* Clean up semaphores */
        sem_destroy(&output_sem);
        for (i = 0; i < y_chars; i++) {
            sem_destroy(&line_sems[i]);
        }
        free(line_sems);

        reset_xterm_color(1);
        return 0;
}