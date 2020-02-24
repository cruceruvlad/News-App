#ifndef _HELPERS_H
#define _HELPERS_H

#include <cstddef>

#define forever while(1)

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

// activare nu stiu ce optiune ca daca o activezi ca face ca drege ca daca inchizi socketul se va putea redeschide din nou
void reUseSocket(int &sockfd){   
	int val=1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int));
}

struct mesaj_udp{     // structura de mesaje clientii udp-server
	char     topic[50];
	uint8_t  type;
	std::byte     data[1500];
};

struct mesaj_topic{	// structura de mesaje server-clientii tcp
    struct in_addr     sender_addr;
    unsigned short     sender_port;
    struct mesaj_udp   msj;
};

struct mesaj_subscriber{	//structura de mesaje clientii tcp-server
    bool state; // asta retine daca comanda e subscribe sau nu
    char topic[50];
    bool SF;
};

#endif
