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
		PrintSocketError();
		return 1;
	}

	InitClient(dnsServerIP);
	ServeForever();

	if (WSACleanup() == SOCKET_ERROR)
	{
		PrintSocketError();
		return 1;
	}
}


/**
* Initializes SOCKET-related objects needed by the nsclient.
*/
int InitClient(char* dnsServerIP)
{
	u_int recvTimeout;
	int returnCode;

	// Assign DNS server address
	returnCode = inet_pton(AF_INET, dnsServerIP, &(nsClient.remoteAddr.sin_addr));
	if (returnCode != 1)
	{
		if (returnCode == 0)
		{
			errorCode = BAD_IP_ADDR;
		}
		PrintSocketError();
		exit(1);
	}
	nsClient.remoteAddr.sin_family = AF_INET;
	nsClient.remoteAddr.sin_port = htons(DNS_PORT);

	// Initialize client's socket (with desired recv timeout of 2 seconds)
	recvTimeout = RECV_TIMEOUT;
	nsClient.sock = socket(AF_INET, SOCK_DGRAM, 17);
	if (nsClient.sock == INVALID_SOCKET)
	{
		PrintSocketError();
		exit(1);
	}
	if (setsockopt(nsClient.sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&recvTimeout, sizeof(recvTimeout)) == SOCKET_ERROR)
	{
		PrintSocketError();
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


	// Serve user until 'quit'
	while (1)
	{
		printf(CLI_PROMPT);
		if (scanf("%s", domainName) != 1)
		{
			PrintError();
			exit(1);
		}


		// Check whether user wrote 'quit'
		if (strcmp(domainName, QUIT) == 0) { break; }

		// Validate given domain name syntax
		if (!CheckDomainName(domainName))
		{
			errorCode = DNS_BAD_NAME;
			PrintSocketError();
		}

		response = dnsQuery(domainName);

		// Check whether results were found
		if (response != NULL)
		{
			// Extract the IP address from the response
			if (response->h_addr_list[0] != NULL)
			{
				resultIP.s_addr = *(u_long*) response->h_addr_list[0];
				if (inet_ntop(AF_INET, &resultIP, outputIP, 256) == NULL)	// **TODO: Validate the length (256)**
				{
					PrintSocketError();
					exit(1);
				}
				printf("%s\n", outputIP);
			}
			FreeHostEnt(response);
		}
		else
		{
			PrintSocketError();
		}
	}
}


/**
* Performs a DNS query to translate given domain name.
* @param :domainName: NULL-terminated domain name
* @return pointer to a HOSTENT structure with the answer details, or NULL if an error occured.
*/
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
		PrintSocketError();
		exit(1);
	}
	free(queryMsg);

	// Parse DNS response
	responseMsg = (char*)malloc(512);		// **TODO: Validate length**
	if (responseMsg == NULL)
	{
		PrintError();
		exit(1);
	}
	addrLen = sizeof(SOCKADDR_IN);
	if (recvfrom(nsClient.sock, responseMsg, 512, 0, (SOCKADDR*)&nsClient.remoteAddr, &addrLen) == SOCKET_ERROR)
	{
		PrintSocketError();
		exit(1);
	}
	responseEntry = ParseResponse(responseMsg, domainName);
	free(responseMsg);

	return responseEntry;
}