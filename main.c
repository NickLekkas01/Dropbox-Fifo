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



int b;
char id[SizeofStrings], common_dir[SizeofStrings], input_dir[SizeofStrings], mirror_dir[SizeofStrings], log_file[SizeofStrings];
pid_t pid1, pid2, pid3;
int fd3;

int main(int argc, char *argv[])
{
    
    if(argc == 13)
    {
        for(int i = 1; i < 13; i+=2)
        {
            if(argv[i][1] == 'n')
            {
                strcpy(id, argv[i+1]);
            }
            else if(argv[i][1] == 'c')
            {
                strcpy(common_dir, argv[i+1]); 
            }
            else if(argv[i][1] == 'i')
            {
                strcpy(input_dir, argv[i+1]); 
            }
            else if(argv[i][1] == 'm')
            {
                strcpy(mirror_dir, argv[i+1]); 
            }
            else if(argv[i][1] == 'b')
            {
                b = atoi(argv[i+1]);
            }
            else if(argv[i][1] == 'l')
            {
                strcpy(log_file, argv[i+1]); 
            }
        }
    }
    else
    {
        printf("Less arguments were given\n");
        exit(-1);
    }
    
    DIR *dir, *dir2, *dir3;
    if (dir = opendir(common_dir)) { closedir(dir); } else { mkdir(common_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);}
    if (dir = opendir(input_dir)) { closedir(dir); } else { perror("Input folder doesn't exist\n"); exit(-1); }
    if (dir = opendir(mirror_dir)) { perror("Mirror folder exists\n"); closedir(dir); exit(-1); } else { mkdir(mirror_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);}

    char string[b];
    char temp_string[b];
    strcpy(string,common_dir);
    strcat(string,"/");
    strcat(string, id);
    strcat(string, ".id");
    //printf("(%s)\n",string);
    int fd, fd2;
    if( access( string, F_OK ) != -1 )
    {
        perror("Another client is running the same id\n");
        exit(-1);
    }
    if((fd = open(string, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0)
        exit(-1);
    else
    {
        sprintf(temp_string, "%d", (int)getpid());
        write(fd, temp_string, strlen(temp_string));
        close(fd);
    }
    handler();

    struct dirent *dirent, *dirent2;
    struct stat statist;
    char temp_id[SizeofStrings], temp_id2[SizeofStrings];
    char temp_string2[b];
    int size, offset, rec_size;
    int status;
    int p_id = 0;
    int p_id2 = 0;
    int len;
    int flag = 0;
    int flag1, flag2;
    flag1 = 0;
    flag2 = 0;
    
    char *token;

    fd3 = open(log_file, O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(fd3 < 0)
    {
        kill(getppid(), SIGUSR1);
        exit(-1);
    }
    /*Write to logfile 00 which represents the start of the file*/
    my_write(2, fd3, "00");
    my_write(1, fd3, "\n");
    my_write(strlen(id), fd3, id);
    my_write(1, fd3, "\n");
    while(1)
    {
        dir = opendir(common_dir);
        if (dir)
        {
            while ((dirent = readdir(dir)) != NULL)
            {
                flag = 0;
                if(dirent->d_name == NULL || strcmp(dirent->d_name,".") == 0 || strcmp(dirent->d_name,"..") == 0 || dirent->d_name[0] == '.')
                    continue;
                strcpy(temp_string, dirent->d_name); 
                token = strtok(temp_string, ".");
                if(token == NULL)
                    continue; 
                strcpy(temp_id, token);  
                token = strtok(NULL, ".");
                if(token == NULL)
                    continue; 
                fflush(stdout);
                if(strcmp(id, temp_id) != 0 && strcmp(token, "id") == 0)
                {
                    strcpy(string, mirror_dir);
                    strcat(string, "/");
                    strcat(string, dirent->d_name);
                    /*If id exists in mirror folder*/
                    if(access(string, F_OK) != -1)
                    {
                        fd = open(string, O_RDONLY);
                        if(fd < 0)
                            exit(-1);
                        p_id = 0;
                        while(read(fd, temp_string2, 1) != 0)
                        {
                            p_id *= 10;
                            p_id += temp_string2[0] -'0';
                        }
                        close(fd);
                        strcpy(string, common_dir);
                        strcat(string, "/");
                        strcat(string, dirent->d_name);
                        /*If id file is not existant common directory delete mirror directory of this client*/
                        if(access(string, F_OK) == -1)
                        {
                            strcpy(string, mirror_dir);
                            strcat(string, "/");
                            strcat(string, temp_id);
                            delete_mirror(string);
                            strcat(string, ".id");
                            if(remove(string) != 0)
                                exit(-1);
                        }                                            
                        else
                        {
                            fd = open(string, O_RDONLY);
                            if( fd < 0 )
                                exit(-1);
                            p_id2 = 0;
                            while(read(fd, temp_string2, 1) != 0)
                            {
                                p_id2 *= 10;
                                p_id2 += temp_string2[0] -'0';
                            }
                            close(fd);
                            /*If processes id are not the same in the id file of mirror and common directory
                            then delete the mirror directory of this client and send/receive again with the new client*/
                            if(p_id != p_id2)
                            {
                                strcpy(string, mirror_dir);
                                strcat(string, "/");
                                strcat(string, temp_id);
                                delete_mirror(string);
                                strcat(string, ".id");
                                if(remove(string) != 0)
                                    exit(-1);
                                flag = 1;
                            }
                            else
                            {
                                continue;
                            }
                        }
                        
                    }
                    else
                    {
                        strcpy(string, common_dir);
                        strcat(string, "/");
                        strcat(string, dirent->d_name);
                        fd = open(string, O_RDONLY);
                        if( fd < 0 )
                            exit(-1);
                        p_id2 = 0;
                        while(read(fd, temp_string2, 1) != 0)
                        {
                            p_id2 *= 10;
                            p_id2 += temp_string2[0] -'0';
                        }
                        close(fd);
                        flag = 1;
                    }

                    if( flag == 1 )
                    {
                        fflush(stdout);
                        int error_happened = 0;
                        int index = 0;
                        do
                        {
                            printf("Try number %d\n", index+1);
                            fflush(stdout);
                            pid1 = 0;
                            pid2 = 0;
                            uninstall_handler();
                            if(pid1 = fork())       /*Parent */
                            {
                                if(pid2 = fork())  /*Parent*/
                                {
                                    handler();
                                    printf("PID=%d\n", pid2);
                                    int status;
                                    flag1 = 0;
                                    flag2 = 0;
                                    do
                                    {
                                        if(flag1 == 0) 
                                        {
                                            flag1 = waitpid(pid2, &status, WNOHANG);
                                            if(flag1 > 0) 
                                            {
                                                status = explain_wait_status(status, pid2);
                                                if (status != 0) error_happened = 1;
                                            }
                                            else if(flag1 < 0) 
                                            { 
                                                printf("Problem in waitpid2\n"); 
                                            }
                                        }
                                        if(flag2 == 0)
                                        {
                                            flag2 = waitpid(pid1, &status, WNOHANG);
                                            if(flag2 > 0) 
                                            {
                                                status = explain_wait_status(status, pid1);
                                                if (status != 0) error_happened = 1;
                                            }
                                            else if(flag2 < 0) 
                                            {
                                                printf("Problem in waitpid1\n"); 
                                            }
                                        }
                                    }
                                    while(flag1 == 0 || flag2 == 0 );
                                    if (error_happened)
                                    {
                                        pid3 = 0;
                                        pid3 = fork();
                                        if(pid3 == 0)
                                        {
                                            strcpy(string, mirror_dir);
                                            strcat(string, "/");
                                            strcat(string, dirent->d_name);
                                            if(remove(string) != 0)
                                                exit(-1);
                                            strcpy(string, mirror_dir);
                                            strcat(string, "/");
                                            strcat(string, temp_id);
                                            delete_mirror(string);
                                            exit(0);
                                        }
                                        waitpid(pid3, &status, 0);
                                        explain_wait_status(status, pid3);
                                    }
                                    else
                                    {
                                        strcpy(string, mirror_dir);
                                        strcat(string, "/");
                                        strcat(string, temp_id);
                                        strcat(string, ".id");
                                        fd = open(string, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                                        if(fd < 0)
                                            exit(-1);
                                        sprintf(temp_string2, "%d", p_id2);                                
                                        my_write(strlen(temp_string2), fd, temp_string2);
                                        close(fd);
                                    }
                                }
                                else                /*Child 2 */
                                {
                                    /*Create fifos for this id*/
                                    strcpy(string, common_dir);
                                    strcat(string,"/");
                                    strcat(string, "id");
                                    strcat(string, id);
                                    strcat(string, "_to_id");
                                    strcat(string, temp_id);
                                    strcat(string, ".fifo");
                                    
                                    if( access(string, F_OK) == -1 )
                                    {
                                        mkfifo(string, 0666);
                                    }
                                    strcpy(string, common_dir);
                                    strcat(string,"/");
                                    strcat(string, "id");
                                    strcat(string, temp_id);
                                    strcat(string, "_to_id");
                                    strcat(string, id);
                                    strcat(string, ".fifo");
                                    if( access(string, F_OK) == -1 )
                                    {
                                        mkfifo(string, 0666);
                                    }

                                    strcpy(string, common_dir);
                                    strcat(string,"/id");
                                    strcat(string, id);
                                    strcat(string, "_to_id");
                                    strcat(string, temp_id);
                                    strcat(string, ".fifo");
                                    fd = open(string, O_WRONLY);
                                    if(fd < 0)
                                    {
                                        kill(getppid(), SIGUSR1);
                                        exit(-1);
                                    }
                                    write_to_fifo(fd, input_dir, b, fd3);

                                    //strcpy(temp_string, "00");
                                    //my_write(2, fd, temp_string);       
                                    
                                    close(fd);
                                    exit(0);
                                }
                                //sleep(2);      
                            }
                            else                    /*Child 1*/
                            {
                                /*Create fifos if they do not exist*/
                                strcpy(string, common_dir);
                                strcat(string,"/");
                                strcat(string, "id");
                                strcat(string, id);
                                strcat(string, "_to_id");
                                strcat(string, temp_id);
                                strcat(string, ".fifo");
                                
                                if( access(string, F_OK) == -1 )
                                {
                                    mkfifo(string, 0666);
                                }
                                strcpy(string, common_dir);
                                strcat(string,"/");
                                strcat(string, "id");
                                strcat(string, temp_id);
                                strcat(string, "_to_id");
                                strcat(string, id);
                                strcat(string, ".fifo");
                                
                                if( access(string, F_OK) == -1 )
                                {
                                    mkfifo(string, 0666);
                                }

                                strcpy(string, common_dir);
                                strcat(string,"/id");
                                strcat(string, temp_id);
                                strcat(string, "_to_id");
                                strcat(string, id);
                                strcat(string, ".fifo");
                                
                                fd = open(string, O_RDONLY | O_NONBLOCK);
                                if(fd < 0)
                                {
                                    kill(getppid(), SIGUSR1);
                                    exit(-1);
                                }
                                strcpy(string, mirror_dir);
                                strcat(string, "/");
                                strcat(string, temp_id);
                                mkdir(string, 0755);
                                //sleep(100);
                                
                                
                                fd_set myset;
                                FD_ZERO(&myset);
                                FD_SET(fd,&myset);
                                struct timeval tim;
                                tim.tv_sec = 30;
                                tim.tv_usec = 0;
                                int select_status;
                                printf("Child with pid %d on waiting select\n", getpid());
                                fflush(stdout);
                                select_status = select(fd+1, &myset, NULL, NULL, &tim);
                                printf("Select returned status %d with remaining time: sec %d and usec %d\n", select_status, (int)tim.tv_sec, (int)tim.tv_usec);
                                fflush(stdout);
                                int exit_code = 0;
                                /*If we can read normally*/
                                if(select_status == 1)
                                {
                                    read_from_fifo(fd, string, b, fd3);
                                }
                                /*If time(30 seconds) has passed*/
                                else if(select_status == 0)
                                {
                                    kill(getppid(), SIGUSR1);
                                    exit_code = -1;
                                }
                                else
                                {
                                    exit_code = -2;
                                }
                                
                                close(fd);  
                                exit(exit_code);
                            }
                            index++;
                        } 
                        while(index < 3 && error_happened);
                    }
                }
            }
            closedir(dir);
        } 
        dir = opendir(mirror_dir);
        if (dir)
        {
            while ((dirent = readdir(dir)) != NULL)
            {
                strcpy(string, mirror_dir);
                strcat(string, "/");
                strcat(string, dirent->d_name);
                stat(string, &statist);
                if (S_ISREG(statist.st_mode) != 0)
                {
                    strcpy(temp_string, dirent->d_name);
                    token = strtok(temp_string, ".");
                    strcpy(temp_id, token);
                    token = strtok(NULL, ".");
                    if(token == NULL)
                        continue;
                    strcpy(string, common_dir);
                    strcat(string, "/");
                    strcat(string, dirent->d_name); 
                    if( access( string, F_OK ) == -1 )
                    {
                        pid3 = 0;
                        pid3 = fork();
                        if(pid3 == 0)
                        {
                            strcpy(string, mirror_dir);
                            strcat(string, "/");
                            strcat(string, dirent->d_name);
                            if(remove(string) != 0)
                                exit(-1);
                            strcpy(string, mirror_dir);
                            strcat(string, "/");
                            strcat(string, temp_id);
                            delete_mirror(string);
                            exit(0);
                        }
                        waitpid(pid3, &status, 0);
                        explain_wait_status(status, pid3);
                    }
                }
            }
            closedir(dir);
        }
        
        //sleep(5);
    }
    close(fd3);
    // rmdir(mirror_dir);
    // if(opendir(common_dir))
    //     rmdir(common_dir);
    
    return 1;    
}
