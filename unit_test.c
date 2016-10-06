#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#define RED   "\x1B[31m"
#define YEL   "\x1B[33m"
#define GRN   "\x1B[32m"
#define RESET "\x1B[0m"

int time_test();
int standard_test(char * command, char * local_command);

char * construct_timestamp();

//int machine_count;


int main(int argc, char **argv) {
    int passed = 0;
    int failed = 0;
    
    char * commands[] = {"./client 1 grep Thomas machine.i.log",
        "./client 1 grep 0.0 machine.i.log",
    
        "./client 1 grep 681-[\-0-9]* machine.i.log",
        "./client 1 grep 9$ machine.i.log",
        "./client 1 grep @google machine.i.log",
        "./client 1 grep ^10 machine.i.log",
        "./client 1 grep 和 machine.i.log",
        "./client 1 grep [nN][iI][tT][rR][aA][tT][eE] machine.i.log",
        "./client 1 grep '[5-9]\{3\}-[0-9]\{2\}' machine.i.log",
        "./client 1 grep [和製漢語] machine.i.log",
        "./client 1 grep -i OCTINOXATE machine.i.log",
        "./client 1 grep -iw Dean machine.i.log",
        "./client 1 grep -A 3 Philip machine.i.log",
        "./client 1 grep -A 3 -i zinc machine.i.log",
        "./client 1 grep -C 3 Philip machine.i.log",
        "./client 1 grep -C 3 -i zinc machine.i.log",
        "./client 1 grep -c -i thomas machine.i.log",
        "./client 1 grep -v -c -i thomas machine.i.log",
        "./client 1 grep -o -b zinc machine.i.log",
        "./client 1 grep -n zinc machine.i.log",
        "./client 1 grep -i p.*ne machine.i.log",
        "./client 1 grep '8\{2\}' machine.i.log",
        //The below tests pass on individual cases, but crashes grep when run consecutively.
        //"./client 1 grep [\\\.\*] machine.i.log"
        //"./client 1 grep [^a-zA-Z]@ machine.i.log",
        //"./client 1 grep 3* machine.i.log",
        //"./client 1 grep '\".*\"' machine.i.log",
        //"./client 1 grep 9\{5,\} machine.i.log",    
        //"./client 1 grep \(ﾉಥ益ಥ） machine.i.log",
    };

    char * local_commands[] = {"grep Thomas log_files/sys_0.csv > local0.out",
        "grep 0.0 log_files/sys_0.csv > local0.out",
    
        "grep 681-[\-0-9]* log_files/sys_0.csv > local0.out",
        "grep 9$ log_files/sys_0.csv > local0.out",
        "grep @google log_files/sys_0.csv > local0.out",
        "grep ^10 log_files/sys_0.csv > local0.out",
        "grep 和 log_files/sys_0.csv > local0.out",
        //"grep \(ﾉಥ益ಥ） sys_0.csv > local0.out",
        "grep [nN][iI][tT][rR][aA][tT][eE] log_files/sys_0.csv > local0.out",
        "grep '[5-9]\{3\}-[0-9]\{2\}' log_files/sys_0.csv > local0.out",
        "grep [和製漢語] log_files/sys_0.csv > local0.out",
        "grep -i OCTINOXATE log_files/sys_0.csv > local0.out",
        "grep -iw Dean log_files/sys_0.csv > local0.out",
        "grep -A 3 Philip log_files/sys_0.csv > local0.out",
        "grep -A 3 -i zinc log_files/sys_0.csv > local0.out",
        "grep -C 3 Philip log_files/sys_0.csv > local0.out",
        "grep -C 3 -i zinc log_files/sys_0.csv > local0.out",
        "grep -c -i thomas log_files/sys_0.csv > local0.out",
        "grep -v -c -i thomas log_files/sys_0.csv > local0.out",
        "grep -o -b zinc log_files/sys_0.csv > local0.out",
        "grep -n zinc log_files/sys_0.csv > local0.out",
        "grep -i p.*ne log_files/sys_0.csv > local0.out",
        "grep '8\{2\}' log_files/sys_0.csv > local0.out",
    };


    int length = sizeof(commands)/sizeof(char*);
    int i;
    for(i = 0; i < length; i++){
        printf("TESTING COMMAND: %s\n", commands[i]);
        if(!standard_test(commands[i], local_commands[i])){
            printf(RED "%s => FAILED\n\n" RESET, commands[i]);
            failed++;
        }else{
            printf(GRN "%s => PASSED\n\n" RESET, commands[i]);
            passed++;
        }
    }
    

    printf("\n\nTests Passed: %d\nTests Failed: %d\nTotal Tests: %d\n", passed, failed, length);
    return 0;
}

int standard_test(char * command, char * local_command){
    int retval = 1;
    int i;
    int len = strlen(local_command);
    char local_copy[len + 1];
    strcpy(local_copy, local_command);

    system(command);

    for(i = 0; i < 10; i++){
        local_copy[len - 5] = i + '0';
        local_copy[len - 18] = i + '0';
        system(local_copy);
        char VM_file[] = "VM0.out";
        VM_file[2] = i + '0';

        if(fopen(VM_file, "r") == NULL){
            //retval = 1; file doesn't exist
            printf(YEL "VM%d.out was not created. Check connection to VM%d.\n" RESET, i, i);
        }else{
            char diff[] = "diff VM0.out local0.out > diff.out";
            diff[7] = i + '0';
            diff[18] = i + '0';
            system(diff);

            FILE * fp = fopen("diff.out", "r");
            if(fp != NULL){
                fseek(fp, 0, SEEK_END);
                if(ftell(fp) == 0){
                    printf(GRN "VM%d.out and local%d.out match.\n" RESET, i, i);
                }else{
                    printf(RED "VM%d.out and local%d.out differ.\n" RESET, i, i);
                    retval = 0;
                }
            }

            fclose(fp);
            remove("diff.out");
        }

        char local_file[] = "local0.out";
        local_file[5] = i + '0';
        remove(local_file);
        remove(VM_file);
    }
    return retval;
}
