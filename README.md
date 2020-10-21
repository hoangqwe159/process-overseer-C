# process-overseer-C
Two programs are required: a server program “overseer” and a client program “controller”, networked via BSD sockets (as covered in week 7). The
overseer runs indefinitely, processing commands sent by controller clients. The
controller only runs for an instant at a time; it is executed with varying arguments to issue commands to the overseer, then terminates.
The usage of the overeseer is shown below.
overseer <port>
The usage of the controller is shown below.
controller <address> <port> {[-o out_file] [-log log_file] [-t seconds]
<file> [arg...] | mem [pid] | memkill <percent>}
  • < > angle brackets indicate required arguments.
  • [ ] brackets indicate optional arguments.
  • ... ellipses indicate an arbitrary quantity of arguments.
  • { } braces indicate required, mutually exclusive options, separated by
    pipes |. That is, one and only one of the following must be chosen:
    – [-o out_file] [-log log_file] [-t seconds] <file> [arg...]
    – mem [pid]
    – memkill <percent>

Demo videos: 
  - Part A: https://youtu.be/ObVm0jOU1BM
  - Part B: https://youtu.be/FQusY57o7Wk
  - Part C: https://youtu.be/vz0RTMGTObc
  - Part D: https://youtu.be/nY-qG9dA39c
  - Part E: https://youtu.be/1zT1GH6T8po
  - CMake: https://youtu.be/ObVm0jOU1BM
  
  Run `make` to make executable files, `make clean` to clean files
