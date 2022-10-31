#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

void *readstdin(void *arg)
{
    // this function is used to read from stdin and send to server
    int *sockfd = (int *)arg;
    char buf[1024];
    // while EOF is not reached we read from stdin and send to server
   while (fgets(buf, 1024, stdin) != NULL)
    {
        send(*sockfd, buf, strlen(buf), 0);
    }
    // when EOF is reached, close the socket
    close(*sockfd);
    exit(0);
}

void *readserver(void *arg)
{
    // this function is used to read from server and print to stdout
    int *sockfd = (int *)arg;
    char buf[1024];
    int buf_size;
    // while EOF is not reached we read from server and print to stdout
    while((buf_size=recv(*sockfd, buf, 1024, 0)) > 0){
        fprintf( stdout, "%s", buf);
        fflush(stdout);
        // clear the buffer
        memset(buf, 0, 1024);
    }
    if(buf_size ==0){
    // when EOF is reached we close the socket and exit
      fprintf(stderr, "psclient: server connection terminated\n");
      fflush(stderr);
    close(*sockfd);
    exit(4);
    }
    return NULL;
}




int main(int argc, char** argv){
if(argc < 3){
    fprintf(stderr, "Usage: psclient portnum name [topic] ...\n");
    exit(1);
}
if(strchr(argv[2], ' ') != NULL || strchr(argv[2], ':') != NULL || strchr(argv[2], '\n') != NULL || strlen(argv[2]) == 0){
    fprintf(stderr, "psclient: invalid name\n");
    exit(2);
}
for(int i = 3; i < argc; i++){
    if(strchr(argv[i], ' ') != NULL || strchr(argv[i], ':') != NULL || strchr(argv[i], '\n') != NULL || strlen(argv[i]) == 0){
        fprintf(stderr, "psclient: invalid topic\n");
        exit(2);
    }
}
int portnum = 0;
if(argv[1][0] >= '0' && argv[1][0] <= '9'){
    portnum = atoi(argv[1] );
}
// create a socket
int sockfd = socket(AF_INET, SOCK_STREAM, 0);
// set up the server address where we want to connect
struct sockaddr_in serveraddr;
serveraddr.sin_family = AF_INET;
serveraddr.sin_port = htons(portnum);
serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
// connect to the server
if(connect(sockfd, (struct sockaddr*) &serveraddr, sizeof(serveraddr)) < 0){
 fprintf(stderr,"psclient: unable to connect to port %d\n", portnum);
    exit(3);
}

// send the name and topics to the server as a single string
char buf[1024];
strcpy(buf, argv[2]);
for(int i = 3; i < argc; i++){
    strcat(buf, " ");
    strcat(buf, argv[i]);
}
write(sockfd, buf, strlen(buf));
// create two threads, one for reading from stdin and one for reading from server
pthread_t tid,tid2;
pthread_create(&tid, NULL, readstdin, (void*) &sockfd);
pthread_create(&tid2, NULL, readserver, (void*) &sockfd);
pthread_join(tid, NULL);
pthread_join(tid2, NULL);
return 0;
}