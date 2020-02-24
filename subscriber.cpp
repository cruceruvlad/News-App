#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <math.h>
#include <iomanip>
#include<iostream>
#include "helpers.h"

using namespace std;

// converteste mesajul primit de la tastatura in comanda subscr/unsub
// daca e comanda exit intoarce 1, daca e bad command -1, daca e success 0
int stdin2protocol(mesaj_subscriber &msjout, char *buf){
    if(strncmp(buf,"exit",4)==0)
            return 1;

    char *token;

    token = strtok(buf," ");
    if(strcmp(token,"subscribe")!=0 && strcmp(token,"unsubscribe")!=0)
            return -1;
    msjout.state = (strcmp(token,"subscribe") ==0) ? true : false;

    token = strtok(nullptr," ");
    if(strlen(token)>50)
            return -1;
    memcpy(&msjout.topic,token,50);
    int len = strlen(msjout.topic);
    if( msjout.topic[len-1] == '\n' )
    msjout.topic[len-1] = 0;

    token = strtok(nullptr," ");
    if(msjout.state == true){
    	if(token==nullptr)
            return -1;
    	if(strcmp(token,"0\n")!=0 && strcmp(token,"1\n")!=0)
            return -1;
    	msjout.SF = (strcmp(token,"1\n") ==0) ? true : false;
    }else{
	  if(token!=nullptr)
		return -1;
    }
    return 0;
}

// afiseaza mesajul primit de la server
void protocol2stdout(mesaj_topic msj){
    printf("%s:%u - %s - ",inet_ntoa(msj.sender_addr),ntohs(msj.sender_port),msj.msj.topic);
    uint32_t nr;
    uint16_t nr2;
    uint8_t nr3;
    switch (msj.msj.type){
        case 0:{
                printf("INT - ");
                if((uint8_t)msj.msj.data[0]==1) printf("-");
                memcpy(&nr,&msj.msj.data[1],4);
                nr = ntohl(nr);
                printf("%u\n",nr);
                break;
               }

        case 1:{
                printf("SHORT_REAL - ");
                memcpy(&nr2,&msj.msj.data[0],2);
                nr2 = ntohs(nr2);
                printf("%.2f\n",nr2/100.00);
                break;
                }

        case 2:{
                printf("FLOAT - ");
                if((uint8_t)msj.msj.data[0]==1) printf("-");
                memcpy(&nr,&msj.msj.data[1],4);
                memcpy(&nr3,&msj.msj.data[5],1);
                nr = ntohl(nr);
                uint8_t precision = (nr == 0) ? 1 : (floor(log10(nr)) + 1);
                cout<<setprecision(precision)<<(float)nr/pow(10,nr3)<<"\n";
                break;
                }

        default: printf("STRING - %s\n",(char*) msj.msj.data);
    }
}


int main(int argc, char *argv[]){
	int sockfd,fdmax,portno,ret;
	char buf[100];
	mesaj_subscriber msjout;
	mesaj_topic msjin;
	struct sockaddr_in serv_addr;
	socklen_t socklen = sizeof(struct sockaddr_in);
	fd_set read_fds, tmp_fds;

	if (argc != 4){
		printf("./subscriber <id> <server_ip> <server_port>\n");
		return 0;
	}

	portno = atoi(argv[3]);
	DIE(portno==0,"bad port");

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
        ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "bad address");
 
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");
	reUseSocket(sockfd); // utilizez chestia aia cu reutilizarea socketului

	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;

	connect(sockfd, (const struct sockaddr *)&serv_addr, socklen);
	DIE(ret < 0, "connect");
	send(sockfd,argv[1],10,0);

	forever{
        tmp_fds = read_fds;
        select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);

		for (int socket = 0; socket <= fdmax; socket++)
			if (FD_ISSET(socket, &tmp_fds)) {
				if(socket==STDIN_FILENO){
					fgets(buf,99,stdin);

					ret = stdin2protocol(msjout,buf);
                    switch (ret){
                        case 1:  close(sockfd);
                                 return 0;

                        case -1: printf("<subscribe/unsubscribe> <topic(max 50 chars)> <0/1(only for subscribe)> (no trailing spaces after)\n");
                                 break;

                        default: send(sockfd,&msjout,sizeof(mesaj_subscriber),0);
                                 if(msjout.state==true) printf("subscribed %s\n",msjout.topic);
                                        else            printf("unsubscribed %s\n",msjout.topic);
                    }
				}

				if(socket==sockfd){
                    ret=recv(sockfd,&msjin,sizeof(mesaj_topic),0);
					if(ret==0){ close(sockfd);return 0;}
					protocol2stdout(msjin);
				}
			}
	}
    return 0;
}
