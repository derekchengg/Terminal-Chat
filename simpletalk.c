#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>

#include "list.h"

static int myPortNumber;
static char *remoteMachineName;
static int remotePortNumber;

int sockfd;

struct addrinfo hints, hints2, *serverInfo, *localInfo, *remoteInfo;

pthread_t keyboardThread, udpOutputThread, udpInputThread, screenOutputThread;

int rv, rv2;

List *sendMsg;
List *receiveMsg;

pthread_mutex_t outgoingMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t outgoingCond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t incomingMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t incomingCond = PTHREAD_COND_INITIALIZER;

void *keyboardInput(void *arg);
void *udpOutput(void *arg);
void *udpInput(void *arg);
void *screenOutput(void *arg);
void freePtr(void *arg);

void freePtr(void *ptr)
{
    ptr = NULL;
}

void *keyboardInput(void *arg)
{
    printf("---------------Welcome to Simple Talk---------------\n");
    char input[256];
    while (1)
    {
        fgets(input, sizeof(input), stdin);
        pthread_mutex_lock(&outgoingMutex);
        List_append(sendMsg, strdup(input));
        pthread_cond_signal(&outgoingCond);
        pthread_mutex_unlock(&outgoingMutex);

        if (input[0] == '!' && input[1] == '\n')
        {
            char terminationMsg[] = "Signalling other port of termination...\n";
            write(1, terminationMsg, strlen(terminationMsg));
        }
    }

    pthread_exit(0);
}

void *udpOutput(void *arg)
{

    while (1)
    {
        pthread_mutex_lock(&outgoingMutex);

        if (List_count(sendMsg) > 0)
        {
            char *message = (char *)List_remove(sendMsg);
            int messageLength = strlen(message);
            int bytesSent = sendto(sockfd, message, messageLength, 0, remoteInfo->ai_addr, remoteInfo->ai_addrlen);

            if (bytesSent == -1)
            {
                perror("Error sending message");
            }

            if (message[0] == '!' && message[1] == '\n')
            {
                pthread_mutex_unlock(&outgoingMutex);
                pthread_cancel(screenOutputThread);
                pthread_cancel(keyboardThread);
                pthread_cancel(udpInputThread);
                pthread_cancel(udpOutputThread);
                pthread_exit(0);
            }

            free(message);
        }
        else
        {
            pthread_cond_wait(&outgoingCond, &outgoingMutex);
        }

        pthread_mutex_unlock(&outgoingMutex);
    }
    pthread_exit(0);
}

void *udpInput(void *arg)
{
    struct sockaddr_storage remoteAdrr;
    socklen_t addrLen = sizeof(remoteAdrr);
    char receivedMessage[256];

    while (1)
    {
        int bytesReceived = recvfrom(sockfd, receivedMessage, sizeof(receivedMessage), 0, (struct sockaddr *)&remoteAdrr, &addrLen);

        if (bytesReceived > 0)
        {
            receivedMessage[bytesReceived] = '\0';
            pthread_mutex_lock(&incomingMutex);
            List_append(receiveMsg, strdup(receivedMessage));
            pthread_cond_signal(&incomingCond);
            pthread_mutex_unlock(&incomingMutex);
        }
    }

    pthread_exit(0);
}

void *screenOutput(void *arg)
{

    while (1)
    {
        pthread_mutex_lock(&incomingMutex);

        if (List_count(receiveMsg) > 0)
        {
            char *message = (char *)List_remove(receiveMsg);
            if (message[0] == '!' && message[1] == '\n') {
                char terminationMsg[] = "The other port has terminated the session...\n";
                write(1, terminationMsg, strlen(terminationMsg));

                pthread_cancel(screenOutputThread);
                pthread_cancel(keyboardThread);
                pthread_cancel(udpInputThread);
                pthread_cancel(udpOutputThread);

                pthread_exit(0);
            }
            write(1, "Other port: ", 12);
            write(1, message, strlen(message));
            free(message);
        }
        else
        {
            pthread_cond_wait(&incomingCond, &incomingMutex);
        }
        pthread_mutex_unlock(&incomingMutex);
    }
    pthread_exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        char usageMsg[] = "Please use the format: ./s-talk [my port number] [remote machine name] [remote port number]\n Example: ./s-talk 6060 csil-cpu3 6001\n";
        write(1, usageMsg, strlen(usageMsg));
        exit(1);
    }

    myPortNumber = atoi(argv[1]);
    remoteMachineName = argv[2];
    remotePortNumber = atoi(argv[3]);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, argv[1], &hints, &serverInfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (localInfo = serverInfo; localInfo != NULL; localInfo = localInfo->ai_next)
    {
        if ((sockfd = socket(localInfo->ai_family, localInfo->ai_socktype, localInfo->ai_protocol)) == -1)
        {
            perror("socket");
            continue;
        }

        if (bind(sockfd, localInfo->ai_addr, localInfo->ai_addrlen) == -1)
        {
            perror("bind");
            close(sockfd);
            continue;
        }
        break;
    }

    memset(&hints2, 0, sizeof(hints2));
    hints2.ai_family = AF_INET;
    hints2.ai_socktype = SOCK_DGRAM;

    if ((rv2 = getaddrinfo(remoteMachineName, argv[3], &hints2, &remoteInfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv2));
        return 1;
    }

    freeaddrinfo(serverInfo);
    freeaddrinfo(remoteInfo);

    sendMsg = List_create();
    receiveMsg = List_create();

    pthread_create(&keyboardThread, NULL, keyboardInput, NULL);
    pthread_create(&udpOutputThread, NULL, udpOutput, NULL);
    pthread_create(&udpInputThread, NULL, udpInput, NULL);
    pthread_create(&screenOutputThread, NULL, screenOutput, NULL);

    pthread_join(keyboardThread, NULL);
    pthread_join(udpOutputThread, NULL);
    pthread_join(udpInputThread, NULL);
    pthread_join(screenOutputThread, NULL);

    close(sockfd);

    char terminationMsg[] = "Session terminated.\n";
    write(1, terminationMsg, strlen(terminationMsg));

    return 0;
}