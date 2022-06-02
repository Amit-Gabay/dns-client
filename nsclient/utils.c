#include "utils.h"


int CheckDomainName(char* domainName)
{
	return 1; // TODO
}


/**
* Encodes a null-terminated domain name into labels based encoding (as in RFC-1035).
*/
char* EncodeDomainName(char* domainName)
{
	char* encodedName;
	int dstIdx = 0;
	int startIdx = 0;
	int endIdx = 0;
	int labelLen;

	encodedName = (char*)malloc(256);		// **TODO: Validate length**
	if (encodedName == NULL)
	{
		perror("ERROR");
		exit(1);
	}

	while (domainName[endIdx] != '\0')
	{
		if (domainName[endIdx] == '.')
		{
			domainName[endIdx] = '\0';
			labelLen = endIdx - startIdx;
			encodedName[dstIdx++] = (char)labelLen;
			strcpy((encodedName + dstIdx), (domainName + startIdx));
			dstIdx += labelLen;
			startIdx = endIdx + 1;
		}
		++endIdx;
	}

	labelLen = endIdx - startIdx;
	encodedName[dstIdx++] = (char)labelLen;
	strcpy((encodedName + dstIdx), (domainName + startIdx));
	startIdx = endIdx;
	dstIdx += labelLen;
	encodedName[dstIdx] = (char) 0;

	return encodedName;
}


/**
* Creates header section for the DNS query to be sent.
*/
char* BuildHeaderSection()
{
	DNS_HEADER* dnsHeader = (DNS_HEADER*)malloc(sizeof(DNS_HEADER));
	if (dnsHeader == NULL)
	{
		perror("ERROR");
		exit(1);
	}

	dnsHeader->id = htons(rand() * 2);
	dnsHeader->qr = 0;
	dnsHeader->opcode = 0;
	dnsHeader->aa = 0;
	dnsHeader->tc = 0;
	dnsHeader->rd = 0x1;
	dnsHeader->ra = 0;
	dnsHeader->z = 0x000;
	dnsHeader->rcode = 0x0000;
	dnsHeader->qdcount = htons(1);
	dnsHeader->ancount = 0;
	dnsHeader->nscount = 0;
	dnsHeader->arcount = 0;

	return dnsHeader;
}


/**
* Creates question section for the DNS query to be sent.
*/
char* BuildQuestionSection()
{
	DNS_QUESTION* dnsQuestion = (DNS_QUESTION*)malloc(sizeof(DNS_QUESTION));
	if (dnsQuestion == NULL)
	{
		perror("ERROR");
		exit(1);
	}

	dnsQuestion->qtype = htons(TYPE_A);	     // A  (a host address)
	dnsQuestion->qclass = htons(CLASS_IN);   // IN (Internet)

	return dnsQuestion;
}


char* BuildQuery(char* domainName, u_int* queryLen)
{
	DNS_HEADER* dnsHeader;
	DNS_QUESTION* dnsQuestion;
	char* encodedName;
	char* query;
	char* pointer;
	unsigned int nameLen;

	// Build Header & Question sections
	dnsHeader = BuildHeaderSection();
	dnsQuestion = BuildQuestionSection();

	// Encode domain name
	encodedName = EncodeDomainName(domainName);
	nameLen = strlen(encodedName) + 1;

	// Compose whole message
	*queryLen = sizeof(DNS_HEADER) + nameLen + sizeof(DNS_QUESTION);
	query = (char*)malloc(*queryLen);
	if (query == NULL)
	{
		perror("ERROR");
		exit(1);
	}
	pointer = query;

	memcpy(pointer, dnsHeader, sizeof(DNS_HEADER));
	pointer += sizeof(DNS_HEADER);
	memcpy(pointer, encodedName, nameLen);
	pointer += nameLen;
	memcpy(pointer, dnsQuestion, sizeof(DNS_QUESTION));

	free(dnsHeader);
	free(dnsQuestion);

	return query;
}


/**
* When parsing a DNS message, domain names in it could be skipped using this function.
*/
char* SkipDomainName(char* rawSection)
{
	u_char labelSize;
	char* tmp = rawSection;

	do
	{
		labelSize = (u_char)rawSection[0];
		if ((labelSize >> 6) == 0b11)
		{
			return rawSection + sizeof(unsigned short);
		}
		rawSection += labelSize + 1;
	} while (labelSize != 0);

	return rawSection;
}


/**
* Searches for an answer to our DNS translation request in a given message section.
*/
int SearchAnswerInSection(char** rawSection, int sectionNum)
{
	RESOURCE_RECORD answerBody;
	int sectionIdx;

	for (sectionIdx = 0; sectionIdx < sectionNum; sectionIdx++)
	{
		*rawSection = SkipDomainName(*rawSection);
		memcpy(&answerBody, *rawSection, sizeof(RESOURCE_RECORD));
		if (ntohs(answerBody.class) == CLASS_IN && ntohs(answerBody.type) == TYPE_A)
		{
			return FOUND;
		}
		*rawSection += sizeof(RESOURCE_RECORD) + ntohs(answerBody.rdlength);
	}

	return NOT_FOUND;
}


/**
* Checks whether the DNS response header is valid.
*/
int CheckResponseHeader(DNS_HEADER* dnsHeader)
{
	u_char isResponse = dnsHeader->qr;
	u_char responseCode = dnsHeader->rcode;

	if (isResponse != 1) { return INVALID; }

	if (responseCode != 0x0000) { return INVALID; }

	return VALID;
}


/**
* Given a raw DNS response message, finds its resource record which contains the translation answer (IPv4 address).
* @return pointer to the beginning of the found answer section, or NULL if no valid answer section was found.
*/
char* FindAnswerBody(char* rawResponse)
{
	DNS_HEADER dnsHeader;
	u_short questionNum;
	u_short answerNum;
	char* rawQuestion;
	int questionIdx;
	char* rawAnswer;
	int isFound;

	memcpy(&dnsHeader, rawResponse, sizeof(DNS_HEADER));
	questionNum = ntohs(dnsHeader.qdcount);
	answerNum = ntohs(dnsHeader.ancount);

	if (answerNum == 0) { return NULL; }

	if (CheckResponseHeader(&dnsHeader) != VALID) { return NULL; }


	rawQuestion = rawResponse + sizeof(DNS_HEADER);

	
	for (questionIdx = 0; questionIdx < questionNum; questionIdx++)
	{
		rawQuestion = SkipDomainName(rawQuestion);
		rawQuestion += sizeof(DNS_QUESTION);
	}

	rawAnswer = rawQuestion;

	isFound = SearchAnswerInSection(&rawAnswer, answerNum);
	if (isFound == FOUND) { return rawAnswer; }

	return NULL;
}


/**
* Parses raw DNS response messages.
* @return HOSTENT structure contains the resultant translated IP address, or NULL if no valid answer was given.
*/
HOSTENT* ParseResponse(char* rawResponse, char* domainName)
{
	HOSTENT* responseEntry;
	char** ipAddresses;
	char* answerIP;
	char* rawAnswer;
	

	// Find the valid answer's resource record
	rawAnswer = FindAnswerBody(rawResponse);
	if (rawAnswer == NULL) { return NULL; }


	// Fill the resultant HOSTENT structure
	responseEntry = (HOSTENT*)malloc(sizeof(HOSTENT));
	if (responseEntry == NULL)
	{
		perror("ERROR");
		exit(1);
	}

	responseEntry->h_name = domainName;
	responseEntry->h_addrtype = AF_INET;
	responseEntry->h_length = IP_ADDR_SIZE;
	responseEntry->h_aliases = NULL;

	ipAddresses = (char**)malloc(2 * sizeof(char*));
	if (ipAddresses == NULL)
	{
		perror("ERROR");
		exit(1);
	}
	ipAddresses[0] = (char*)malloc(IP_ADDR_SIZE);
	if (ipAddresses[0] == NULL)
	{
		perror("ERROR");
		exit(1);
	}
	answerIP = rawAnswer + sizeof(RESOURCE_RECORD);

	memcpy(ipAddresses[0], answerIP, IP_ADDR_SIZE);
	ipAddresses[1] = NULL;
	responseEntry->h_addr_list = ipAddresses;

	return responseEntry;
}


/**
* Prints textual error message.
*/
void printSocketError(int errorCode)
{
	if (errorCode == GET_ERR_CODE)
	{
		errorCode = WSAGetLastError();
	}

	fprintf(stderr, "ERROR: Failed with error code 0x%x\n", errorCode);
}