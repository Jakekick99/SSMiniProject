#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(){
    int sock = 0, valread, client_fd;
    struct sockaddr_in serv_addr;
    char buffer[1024] = "admin";
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
 
    // Convert IPv4 and IPv6 addresses from text to binary
    // form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
 
    if ((client_fd = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
    send(sock, buffer, strlen(buffer), 0);
    valread = read(sock, buffer, 1024);
    buffer[valread]='\0';
    while(strcmp(buffer,"END\0")!=0){
    	printf("%s\n",buffer);
    	scanf("%s",buffer);
    	printf("------------------\n");
    	send(sock, buffer, strlen(buffer), 0);
    	valread = read(sock, buffer, 1024);
    	buffer[valread]='\0';
    }
 
    // closing the connected socket
    close(client_fd);
    return 0;
}
