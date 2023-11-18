#include "common.h"

pthread_t waitingThread;

int inputCommand(char *cmd, char *content){
    cmd[strlen(cmd) - 1] = '\0';
    char * temp = malloc(sizeof(char) * 2048);
    strcpy(temp, cmd);
    char *command = strtok(temp, " ");
    //char *content = " ";
    if(strcmp(command, "publish") == 0){
        if (strcmp(strtok(NULL, " "), "in"))
        {
            return ERROR;
        }
        
        strcpy(content, temp + 11);
        return NEW_POST;
    }else if(strcmp(command, "list") == 0){
        if (strcmp(strtok(NULL, " "), "topics"))
        {
            return ERROR;
        }
        return TOPICS_LIST;
    }else if(strcmp(command, "subscribe") == 0){
        strcpy(content, temp + 10);
        printf("%s\n", content);
        return SUBSCRIBE;

    }else if(strcmp(command, "exit") == 0){
        return DESCONTECT_FROM_SERVER;

    }else if(strcmp(command, "unsubscribe") == 0){
        strcpy(content, temp + 15);
        return UNSUBSCRIBE;
    }else{
        return -1;
    }

}

void serverResponseHandler(struct BlogOperation serverResponse){
    if(serverResponse.operation_type == TOPICS_LIST || serverResponse.operation_type == SUBSCRIBE || serverResponse.operation_type == UNSUBSCRIBE || serverResponse.operation_type == ERROR){
        printf("%s\n", serverResponse.content);
    }else if(serverResponse.operation_type == NEW_POST){
        printf("new post added in %s by %02d\n%s", serverResponse.topic, serverResponse.client_id, serverResponse.content);
    }else if(serverResponse.operation_type == DESCONTECT_FROM_SERVER){
        printf("exit\n");
    }
}
void *serverData(void *data){
    int *sockfd = (int *)data;
    while(1){
        struct BlogOperation serverResponse;
        receive_all(*sockfd, &serverResponse, sizeof(struct BlogOperation));
        serverResponseHandler(serverResponse);
        if(serverResponse.operation_type == DESCONTECT_FROM_SERVER){
            close(*sockfd);
            break;
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char **argv){
    if(argc != 3){
        printf("Usage: ./server <ip> <port>\n");
        exit(1);
    }
    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage)) {
        logexit("addparse");
    }
    int sockfd = socket(storage.ss_family, SOCK_STREAM, 0);
    if (sockfd == -1) {
        logexit("socket");
    }
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != connect(sockfd, addr, sizeof(storage))) {
        logexit("connect");
    }

    struct BlogOperation req = createBlogOperation(0, NEW_CONECTION, 0, " ", " ");
    int count = send(sockfd, &req, sizeof(req), 0); // send client_operation to server
    if(count != sizeof(req)) logexit("send");
    struct BlogOperation res;
    receive_all(sockfd, &res, sizeof(res)); // receive server response
    int currentId = 0;
    currentId = res.client_id;
    printf("client %02d connected\n", currentId + 1);
    pthread_create(&waitingThread, NULL, (void*) &serverData, (void*) &sockfd);

    char cmd[2048];

    while(1){
        fgets(cmd, 2048, stdin);
        char * topic = malloc(sizeof(char) * 2048);
        int command = inputCommand(cmd, topic);
        switch(command){
            case NEW_POST:
                fgets(cmd, 2048, stdin);
                char content[2048];
                strcpy(content, cmd);
                req = createBlogOperation(currentId, command, 0, topic, content);
                break;
            case TOPICS_LIST:
                req = createBlogOperation(currentId, command, 0, " ", " ");
                break;
            case SUBSCRIBE:
                req = createBlogOperation(currentId, command, 0, topic, " ");
                break;
            case UNSUBSCRIBE:
                req = createBlogOperation(currentId, command, 0, topic, " ");
                break;
            case DESCONTECT_FROM_SERVER:
                req = createBlogOperation(currentId, command, 0, " ", " ");
                break;
            case ERROR:
                printf("Invalid command\n");
                break;
        }
        if(command != ERROR){
            count = send(sockfd, &req, sizeof(req), 0); // send req to server
            //printBlogOperation(req);
            if(count != sizeof(req)) logexit("send");
        }
        
    }
}