#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "utils.h"
#include "nsclient.h"
// #include <Ws2tcpip.h> - Check if needed

#pragma comment(lib, "ws2_32.lib")


unsigned int buffer_len;
SOCKADDR_IN dnsServerAddr;


int main(int argc, char** argv)
{
	char* dnsServerIp;
	WSADATA wsaData;
	SOCKET sock;

	if (argc != 2)
	{
		printf("Invalid number of arguments %d\n", argc);
		return 0;
	}

	dnsServerIp = argv[1];

	WSAStartup(MAKEWORD(2, 2), &wsaData);

	/* Fill the DNS server address */
	inet_pton(AF_INET, dnsServerIp, &(dnsServerAddr.sin_addr));
	dnsServerAddr.sin_family = AF_INET;
	dnsServerAddr.sin_port = htons(53);

	ServeForever();
}


int ServeForever()
{
	char domainName[256]; // Validate the length
	HOSTENT *response;
	struct in_addr resultIp;
	char ipAddress[256]; // Validate the length

	while (1)
	{
		printf("nsclient> ");
		scanf("%s", domainName);

		if (!CheckDomainName(domainName))
		{
			printf("ERROR: BAD NAME\n");
			return 0;
		}

		if (strcmp(domainName, "quit") == 0)
		{
			break;
		}

		response = dnsQuery(domainName);

		if (response != NULL)
		{
			if (response->h_addr_list[0] != NULL)
			{
				resultIp.s_addr = *(u_long*) response->h_addr_list[0];
				inet_ntop(AF_INET, &resultIp, ipAddress, 256);
				printf("%s\n", ipAddress);
			}
		}
		else
		{
			printf("ERROR: NONEXISTENT\n");
		}
	}
}


HOSTENT* dnsQuery(char* domainName)
{
	char* dnsQuery;
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 17);
	// setsockopt() TODO: set timeout to 2 seconds.
	u_int queryLen;

	// Send DNS query
	dnsQuery = BuildQuery(domainName, &queryLen);
	sendto(sock, dnsQuery, queryLen, 0, (SOCKADDR*)&dnsServerAddr, sizeof(SOCKADDR_IN));
	free(dnsQuery);

	// Parse DNS response
	char* dnsResponse = (char*)malloc(512); //Validate length
	int addrLen = sizeof(SOCKADDR_IN);
	recvfrom(sock, dnsResponse, 512, 0, (SOCKADDR*)&dnsServerAddr, &addrLen);
	HOSTENT* responseEntry = ParseResponse(dnsResponse, domainName);

	return responseEntry;
}