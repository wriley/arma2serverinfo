#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

int timedout = 0;

#define TIMEOUT_SECS    5
void CatchAlarm(int unused);
char *getString(char *rbuf, int *sPtr);

int main(int argc, char* argv[]) {
    int sockfd,n,i,j,sPtr;
    struct sockaddr_in servaddr,cliaddr;
    char rbuf[1500];
    char A2S_INFO[25] = {0xFF, 0xFF, 0xFF, 0xFF, 0x54, 0x53, 0x6F, 0x75, 0x72, 0x63, 0x65, 0x20, 0x45, 0x6E, 0x67, 0x69, 0x6E, 0x65, 0x20, 0x51, 0x75, 0x65, 0x72, 0x79, 0x00};
    struct sigaction myAction;
    char *info;

    if(argc != 3) {
        printf("Usage: %s <hostname> <port>\n", argv[0]);
        exit(1);
    }

    myAction.sa_handler = CatchAlarm;
    if (sigfillset(&myAction.sa_mask) < 0) {
        fprintf(stderr, "sigfillset() failed\n");
        exit(1);
    }
    myAction.sa_flags = 0;

    if (sigaction(SIGALRM, &myAction, 0) < 0) {
        fprintf(stderr, "sigaction() failed\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1) {
        fprintf(stderr, "Error creating socket\n");
        exit(1);
    }

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(atoi(argv[2]));

    fprintf(stderr, "sending A2S_INFO query\n");
    if(sendto(sockfd, A2S_INFO, 25, 0, (struct sockaddr *)&servaddr,sizeof(servaddr)) < 1) {
        fprintf(stderr, "Error sending\n");
        close(sockfd);
        exit(1);
    }

    alarm(TIMEOUT_SECS);
    n = recvfrom(sockfd, rbuf, 1500, 0, NULL, NULL);
    alarm(0);

    if(timedout == 1) {
        fprintf(stderr, "timed out waiting for response\n");
        close(sockfd);
        exit(1);
    }

    if(n == -1) {
        fprintf(stderr, "Error receiving\n");
        close(sockfd);
        exit(1);
    }
    fprintf(stderr, "received %d bytes\n", n);
    close(sockfd);

    if(rbuf[4] != 0x49) {
        printf("Error: header was %c instead of 0x49\n");
        exit(1);
    }

    // https://developer.valvesoftware.com/wiki/Server_queries#Response_Format
    //Header
    printf("HEADER: 0x%x\n", rbuf[4]);

    // Protocol
    printf("PROTOCOL: 0x%x\n", rbuf[5]);

    // Name
    sPtr = 5;
    info = getString(rbuf, &sPtr);
    printf("NAME: %s\n", info);
    free(info);

    // Map
    info = getString(rbuf, &sPtr);
    printf("MAP: %s\n", info);
    free(info);

    // Folder
    info = getString(rbuf, &sPtr);
    printf("FOLDER: %s\n", info);
    free(info);

    // Game
    info = getString(rbuf, &sPtr);
    printf("GAME: %s\n", info);
    free(info);

    // ID
    unsigned char id1 = rbuf[sPtr++];
    unsigned char id2 = rbuf[sPtr++];
    uint16_t id = id2<<8 | id1;
    printf("ID: %d\n", id);

    // Players
    printf("PLAYERS: %d\n", rbuf[sPtr++]);

    // Max. Players
    printf("MAXPLAYERS: %d\n", rbuf[sPtr++]);

    // Bots
    printf("BOTS: %d\n", rbuf[sPtr++]);

    // Server type
    printf("SERVERTYPE: %c\n", rbuf[sPtr++]);

    // Environment
    printf("ENVIRONMENT: %c\n", rbuf[sPtr++]);

    // Visibility
    printf("VISIBILITY: %d\n", rbuf[sPtr++]);

    // VAC
    printf("VAC: %d\n", rbuf[sPtr++]);

    return 0;
}

void CatchAlarm(int unused) {
    timedout = 1;
}

char *getString(char *rbuf, int *sPtr) {
    char *line = NULL, *tmp = NULL;
    size_t size = 0, index = 0;
    int ch = EOF;
    while (ch) {
        ch = rbuf[(*sPtr)++];

        if (ch == EOF || ch == '\n') {
            ch = 0;
        }

        if (size <= index) {
            size += 4;
            tmp = realloc(line, size);
            if (!tmp) {
                free(line);
                line = NULL;
                break;
            }
            line = tmp;
        }
        line[index++] = ch;
    }
    return line;
}
