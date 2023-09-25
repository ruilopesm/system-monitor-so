# System-Monitor

## Operating Systems | Sistemas Operativos
## Grade: 19/20 :star:

This project was developed during the second semester of the 2nd year of the Software Engineering degree @ University of Minho.

The project consisted of developing a system monitor, similar to the `htop` command, using the C programming language and specially only system calls.

The project architecture is based on a server-client model, where the server is responsible for collecting the information and the client for running a program and displaying the result.

Examples of commands that can be used by a client:

- `execute -u "ls"`, which executes the command `ls` and displays the result;
- `execute -p "ls -la | grep x | wc -l"`, which executes a pipeline of commands and displays the result;
- `status`, which displays all the current programs being executed by all the connected clients;
- `stats-time <PID-x> <PID-y>`, which displays the CPU time used by the programs with the PIDs x and y;
- `stats-uniq <PID-x> <PID-y>`, which displays the number of unique commands executed by the programs with the PIDs x and y;
- `stats-command <PID-x> <PID-y> <command>`, which displays the number of times the command was executed by the programs with the PIDs x and y;

## Installing and running the project

#### Clone the repository

```bash
$ git clone git@github.com:ruilopesm/System-Monitor-SO.git
```

#### Compiling  
```bash
$ cd System-Monitor-SO
$ make
```

Note: You can also use `make client` or `make server` to compile only the client or the server, respectively.

#### Running the server
```bash
$ ./bin/monitor
```

#### Spawning a client
```bash
$ ./bin/tracer execute -u "ls"
```

# Developed by:
- [Rui Lopes](https://github.com/ruilopesm)
- [Daniel Pereira](https://github.com/danielsp45)
- [Pedro Sousa](https://github.com/Pdf0)
