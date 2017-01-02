/*
 * fsh.c - the Feeble SHell.
 */

#include <stdio.h>
#include <unistd.h>
#include "fsh.h"
#include "parse.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

int showprompt = 1;
int laststatus = 0;  /* set by anything which runs a command */
char *com;

int main()
{
    char buf[1000];
    struct parsed_line *p;
    extern void execute(struct parsed_line *p);
    fflush(stdout);
    while (1) {
        if (showprompt)
            printf("$ ");
        if (fgets(buf, sizeof buf, stdin) == NULL)
            break;
        if ((p = parse(buf))) {
            execute(p);
            freeparse(p);
        }
    }
    return(laststatus);
}


void execute(struct parsed_line *p) {
    laststatus = 0;
    extern int builtin_exit(char **argv);
    extern int builtin_cd(char **argv);
    extern void prog(struct parsed_line *p, int nxt);
    int pid, pid2, pfd[2], status;

    if (p->pl != NULL && !strcmp(p->pl->argv[0], "exit")) {
        laststatus = builtin_exit(p->pl->argv);
        return;
    } else if (p->pl != NULL && !strcmp(p->pl->argv[0], "cd")) {
            builtin_cd(p->pl->argv);
            return;
    }
    fflush(stdout);
    switch(pid = fork()) {
    case -1:
        perror("fork");
        laststatus = 1;
        exit(1);
    case 0:
        if (pipe(pfd)) {
            perror("pipe");
            laststatus = 1;
            exit(1);
        } 
        switch(pid2 = fork()) {
        case -1:
            perror("fork");
            laststatus = 1;
            exit(1);
        case 0:
            close(pfd[0]);
            if (dup2(pfd[1], 1) == -1) {
                perror("dup2");
                laststatus = 1;
            }
            if (close(pfd[1]) == -1) {
                perror("close");
                laststatus = 1;
            }
            prog(p, 1);
            //perror("next");
            exit(0);
        default:
            close(pfd[1]);
            if (dup2(pfd[0], 0) == -1) {
                perror("dup2");
                laststatus = 1;
                exit(1);
            }
            if (close(pfd[0]) == -1) {
                perror("close");
                laststatus = 1;
                exit(1);
            }
            prog(p, 0);
            perror("next");
            exit(1);
        }
        break;
    default:
        pid = wait(&status);
    }
}

void prog(struct parsed_line *p, int nxt) {
    extern char **environ;
    extern int builtin_cd(char **argv);
    extern int builtin_exit(char **argv);
    extern int valid(char *command);
    struct parsed_line *cmd;
 
    for (cmd = p; cmd; cmd = cmd->next) {
        if (cmd != p) {
            switch(cmd->conntype) {
            case CONN_SEQ:
                break;
            case CONN_AND:
                if (laststatus == 0) {
                    break;
                } else {
                    laststatus = 1;
                    exit(1);
                }
            case CONN_OR:
                if (laststatus == 1) {
                    break;
                } else {
                    laststatus = 1;
                    exit(1);
                }
            }
        }
        if (p->inputfile) {
            if (access(p->inputfile, F_OK) != -1) {
                int in = open(p->inputfile, O_RDONLY, 0);
                dup2(in, STDIN_FILENO);
                close(in);
            } else {
                fprintf(stderr, "%s: No such file or directory\n", p->inputfile);
                laststatus = 1;
                exit(1);
            }
        }
        if (p->outputfile) {
            int out = open(p->outputfile, O_WRONLY | O_CREAT, 0666);
            if (out == -1) {
                perror("outputfile");
                laststatus = 1;
                exit(1);
            }
            dup2(out, STDOUT_FILENO);
            close(out);
        }
        if (p->pl && (nxt == 0)) {
            if (strchr(p->pl->argv[0], '/')) {
                laststatus = execve(p->pl->argv[0], p->pl->argv, environ);
            } else if (valid(p->pl->argv[0]) == 0) {
                laststatus = execve(com, p->pl->argv, environ);
            }
        } else if (p->pl->next && (nxt == 1)) {
            if (strchr(p->pl->next->argv[0], '/')) {
                laststatus = execve(p->pl->next->argv[0], p->pl->next->argv, environ);
            } else if (valid(p->pl->next->argv[0]) == 0) {
                laststatus = execve(com, p->pl->next->argv, environ);
            }
        }
    }
}

/*
void cnnct(char *cmnd) {
    if (strchr(cmnd, '/')) {
        laststatus = execve(p->pl->next->argv[0], p->pl->next->argv, environ);
    } else if (valid(cmnd) == 0) {
        laststatus = execve(com, p->pl->next->argv, environ);
    }
}*/

int valid(char *command) {
    char cmnd1[1000], cmnd2[1000];
    sprintf(cmnd1, "%s%s", "/bin/", command);
    sprintf(cmnd2, "%s%s", "/usr/bin/", command);
    if (access(cmnd1, X_OK)==0) {
        com = cmnd1;
    } else if (access(cmnd2, X_OK)==0) {
        com = cmnd2;
    } else if (access(command, X_OK)==0) {
        com = command;
    } else {
        fprintf(stderr, "%s: Command not found.\n", command);
        laststatus = 1;
        exit(1);
    }
    return(0);
}
