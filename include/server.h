#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>

#include "common.h"

struct Client{
    int id_client;
    int sock;
    pthread_t thread;
};

struct Post{
    int author;
    char content[2048];
};

struct Topic{
    int id_topic;
    char name[50];
    struct Client subscribers[10];
    int bool_subscribers[10];
    int subscribers_count;
    struct Post posts[100];
    int posts_count;
};

struct Blog{
    struct Topic topics[100];
    struct Client clients[10];
    int bool_clients[10];
    int topics_count;
    int clients_count;
};

struct Client newClient(int id, int sock);
struct Post newPost(int author, char* content);
struct Topic newTopic(int id, char* name);
void newBlog();
int lookForTopic(char* topic);
void* threadsClient(void* clientThread);
void operationType(struct BlogOperation clientRequest);

#endif