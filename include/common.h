#pragma once
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define NEW_CONECTION 1
#define NEW_POST 2
#define TOPICS_LIST 3
#define SUBSCRIBE 4
#define DISCONNECT 5
#define UNSUBSCRIBE 6
#define MAX_USERS 10
#define ERROR -1

struct BlogOperation {
    int client_id;
    int operation_type;
    int server_response;
    char topic[50];
    char content[2048];
};

void logexit(char *msg);
int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);
int server_sockaddr_init(const char *proto, const char *portstr, struct sockaddr_storage *storage);
size_t receive_all(int socket, void *buffer, size_t size);
struct BlogOperation createBlogOperation(int client_id, int operation_type, int server_response, char* topic, char* content);
