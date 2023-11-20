#include "common.h"
#include "server.h"

sem_t mutex;
struct Blog MediumBlog;

struct Client newClient(int id, int sock){
    struct Client client;
    client.id_client = id;
    client.sock = sock;
    return client;
}

struct Post newPost(int author, char* content){
    struct Post post;
    post.author = author;
    strcpy(post.content, content);
    return post;
}

struct Topic newTopic(int id_topic, char* name){
    struct Topic topic;    
    topic.id_topic = id_topic;
    topic.subscribers_count = 0;
    topic.posts_count = 0;
    strcpy(topic.name, name);
    for(int i = 0; i < 10; i++){
        topic.subscribers[i].id_client = 0;
    }
    return topic;
}

void newBlog(){
    MediumBlog.topics_count = 0;
    MediumBlog.clients_count = 0;
    for(int i = 0; i < 10; i++){
        MediumBlog.bool_clients[i] = 0;
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
            request.operation_type = DISCONNECT;
            request.client_id = clientData->id_client;
        }
        operationType(request);
        if(request.operation_type == DISCONNECT){
            close(csock);
            break;
        }
    }
    pthread_exit(EXIT_SUCCESS);
}

void operationType(struct BlogOperation clientRequest){
    struct BlogOperation serverResponse = createBlogOperation(clientRequest.client_id, 0, 0, " ", " ");
    int topic_id = 0;
        switch ((clientRequest.operation_type))
        {
        case NEW_POST:
            topic_id = lookForTopic(clientRequest.topic);
            if(topic_id != -1){
                MediumBlog.topics[topic_id].posts[MediumBlog.topics[topic_id].posts_count] = newPost(clientRequest.client_id, clientRequest.content);
                MediumBlog.topics[topic_id].posts_count++;
                printf("new post added in %s by %02d\n", clientRequest.topic, clientRequest.client_id + 1);
                for(int i = 0; i < MAX_USERS; i++){
                    if(MediumBlog.topics[topic_id].bool_subscribers[i] == 1){
                        serverResponse = createBlogOperation(clientRequest.client_id + 1, 2, 1, clientRequest.topic, clientRequest.content);
                        size_t count_bytes_sent = send(MediumBlog.clients[i].sock, &serverResponse, sizeof(struct BlogOperation), 0);
                        if(count_bytes_sent != sizeof(struct BlogOperation)) logexit("send");
                    }
                }
            }
            else{
                
                MediumBlog.topics[MediumBlog.topics_count] = newTopic(MediumBlog.topics_count, clientRequest.topic);
                MediumBlog.topics[MediumBlog.topics_count].posts[0] = newPost(clientRequest.client_id, clientRequest.content);
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

            int count_bytes_send = send(MediumBlog.clients[clientRequest.client_id].sock, &serverResponse, sizeof(struct BlogOperation), 0);
            if(count_bytes_send != sizeof(struct BlogOperation)){
                logexit("send");
            }

            break;

        case SUBSCRIBE:
            topic_id = lookForTopic(clientRequest.topic);
            if(topic_id != -1){ // topic exists
                if(MediumBlog.topics[topic_id].bool_subscribers[clientRequest.client_id] == 1){ // client already subscribed
                    serverResponse = createBlogOperation(clientRequest.client_id, -1, 1, " ", "error: already subscribed\n");
                    int count_bytes_send = send(MediumBlog.clients[clientRequest.client_id].sock, &serverResponse, sizeof(struct BlogOperation), 0);
                    if(count_bytes_send != sizeof(struct BlogOperation)){
                        logexit("send");
                    }
                }else{
                    //MediumBlog.topics[topic_id].subscribers[MediumBlog.topics[topic_id].subscribers_count].id_client = clientRequest.client_id;
                    MediumBlog.topics[topic_id].bool_subscribers[clientRequest.client_id] = 1;
                    MediumBlog.topics[topic_id].subscribers_count++;
                    printf("client %02d subscribed to %s\n", clientRequest.client_id + 1, clientRequest.topic);
                }
            }
            else{
                MediumBlog.topics[MediumBlog.topics_count] = newTopic(MediumBlog.topics_count, clientRequest.topic);
                MediumBlog.topics[MediumBlog.topics_count].bool_subscribers[clientRequest.client_id] = 1;
                MediumBlog.topics[MediumBlog.topics_count].subscribers_count++;
                MediumBlog.topics_count++;
                printf("client %02d subscribed to %s\n", clientRequest.client_id + 1, clientRequest.topic);
            }
            break;

        case DISCONNECT:
            printf("client %02d disconnected\n", clientRequest.client_id + 1);
            MediumBlog.bool_clients[clientRequest.client_id] = 0;
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
                if(MediumBlog.topics[topic_id].bool_subscribers[clientRequest.client_id] == 1){
                    MediumBlog.topics[topic_id].bool_subscribers[clientRequest.client_id] = 0;
                    MediumBlog.topics[topic_id].subscribers_count--;
                    printf("client %02d unsubscribed from %s\n", clientRequest.client_id + 1, clientRequest.topic);
                }
            }
            break;
        }
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

    newBlog();

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
            if(MediumBlog.bool_clients[i] == 0){
                index = i;
                clientRequest.client_id = i;
                MediumBlog.clients[i] = newClient(clientRequest.client_id, csock);
                MediumBlog.bool_clients[i] = 1;
                MediumBlog.clients_count++;
                printf("client %02d connected\n", MediumBlog.clients[i].id_client + 1);
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

