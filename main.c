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
void hexDump (char *desc, void *addr, int len);

int main(int argc, char* argv[]) {
    int sockfd,n,i,j,sPtr;
    struct sockaddr_in servaddr,cliaddr;
    unsigned char rbuf[1500], edf, s1, s2, i1, i2, i3, i4;
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
        printf("Error: header was 0x%x instead of 0x49\n", rbuf[4]);
        exit(1);
    }

    //hexDump("rbuf", &rbuf, n);

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
    s1 = rbuf[sPtr++];
    s2 = rbuf[sPtr++];
    uint16_t id = s2<<8 | s1;
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

    // Version
    info = getString(rbuf, &sPtr);
    printf("VERSION: %s\n", info);
    free(info);

    // Check for extra data
    if(n > sPtr) {
        // EDF
        edf = rbuf[sPtr++];

        // PORT
        if(edf & 0x80) {
            s1 = rbuf[sPtr++];
            s2 = rbuf[sPtr++];
            uint16_t port = s1 | s2<<8;
            printf("PORT: %d\n", port);
        }

        // STEAMID
        if(edf & 0x10) {
            sPtr += 8;
        }

        // SERVER TAGS
        if(edf & 0x20) {
            info = getString(rbuf, &sPtr);
            printf("SERVER TAGS: %s\n", info);
            free(info);
        }
    }

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

// from http://stackoverflow.com/questions/7775991/how-to-get-hexdump-of-a-structure-data
void hexDump (char *desc, void *addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != NULL)
        printf ("%s:\n", desc);

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf ("  %s\n", buff);

            // Output the offset.
            printf ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf ("  %s\n", buff);
}
