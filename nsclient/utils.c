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
	DNS_HEADER_T* dnsHeader = (DNS_HEADER_T*)malloc(sizeof(DNS_HEADER_T));

	dnsHeader->id = htons(rand() * 2);
	dnsHeader->qr = 0;
	dnsHeader->opcode = 0;
	dnsHeader->aa = 0;
	dnsHeader->tc = 0;
	dnsHeader->rd = 0;
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

	dnsQuestion->qtype = htons(1);    // A  (a host address)
	dnsQuestion->qclass = htons(1);   // IN (Internet)

	return dnsQuestion;
}


char* BuildQuery(char* domainName, u_int* queryLen)
{
	// Build Header & Question sections
	DNS_HEADER_T* dnsHeader = BuildHeaderSection();
	DNS_QUESTION* dnsQuestion = BuildQuestionSection();

	// Encode domain name
	char* encodedName = EncodeDomainName(domainName);
	unsigned int nameLen = strlen(encodedName) + 1;

	// Compose whole message
	*queryLen = sizeof(DNS_HEADER_T) + nameLen + sizeof(DNS_QUESTION);
	char* query = (char*)malloc(*queryLen);
	char* pointer = query;

	memcpy(pointer, dnsHeader, sizeof(DNS_HEADER_T));
	pointer += sizeof(DNS_HEADER_T);
	memcpy(pointer, encodedName, nameLen);
	pointer += nameLen;
	memcpy(pointer, dnsQuestion, sizeof(DNS_QUESTION));

	free(dnsHeader);
	free(dnsQuestion);

	return query;
}


HOSTENT* ParseResponse(char* rawResponse)
{
	DNS_MESSAGE_BUFFER dnsResponse;
	DNS_RECORD records;
	memcpy(&dnsResponse.MessageHead, rawResponse, sizeof(DNS_HEADER));
	rawResponse += sizeof(DNS_HEADER);
	memcpy(&dnsResponse.MessageBody, rawResponse, 512 - sizeof(DNS_HEADER));
	DnsExtractRecordsFromMessage_UTF8(&dnsResponse, 512, &records);
	unsigned int resultIp = records.pNext->Data.A.IpAddress;

	HOSTENT* responseEntry = (HOSTENT*)malloc(sizeof(HOSTENT));
	responseEntry->h_name = *(records.pNext->pName);
	char** aliases = (char**)malloc(sizeof(char*));
	aliases[0] = NULL;
	responseEntry->h_aliases = aliases;
	responseEntry->h_addrtype = AF_INET;
	responseEntry->h_length = sizeof(unsigned int);
	char** addresses = (char**)malloc(2*sizeof(char*));
	addresses[0] = resultIp;
	addresses[1] = NULL;
	responseEntry->h_addr_list = addresses;

	return responseEntry;
}