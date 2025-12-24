# Operating System Simulator
A C-based simulation of a distributed multi-node system. 
Features include: 
- Concurrency: Simulates independent nodes using POSIX threads with synchronized logical clocks.
- Scheduling: Implements Round Robin, Shortest Job First, and Priority-based preemptive scheduling algorithms.
- IPC: Handles inter-process communication via synchronous blocking SEND/RECV primitives.
- Synchronization: Utilizes custom monitors, barriers, and mutexes to manage critical sections and thread safety.

Built as a course project for CSCI 3120.\
Note: Some code files are built upon base code provided by instructors; this has been noted in the file comments wherever applicable.


