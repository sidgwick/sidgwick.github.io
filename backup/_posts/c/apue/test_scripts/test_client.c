#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main()
{
    int fd, i;
    char buf[1024];
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
    addr.sin_port = htons(11212);

    fd = socket(AF_INET, SOCK_STREAM, 0);
    connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    for (i = 0; i < 1024; i++) {
        read(fd, buf, 1);
        printf("%c\n", buf);
        fflush(stdout);
        sleep(3);
    }
}
