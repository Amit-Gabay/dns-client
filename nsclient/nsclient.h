#ifndef NSCLIENT_H_
#define NSCLIENT_H_

int main(int argc, char **argv);
int InitClient(char *dnsServerIP);
void ServeForever();
struct hostent *dnsQuery(char *domainName);

struct
{
	SOCKET sock;
	SOCKADDR_IN remoteAddr;
} typedef CLIENT;

#define CLI_PROMPT "nsclient> "
#define QUIT "quit"

#define DNS_PORT (53)
#define RECV_TIMEOUT (2000)

extern int errorCode;

#endif