#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include "helpers.h"
#include "forum.h"

using namespace std;

#define MAX_CLIENTS 100

int main(int argc, char **argv){

Forum forum;
char buf[11];
mesaj_udp        msj;
mesaj_topic      msj2;
mesaj_subscriber msj3;
int sock_udp, sock_tcp, fdmax,portno,ret;
struct sockaddr_in serv_addr, cli_addr;
socklen_t socklen = sizeof(struct sockaddr_in);
fd_set read_fds, tmp_fds;

if (argc != 2){
		printf("./server <port>\n");
		return 0;
}

//chestii cu setarea serverului
portno = atoi(argv[1]);
DIE(portno==0,"bad port");

FD_ZERO(&read_fds);
FD_ZERO(&tmp_fds);

serv_addr.sin_family = AF_INET;
serv_addr.sin_addr.s_addr = INADDR_ANY;
serv_addr.sin_port = htons(portno);

sock_udp= socket(AF_INET,SOCK_DGRAM,0);
DIE(sock_udp < 0, "socketUDP");
sock_tcp= socket(AF_INET,SOCK_STREAM,0);
DIE(sock_tcp < 0, "socketTCP");

// reutilizarea socketilor 
reUseSocket(sock_tcp);
reUseSocket(sock_udp);


bind(sock_udp, (const struct sockaddr *)&serv_addr, socklen);
DIE(ret < 0, "bind");
bind(sock_tcp, (const struct sockaddr *)&serv_addr, socklen);
DIE(ret < 0, "bind");

listen(sock_tcp, MAX_CLIENTS);
DIE(ret < 0, "listen");

FD_SET(STDIN_FILENO, &read_fds);
FD_SET(sock_tcp, &read_fds);
fdmax = sock_tcp;
FD_SET(sock_udp, &read_fds);
fdmax = fdmax > sock_udp ? fdmax : sock_udp;

forever{
		tmp_fds = read_fds;
        select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);

        for (int socket = 0; socket <= fdmax; socket++)
			if (FD_ISSET(socket, &tmp_fds)) {
				if(socket==sock_udp){   // primesc mesaj de la udp
                    memset(&msj.data,0,1500);
                    recvfrom(sock_udp, &msj, sizeof(mesaj_udp), 0, (struct sockaddr *) &cli_addr, &socklen);

		    // prelucrez mesajul
                    msj2.sender_addr=cli_addr.sin_addr;	
                    msj2.sender_port=cli_addr.sin_port;
                    msj2.msj=msj;
                    forum.sendToSubscribers(msj2);  // il trimit la subscriberi
                    continue;
                }

                if(socket==sock_tcp){	// cerere noua de conexiune
                    int newSockCli=accept(sock_tcp, (struct sockaddr *)&cli_addr, &socklen);
                    reUseSocket(newSockCli);
                    FD_SET(newSockCli, &read_fds);
                    fdmax = fdmax > newSockCli ? fdmax : newSockCli;
                    recv(newSockCli, buf, 10, 0);
                    ret = forum.updateUsers(buf,newSockCli);// update la user list si 
                    if(ret == 1) printf("Client (%s) reconnected ",buf);  //client daca e vechi
                        else     printf("New client (%s) connected ",buf); //new client daca e nou
                    printf("from %s:%u\n",inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
                    continue;
                }

                if(socket==STDIN_FILENO){  //input de la tastatura care poate fi doar exit
                        fgets(buf,10,stdin);
                        if(strncmp(buf,"exit",4)!=0) { printf("Only \"exit\" command\n");continue;}
			//deconectez toti utilizatorii
                        for(int i = fdmax;i >=3;i--) if(FD_ISSET(i, &read_fds)) { close(i); FD_CLR(i,&read_fds); }
                        return 0;
                }

                // daca nu e nicio varianta din cele de mai sus inseamna ca am primit msj de la un client
                ret = recv(socket, &msj3, sizeof(mesaj_subscriber), 0);
                if(ret == 0){  
                        printf("Client (%s) disconnected\n",forum.disconectUser(socket));
                        close(socket);
                        FD_CLR(socket, &read_fds);
                        continue;
                }
		//daca tipul mesajului e subscr atunci fa asa, daca e unsub fa asa
                if(msj3.state == true) forum.subscribe(socket,msj3.topic,msj3.SF);
                        else           forum.unsubscribe(socket,msj3.topic);
            }
    }
}
