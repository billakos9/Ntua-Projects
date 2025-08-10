/*
 * mmap.c
 *
 * Examining the virtual memory of processes.
 *
 * Operating Systems course, CSLab, ECE, NTUA
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <signal.h>
#include <sys/wait.h>

#include "help.h"

#define RED     "\033[31m"
#define RESET   "\033[0m"

char *heap_private_buf;
char *heap_shared_buf;

char *file_shared_buf;

uint64_t buffer_size;

size_t file_size = 0;


/*
* Child process' entry point.
*/
void child(void)
{
    uint64_t pa;

    /*
    * Step 7 - Child
    */
    if (0 != raise(SIGSTOP))
        die("raise(SIGSTOP)");
    /*
    * TODO: Write your code here to complete child's part of Step 7.
    */

    printf("[Child %d] VM map:\n", getpid());
    show_maps();

    /*
    * Step 8 - Child
    */

    if (0 != raise(SIGSTOP))
        die("raise(SIGSTOP)");
    /*
    * TODO: Write your code here to complete child's part of Step 8.
    */

    uint64_t pa_child = get_physical_address((uint64_t)heap_private_buf);

    printf("\n[Child  %d] Step 8: physical address of private buffer\n", getpid());
    if (pa_child)
        printf("  Child  PA: 0x%lx\n", pa_child);
    else
        printf("  Child  PA: (no physical page!)\n");


    /*
    * Step 9 - Child
    */
    if (0 != raise(SIGSTOP))
        die("raise(SIGSTOP)");
    /*
    * TODO: Write your code here to complete child's part of Step 9.
    */

    heap_private_buf[0] = 'X';
    uint64_t pa_child_after =
        get_physical_address((uint64_t)heap_private_buf);

    printf("\n[Child %d] Step 9: wrote to private buffer\n", getpid());
    printf("    New Child  PA: 0x%lx\n", pa_child_after);


    /*
    * Step 10 - Child
    */
    if (0 != raise(SIGSTOP))
        die("raise(SIGSTOP)");
    /*
    * TODO: Write your code here to complete child's part of Step 10.
    */

    heap_shared_buf[0] = 'S';

    /* Βρίσκουμε τη φυσική διεύθυνση μετά το write                */
    uint64_t pa_child_shared =
        get_physical_address((uint64_t)heap_shared_buf);

    printf("\n[Child  %d] Step 10: wrote to shared buffer\n", getpid());
    printf("    Child  shared PA: 0x%lx\n", pa_child_shared);


    /*
    * Step 11 - Child
    */
    if (0 != raise(SIGSTOP))
        die("raise(SIGSTOP)");
    /*
    * TODO: Write your code here to complete child's part of Step 11.
    */

    if (mprotect(heap_shared_buf, buffer_size, PROT_READ) != 0)
        die("mprotect (child, read-only)");

    printf("\n[Child  %d] Step 11: set shared buffer to read-only\n", getpid());
    show_maps();                  /* θα φανεί ως r--s             */


    /*
    * Step 12 - Child
    */
   
    /*
    * TODO: Write your code here to complete child's part of Step 12.
    */

    printf("\n[Child  %d] Step 12: unmap all buffers\n", getpid());
 
    /* 1. private buffer */
    if (heap_private_buf && heap_private_buf != MAP_FAILED) {
         if (munmap(heap_private_buf, buffer_size) == -1)
                 perror("munmap heap_private_buf");
         heap_private_buf = NULL;
    }
 
    /* 2. shared buffer */
    if (heap_shared_buf && heap_shared_buf != MAP_FAILED) {
         if (munmap(heap_shared_buf, buffer_size) == -1)
                 perror("munmap heap_shared_buf");
         heap_shared_buf = NULL;
    }
 
    /* 3. file-mapped buffer */           
    if (file_shared_buf && file_shared_buf != MAP_FAILED) {
         if (munmap(file_shared_buf, file_size) == -1)
                 perror("munmap file_shared_buf");
         file_shared_buf = NULL;
    }
 
    /* Τυπώνουμε χάρτη μνήμης για να επαληθεύσουμε ότι έφυγαν */
    show_maps();
}

/*
* Parent process' entry point.
*/
void parent(pid_t child_pid) {
    uint64_t pa;
    int status;

    /* Wait for the child to raise its first SIGSTOP. */
    if (-1 == waitpid(child_pid, &status, WUNTRACED))
        die("waitpid");

    /*
    * Step 7: Print parent's and child's maps. What do you see?
    * Step 7 - Parent
    */
    printf(RED "\nStep 7: Print parent's and child's map.\n" RESET);
    press_enter();

    /*
    * TODO: Write your code here to complete parent's part of Step 7.
    */

    // Print parent's map
    printf("\n[Parent %d] VM map:\n", getpid());
    show_maps();

    if (-1 == kill(child_pid, SIGCONT))
        die("kill");
    if (-1 == waitpid(child_pid, &status, WUNTRACED))
        die("waitpid");


    /*
    * Step 8: Get the physical memory address for heap_private_buf.
    * Step 8 - Parent
    */
    printf(RED "\nStep 8: Find the physical address of the private heap "
        "buffer (main) for both the parent and the child.\n" RESET);
    press_enter();

    /*
    * TODO: Write your code here to complete parent's part of Step 8.
    */

    printf("\n[Parent %d] Step 8: physical address of private buffer\n", getpid());

    uint64_t pa_parent = get_physical_address((uint64_t)heap_private_buf);
    if (pa_parent)
        printf("  Parent PA: 0x%lx\n", pa_parent);
    else
        printf("  Parent PA:  (no physical page!)\n");

        
    if (-1 == kill(child_pid, SIGCONT))
        die("kill");
    if (-1 == waitpid(child_pid, &status, WUNTRACED))
        die("waitpid");


    /*
    * Step 9: Write to heap_private_buf. What happened?
    * Step 9 - Parent
    */
    printf(RED "\nStep 9: Write to the private buffer from the child and "
        "repeat step 8. What happened?\n" RESET);
    press_enter();

    /*
    * TODO: Write your code here to complete parent's part of Step 9.
    */

    if (-1 == kill(child_pid, SIGCONT))
        die("kill");
    if (-1 == waitpid(child_pid, &status, WUNTRACED))
        die("waitpid");
    
    
    printf("\n[Parent %d] Step 9: after child wrote to private buffer\n",
        getpid());
 
    uint64_t pa_parent_after = get_physical_address((uint64_t)heap_private_buf);
 
    printf("    Parent PA: 0x%lx\n", pa_parent_after);

    


    /*
    * Step 10: Get the physical memory address for heap_shared_buf.
    * Step 10 - Parent
    */
    printf(RED "\nStep 10: Write to the shared heap buffer (main) from "
        "child and get the physical address for both the parent and "
        "the child. What happened?\n" RESET);
    press_enter();

    /*
    * TODO: Write your code here to complete parent's part of Step 10.
    */

    if (-1 == kill(child_pid, SIGCONT))
        die("kill");
    if (-1 == waitpid(child_pid, &status, WUNTRACED))
        die("waitpid");

    uint64_t pa_parent_shared =
        get_physical_address((uint64_t)heap_shared_buf);

    printf("    Parent shared PA: 0x%lx\n", pa_parent_shared);

    /* ο γονιός «βλέπει» το γράψιμο   */
    printf("    Parent reads first byte: %c\n", heap_shared_buf[0]);


    /*
    * Step 11: Disable writing on the shared buffer for the child
    * (hint: mprotect(2)).
    * Step 11 - Parent
    */
    printf(RED "\nStep 11: Disable writing on the shared buffer for the "
        "child. Verify through the maps for the parent and the "
        "child.\n" RESET);
    press_enter();

    /*
    * TODO: Write your code here to complete parent's part of Step 11.
    */
        
    if (-1 == kill(child_pid, SIGCONT))
        die("kill");
    if (-1 == waitpid(child_pid, &status, 0))
        die("waitpid");


    printf("[Parent] VM map after child mprotect():\n");
    show_maps();


    /*
    * Step 12: Free all buffers for parent and child.
    * Step 12 - Parent
    */
    printf(RED "\nStep 12: Free all buffers for parent and child." RESET);
    press_enter();

    /*
    * TODO: Write your code here to complete parent's part of Step 12.
    */

    printf("\n[Parent %d] Step 12: unmap all buffers\n", getpid());
 
    /* 1. private buffer */
    if (heap_private_buf && heap_private_buf != MAP_FAILED) {
         if (munmap(heap_private_buf, buffer_size) == -1)
                 perror("munmap heap_private_buf");
         heap_private_buf = NULL;
    }
 
    /* 2. shared buffer */
    if (heap_shared_buf && heap_shared_buf != MAP_FAILED) {
         if (munmap(heap_shared_buf, buffer_size) == -1)
                 perror("munmap heap_shared_buf");
         heap_shared_buf = NULL;
    }
 
    /* 3. file-mapped buffer */           
    if (file_shared_buf && file_shared_buf != MAP_FAILED) {
         if (munmap(file_shared_buf, file_size) == -1)
                 perror("munmap file_shared_buf");
         file_shared_buf = NULL;
    }
 
    /* Τυπώνουμε χάρτη μνήμης για να επαληθεύσουμε ότι έφυγαν */
    show_maps();
}


int main(void) {
    pid_t mypid, p;
    int fd = -1;
    uint64_t pa;

    mypid = getpid();
    buffer_size = 1 * get_page_size();

    /*
    * Step 1: Print the virtual address space layout of this process.
    */
    printf(RED "\nStep 1: Print the virtual address space map of this "
        "process [%d].\n" RESET, mypid);
    press_enter();
    /*
    * TODO: Write your code here to complete Step 1.
    */

    show_maps();

    /*
    * Step 2: Use mmap to allocate a buffer of 1 page and print the map
    * again. Store buffer in heap_private_buf.
    */
    printf(RED "\nStep 2: Use mmap(2) to allocate a private buffer of "
        "size equal to 1 page and print the VM map again.\n" RESET);
    press_enter();
    /*
     * TODO: Write your code here to complete Step 2.
    */
    heap_private_buf = mmap(NULL,                 /* αφήνουμε το kernel να επιλέξει VA */
        buffer_size,          /* ακριβώς 1 page                 */
        PROT_READ | PROT_WRITE,/* δικαιώματα R/W               */
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,                   /* fd = -1 για ανώνυμο mapping    */
        0);                   /* offset = 0                     */

    if (heap_private_buf == MAP_FAILED)
    die("mmap (private buffer)");

    printf("Private buffer allocated at VA %p (size: %lu B)\n",
    heap_private_buf, buffer_size);

    /*  Τυπώνουμε ξανά τον χάρτη εικονικής μνήμης για να δούμε τη νέα VMA     */
    show_maps();

    /*
    * Step 3: Find the physical address of the first page of your buffer
    * in main memory. What do you see?
    */
    printf(RED "\nStep 3: Find and print the physical address of the "
        "buffer in main memory. What do you see?\n" RESET);
    press_enter();

    /*
    * TODO: Write your code here to complete Step 3.
    */

    pa = get_physical_address((uint64_t)heap_private_buf);
    if (pa) {
        printf("Physical address: 0x%lx\n", pa);
    }



    /*
    * Step 4: Write zeros to the buffer and repeat Step 3.
    */
    printf(RED "\nStep 4: Initialize your buffer with zeros and repeat "
        "Step 3. What happened?\n" RESET);

    press_enter();
    /*
     * TODO: Write your code here to complete Step 4.
     */

    memset(heap_private_buf, 0, buffer_size);

    pa = get_physical_address((uint64_t)heap_private_buf);
    if (pa)
        printf("After memset → Physical address: 0x%lx\n", pa);
    else
        printf("Still no physical page!\n");


    /*
     * Step 5: Use mmap(2) to map file.txt (memory-mapped files) and print
     * its content. Use file_shared_buf.
     */
    printf(RED "\nStep 5: Use mmap(2) to read and print file.txt. Print "
            "the new mapping information that has been created.\n" RESET);
    press_enter();
    /*
     * TODO: Write your code here to complete Step 5.
    */
   

    fd = open("file.txt", O_RDONLY);
    if (fd < 0)
        die("open(\"file.txt\")");

    /* Πάρε το μέγεθος του αρχείου */
    struct stat st;
    if (fstat(fd, &st) < 0)
        die("fstat(\"file.txt\")");
    file_size = st.st_size;

    /* Κάνε mmap ολόκληρου του αρχείου ως read-only, private */
    file_shared_buf = mmap(NULL,
                        file_size,
                        PROT_READ,
                        MAP_PRIVATE,
                        fd,
                        0);
    if (file_shared_buf == MAP_FAILED)
        die("mmap(file.txt)");

    /* Εκτύπωση περιεχομένου αρχείου */
    printf("Contents of file.txt (size: %zu bytes):\n", file_size);
    fwrite(file_shared_buf, 1, file_size, stdout);
    printf("\n");

    /* Εκτύπωση νέας κατανομής μνήμης */
    show_maps();

    /* Κλείσιμο του file descriptor */
    if (close(fd) < 0)
        perror("close(\"file.txt\")");
    fd = -1;  /* Reset fd to avoid closing it again later */


    /* Step 6: Use mmap(2) to allocate a shared buffer of 1 page. Use
    * heap_shared_buf.
    */
    printf(RED "\nStep 6: Use mmap(2) to allocate a shared buffer of size "
            "equal to 1 page. Initialize the buffer and print the new "
            "mapping information that has been created.\n" RESET);
    press_enter();
    /*
    * TODO: Write your code here to complete Step 6.
    */

    /* 1. Κλήση mmap –  MAP_SHARED | MAP_ANONYMOUS          */
    heap_shared_buf = mmap(NULL,     /* VA επιλογή kernel   */
            buffer_size,             /* 1 page              */
            PROT_READ | PROT_WRITE,  /* R/W                 */
            MAP_SHARED | MAP_ANONYMOUS,
            -1,                      /* fd = -1 (anonymous) */
            0);                      /* offset              */

    if (heap_shared_buf == MAP_FAILED)
        die("mmap (shared buffer)");

    printf("Shared buffer allocated at VA %p (size: %lu B)\n",
    heap_shared_buf, buffer_size);

    /* 2. Αρχικοποιούμε με κάτι (π.χ. μηδενικά)               */
    memset(heap_shared_buf, 0, buffer_size);

    /* 3. Τυπώνουμε ξανά τον χάρτη   */
    show_maps();


    p = fork();
    if (p < 0)
        die("fork");
    if (p == 0) {
        child();
        return 0;
    }

    parent(p);

    if (fd >= 0) {
        if (-1 == close(fd))
            perror("close");
    }
    return 0;
}


    

