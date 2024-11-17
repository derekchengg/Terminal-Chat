# Terminal Chat

This is a basic chat application implemented in C using sockets and threads for communication between two ports on different Linux machines. The application allows users to exchange messages in a terminal-like interface.

## Usage

Compile the application using a C compiler:

```bash
make
```

### How to Run:

How to get name of machine:
```bash
hostname
```
```
./simpletalk [my port number] [remote machine name] [remote port number]
```
Example
```
./simpletalk 6060 csil-cpu3 6001
```
## Implementation Details
- The application uses UDP sockets for communication.
- Threads are utilized to handle keyboard input, sending/receiving messages, and displaying messages on the screen.
- Terminating the conversation can be signaled by entering `!` and hitting `enter`.
- The application handles errors related to socket creation, binding, and message sending/receiving.
  
## Code Structure
- `simpletalk.c` contains the main application code.
- `list.h` and `list.c` implements a simple linked list data structure used within the application for managing messages.
