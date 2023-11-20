#include "common.h" 

void logexit(char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage){

    if( addrstr == NULL || portstr == NULL ) {
        return -1;
    }

    uint16_t port = (uint16_t)atoi(portstr);

    if (port == 0) {
        return -1;
    }

    port = htons(port); //host to network (litle endian)

    struct in_addr inaddr4; //32 bit IP Address
    if (inet_pton(AF_INET, addrstr, &inaddr4)) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4 -> sin_family = AF_INET;
        addr4 -> sin_port = port;
        addr4 -> sin_addr = inaddr4;
        return 0;
    }

    struct in6_addr inaddr6; //128 bit IP Address
    if (inet_pton(AF_INET6, addrstr, &inaddr6)) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6 -> sin6_family = AF_INET6;
        addr6 -> sin6_port = port;
        memcpy(&(addr6 -> sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;
    }
    return -1;
}

int server_sockaddr_init(const char *proto, const char *portstr, struct sockaddr_storage *storage){
    uint16_t port = (uint16_t)atoi(portstr);
    if (port == 0) {
        return -1;
    }
    port = htons(port);

    memset(storage, 0, sizeof(*storage));

    if(0 == strcmp(proto, "v4")){
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4 -> sin_family = AF_INET;
        addr4 -> sin_addr.s_addr = INADDR_ANY; //Qualquer add disponivel no computador
        addr4 -> sin_port = port;
        return 0;
    }

    else if(0 == strcmp(proto, "v6")){
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6 -> sin6_family = AF_INET6;
        addr6 -> sin6_addr = in6addr_any; //Qualquer add disponivel no computador v6
        addr6 -> sin6_port = port;
        return 0;
    }

    else{
        return -1;
    }

}

size_t receive_all(int socket, void *buffer, size_t size)
{
    size_t total_received = 0;
    while (total_received < size)
    {
        size_t bytes_received = recv(socket, buffer + total_received, size - total_received, 0);
        if (bytes_received <= 0)
        {
            if (bytes_received == 0)
                return total_received;
            else
            {
                perror("Error receiving data");
                return -1;
            }
        }
        total_received += bytes_received;
    }
    return total_received;
}

struct BlogOperation createBlogOperation(int client_id, int operation_type, int server_response, char* topic, char* content){
    struct BlogOperation operation;
    operation.client_id = client_id;
    operation.operation_type = operation_type;
    operation.server_response = 0;
    strcpy(operation.topic, topic);
    strcpy(operation.content, content);
    return operation;
}