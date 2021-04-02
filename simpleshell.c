#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>

#define MAX_LINE 80 // the max length command

int main(void)
{
    char *args[MAX_LINE/2 + 1]; // command line arguments
    int should_run = 1; // flag to determine when to exit program
    
    while (should_run){
        printf("^_^> ");
        fflush(stdout);

        /*=============================================*/
        /* get user input, split, and store to args    */
        /*=============================================*/
        char str_read[MAX_LINE];                // array that store raw user input
        fgets(str_read, MAX_LINE, stdin);       // read user input and store to str_read
        str_read[strlen(str_read)-1] = 0;       // remove \n
        char* temp = strtok(str_read, " ");     // split string by ' '
        int arg_num = 0;                        // number of arguments
        while (temp != NULL){
            args[arg_num] = temp;               // args[i] = i th input
            temp = strtok(NULL, " ");
            arg_num++;
        }
        args[arg_num] = NULL;                   // input NULL at the next index of last argument

        if (arg_num > 0)
            if (*args[0]==*"exit")              // exit program
                should_run = 0;        
        
        /*===============================================*/
        /* set no_wait flag 1 when args has & at the end */
        /*===============================================*/
        int no_wait = 0;
        if (arg_num > 0){
            if (*args[arg_num-1]==*"&"){
                no_wait = 1;
                args[arg_num-1] = NULL;
                arg_num--;
            }
        }

        /*===========================================================*/
        /* seperate args into args1 and args2 when it has < or > or |*/
        /*===========================================================*/
        char *args1[MAX_LINE/4 + 1];        // first args
        char *args2[MAX_LINE/4 + 1];        // second args
        char cmd_opt = 0;                   // <, >, |  

        for (int i = 0; i < arg_num; i++){   
            if (*args[i]== *"<"){           // <
                cmd_opt = *args[i];
                int j = 0;
                for (j = 0; j < i; j++)
                    args1[j] = args[j];     // copy first args to args1
                args1[j] = NULL;            // insert NULL next to last arg of args1

                for (j = 0; i+j < arg_num; j++)
                    args2[j] = args[i+j+1]; // copy second args to args2
                args2[j] = NULL;            // insert NULL next to last arg of args2
                break;
            }
            else if (*args[i]== *">"){      // >
                cmd_opt = *args[i];
                int j = 0;
                for (j = 0; j < i; j++)
                    args1[j] = args[j];
                args1[j] = NULL;

                for (j = 0; i+j < arg_num; j++)
                    args2[j] = args[i+j+1];
                args2[j] = NULL;
                break;
            }
            else if (*args[i]== *"|"){       // |
                cmd_opt = *args[i];
                int j = 0;
                for (j = 0; j < i; j++)
                    args1[j] = args[j];
                args1[j] = NULL;

                for (j = 0; i+j < arg_num; j++)
                    args2[j] = args[i+j+1];
                args2[j] = NULL;
                break;
            }            
        }

        /*============================================*/
        /*  execute arguments for each cases <, >, |  */
        /*============================================*/
        if (cmd_opt == '|'){                 // pipe
            int fd[2];
            pipe(fd);

            pid_t pid_one, pid_two;
            pid_one = fork();              // create child process args2

            if (pid_one < 0){
                fprintf(stderr, "Fork failed");
                return 1;
            }
            else if (pid_one == 0){        // simpleshell's child process(args2)
                pid_two = fork();
                if (pid_two < 0){
                    fprintf(stderr, "Fork failed");
                    return 1;
                }
                else if (pid_two == 0){         // args2's child process(args1)
                    dup2(fd[1],1);                  // make args1's output to pipe's write end
                    close(fd[0]);                   // close pipe's read end
                    execvp(args1[0],args1);         // execute args1
                }
                else{                            // args1's parent process(args2)
                    wait(NULL);                 // wait args1
                    dup2(fd[0],0);                      // make args2's input to pipe's read end
                    close(fd[1]);                       // close pipe's write end
                    execvp(args2[0],args2);             // execute args2
                }
            }
            else if (pid_one > 0){              // parent process - simpleshell
                if (no_wait)
                    waitpid(pid_one, NULL, WNOHANG);
                else
                    wait(NULL);
            }
        }
        else if (cmd_opt == '>'){               // redirection
            int out = open(args2[0], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);

            pid_t pid;
            pid = fork();

            if (pid < 0){                           // error occurred
                fprintf(stderr, "Fork failed");
                printf("fork failed");
                return 1;
            }
            else if (pid == 0){                     // child process
                dup2(out, 1);
                execvp(args1[0], args1);
            }
            else{                                   // parent process
                if (no_wait)
                    waitpid(pid, NULL, WNOHANG);
                else
                    wait(NULL);
            }
        }
        else if (cmd_opt == '<'){
            int in = open(args2[0], O_RDONLY);

            pid_t pid;
            pid = fork();

            if (pid < 0){                    // error occurred
                fprintf(stderr, "Fork failed");
                printf("fork failed");
                return 1;
            }
            else if (pid == 0){              // child process
                dup2(in, 0);
                execvp(args1[0], args1);
            }
            else{                            // parent process
                if (no_wait)
                    waitpid(pid, NULL, WNOHANG);
                else
                    wait(NULL);
            }
        }
        else{
            pid_t pid;
            pid = fork();

            if (pid < 0){                    // error occurred
                fprintf(stderr, "Fork failed");
                printf("fork failed");
                return 1;
            }
            else if (pid == 0){              // child process
                execvp(args[0], args);
            }
            else{                            // parent process
                if (no_wait)
                    waitpid(pid, NULL, WNOHANG);
                else
                    wait(NULL);
            }
        }
    }
    
    return 0;
}
