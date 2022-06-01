#include "utils.h"


int CheckDomainName(char* domainName)
{
	return 1; // TODO
}


char* EncodeDomainName(char* domainName)
{
	char* encodedName = (char*)malloc(256); // Validate the length
	int dstIdx = 0;
	int startIdx = 0;
	int endIdx = 0;
	int labelLen;

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


char* BuildHeaderSection()
{
	DNS_HEADER* dnsHeader = (DNS_HEADER*)malloc(sizeof(DNS_HEADER));

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


char* BuildQuestionSection()
{
	DNS_QUESTION* dnsQuestion = (DNS_QUESTION*)malloc(sizeof(DNS_QUESTION));

	dnsQuestion->qtype = htons(TYPE_A);	     // A  (a host address)
	dnsQuestion->qclass = htons(CLASS_IN);   // IN (Internet)

	return dnsQuestion;
}


char* BuildQuery(char* domainName, u_int* queryLen)
{
	// Build Header & Question sections
	DNS_HEADER* dnsHeader = BuildHeaderSection();
	DNS_QUESTION* dnsQuestion = BuildQuestionSection();

	// Encode domain name
	char* encodedName = EncodeDomainName(domainName);
	unsigned int nameLen = strlen(encodedName) + 1;

	// Compose whole message
	*queryLen = sizeof(DNS_HEADER) + nameLen + sizeof(DNS_QUESTION);
	char* query = (char*)malloc(*queryLen);
	char* pointer = query;

	memcpy(pointer, dnsHeader, sizeof(DNS_HEADER));
	pointer += sizeof(DNS_HEADER);
	memcpy(pointer, encodedName, nameLen);
	pointer += nameLen;
	memcpy(pointer, dnsQuestion, sizeof(DNS_QUESTION));

	free(dnsHeader);
	free(dnsQuestion);

	return query;
}


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


char* FindAnswerBody(char* rawResponse)
{
	DNS_HEADER dnsHeader;
	memcpy(&dnsHeader, rawResponse, sizeof(DNS_HEADER));
	u_short questionNum = ntohs(dnsHeader.qdcount);
	u_short answerNum = ntohs(dnsHeader.ancount);
	u_short authorativeNum = ntohs(dnsHeader.nscount);
	u_short additionalNum = ntohs(dnsHeader.arcount);

	if (answerNum == 0) { return NULL; }

	char* rawQuestion = rawResponse + sizeof(DNS_HEADER);
	
	int questionIdx;
	
	for (questionIdx = 0; questionIdx < questionNum; questionIdx++)
	{
		rawQuestion = SkipDomainName(rawQuestion);
		rawQuestion += sizeof(DNS_QUESTION);
	}

	char* rawAnswer = rawQuestion;

	int isFound = SearchAnswerInSection(&rawAnswer, answerNum);
	if (isFound == FOUND) { return rawAnswer; }

	return NULL;
}


HOSTENT* ParseResponse(char* rawResponse, char* domainName)
{
	HOSTENT* responseEntry = (HOSTENT*)malloc(sizeof(HOSTENT));
	char* rawAnswer = FindAnswerBody(rawResponse);
	if (rawAnswer == NULL) { return NULL; }

	responseEntry->h_name = domainName;
	responseEntry->h_addrtype = AF_INET;
	responseEntry->h_length = 4;
	responseEntry->h_aliases = NULL;
	char** addresses = (char**)malloc(2 * sizeof(char*));
	addresses[0] = (char*)malloc(sizeof(u_long));
	memcpy(addresses[0], (rawAnswer + sizeof(RESOURCE_RECORD)), 4);
	addresses[1] = NULL;
	responseEntry->h_addr_list = addresses;

	return responseEntry;
}