Files:
    log_files: contains randomly generated log files
    given_files: contains log files provided by CS425 staff
    agg_log: 60MB log file created by merging some of the given log files
    client.c: client code
    server.c: server code
    host_list: contains hosts to be run by client
    unit_test.c: unit test code
    update.sh: bash script to compile client.c, server.c, and unit_test.c producing executables server, client, and unit_test

Compiling the code:
    gcc -pthread client.c -o client
    gcc -pthread server.c -o server
    gcc unit_test.c -o unit_test

    or

    ./update.sh

Running the code:
    For server: Start server on target machines (start before running client)
        ./server

    For client:
        ./client [FLAG] [COMMAND] [ARGUMENTS] => example: ./client 0 grep hello machine.i.log

        FLAG: 0 or 1
            0 = output is displayed in terminal standard out
            1 = output is saved to files VMX.out, where 'X' denotes the VM the output was received
            An output file line_no_file is also produced local to the client filled with line counts for each VM output as well as the total
            error_file is produced local to the client and contains any connection error that occured during grep

        COMMAND:
            If grep search contains '(' or ')', enclose entire phrase in single quotes

        ARGUMENTS:
            If file, target file must be contained in same directory as server file

Unit Testing:
   Run unit tests with ./unit_test
   Log files on servers must be in the same directory as the server and named machine.i.log
   Diffs the grep output from each machine for machine.i.log with the sys_X.csv files. sys_0 corresponds to VM1, sys_1 to VM2, and so on.
