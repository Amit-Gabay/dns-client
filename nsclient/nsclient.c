#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "utils.h"
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
// #include <Ws2tcpip.h> - Check if needed
#include <errno.h>

#pragma comment(lib, "ws2_32.lib")

struct sockaddr_in DNS_SERVER_ADDR;
SOCKET sock;


int main(int argc, char** argv)
{
	char* dns_server_ip;
	WSADATA wsaData;
	SOCKET sock;

	if (argc != 2)
	{
		printf("Invalid number of arguments %d\n", argc);
		return 0;
	}

	dns_server_ip = argv[1];

	WSAStartup(MAKEWORD(2, 2), &wsaData);

	sock = socket(AF_INET, SOCK_DGRAM, 17);

	/* Fill the DNS server address */
	inet_pton(AF_INET, dns_server_ip, &(DNS_SERVER_ADDR.sin_addr));
	DNS_SERVER_ADDR.sin_family = AF_INET;
	DNS_SERVER_ADDR.sin_port = htons(53);

	serve_forever();
}


int serve_forever()
{
	char domain_name[256]; // Validate the length
	struct hostent *response;
	struct in_addr result_addr;

	while (1)
	{
		printf("nsclient> ");
		scanf("%s", domain_name);

		if (strcmp(domain_name, "quit") == 0)
		{
			break;
		}

		response = gethostbyname(domain_name);
		if (response != NULL)
		{
			if (response->h_addr_list[0] != NULL)
			{
				result_addr.s_addr = *(u_long*) response->h_addr_list[0];
				printf("%s\n", inet_ntoa(result_addr));
			}
		}
	}
}