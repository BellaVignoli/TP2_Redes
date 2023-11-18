#include "common.h"
#include "server.h"

sem_t mutex;
struct Blog MediumBlog;

struct Client initClient(int id, int sock){
    struct Client client;
    client.id_client = id;
    client.sock = sock;
    return client;
}

struct Post initPost(int author, char* content){
    struct Post post;
    post.author = author;
    strcpy(post.content, content);
    return post;
}

struct Topic initTopic(int id_topic, char* name){
    struct Topic topic;    
    topic.id_topic = id_topic;
    topic.subscribers_count = 0;
    topic.posts_count = 0;
    strcpy(topic.name, name);
    return topic;
}

void initBlog(){
    MediumBlog.topics_count = 0;
    MediumBlog.clients_count = 0;
    for(int i = 0; i < 10; i++){
        MediumBlog.clients[i].id_client = -1;
    }
}

int lookForTopic(char* topic){
    //printf("%d\n", MediumBlog.topics_count);
    for(int i = 0; i < MediumBlog.topics_count; i++){
        if(strcmp(MediumBlog.topics[i].name, topic) == 0){
            return i;
        }
    }
    return -1;
}

void* threadsClient(void* clientThread)
{
    struct Client *clientData = (struct Client*) clientThread;
    int csock = clientData->sock;
    struct BlogOperation request;
    while(true){
        size_t count_bytes_received = receive_all(csock, &request, sizeof(struct BlogOperation));
        if(count_bytes_received == 0){
            request.operation_type = DESCONTECT_FROM_SERVER;
        }
        struct BlogOperation response = operationType(request);
        count_bytes_received = send(csock, &response, sizeof(struct BlogOperation), 0);
        if(count_bytes_received != sizeof(struct BlogOperation)){
            logexit("send");
        }
        if(request.operation_type == DESCONTECT_FROM_SERVER){
            close(csock);
            break;
        }
    }
    pthread_exit(EXIT_SUCCESS);
}

struct BlogOperation operationType(struct BlogOperation clientRequest){
    struct BlogOperation serverResponse = createBlogOperation(clientRequest.client_id, 0, 0, " ", " ");
    int topic_id = 0;
        switch ((clientRequest.operation_type))
        {
        case NEW_POST:
            topic_id = lookForTopic(clientRequest.topic);
            if(topic_id != -1){
                MediumBlog.topics[topic_id].posts[MediumBlog.topics[topic_id].posts_count] = initPost(clientRequest.client_id, clientRequest.content);
                MediumBlog.topics[topic_id].posts_count++;
                printf("new post added in %s by %02d\n", clientRequest.topic, clientRequest.client_id);
                for(int i = 0; i < MAX_USERS; i++){
                    if(MediumBlog.topics[topic_id].subscribers[i].id_client != 0){
                        serverResponse = createBlogOperation(clientRequest.client_id, 2, 1, clientRequest.topic, clientRequest.content);
                        size_t count_bytes_sent = send(MediumBlog.topics[topic_id].subscribers[i].sock, &serverResponse, sizeof(struct BlogOperation), 0);
                        if(count_bytes_sent != sizeof(struct BlogOperation)) logexit("send");
                    }
                }
            }
            else{
                
                MediumBlog.topics[MediumBlog.topics_count] = initTopic(MediumBlog.topics_count, clientRequest.topic);
                MediumBlog.topics[MediumBlog.topics_count].posts[0] = initPost(clientRequest.client_id, clientRequest.content);
                MediumBlog.topics_count++;
                MediumBlog.topics[MediumBlog.topics_count].posts_count++;
                printf("new post added in %s by %02d\n", clientRequest.topic, clientRequest.client_id + 1);
            }
            break;

        case TOPICS_LIST:
            if(MediumBlog.topics_count == 0){
                serverResponse = createBlogOperation(clientRequest.client_id, TOPICS_LIST, 1, " ", "no topics available");
            }
            else{
                char* topics = malloc(sizeof(char) * 1024);
                for(int i = 0; i < MediumBlog.topics_count; i++){
                    strcat(topics, MediumBlog.topics[i].name);
                    if(i != MediumBlog.topics_count - 1) strcat(topics, "; ");
                }
                serverResponse = createBlogOperation(clientRequest.client_id, TOPICS_LIST, 1, " ", topics);
            }

            break;

        case SUBSCRIBE:
            topic_id = lookForTopic(clientRequest.topic);
            if(topic_id != -1){
                if((MediumBlog.topics[topic_id].subscribers[MediumBlog.topics[topic_id].subscribers_count].id_client) = clientRequest.client_id){
                    serverResponse = createBlogOperation(clientRequest.client_id, -1, 1, " ", "error: already subscribed\n");
                }else{
                    MediumBlog.topics[topic_id].subscribers[MediumBlog.topics[topic_id].subscribers_count].id_client = clientRequest.client_id;
                    MediumBlog.topics[topic_id].subscribers_count++;
                    printf("client %02d subscribed to %s\n", clientRequest.client_id + 1, clientRequest.topic);
                }
            }
            else{
                MediumBlog.topics_count++;
                MediumBlog.topics[MediumBlog.topics_count] = initTopic(MediumBlog.topics_count, clientRequest.topic);
                MediumBlog.topics[MediumBlog.topics_count].subscribers[MediumBlog.topics[MediumBlog.topics_count].subscribers_count].id_client = clientRequest.client_id;
                MediumBlog.topics[MediumBlog.topics_count].subscribers_count++;
                printf("client %02d subscribed to %s\n", clientRequest.client_id + 1, clientRequest.topic);
            }
            break;

        case DESCONTECT_FROM_SERVER:
            MediumBlog.clients[clientRequest.client_id].id_client = -1;
            MediumBlog.clients_count--;
            for(int i = 0; i < MediumBlog.topics_count; i++){
                for(int j = 0; j < MediumBlog.topics[i].subscribers_count; j++){
                    if(MediumBlog.topics[i].subscribers[j].id_client == clientRequest.client_id){
                        MediumBlog.topics[i].subscribers[j].id_client = -1;
                        MediumBlog.topics[i].subscribers_count--;
                    }
                }
            }
            break;

        case UNSUBSCRIBE:
            topic_id = lookForTopic(clientRequest.topic);
            if(topic_id != -1){
                if((MediumBlog.topics[topic_id].subscribers[MediumBlog.topics[topic_id].subscribers_count].id_client) = clientRequest.client_id){
                    MediumBlog.topics[topic_id].subscribers[MediumBlog.topics[topic_id].subscribers_count].id_client = -1;
                    MediumBlog.topics[topic_id].subscribers_count--;
                    printf("client %02d unsubscribed from %s\n", clientRequest.client_id + 1, clientRequest.topic);
                }else{
                    serverResponse = createBlogOperation(clientRequest.client_id, -3, 1, " ", "error: not subscribed\n");
                }
            }
            else{
                serverResponse = createBlogOperation(clientRequest.client_id, -4, 1, " ", "error: topic does not exist\n");
            }
            break;
        }
    return serverResponse;
}

int main(int argc, char *argv[]){
    if(argc != 3){
        printf("Usage: ./server <ip> <port>\n");
        exit(1);
    }
    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) {
        logexit("serverSockaddrInitt");
    }
    int sockfd = socket(storage.ss_family, SOCK_STREAM, 0);
    if (sockfd == -1) {
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(sockfd, addr, sizeof(storage))) {
        logexit("bind");
    }

    if (0 != listen(sockfd, 10)) {
        logexit("listen");
    }

    initBlog();

    while(1){
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(sockfd, caddr, &caddrlen);
        if (csock == -1) {
            logexit("accept");
        }
        struct BlogOperation clientRequest;
        int index = -1;
        for(int i = 0; i < 10; i++){
            //printf("%d\n", MediumBlog.clients[i].id_client);
            if(MediumBlog.clients[i].id_client == -1){
                index = i;
                clientRequest.client_id = index;
                MediumBlog.clients[index] = initClient(clientRequest.client_id, csock);
                MediumBlog.clients_count++;
                printf("client %02d connected\n", MediumBlog.clients[index].id_client + 1);
                break;
            }
        }
        struct BlogOperation res = createBlogOperation(clientRequest.client_id, NEW_CONECTION, 1, " ", " ");
        //printf("%d\n", res.server_response);
        size_t count_bytes_sent = send(csock, &res, sizeof(struct BlogOperation), 0);
        if(count_bytes_sent != sizeof(struct BlogOperation)) logexit("send");
        if(pthread_create(&MediumBlog.clients[index].thread, NULL, threadsClient, &MediumBlog.clients[index]) != 0){
            logexit("pthread_create");
        }
    }

    for(int i = 0; i < 10; i++){
        pthread_join(MediumBlog.clients[i].thread, NULL);
    }
}

