#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>

#include "header.h"

extern int b;
extern char id[SizeofStrings], common_dir[SizeofStrings], input_dir[SizeofStrings], mirror_dir[SizeofStrings], log_file[SizeofStrings];
extern pid_t pid1, pid2, pid3;
extern int fd3;


int explain_wait_status(int status, int pid)
{
    int result = 0;
    if(WIFEXITED(status))
    {
        printf("Child with pid %d exited with status %d\n", pid, WEXITSTATUS(status));
        result = WEXITSTATUS(status);
    }
    else if(WIFSIGNALED(status))
    {
        printf("Child with pid %d was terminated by signal with id %d\n", pid, WTERMSIG(status));
        result = WTERMSIG(status);
    }
    else
    {
        printf("Child with pid %d has unknown status\n", pid);
        result = -1;
    }
    return result;
}

void my_write(int bytes, int fd, char *string)
{
    int to_write = bytes;
    int written = 0;
    int result = 0;
    do
    {
        fflush(stdout);
        result = write(fd, string + written, to_write - written);
        if (result < 0)
        {
            printf("An error occured when writing to pipe\n");
            fflush(stdout);
            exit(-1);
        }
        if (result == 0)
        {
            //printf("Pipe closed\n");
            fflush(stdout);
            //exit(1);
        }
        written += result;
    }
    while(written < to_write);
}

void my_read(int bytes, int fd, char *string)
{
    int to_read = bytes;
    int read_amount = 0;
    int result = 0;
    do
    {
        result = read(fd, string + read_amount, to_read - read_amount);
        //printf("ERROR:%s\n",strerror(read_amount));
        if (result < 0)
        {
            printf("An error occured when reading from pipe\n");
            fflush(stdout);
            exit(-1);
        }
        if (result == 0)
        {
            //printf("Pipe closed\n");
            fflush(stdout);
            //exit(1);
        }
        read_amount += result;
    }
    while(read_amount < to_read);
}

void write_to_fifo(int fd, char *path, int b, int fd3)
{
    struct stat statist;
    struct dirent *dirent;
    char temp[SizeofStrings];
    char temp_string[SizeofStrings];
    char temp_string2[SizeofStrings];
    char string[SizeofStrings];
    int size, offset, fd2, len;
    DIR *dir = opendir(path);
    if(dir)
    {
        while((dirent = readdir(dir)) != NULL)
        {
            if(strcmp(dirent->d_name,".") == 0 || strcmp(dirent->d_name,"..") == 0 || dirent->d_name[0] == '.')
                continue;
            strcpy(temp, path);
            strcat(temp, "/");
            strcat(temp, dirent->d_name);
            stat(temp, &statist);
            if (S_ISREG(statist.st_mode) == 0)
            {
                /*Write 01(directory) to fifo*/
                my_write(2, fd, "01");
                
                sprintf(temp_string, "%02d",(int)strlen(dirent->d_name));
                printf("LEN: %s\n",temp_string);
                my_write(2, fd, temp_string);
                len = atoi(temp_string);
                
                strcpy(temp_string, dirent->d_name);
                printf("NAME: %s\n",temp_string);
                my_write(len, fd, temp_string);
                
                sprintf(temp_string, "%04d",(int)statist.st_size);
                printf("Size: %ld\n",statist.st_size);
                my_write(4, fd, temp_string);

                write_to_fifo(fd, temp, b, fd3);
            }
            else
            {    
                /*Write 02(file) to fifo*/
                my_write(2, fd, "02");

                /*Write len of name to fifo*/
                sprintf(temp_string, "%02d",(int)strlen(dirent->d_name));
                printf("LEN: %s\n",temp_string);
                my_write(2, fd, temp_string);
                len = atoi(temp_string);
                
                /*Write name to fifo*/
                strcpy(temp_string, dirent->d_name);
                printf("NAME: %s\n",temp_string);
                my_write(len, fd, temp_string);

                /*Write 02(sender) to logfile*/
                strcpy(temp_string2, "02");                
                strcat(temp_string2, "\n");
                my_write(strlen(temp_string2),fd3, temp_string2);

                /*Write name to logfile*/
                strcpy(temp_string2, temp_string);                
                strcat(temp_string2, "\n");
                my_write(strlen(temp_string2),fd3, temp_string2);
                
                sprintf(temp_string, "%04d",(int)statist.st_size);
                printf("Size: %ld\n",statist.st_size);
                my_write(4, fd, temp_string);

                /*Write size to logfile*/
                strcpy(temp_string2, temp_string);
                strcat(temp_string2, "\n");
                my_write(strlen(temp_string2),fd3, temp_string2);
                
                size = 0;
                offset = 0;
                fd2 = open(temp, O_RDONLY);
                
                if(fd2 < 0)
                {
                    kill(getppid(), SIGUSR1);
                    exit(-1);
                }
                /*Write text to logfile*/
                while(offset != statist.st_size)
                {
                    if(b < statist.st_size - offset)
                        size = b;
                    else
                        size = statist.st_size - offset;                                    
                    my_read(size, fd2, temp_string);
                    printf("Text: %s\n",temp_string);
                    my_write(size, fd, temp_string);
                    offset += size;
                }   
                close(fd2);
            }
        }
        my_write(2, fd, "00");
        closedir(dir);
    }
    else
    {
        kill(getppid(), SIGUSR1);
        exit(-1);
    }
    
}

void read_from_fifo(int fd, char *path, int b, int fd3)
{
    struct stat statist;
    struct dirent *dirent;
    char temp[SizeofStrings];
    char temp_string[SizeofStrings];
    char temp_string2[SizeofStrings];
    char string[SizeofStrings];
    int  size, offset, fd2, len, rec_size;
    DIR *dir = opendir(path);
    if(dir)
    {
        while(1)
        {   
            strcpy(temp, path);                         
            my_read(2, fd, temp_string);
            temp_string[2] = '\0';
            /*Ended directory*/
            if(strcmp(temp_string, "00") == 0)
                break;
            /*Read a directory*/
            if(strcmp(temp_string, "01") == 0)
            {
                /*Read len of name from fifo*/
                my_read(2, fd, temp_string);
                temp_string[2] = '\0';
                
                printf("REC LEN:%d\n",atoi(temp_string));
                len = atoi(temp_string);
                
                /*Read name from fifo*/
                my_read(len, fd, temp_string);
                temp_string[len] = '\0';
                printf("REC NAME:%s\n",temp_string);
                strcat(temp,"/");
                strcat(temp,temp_string);

                /*Read size of file from fifo*/
                my_read(4, fd, temp_string);
                temp_string[4] = '\0';
                printf("REC SIZE:(%s)\n",temp_string);
                rec_size = atoi(temp_string);
                    
                mkdir(temp, 0755);
                read_from_fifo(fd, temp, b, fd3);
            }
            /*Read a file*/
            else if(strcmp(temp_string, "02") == 0)
            {
                /*Read len of name from fifo*/
                my_read(2, fd, temp_string);
                temp_string[2] = '\0';
                //if(strcmp(temp_string, "00") == 0)
                //    break;
                printf("REC LEN:%d\n",atoi(temp_string));
                len = atoi(temp_string);
                
                /*Read name from fifo*/
                my_read(len, fd, temp_string);
                temp_string[len] = '\0';
                printf("REC NAME:%s\n",temp_string);
                
                /*Write 01(receiver) to logfile*/
                strcpy(temp_string2, "01");                
                strcat(temp_string2, "\n");
                my_write(strlen(temp_string2),fd3, temp_string2);

                /*Write name to logfile*/
                strcpy(temp_string2, temp_string);                
                strcat(temp_string2, "\n");
                my_write(strlen(temp_string2),fd3, temp_string2);
                
                strcat(temp,"/");
                strcat(temp, temp_string);

                /*Read size of file from fifo*/
                my_read(4, fd, temp_string);
                temp_string[4] = '\0';
                printf("REC SIZE:(%s)\n",temp_string);
                
                /*Write size to logfile*/
                strcpy(temp_string2, temp_string);
                strcat(temp_string2, "\n");
                my_write(strlen(temp_string2),fd3, temp_string2);

                rec_size = atoi(temp_string);
                fd2 = open(temp, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                if(fd2 < 0)
                {
                    kill(getppid(), SIGUSR1);
                    exit(-1);
                }
                size = 0;
                offset = 0;
                /*Read text from fifo*/
                while(offset != rec_size)
                {
                    if(b < rec_size - offset)
                        size = b;
                    else
                        size = rec_size - offset;
                    my_read(size, fd, temp_string);
                    temp_string[size] = '\0';
                    printf("REC TEXT:%s\n",temp_string);
                    my_write(size, fd2, temp_string);
                    offset += size;
                }
                close(fd2);
            }
        }
        closedir(dir);
    }
    else
    {
        kill(getppid(), SIGUSR1);
        exit(-1);
    }
}

/*Delete everything recursively in path*/
void delete_mirror(char *path)
{
    char temp[SizeofStrings];
    struct dirent *dirent;
    struct stat statist;
    DIR *dir = opendir(path);
    if(dir)
    {
        while ((dirent = readdir(dir)) != NULL)
        {
            if(dirent->d_name == NULL || strcmp(dirent->d_name,".") == 0 || strcmp(dirent->d_name,"..") == 0 )
                continue;
            strcpy(temp, path);
            strcat(temp, "/");
            strcat(temp, dirent->d_name);
            stat(temp, &statist);
            if (S_ISREG(statist.st_mode) == 0)
            {
                delete_mirror(temp);
            }
            else
            {
                while(remove(temp) != 0);
            }
        }
        closedir(dir);
        while(rmdir(path) != 0);
    }
}

/*If SIGINT OR SIQUIT arrived delete mirror folder and id file in mirror*/
void signal_arrived(int signal)
{
    char temp[SizeofStrings];
    delete_mirror(mirror_dir);
    strcpy(temp, common_dir);
    strcat(temp, "/");
    strcat(temp, id);
    strcat(temp, ".id");
    if(remove(temp) != 0)
        exit(-1);
    /*Write 03 to logfile which shows that a client closed */
    my_write(2, fd3, "03");
    my_write(1, fd3, "\n");
    close(fd3);
    exit(0);
}

/*If SIGUSR arrived then kill the children*/
void sigusr_arrived(int signal)
{
    if(pid1 != 0) kill(pid1, SIGKILL);
    if(pid2 != 0) kill(pid2, SIGKILL);
    printf("Received SIGUSR from child\n");
    fflush(stdout);
} 


void handler (void)
{
  struct sigaction setup_action;
  sigset_t block_mask;

  sigemptyset (&block_mask);
  /* Block other terminal-generated signals while handler runs. */
  sigaddset (&block_mask, SIGINT);
  sigaddset (&block_mask, SIGUSR1);
  setup_action.sa_handler = signal_arrived;
  setup_action.sa_flags = 0;
  sigaction (SIGQUIT, &setup_action, NULL);
  
  sigemptyset (&block_mask);
  sigaddset (&block_mask, SIGQUIT);
  sigaddset (&block_mask, SIGUSR1);
  setup_action.sa_mask = block_mask;
  sigaction (SIGINT, &setup_action, NULL);
  
  setup_action.sa_handler = sigusr_arrived;
  sigemptyset (&block_mask);
  sigaddset (&block_mask, SIGINT);
  sigaddset (&block_mask, SIGQUIT);
  setup_action.sa_mask = block_mask;
  sigaction (SIGUSR1, &setup_action, NULL);
}

void uninstall_handler (void)
{
  struct sigaction setup_action;
  sigset_t block_mask;

  sigemptyset (&block_mask);
  /* Block other terminal-generated signals while handler runs. */
  setup_action.sa_handler = SIG_DFL;
  setup_action.sa_flags = 0;
  sigaction (SIGQUIT, &setup_action, NULL);
  
  sigemptyset (&block_mask);
  setup_action.sa_mask = block_mask;
  sigaction (SIGINT, &setup_action, NULL);
  
  setup_action.sa_handler = SIG_DFL;
  sigemptyset (&block_mask);
  setup_action.sa_mask = block_mask;
  sigaction (SIGUSR1, &setup_action, NULL);
}