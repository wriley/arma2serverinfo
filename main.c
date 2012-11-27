#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[])
{
    int sockfd,n,i,j;
    struct sockaddr_in servaddr,cliaddr;
    char rbuf[1500],sbuf[15];
    char challenge_request[7] = {0xFE, 0xFD, 0x09, 0xAB, 0xAB, 0xAB, 0xFF};

    if(argc != 3)
    {
        printf("Usage: argv[0] <hostname> <port>\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1)
    {
        fprintf(stderr, "Error creating socket\n");
        exit(1);
    }

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(atoi(argv[2]));

    fprintf(stderr, "sending challenge request\n");
    if(sendto(sockfd, challenge_request, strlen(challenge_request), 0,
        (struct sockaddr *)&servaddr,sizeof(servaddr)) < 1)
    {
        fprintf(stderr, "Error sending\n");
        exit(1);
    }
    n = recvfrom(sockfd, rbuf, 1500, 0, NULL, NULL);
    if(n == -1)
    {
        fprintf(stderr, "Error receiving\n");
        close(sockfd);
        exit(1);
    }
    fprintf(stderr, "received %d bytes\n", n);
    for(i = 0; i < n; i++)
    {
        fprintf(stderr, "0x%02x, ", rbuf[i]);
    }
    fprintf(stderr, "\n");

    char temp[n-5];
    for(i = 0; i < n - 5; i++)
    {
        temp[i] = rbuf[i+5];
    }
    for(i = 0; i < n-5; i++)
    {
        fprintf(stderr, "temp[%02d] = 0x%02x = %c\n", i, temp[i], temp[i]);
    }
    int s = atoi(temp);

    bzero(sbuf, 15);
    sbuf[0] = 0xFE;
    sbuf[1] = 0xFD;
    sbuf[2] = 0x00;
    sbuf[3] = 0xAB;
    sbuf[4] = 0xAB;
    sbuf[5] = 0xAB;
    sbuf[6] = 0xFF;
    sbuf[7] = s >> 24;
    sbuf[8] = s >> 16;
    sbuf[9] = s >> 8;
    sbuf[10] = s >> 0;
    sbuf[11] = 0xFF;
    sbuf[12] = 0xFF;
    sbuf[13] = 0xFF;
    sbuf[14] = 0x01;

    fprintf(stderr, "sending query\n");
    if(sendto(sockfd, sbuf, 15, 0,
        (struct sockaddr *)&servaddr,sizeof(servaddr)) < 1)
    {
        fprintf(stderr, "Error sending\n");
        exit(1);
    }

    n = recvfrom(sockfd, rbuf, 1500, 0, NULL, NULL);
    if(n == -1)
    {
        fprintf(stderr, "Error receiving\n");
        close(sockfd);
        exit(1);
    }
    fprintf(stderr, "received %d bytes\n", n);

    for(i = 16; i < n; i++)
    {
        if(rbuf[i] == '\0')
        {
            printf("\n");
        } else {
            printf("%c", rbuf[i]);
        }
    }

    close(sockfd);
    return 0;
}
