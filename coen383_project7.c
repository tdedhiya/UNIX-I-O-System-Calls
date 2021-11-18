#include <stdio.h> 
#include <unistd.h>
#include <sys/types.h> 
#include <string.h> 
#include <sys/time.h> 
#include <stdlib.h>
#include <stdlib.h> 
#include <fcntl.h> 
#include <sys/types.h> 
#include <sys/ioctl.h> 
#include <time.h>

#define BUFFER_SIZE 1024 
#define READ_END     0 
#define WRITE_END    1 
#define CHILD_NUM 5

void writeTofile(char *message) {
    FILE *outFD = fopen("output.txt", "a+");

    if(outFD == NULL) {
        printf("Error while opening output file for writing\n");
        return;
    }

    fprintf(outFD, "%s", message);
    fclose(outFD);
}

int main(void) 
{ 
    char read_msg[BUFFER_SIZE]; 
     
    pid_t pid;  // child process id 
    fd_set pipes;  // file descriptors for the pipe 
    int fd_list[5];
    int fd[2];
    int highestFD;  //highest file descriptor
    FD_ZERO(&pipes);     // initialize inputs to the empty set 
    
    int i;
    // Fork a child process. 
    for (i= 0; i < CHILD_NUM; ++i) {
        if (pipe(fd) == -1) { 
            fprintf(stderr,"pipe() failed"); 
            return 1; 
        }
        srand(i); //make each child process have a different random sequence
        pid = fork(); 
        if (pid > 0) {   
            // PARENT PROCESS – read from the pipe.
            close(fd[WRITE_END]);  //neer need to write
            FD_SET(fd[READ_END], &pipes);   // set file descriptor 
            if (fd[READ_END] >= highestFD) highestFD = fd[READ_END] + 1;
            fd_list[i] = fd[READ_END];
        } 
        else if (pid == 0) {  
            // CHILD PROCESS. - write to the pipe
            close(fd[READ_END]); //never need to read
            sleep(1); //wait for all processes to be initialized
            break;
        } 
        else { 
            fprintf(stderr, "fork() failed"); 
            return 1; 
        } 
    }
    //get start time
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    int start_sec = (int) current_time.tv_sec;
    double start_msec = (double)(current_time.tv_usec)/1000;
    int current_sec = start_sec;
    double current_msec = start_msec;
    if (i < CHILD_NUM) { //if a child
        //children
        char write_msg[BUFFER_SIZE]; 
        int message_count = 1;
        char current_message[200];
        while (current_sec - start_sec - (current_msec > start_msec ? 0 : 1) < 30) { //while 30 seconds havent passed
            if (i < 4) {
                sleep(rand() % 4); //sleep for a random amount of time from 0 - 3 seconds
                sprintf(current_message, "Message %d", message_count); //generate a new message
                ++message_count;
            } else {
                scanf("%s", current_message); //wait for user input
            }
            //update current time
            gettimeofday(&current_time, NULL);
            current_sec = (int) current_time.tv_sec;;
            current_msec = (double)(current_time.tv_usec)/1000;
            sprintf(write_msg, "%d:%f:Child %d %s", current_sec - start_sec - (current_msec > start_msec ? 0 : 1), current_msec > start_msec ? current_msec - start_msec : current_msec + (1000 - start_msec), i, current_message); //append timestamp to message and adjust for start time
            write(fd[WRITE_END], write_msg, strlen(write_msg) + 1); //send message
        }
        close(fd[WRITE_END]); 
    } else {       
    	//parent process
        // 2.5 seconds. 
        struct timeval timeout;
        timeout.tv_sec = 2; 
        timeout.tv_usec = 500000;
        fd_set inputfds;
        int result, nread;
        char buffer[2048];
        int closed[5] = {0, 0, 0, 0, 0}; //list of closed pipe flags
        int childCount = CHILD_NUM;
        while(childCount > 0)  { //while still active children
            inputfds = pipes;
            
    
            // Get select() results.  
            result = select(highestFD, &inputfds, NULL, NULL, &timeout); 
    
            // Check the results. 
            //   No input:  the program loops again. 
            //   Error:     terminate. 
            //   Input: check through pipes for content
            switch(result) { 
                case 0: { break; }
                case -1: {    // error 
                    perror("select"); 
                    exit(1); 
                } 
    
                default: {    // Got input 
                            //check through file descriptors
                            for( int i = 0; i < CHILD_NUM; ++i ) {
                                //if find one that is not closed and has content
                                if (!closed[i] && FD_ISSET(fd_list[i], &inputfds)) {  
                                    ioctl(fd_list[i],FIONREAD, &nread); // read # of bytes available 
                                    nread = read(fd_list[i],buffer, nread);
                                    //if the pipe closed on the other end
                                    if (nread == 0) {
                                        closed[i] = 1;
                                        --childCount;
                                    } else {
                                        buffer[nread] = '\0';
                                        //get current time
                                        gettimeofday(&current_time, NULL);
                                        current_sec = (int) current_time.tv_sec;;
                                        current_msec = (double)(current_time.tv_usec)/1000;
                                        char timestamped[nread + 100];
                                        //append timestamp
                                        sprintf(timestamped, "%d:%f %s\n", current_sec - start_sec - (current_msec > start_msec ? 0 : 1), current_msec > start_msec ? current_msec - start_msec : current_msec + (1000 - start_msec), buffer); //append parrent timestamp to message and adjust for start time
                                        writeTofile(timestamped);
                                    }
                                    
                                }
                            }
                        
                        break; 
                } 
            } 
        }
        for (int i = 0; i < CHILD_NUM; ++i) {
            close(fd_list[i]); //close my read end
        }
    }
    
    return 0; 
} 
