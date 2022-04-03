#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main()
{
    int fd, cfd;
    char msg[] = "How about you?\n";
    struct sockaddr_in addr, caddr;
    socklen_t len;

    fd = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(11212);
    len = sizeof(addr);

    bind(fd, (struct sockaddr *)&addr, len);

    listen(fd, 100);

    cfd = accept(fd, (struct sockaddr *)&caddr, &len);

    printf("write to cfd\n");
    fflush(stdout);
    write(cfd, msg, strlen(msg));
    printf("will to close cfd\n");
    fflush(stdout);
    close(cfd);
    printf("cfd closed...\n");
    fflush(stdout);
}
