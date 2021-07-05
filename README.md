# Three-phase commit (3PC) demo
An implementation of a [three-phase commit (3PC)](https://en.wikipedia.org/wiki/Three-phase_commit_protocol) protocol
for transaction management in a distributed system.

This program simulates a protocol run on a distributed system with a user-defined number of nodes.
It uses [OpenMPI](https://www.open-mpi.org) to provision such a system and serve as a medium of communication in this system.

You can force a certain node to crash by inputting its number (starting from 0) to STDIN and pressing ENTER.
If you don't force any node to crash, the protocol should proceed successfully, and the program should exit.

## Build prerequisites
    CMake 3.9 (it will probably compile using older versions too, see the last paragraph)
    C++17 compliant compiler
    OpenMPI

## Build instructions
```
git clone https://github.com/kwencel/3PC
cd 3PC
cmake .
make
```

## How to use
Invoke compiled executables by mpirun with at least 2 processes, for example:
```
mpirun -np 3 3PC | tee log && clear && echo "----- SORTED LOG -----" && cat log | sort
```
It will run the program and stream the logs as they are generated.
After the program completes however, it will display the log sorted by the messages
[lamport timestamp](https://en.wikipedia.org/wiki/Lamport_timestamp), which is crucial for analysing the program runtime.

## Older CMake version?
Try to change the minimum required version in CMakeLists.txt to match the version you have installed. There shouldn't be any issues.
