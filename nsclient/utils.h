#ifndef UTILS_H_
#define UTILS_H_

#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
//#include <windns.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")
//#pragma comment(lib, "dnsapi.lib")


struct {
	unsigned short id;

	unsigned char rd : 1;
	unsigned char tc : 1;
	unsigned char aa : 1;
	unsigned char opcode : 4;
	unsigned char qr : 1;

	unsigned char rcode : 4;
	unsigned char z : 3;
	unsigned char ra : 1;

	unsigned short qdcount;
	unsigned short ancount;
	unsigned short nscount;
	unsigned short arcount;
} typedef DNS_HEADER;

struct {
	unsigned short qtype;
	unsigned short qclass;
} typedef DNS_QUESTION;

struct {
	unsigned short type;
	unsigned short class;
	unsigned short ttl_1;
	unsigned short ttl_2;
	unsigned short rdlength;
} typedef RESOURCE_RECORD;


#define FOUND			(1)
#define NOT_FOUND		(0)

#define VALID			(1)
#define INVALID			(0)

#define TYPE_A			(1)
#define CLASS_IN		(1)

#define IP_ADDR_SIZE	(4)

#define NO_ERR			(0)
#define DNS_BAD_FORMAT	(9001)
#define DNS_SERVER_FAIL	(9002)
#define DNS_NON_EXIST	(9003)
#define DNS_NOT_SUPPORT	(9004)
#define DNS_REFUSED		(9005)
#define DNS_BAD_RCODE	(9006)
#define DNS_BAD_NAME	(9007)
#define BAD_IP_ADDR		(9008)


char* EncodeDomainName(char* domainName);
int CheckDomainName(char* domainName);
char* BuildHeaderSection();
char* BuildQuestionSection();
char* BuildQuery(char* domainName);
char* FindAnswerBody(char* rawResponse);
char* SkipDomainName(char* rawSection);
HOSTENT* ParseResponse(char* rawResponse, char* domainName);
int CheckResponseHeader(DNS_HEADER* dnsHeader);

void FreeHostEnt(HOSTENT* hostent);
void PrintSocketError();
void PrintError();

#endif