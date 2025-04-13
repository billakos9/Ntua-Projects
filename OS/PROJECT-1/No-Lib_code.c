#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h> //read , write, close
#include <string.h>
#include <stdio.h>


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

    // Handle negative numbers
    if (num < 0) {
        is_negative = 1; //keep track of the sign
        num = -num;  // Make it positive for conversion
    }

    // Extract digits (stored in reverse order)
    do {
        buffer[i++] = '0' + (num % 10);  // Get last digit and convert to char
        num /= 10;  // Remove last digit
    } while (num > 0);

    // Add negative sign if needed
    if (is_negative) {
        buffer[i++] = '-';
    }

    // Reverse the buffer to get the correct order
    for (int j = 0; j < i / 2; j++) {
        char temp = buffer[j];
        buffer[j] = buffer[i - j - 1];
        buffer[i - j - 1] = temp;
    }

    // Null-terminate the string
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

    itoa_sys(total_count, total_count_str); // Convert total_count to string
    close(fd1); /* close the file for reading */
    
    size_t written = 0;

    /* write the result in the output file */
    ssize_t wfile0, wfile1, wfile2, wfile3, wfile4;
    do {
        char buffer0[] = "The character ";
        wfile0 = write (fd2, buffer0 + written, 14 - written);
        if (wfile0 == -1) {
            perror("Problem writing the result\n");
            close(fd2);
            return 1;
        }
        written += wfile0;
        if (written == 14) {
            break;
        }
    }
    while (wfile0 != 0);
    
    written = 0;
    size_t count = 1;
    do {
        wfile1 = write(fd2, &c2c, count);
        if (wfile1 == -1) {
            perror("Problem writing the result\n");
            close(fd2);
            return 1;
        }
        written += wfile1;
        if (written == 1) {
            break;
        }
    }
    while (wfile1 != 1);


    written = 0;
    do {
        char buffer0[] = " appears in output file ";
        wfile2 = write(fd2, buffer0 + written, 24 - written);
        if (wfile2 == -1) {
            perror("Problem writing the result\n");
            close(fd2);
            return 1;
        }
        written += wfile2;
        if (written == 24) {
            break;
        }
    }
    while (wfile2 != 0);

    written = 0;
    size_t len = strlen(total_count_str);
    do {
        wfile3 = write(fd2, total_count_str + written, len - written);
        if (wfile3 == -1) {
            perror("Problem writing the result\n");
            close(fd2);
            return 1;
        }
        written += wfile3;
    }
    while (written < len);

    written = 0;
    do {
        char buffer0[] = " times.\n";
        wfile4 = write(fd2, buffer0 + written , 6 - written);
        if (wfile4 == -1) {
            perror("Problem writing the result\n");
            close(fd2);
            return 1;
        }
        written += wfile4;
        if (written == 6) {
            break;
        }
    }
    while (wfile4 != 0);

    /* close the output file and finish */
    close(fd2);
    return 0;
}


