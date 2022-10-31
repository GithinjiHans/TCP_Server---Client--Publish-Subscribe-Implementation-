#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
// #include <unistd.h>
struct clients_data {
  char client_name[1024];
  int client_sockets;
};

struct topics {
  char topic[1024];
  int sockets[1024];
  int sockets_count;
};
struct clients_data clients[1024];
struct topics topic_list[1024];

int clients_connected = 0;
int pub_operations = 0;
int sub_operations = 0;
int unsub_operations = 0;
int completed_clients = 0;
int topics_count = 0;

// create a mutex lock
pthread_mutex_t lock;
pthread_mutex_t m1;
pthread_mutex_t m2;
pthread_mutex_t m3;
pthread_mutex_t m4;
pthread_mutex_t m5;

void subscription_handler(char *token, int client_socket) {
  //  check if the topic is already in the topic list
  int topic_exists = 0;
  int client_exists = 0;
  int client_is_named =1;
   for(int l=0;l < clients_connected;l++){
    if(client_socket == clients[l].client_sockets){
            if(strcmp(clients[l].client_name,"")==0){
              client_is_named =0;
              break;
            }
            }
          }
  for (int i = 0; i < topics_count; i++) {
    if (strcmp(token, topic_list[i].topic) == 0) {
      // if the topic is already in the topic list then add the client
      // to the list of clients that are interested in that topic
      for (int j = 0; j < topics_count; j++) {
        if (strcmp(token, topic_list[j].topic) == 0) {
          // add the client to the list of clients that are interested
          // in that topic
          // if the client is already in the list then don't add it
          for (int k = 0; k < topic_list[j].sockets_count; k++) {
            if (client_socket == topic_list[j].sockets[k]) {
              client_exists = 1;
              break;
            }
          }
          if (client_exists == 0 && client_is_named == 1) {
            // loop and get a position where the socket is 0
            for (int l = 0; l <= topic_list[j].sockets_count; l++) {
              if (topic_list[j].sockets[l] == 0) {
                topic_list[j].sockets[l] = client_socket;
                topic_list[j].sockets_count++;
                  sub_operations++;
                break;
              }
            }
          }
          break;
        }
      }
      topic_exists = 1;
      break;
    }
  }
  // if the topic is not in the topic list then add it to the topic list
  if (topic_exists == 0 && client_is_named == 1) {
    // add the topic to the topic struct
    strcpy(topic_list[topics_count].topic, token);
    topic_list[topics_count].sockets[topic_list[topics_count].sockets_count] =
        client_socket;
    topic_list[topics_count].sockets_count++;
      sub_operations++;
    topics_count++;
  }
}
void unsubscription_handler(char *token, int client_socket) {
  // check if the topic is in the topic list
  int topic_exists = 0;
  int is_client_subscribed = 0;
  for (int i = 0; i < topics_count; i++) {
    if (strcmp(token, topic_list[i].topic) == 0) {
      // if the topic is in the topic list then remove the client from the list
      // of clients that are interested in that topic
      for (int j = 0; j < topic_list[i].sockets_count; j++) {
        if (client_socket == topic_list[i].sockets[j]) {
          topic_list[i].sockets[j] = -1;
          unsub_operations++;
          is_client_subscribed = 1;
          // send(client_socket,token,strlen(token),0);
          // rearrange the array
          for (int k = j; k < topic_list[i].sockets_count; k++) {
            topic_list[i].sockets[k] = topic_list[i].sockets[k + 1];
          }
          topic_list[i].sockets_count--;
          break;
        }
      }
      // if (is_client_subscribed == 0) {
      //   send(client_socket, token, strlen(token), 0);
      // }
      topic_exists = 1;
      break;
    }
  }
}

void unsub_handler(int client_fd) {
  // remove the client from all the topics
  for (int i = 0; i < topics_count; i++) {
    for (int j = 0; j < topic_list[i].sockets_count; j++) {
      if (client_fd == topic_list[i].sockets[j]) {
        topic_list[i].sockets[j] = -1;
        // printf("unsubscribed");
        unsub_operations++;
        // rearrange the array
        for (int k = j; k < topic_list[i].sockets_count; k++) {
          topic_list[i].sockets[k] = topic_list[i].sockets[k + 1];
        }
        topic_list[i].sockets_count--;
        break;
      }
    }
  }
}

void publication_handler(char *token, int client_fd, char *message,
                         char *client_name) {
  // check if the topic is in the topic list
  int topic_exists = 0;
  for (int i = 0; i < topics_count; i++) {
    if (strcmp(token, topic_list[i].topic) == 0) {
        // if the client has subscribed to the topic then send the message to
        // all the clients that have subscribed to the topic
          for (int k = 0; k < topic_list[i].sockets_count; k++) {
            if (send(topic_list[i].sockets[k], message, strlen(message), 0) ==
                -1) {
              break;
            }
          }
      // delete the message
      memset(message, 0, strlen(message));
      topic_exists = 1;
      pub_operations++;
    }
  }
  if (topic_exists == 0) {
    // if the client has a name then increment the number of publications
    // that the client has made
    for (int i = 0; i < clients_connected; i++) {
      if (client_fd == clients[i].client_sockets) {
       if(strcmp(clients[i].client_name,"")!=0){
        pub_operations++;
       }
        break;
      }
    }
    // delete the message
    memset(message, 0, strlen(message));
  }
}

void client_naming(int client_fd, char *client_name) {
  // check if the client has already been named
  // printf("client name is %s and client fd is %d \n", client_name, client_fd);
  int client_exists = 0;
  int name_exists = 0;
  int client_named = 0;
  // check if the name eists
  for (int i = 0; i < clients_connected; i++) {
    if (strcmp(client_name, clients[i].client_name) == 0) {
      name_exists = 1;
      break;
    }
  }
  // cheeck if the client exists
    for (int i = 0; i < clients_connected; i++) {
      if (client_fd == clients[i].client_sockets) {
        client_exists = 1;
        break;
      }
    }
  if(name_exists ==1 && client_exists == 0){
      clients[clients_connected].client_sockets = client_fd;
      strcpy(clients[clients_connected].client_name,"");
      // printf("Hello");
          }
  if (name_exists == 0) {
    // if the client does not exist then add it to the client list
    if (client_exists == 0) {
      clients[clients_connected].client_sockets = client_fd;
      fprintf(stderr, "name %s\n", client_name);
      strcpy(clients[clients_connected].client_name, client_name);
      client_named = 1;
    }
    // if the client exists then update the name
    if (client_exists == 1) {
      for (int i = 0; i < clients_connected; i++) {
        if (client_fd == clients[i].client_sockets) {
          //  if the current name is blank
          if (strcmp(clients[i].client_name, "\0")==0 ||
              strcmp(clients[i].client_name, " ") == 0 ||
              strcmp(clients[i].client_name, "\n") == 0 ||
              strcmp(clients[i].client_name, "") == 0) {
            strcpy(clients[i].client_name, client_name);
            // printf("We'll thenks for naming this\n");
            client_named = 1;
          }
          break;
        }
      }
    }
  }
}

void client_remove(int client_fd) {
  // remove the client from the clients struct
  for (int i = 0; i < clients_connected; i++) {
    if (client_fd == clients[i].client_sockets) {
      clients[i].client_sockets = -1;
      // rearrange the array
      for (int j = i; j < clients_connected; j++) {
        clients[j].client_sockets = clients[j + 1].client_sockets;
        strcpy(clients[j].client_name, clients[j + 1].client_name);
      }
      clients_connected--;
    }
  }
}

void *handle_client(void *arg) {
  int client_socket = *(int *)arg;
  char buffer[1024];
  int read_size;
  //  the server should read from all the clients and print the message using
  while ((read_size = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
    buffer[read_size] = '\0';
    char message[1024];
    strcpy(message, buffer);
    // tokenize the buffer
    char *token = strtok(buffer, " ");
    // check if the first token is sub
    if (strcmp("sub", token) == 0) {
      pthread_mutex_lock(&m1);
      while (token != NULL) {
        token = strtok(NULL, " ");
        if (token != NULL && strcmp(token, " ") != 0) {
          // remove the newline character from the token
          token[strcspn(token, "\n")] = 0;
          // call the subscribe function to add the client to the topic
          subscription_handler(token, client_socket);
        }
      }
      pthread_mutex_unlock(&m1);
    }
    // check if the first token is unsub
    else if (strcmp("unsub", token) == 0) {
      pthread_mutex_lock(&m2);
      while (token != NULL) {
        token = strtok(NULL, " ");
        if (token != NULL && strcmp(token, " ") != 0) {
          // remove the newline character from the token
          token[strcspn(token, "\n")] = 0;
          // call the subscribe function to add the client to the topic
          unsubscription_handler(token, client_socket);
        }
      }
      pthread_mutex_unlock(&m2);
    } else if (strcmp("pub", token) == 0) {
      pthread_mutex_lock(&lock);
      char modified_message[1024];
      char client_name[1024];
      token = strtok(NULL, " ");
      // if the client has entered a topic
      if (token != NULL && strcmp(token, " ") != 0 &&
          strcmp(token, "\n") != 0) {
        // remove the space from the token
        token[strcspn(token, " ")] = 0;
        char *pub_message = message + strlen(token) + 5;
        // remove the space at the beginning of the message
        // if the client has entered a message
        if (pub_message == NULL || strcmp(pub_message, " ") == 0 ||
            strcmp(pub_message, "\n") == 0 || strcmp(pub_message, "") == 0) {
          send(client_socket, ":invalid\n", strlen(":invalid\n"), 0);
          // exit(0);
        } else {
          // remove the pub keyword and the current topic from the buffer
          // remove the newline character from the token
          token[strcspn(token, "\n")] = 0;
          // call the subscribe function to add the client to the topic
          for (int k = 0; k < clients_connected; k++) {
            if (client_socket == clients[k].client_sockets) {
              strcpy(client_name, clients[k].client_name);
              break;
            }
          }
          strcat(modified_message, client_name);
          strcat(modified_message, ":");
          strcat(modified_message, token);
          strcat(modified_message, ":");
          // pub_message[strcspn(pub_message," ")]=0
          strcat(modified_message, pub_message);
          publication_handler(token, client_socket, modified_message,
                              client_name);
        }
        // clear the buffer and the message
        memset(message, 0, strlen(message));
        memset(modified_message, 0, strlen(modified_message));
        memset(pub_message, 0, strlen(pub_message));
        memset(buffer, 0, strlen(buffer));
      } else {
        send(client_socket, ":invalid\n", strlen(":invalid\n"), 0);
      }
      pthread_mutex_unlock(&lock);
    }  else if (strcmp("name", token) == 0) {
      pthread_mutex_lock(&m4);
      // get the client name
      token = strtok(NULL, " ");
      // if the token is blank
      if (strcmp("", token) == 0 || strcmp(" ", token) == 0 ||
          strcmp("\n", token) == 0) {
        send(client_socket, ":invalid\n", strlen(":invalid\n"), 0);
      } else {
        // remove the newline character from the token
        token[strcspn(token, "\n")] = 0;
        client_naming(client_socket, token);
      }
      pthread_mutex_unlock(&m4);
    }else if(strcmp("open\n", token) == 0){

    } 
    else if(strcmp("close\n",token)==0){
         client_remove(client_socket);
         completed_clients++;
    }else {
      pthread_mutex_lock(&m5);
      // send a message to the client that the command is not valid
      char *invalid_command = ":invalid\n";
      // fprintf(stderr, "%s", invalid_command);
      if (send(client_socket, invalid_command, strlen(invalid_command), 0) ==
          -1) {
        break;
      }
      //  delete everything from the buffer
      memset(buffer, 0, strlen(buffer));
      pthread_mutex_unlock(&m5);
    }
  }
  if (read_size == 0) {
    client_remove(client_socket);
    completed_clients++;
  }
  return NULL;
}
void* readstdin(){
     char buf[1024];
    // while EOF is not reached we read from stdin and send to server
   while (
    fgets(buf, 1024, stdin)
   )
    {
      if (strcmp("sighup\n", buf) == 0) {
      // remove the new line character from the token
      fprintf(stderr,
              "Connected clients: %d\nCompleted clients: %d\npub operations: "
              "%d\nsub operations: %d\nunsub operations: %d\n",
              clients_connected, completed_clients, pub_operations,
              sub_operations, unsub_operations);
      // fflush(stderr);
    }
    }
    return NULL;
}
int main(int argc, char **argv) {
  // Command line input error checking
  if (argc > 1) {
    for (int i = 0; i < strlen(argv[1]); i++) {
      if (argv[1][i] < '0' || argv[1][i] > '9') {
        fprintf(stderr, "Usage: psserver connections [portnum]\n");
        exit(1);
      }
    }
  }

  if (argc < 2 || strlen(argv[1]) == 0 ||
      (strchr(argv[1], '0') == NULL && atoi(argv[1]) == 0) ||
      atoi(argv[1]) < 0 || argc > 3) {
    fprintf(stderr, "Usage: psserver connections [portnum]\n");
    exit(1);
  }
  // argv[2] is the port number and is optional
  if (argc == 3) {
    // if the port number is not a number then exit
    for (int i = 0; i < strlen(argv[2]); i++) {
      if (argv[2][i] < '0' || argv[2][i] > '9') {
        fprintf(stderr, "Usage: psserver connections [portnum]\n");
        exit(1);
      }
    }
    // if the port number is not in the range of 1024-65535 then exit
    if (atoi(argv[2]) != 0 && (atoi(argv[2]) < 1024 || atoi(argv[2]) > 65535)) {
      fprintf(stderr, "Usage: psserver connections [portnum]\n");
      exit(1);
    }
  }

  // implementation
  int server_port = 0;
  int max_connections = atoi(argv[1]);
  // if the port number is not specified then use an ephemeral port which means
  // the OS will assign a port number for us if the port number is specified
  // then use that port number
  if (argc == 2) {
    server_port = 0;
  } else {
    server_port = atoi(argv[2]);
  }
  // create a socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  // if the socket creation fails then exit
  if (sockfd < 0) {
    // fprintf(stderr,"psserver: socket creation failed\n");
    return -1;
  }

  // create a socket address for the server
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(server_port);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  // bind the socket to the socket address
  if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    fprintf(stderr, "psserver: unable to open socket for listening\n");
    exit(2);
  }
  // listen for connections that are coming in
  // printf("%d\n",listen(sockfd, max_connections));
  if (listen(sockfd, max_connections) < 0) {
    fprintf(stderr, "psserver: unable to open socket for listening\n");
    // flush the output buffer
    fflush(stderr);
    exit(2);
  } else {
    // print the port number that the server is listening on
    struct sockaddr_in server_addr;
    socklen_t server_addr_len = sizeof(server_addr);
    getsockname(sockfd, (struct sockaddr *)&server_addr, &server_addr_len);
    fprintf(stderr, "%d\n", ntohs(server_addr.sin_port));
    // flush the output buffer
    fflush(stderr);
  }
  pthread_t thread_s;
  pthread_create(&thread_s, NULL, readstdin, NULL);
  pthread_detach(thread_s);
  while (1) {
    // accept a connection from a client
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd =
        accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
    // if the connection fails then exit
    if (client_fd < 0) {
      // flush the output buffer
      return -1;
    }
    // if the number of connections is less than the max_connections then accept
    // the connection
    if (clients_connected < max_connections) {
      // the client will send its name to the server so we'll receive the name
      // from the client
      char message[1024];
      memset(message, 0, sizeof(message));
      int bytes_received = recv(client_fd, message, sizeof(message), 0);
      // once the message is received, we'll tokenize the message to get the
      // client name which is the second token
      char *token = strtok(message, " ");
      // strcpy(clients[clients_connected].client_name, token);
      // clients[clients_connected].client_sockets = client_fd;
      fflush(stdout);
      client_naming(client_fd, token);
      clients_connected++;
      fflush(stdout);
      //  the other tokens are the topics that the client is interested in
      while (token != NULL) {
        token = strtok(NULL, " ");
        if (token != NULL) {
          // call the subscribe function to add the client to the topic
          subscription_handler(token, client_fd);
        }
      }
      //  print the topics and the clients that are interested in those topics
      // create a thread to handle the client
      pthread_t thread;
      if (pthread_create(&thread, NULL, handle_client, (void *)&client_fd) <
          0) {
        fprintf(stderr, "psserver: unable to create thread\n");
        fflush(stderr);
        return -1;
      }
    }
  }
  return 0;
}