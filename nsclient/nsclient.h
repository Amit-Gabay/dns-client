#ifndef NSCLIENT_H_
#define NSCLIENT_H_

int main(int argc, char** argv);
int ServeForever();
struct hostent* dnsQuery(char* domainName);

#endif