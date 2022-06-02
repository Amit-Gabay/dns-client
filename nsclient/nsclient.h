#ifndef NSCLIENT_H_
#define NSCLIENT_H_

int main(int argc, char** argv);
int InitClient(char* dnsServerIP);
int ServeForever();
struct hostent* dnsQuery(char* domainName);

struct {
	SOCKET sock;
	SOCKADDR_IN remoteAddr;
} typedef CLIENT;

#define CLI_PROMPT		"nsclient> "
#define QUIT			"quit"
#define ERR_BAD_NAME	"ERROR: BAD NAME"
#define ERR_NON_EXIST	"ERROR: NONEXISTENT"

#define DNS_PORT		(53)
#define RECV_TIMEOUT	(2000)

#endif