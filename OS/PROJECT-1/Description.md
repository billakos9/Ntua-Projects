- 1.1: Implement a program with exactly the same functionality, but using only system calls, without using the C standard library.

- 1.2:
  a) Create a child process that greets the world, mentioning its PID and its parent's PID.
           The parent prints the child's PID and waits for it to terminate.

  b) Define a variable x in the parent before creating the child.
  Assign a different value to x in the parent and the child, then print it.
  
  c) Assign the child process to search for the character in the file (but not handle the full file operations).

  d) Create a child process that executes the code you wrote in Question 1.1.

- 1.3:
  Extend the program from Question 2 to create P child processes (with P defined as a constant in your code).
  Each child will search for the character in the file in parallel, and the parent will collect and print the total result.
  When the program receives Control+C (SIGINT), instead of terminating, it should print the total number of processes currently searching the file.

  ## - 1.4:
  Implement a complete application for parallel character counting in a file.
  The application should receive from the user: the filename, and the character to search for.
  During execution, the user should be able to issue commands to:

  - Add or remove worker processes (search workers), Display information about the active workers,
    Show progress updates (percentage of completion and the number of characters found so far).

  The application must be modular and organized into three independent components:

  - Front-end:
  A process that communicates with the user, handles commands directly, or forwards them to the dispatcher.

  - Dispatcher:
  A process responsible for distributing the workload among workers.
  It receives commands from the front-end (e.g., to add or remove workers), assigns file portions to workers, collects partial results, and returns the total         result to the front-end.

  - Workers:
  There should be a dynamic number of worker processes.
  Each worker repeatedly receives a portion of the file (a starting byte position and a number of bytes to search) and returns the result to the dispatcher.
