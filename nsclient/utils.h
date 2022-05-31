#ifndef UTILS_H_
#define UTILS_H_

#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <windns.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")


struct {
	unsigned short id;
	unsigned char qr : 1;
	unsigned char opcode : 4;
	unsigned char aa : 1;
	unsigned char tc : 1;
	unsigned char rd : 1;
	unsigned char ra : 1;
	unsigned char z : 3;
	unsigned char rcode : 4;
	unsigned short qdcount;
	unsigned short ancount;
	unsigned short nscount;
	unsigned short arcount;
} typedef DNS_HEADER_T;

struct {
	unsigned short qtype;
	unsigned short qclass;
} typedef DNS_QUESTION;

struct {
	char* name;
	unsigned short type;
	unsigned short class;
	unsigned int ttl;
	unsigned short rdlength;
	unsigned char* rdata;
} typedef RESOURCE_RECORD;


char* EncodeDomainName(char* domainName);
int CheckDomainName(char* domainName);
char* BuildHeaderSection();
char* BuildQuestionSection();
char* BuildQuery(char* domainName);
HOSTENT* ParseResponse(char* rawResponse);

#endif