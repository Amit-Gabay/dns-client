#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include "utils.h"
#include "nsclient.h"
// #include <Ws2tcpip.h> - Check if needed

#pragma comment(lib, "ws2_32.lib")

//unsigned int buffer_len;    **TODO: Check if needed**

CLIENT nsClient;


int main(int argc, char** argv)
{
	char* dnsServerIP;
	WSADATA wsaData;
	int errorCode;

	// Extract IP address from command line arguments
	if (argc != 2)
	{
		fprintf(stderr, "ERROR: Invalid number of arguments!\nGiven:%d\nExpected:2\n", argc);
		return 1;
	}
	dnsServerIP = argv[1];

	errorCode = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (errorCode != 0)
	{ 
		printSocketError(errorCode);
		return 1;
	}

	InitClient(dnsServerIP);
	ServeForever();

	errorCode = WSACleanup();
	if (errorCode != 0)
	{
		printSocketError(errorCode);
		return 1;
	}
}


/**
* Initializes socket related objects needed by the nsclient.
*/
int InitClient(char* dnsServerIP)
{
	u_int recvTimeout;

	// Assign DNS server address
	inet_pton(AF_INET, dnsServerIP, &(nsClient.remoteAddr.sin_addr));
	nsClient.remoteAddr.sin_family = AF_INET;
	nsClient.remoteAddr.sin_port = htons(DNS_PORT);

	// Initialize client's socket (with desired recv timeout)
	recvTimeout = RECV_TIMEOUT;
	nsClient.sock = socket(AF_INET, SOCK_DGRAM, 17);
	if (nsClient.sock == INVALID_SOCKET)
	{
		printSocketError(GET_ERR_CODE);
		exit(1);
	}
	if (setsockopt(nsClient.sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recvTimeout, sizeof(recvTimeout)) == SOCKET_ERROR)
	{
		printSocketError(GET_ERR_CODE);
		exit(1);
	}
}


/**
* Serves the client by translating given domain names, until 'quit' is sent from user.
*/
int ServeForever()
{
	char domainName[256];		// **TODO: Validate the length**
	HOSTENT *response;
	struct in_addr resultIP;
	char outputIP[256];			// **TODO: Validate the length**


	while (1)
	{
		printf(CLI_PROMPT);
		if (scanf("%s", domainName) != 1)
		{
			perror("ERROR");
			exit(1);
		}


		// Check whether user wrote 'quit'
		if (strcmp(domainName, QUIT) == 0) { break; }

		// Validate given domain name syntax
		if (!CheckDomainName(domainName))
		{
			fprintf(stderr, "%s\n", ERR_BAD_NAME);
		}

		response = dnsQuery(domainName);

		if (response != NULL)
		{
			// Extract the IP address from the response
			if (response->h_addr_list[0] != NULL)
			{
				resultIP.s_addr = *(u_long*) response->h_addr_list[0];
				if (inet_ntop(AF_INET, &resultIP, outputIP, 256) == NULL)	// **TODO: Validate the length (256)**
				{
					printSocketError(GET_ERR_CODE);
					exit(1);
				}
				printf("%s\n", outputIP);
			}
		}
		else
		{
			// If no results were found
			fprintf(stderr, "%s\n", ERR_NON_EXIST);
		}
	}
}


HOSTENT* dnsQuery(char* domainName)
{
	char* queryMsg;
	char* responseMsg;
	u_int queryLen;
	int addrLen;
	HOSTENT* responseEntry;

	// Send DNS query to chosen DNS server
	queryMsg = BuildQuery(domainName, &queryLen);
	if (sendto(nsClient.sock, queryMsg, queryLen, 0, (SOCKADDR*)&nsClient.remoteAddr, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		printSocketError(GET_ERR_CODE);
		exit(1);
	}
	free(queryMsg);

	// Parse DNS response
	responseMsg = (char*)malloc(512);		// **TODO: Validate length**
	if (responseMsg == NULL)
	{
		perror("ERROR");
		exit(1);
	}
	addrLen = sizeof(SOCKADDR_IN);
	if (recvfrom(nsClient.sock, responseMsg, 512, 0, (SOCKADDR*)&nsClient.remoteAddr, &addrLen) == SOCKET_ERROR)
	{
		printSocketError(GET_ERR_CODE);
		exit(1);
	}
	responseEntry = ParseResponse(responseMsg, domainName);

	return responseEntry;
}