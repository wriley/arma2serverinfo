#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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
    char A2S_PLAYER[9] = {0xFF, 0xFF, 0xFF, 0xFF, 0x55, 0xFF, 0xFF, 0xFF, 0xFF};
    char A2S_RULES[9] = {0xFF, 0xFF, 0xFF, 0xFF, 0x56, 0xFF, 0xFF, 0xFF, 0xFF};
    struct sigaction myAction;
    char *info, *val;

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

    if(rbuf[4] != 0x49) {
        printf("Error: header was 0x%x instead of 0x49\n", rbuf[4]);
        close(sockfd);
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

        // Keywords
        if(edf & 0x20) {
            info = getString(rbuf, &sPtr);
            printf("KEYWORDS: %s\n", info);
            free(info);
        }
    }

    // reset sPtr
    sPtr = 5;

    // Get rules
    fprintf(stderr, "sending A2S_RULES query\n");
    if(sendto(sockfd, A2S_RULES, 25, 0, (struct sockaddr *)&servaddr,sizeof(servaddr)) < 1) {
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
    //hexDump("rbuf", &rbuf, n);

    if(rbuf[4] != 0x41) {
        printf("Error: header was 0x%x instead of 0x41\n", rbuf[4]);
        close(sockfd);
        exit(1);
    }

    // Challenge number
    i1 = rbuf[sPtr++];
    i2 = rbuf[sPtr++];
    i3 = rbuf[sPtr++];
    i4 = rbuf[sPtr++];
    uint32_t chnum = i4<<24 | i3<<16 | i2<<8 | i1;
    fprintf(stderr,"Challenge number: %d\n", chnum);

    A2S_RULES[5] = chnum;
    A2S_RULES[6] = chnum >> 8;
    A2S_RULES[7] = chnum >> 16;
    A2S_RULES[8] = chnum >> 24;

    //hexDump("A2S_RULES", &A2S_RULES, sizeof(A2S_RULES));

    fprintf(stderr, "sending A2S_RULES query\n");
    if(sendto(sockfd, A2S_RULES, 25, 0, (struct sockaddr *)&servaddr,sizeof(servaddr)) < 1) {
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
    //hexDump("rbuf", &rbuf, n);

    if(rbuf[4] != 0x45) {
        printf("Error: header was 0x%x instead of 0x45\n", rbuf[4]);
        close(sockfd);
        exit(1);
    }

    sPtr = 5;
    s1 = rbuf[sPtr++];
    s2 = rbuf[sPtr++];
    uint8_t Rules = s1 | s2<<8;
    
    if(Rules > 0) {
        printf("RULE LIST:\n");
    }

    for(i = 0; i < Rules; i++) {
        // Name
        info = getString(rbuf, &sPtr);
        // Value
        val = getString(rbuf, &sPtr);
 
        printf("%s %s\n", info, val);
    }

    // reset sPtr
    sPtr = 5;

    // Get players
    fprintf(stderr, "sending A2S_PLAYER query\n");
    if(sendto(sockfd, A2S_PLAYER, 25, 0, (struct sockaddr *)&servaddr,sizeof(servaddr)) < 1) {
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
    //hexDump("rbuf", &rbuf, n);

    if(rbuf[4] != 0x41) {
        printf("Error: header was 0x%x instead of 0x41\n", rbuf[4]);
        close(sockfd);
        exit(1);
    }

    // Challenge number
    i1 = rbuf[sPtr++];
    i2 = rbuf[sPtr++];
    i3 = rbuf[sPtr++];
    i4 = rbuf[sPtr++];
    chnum = i4<<24 | i3<<16 | i2<<8 | i1;
    fprintf(stderr,"Challenge number: %d\n", chnum);

    A2S_PLAYER[5] = chnum;
    A2S_PLAYER[6] = chnum >> 8;
    A2S_PLAYER[7] = chnum >> 16;
    A2S_PLAYER[8] = chnum >> 24;

    //hexDump("A2S_PLAYER", &A2S_PLAYER, sizeof(A2S_PLAYER));

    fprintf(stderr, "sending A2S_PLAYER query\n");
    if(sendto(sockfd, A2S_PLAYER, 25, 0, (struct sockaddr *)&servaddr,sizeof(servaddr)) < 1) {
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
    //hexDump("rbuf", &rbuf, n);

    if(rbuf[4] != 0x44) {
        printf("Error: header was 0x%x instead of 0x44\n", rbuf[4]);
        close(sockfd);
        exit(1);
    }

    sPtr = 5;
    uint8_t Players = rbuf[sPtr++];
    uint8_t Index = 0;
    
    if(Players > 0) {
        printf("PLAYER LIST:\n");
    }

    for(i = 0; i < Players; i++) {
        // Index (this seems to be always 0 ??)
        Index = rbuf[sPtr++];
        // Name
        info = getString(rbuf, &sPtr);
        // Score
        i1 = rbuf[sPtr++];
        i2 = rbuf[sPtr++];
        i3 = rbuf[sPtr++];
        i4 = rbuf[sPtr++];
        uint32_t Score = i4<<24 | i3<<16 | i2<<8 | i1;
        // Duration
        i1 = rbuf[sPtr++];
        i2 = rbuf[sPtr++];
        i3 = rbuf[sPtr++];
        i4 = rbuf[sPtr++];
        uint32_t d = i4<<24 | i3<<16 | i2<<8 | i1;
        float Duration = *(float *)&d;
        
        printf("%s %d %.0f\n", info, Score, Duration);
    }

    close(sockfd);
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
