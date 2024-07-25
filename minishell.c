#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/wait.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>
#define BLUE "\x1b[34;1m"
#define DEFAULT "\x1b[0m"

// checking hidden property

volatile sig_atomic_t interrupted;
void handler(int x){
    interrupted = 1;
}

int pid_up(const void* a, const void* b){
    char* a1 = *(char**)a;
    char* b1 = *(char**)b;

    if (atoi(a1) < atoi(b1)){
        return -1;
    }
    else if (atoi(a1) > atoi(b1)){
        return 1;
    }
    else{
        return 0;
    }
}


void lphelper(char* p){
    char pathn[256];
    char* path = "/proc/";
    if (chdir(path) == -1){
        fprintf(stderr, "Error: Cannot change directory to %s. %s.\n", path, strerror(errno));
    }
    p = getcwd(pathn, 256);
    if (p == NULL) 
    {
        fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
    }
    
    DIR* d;
    struct dirent* dp;

    d = opendir(p);
    if (d == NULL){
        fprintf(stderr, "Error: opendir() failed. %s.\n", strerror(errno));
    }

    char* pidlist[4096];
    int entry = 0;
    while ((dp = readdir(d)) != NULL)
    {
        if (atoi(dp->d_name) != 0){
            pidlist[entry] = dp->d_name;
            entry += 1;
        }
    }

    qsort(pidlist, entry, sizeof(char*), &pid_up);

    for (int i = 0; i < entry; i++)
    {
        char* dirname = pidlist[i];
        if (chdir(dirname) == -1){
            fprintf(stderr, "Error: Cannot change directory to %s. %s.\n", dirname, strerror(errno));
        }
        //free(p);
        p = getcwd(pathn, 256);
        if (p == NULL) 
        {
            fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
        }

        struct stat fileinf;
        int status = stat(p, &fileinf);

        if (status != 0){
            fprintf(stderr, "Error: stat() failed. %s.\n", strerror(errno));
        }

        uid_t userid = fileinf.st_uid;
        struct passwd* x = getpwuid(userid);
        if (x == NULL){
            fprintf(stderr, "Error: Cannot get passwd entry. %s.\n", strerror(errno));
        }
        char* userName = x->pw_name;

        FILE* stream;
        char* line = NULL;
        size_t len = 0;
        ssize_t nread = 0;

        if ((stream = fopen("cmdline", "r")) == NULL) {
            fprintf(stderr, "Error: fopen() failed. %s.\n",strerror(errno));
        }
        while ((nread = getline(&line, &len, stream)) != -1) {
            printf("%s %s %s\n", dirname,userName, line);
        }
        free(line);
        fclose(stream);

        if (chdir(path) == -1){
            fprintf(stderr, "Error: Cannot change directory to %s. %s.\n", path, strerror(errno));
        }
        //free(p);
        //p = getcwd(NULL,0);
        getcwd(pathn, 256);
        if (p == NULL) 
        {
            fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
        }
    }
    //free(p);
    closedir(d);
}

int main(int argc, char* argv[]) {

    char* pathname;
    char pathnew[256];
    pathname = getcwd(pathnew,256);

    struct sigaction action = {0};
    struct sigaction old = {0};

    while(1)
    {
        action.sa_handler = handler;
        if(sigaction(SIGINT, &action, &old) == -1){
            fprintf(stderr, "Error: Cannot register signal handler. %s.\n", strerror(errno));
        }
        char command[4096];
        int nargc = 0;
        char* nargv[4096];

        //pathname = getcwd(NULL, 0);
        pathname = getcwd(pathnew,256);
        if (pathname == NULL) 
        {
            fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
        }
        printf("%s[%s]%s> ", BLUE, pathname, DEFAULT);
        //free(pathname);

        if(fgets(command, 4096, stdin) == NULL){
            if (interrupted)
            {
                interrupted = 0;
                if (sigaction(SIGINT, &old, NULL) == -1){
                    fprintf(stderr, "Error: Cannot register signal handler. %s.\n", strerror(errno));
                }
                printf("\n");
                continue;
            }
            else{
                fprintf(stderr, "Error: Failed to read from stdin. %s.\n", strerror(errno));
            }
        }

        command[strlen(command)-1] = '\0'; 

        char* token;
        char* delimeter = " ";
        token = strtok(command, delimeter);
        while (token != NULL){
            nargv[nargc] = token;
            token = strtok(NULL, delimeter);
            nargc += 1;
        }
        nargv[nargc] = NULL;

        if (nargc == 0){

            continue;
        }
        else if (strcmp(nargv[0], "cd") == 0){
            if (nargc == 1)
            {
                uid_t user = getuid();
                struct passwd* x = getpwuid(user);
                if (x == NULL){
                    fprintf(stderr, "Error: Cannot get passwd entry. %s.\n", strerror(errno));
                }

                if (chdir(x->pw_dir) == -1){
                    fprintf(stderr, "Error: Cannot change directory to %s. %s.\n", x->pw_dir, strerror(errno));
                }
                //free(pathname);
                //pathname = getcwd(NULL, 0);
                pathname = getcwd(pathnew,256);
                if (pathname == NULL) 
                {
                    fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
                }

            }
            else if (nargc == 2)
            {
                if (nargv[1][0] == '~'){
                    if (strcmp(nargv[1], "~") == 0)
                    {
                        uid_t user = getuid();
                        struct passwd* x = getpwuid(user);
                        if (x == NULL){
                            fprintf(stderr, "Error: Cannot get passwd entry. %s.\n", strerror(errno));
                        }

                        if (chdir(x->pw_dir) == -1){
                            fprintf(stderr, "Error: Cannot change directory to %s. %s.\n", x->pw_dir, strerror(errno));
                        }
                        //free(pathname);
                        //pathname = getcwd(NULL, 0);
                        pathname = getcwd(pathnew,256);
                        if (pathname == NULL) 
                        {
                            fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
                        }
                    }
                    else{
                        //char* temppath;
                        char temppath[256];
                        uid_t user = getuid();
                        struct passwd* x = getpwuid(user);
                        if (x == NULL){
                            fprintf(stderr, "Error: Cannot get passwd entry. %s.\n", strerror(errno));
                        }
                        //temppath = x->pw_dir;
                        strcpy(temppath, x->pw_dir);
                        strcat(temppath, nargv[1] + 1);
                        
                        if (chdir(temppath) == -1){
                            fprintf(stderr, "Error: Cannot change directory to %s. %s.\n", temppath, strerror(errno));
                        }
                        //free(pathname);
                        //pathname = getcwd(NULL,0);
                        pathname = getcwd(pathnew,256);
                        if (pathname == NULL) 
                        {
                            fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
                        }

                        //free(temppath);
                    }
                }
                else{
                    if (chdir(nargv[1]) == -1){
                        fprintf(stderr, "Error: Cannot change directory to %s. %s.\n", nargv[1], strerror(errno));
                    }
                    //free(pathname);
                    //pathname = getcwd(NULL, 0);
                    pathname = getcwd(pathnew,256);
                    if (pathname == NULL) 
                    {
                        fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
                    }
                }
            }
            else{
                fprintf(stderr, "Too many arguments to cd.\n");
            }
            //free(pathname);
        }
        else if (strcmp(nargv[0], "exit") == 0 && nargc == 1){
            //free(pathname);
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(nargv[0], "pwd") == 0 && nargc == 1){
            printf("%s\n", pathname);
        }
        else if (strcmp(nargv[0], "lf") == 0 && nargc == 1){
            DIR* directory;

            struct dirent* dp;

            directory = opendir(pathname);
            if (directory == NULL){
                fprintf(stderr, "Error: opendir() failed. %s.\n", strerror(errno));
            }

            while((dp = readdir(directory)) != NULL){
                if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) //CHECK THE HIDDEN PROPERTY
                {
                    printf("%s\n", dp->d_name);
                }
            }

            closedir(directory);
        }
        else if (strcmp(nargv[0], "lp") == 0 && nargc == 1){
            char* temp = pathname;
            lphelper(pathname);
            if (chdir(temp) == -1){
                fprintf(stderr, "Error: Cannot change directory to %s. %s.\n", temp, strerror(errno));
            }
            //pathname = getcwd(NULL,0);
            pathname = getcwd(pathnew,256);
            if (pathname == NULL) 
            {
                fprintf(stderr, "Error: Cannot get current working directory. %s.\n", strerror(errno));
            }
        }
        else{
            pid_t pid;
            int status;

            if ((pid = fork()) < 0){
                return -1;
            }
            else if (pid == 0) 
            {
                char temppath[4096];
                temppath[0] = '\0';
                strcat(temppath, "/bin/");
                strcat(temppath, nargv[0]);

                execv(temppath, nargv);
                fprintf(stderr, "Error: exec() failed. %s.\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            else{
                if (waitpid(pid, &status, 0) == -1){
                    if (interrupted){
                        interrupted = 0;
                        if(sigaction(SIGINT, &old, NULL) == -1){
                            fprintf(stderr, "Error: Cannot register signal handler. %s.\n", strerror(errno));
                        }
                        printf("\n");
                        continue;
                    }
                    else{
                        fprintf(stderr, "Error: wait() failed. %s.\n", strerror(errno));
                    }
                }
            }
        }
        
        //free(pathname);
    }
    //free(pathname);
    return 0;
}
