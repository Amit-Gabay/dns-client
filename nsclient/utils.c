#include "utils.h"

int transactionID = 0;
int errorCode = NO_ERR;

int CheckDomainName(char *domainName)
{
	int i = 0;
	int j = 0;

	while (domainName[i] != 0)
	{
		if (!((domainName[i] >= '0' && domainName[i] <= '9')	// digits
			  || (domainName[i] >= 'A' && domainName[i] <= 'Z') // Capital letters
			  || (domainName[i] >= 'a' && domainName[i] <= 'z') // letters
			  || (domainName[i] == '.')
			  || (domainName[i] == '-')))
		{
			return 0;
		}
		if (domainName[i] != '.') { j++; }
		else if (j > 63 || j == 0)
		{
			return 0;
		}
		else { j = 0; }
		i++;
	}
	if (i > 255)
	{
		return 0;
	}

	return 1;
}

/**
 * Encodes a null-terminated domain name into labels based encoding (as in RFC-1035).
 */
char *EncodeDomainName(char *domainName)
{
	char clonedName[256];
	char *encodedName;
	int dstIdx = 0;
	int startIdx = 0;
	int endIdx = 0;
	int labelLen;

	encodedName = (char *)malloc(256); // **TODO: Validate length**
	if (encodedName == NULL)
	{
		PrintError();
		exit(1);
	}

	// Clone given domain name, for original domain name won't be corrupted
	strcpy(clonedName, domainName);

	// Splits domain name by replacing each "." in the cloned domain name by NULL.
	while (clonedName[endIdx] != '\0')
	{
		if (clonedName[endIdx] == '.')
		{
			clonedName[endIdx] = '\0';
			labelLen = endIdx - startIdx;
			encodedName[dstIdx++] = (char)labelLen;
			strcpy((encodedName + dstIdx), (clonedName + startIdx));
			dstIdx += labelLen;
			startIdx = endIdx + 1;
		}
		++endIdx;
	}

	labelLen = endIdx - startIdx;
	encodedName[dstIdx++] = (char)labelLen;
	strcpy((encodedName + dstIdx), (clonedName + startIdx));
	startIdx = endIdx;
	dstIdx += labelLen;
	encodedName[dstIdx] = (char)0;

	return encodedName;
}

/**
 * Creates header section for the DNS query to be sent.
 */
DNS_HEADER *BuildHeaderSection()
{
	DNS_HEADER *dnsHeader = (DNS_HEADER *)malloc(sizeof(DNS_HEADER));
	if (dnsHeader == NULL)
	{
		PrintError();
		exit(1);
	}

	dnsHeader->id = transactionID++;
	dnsHeader->qr = 0; // It's a query
	dnsHeader->opcode = 0;
	dnsHeader->aa = 0;
	dnsHeader->tc = 0;
	dnsHeader->rd = 0x1; // Recursion is desired
	dnsHeader->ra = 0;
	dnsHeader->z = 0x000;
	dnsHeader->rcode = 0x0000;
	dnsHeader->qdcount = htons(1); // There's a single question
	dnsHeader->ancount = 0;
	dnsHeader->nscount = 0;
	dnsHeader->arcount = 0;

	return dnsHeader;
}

/**
 * Creates question section for the DNS query to be sent.
 */
DNS_QUESTION *BuildQuestionSection()
{
	DNS_QUESTION *dnsQuestion = (DNS_QUESTION *)malloc(sizeof(DNS_QUESTION));
	if (dnsQuestion == NULL)
	{
		PrintError();
		exit(1);
	}

	dnsQuestion->qtype = htons(TYPE_A);	   // A  (a host address)
	dnsQuestion->qclass = htons(CLASS_IN); // IN (Internet)

	return dnsQuestion;
}

char *BuildQuery(char *domainName, u_int *queryLen)
{
	DNS_HEADER *dnsHeader;
	DNS_QUESTION *dnsQuestion;
	char *encodedName;
	char *query;
	char *pointer;
	unsigned int nameLen;

	// Build Header & Question sections
	dnsHeader = BuildHeaderSection();
	dnsQuestion = BuildQuestionSection();

	// Encode domain name
	encodedName = EncodeDomainName(domainName);
	nameLen = (unsigned int)strlen(encodedName) + 1;

	// Compose whole DNS message
	*queryLen = sizeof(DNS_HEADER) + nameLen + sizeof(DNS_QUESTION);
	query = (char *)malloc(*queryLen);
	if (query == NULL)
	{
		PrintError();
		exit(1);
	}
	// for (int i = 0; i < *queryLen; i++)
	// {
	// 	query[i] = 0;
	// }
	pointer = query;

	memcpy(pointer, dnsHeader, sizeof(DNS_HEADER));
	pointer += sizeof(DNS_HEADER);
	memcpy(pointer, encodedName, nameLen);
	pointer += nameLen;
	memcpy(pointer, dnsQuestion, sizeof(DNS_QUESTION));

	free(dnsHeader);
	free(dnsQuestion);
	free(encodedName);

	return query;
}

/**
 * When parsing a DNS message, domain names in it could be skipped using this function.
 */
char *SkipDomainName(char *rawSection)
{
	u_char labelSize;
	char *tmp = rawSection;

	do
	{
		labelSize = (u_char)rawSection[0];
		// Check whether it's a compressed suffix
		if ((labelSize >> 6) == 0b11)
		{
			return rawSection + sizeof(unsigned short);
		}
		rawSection += labelSize + 1; // TODO arothmetic overflow might be problematic on long addreses
	} while (labelSize != 0);

	return rawSection;
}

/**
 * Searches for an answer to our DNS translation request in a given message section.
 */
int SearchAnswerInSection(char **rawSection, int sectionNum)
{
	RESOURCE_RECORD answerBody;
	int sectionIdx;

	// Search for the first answer section with the desired answer type (if there is)
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
int CheckResponseHeader(DNS_HEADER *dnsHeader)
{
	u_char isResponse = dnsHeader->qr;
	u_char responseCode = dnsHeader->rcode;

	if (isResponse != 1)
	{
		errorCode = DNS_NON_EXIST;
		return INVALID;
	}

	if (responseCode != 0x0000)
	{
		if (responseCode == 0x0001)
		{
			errorCode = DNS_BAD_FORMAT;
		}
		else if (responseCode == 0x0002)
		{
			errorCode = DNS_SERVER_FAIL;
		}
		else if (responseCode == 0x0003)
		{
			errorCode = DNS_NON_EXIST;
		}
		else if (responseCode == 0x0004)
		{
			errorCode = DNS_NOT_SUPPORT;
		}
		else if (responseCode == 0x0005)
		{
			errorCode = DNS_REFUSED;
		}
		else
		{
			errorCode = DNS_BAD_RCODE;
		}
		return INVALID;
	}

	return VALID;
}

/**
 * Given a raw DNS response message, finds its resource record which contains the translation answer (IPv4 address).
 * @return pointer to the beginning of the found answer section, or NULL if no valid answer section was found.
 */
char *FindAnswerBody(char *rawResponse)
{
	DNS_HEADER dnsHeader;
	u_short questionNum;
	u_short answerNum;
	char *rawQuestion;
	int questionIdx;
	char *rawAnswer;
	int isFound;

	// Extract response header
	memcpy(&dnsHeader, rawResponse, sizeof(DNS_HEADER));
	questionNum = ntohs(dnsHeader.qdcount);
	answerNum = ntohs(dnsHeader.ancount);

	if (CheckResponseHeader(&dnsHeader) != VALID)
	{
		return NULL;
	}

	if (answerNum == 0)
	{
		errorCode = DNS_NON_EXIST;
		return NULL;
	}

	// Find where the relevant answer begins (if there is)
	rawQuestion = rawResponse + sizeof(DNS_HEADER);

	for (questionIdx = 0; questionIdx < questionNum; questionIdx++)
	{
		rawQuestion = SkipDomainName(rawQuestion);
		rawQuestion += sizeof(DNS_QUESTION);
	}

	rawAnswer = rawQuestion;

	isFound = SearchAnswerInSection(&rawAnswer, answerNum);
	if (isFound == FOUND)
	{
		return rawAnswer;
	}

	errorCode = DNS_NON_EXIST;
	return NULL;
}

/**
 * Parses raw DNS response messages.
 * @return HOSTENT structure contains the resultant translated IP address, or NULL if no valid answer was given.
 */
HOSTENT *ParseResponse(char *rawResponse, char *domainName)
{
	HOSTENT *responseEntry;
	char **ipAddresses;
	char *answerIP;
	char *rawAnswer;

	// Find the valid answer's resource record
	rawAnswer = FindAnswerBody(rawResponse);
	if (rawAnswer == NULL)
	{
		return NULL;
	}

	// Fill the resultant HOSTENT structure
	responseEntry = (HOSTENT *)malloc(sizeof(HOSTENT));
	if (responseEntry == NULL)
	{
		PrintError();
		exit(1);
	}

	responseEntry->h_name = domainName;
	responseEntry->h_addrtype = AF_INET;
	responseEntry->h_length = IP_ADDR_SIZE;
	responseEntry->h_aliases = NULL;

	ipAddresses = (char **)malloc(2 * sizeof(char *));
	if (ipAddresses == NULL)
	{
		PrintError();
		exit(1);
	}
	ipAddresses[0] = (char *)malloc(IP_ADDR_SIZE);
	if (ipAddresses[0] == NULL)
	{
		PrintError();
		exit(1);
	}
	answerIP = rawAnswer + sizeof(RESOURCE_RECORD);

	memcpy(ipAddresses[0], answerIP, IP_ADDR_SIZE);
	ipAddresses[1] = NULL;
	responseEntry->h_addr_list = ipAddresses;

	return responseEntry;
}

/**
 * Frees HOSTENT structure and its fields.
 */
void FreeHostEnt(HOSTENT *hostent)
{
	free(hostent->h_addr_list[0]);
	free(hostent->h_addr_list);
	free(hostent);
}

/**
 * Prints textual socket-based error message.
 */
void PrintSocketError()
{
	if (errorCode == NO_ERR)
	{
		errorCode = WSAGetLastError();
	}

	if (errorCode == DNS_BAD_NAME)
	{
		fprintf(stderr, "ERROR: BAD NAME\n");
	}

	else if (errorCode == DNS_BAD_FORMAT)
	{
		fprintf(stderr, "ERROR: BAD FORMAT\n");
	}

	else if (errorCode == DNS_SERVER_FAIL)
	{
		fprintf(stderr, "ERROR: SERVER FAILURE\n");
	}

	else if (errorCode == DNS_NON_EXIST)
	{
		fprintf(stderr, "ERROR: NONEXISTENT\n");
	}

	else if (errorCode == DNS_NOT_SUPPORT)
	{
		fprintf(stderr, "ERROR: REQUEST NOT SUPPORTED\n");
	}

	else if (errorCode == DNS_REFUSED)
	{
		fprintf(stderr, "ERROR: OPERATION REFUSED\n");
	}

	else if (errorCode == DNS_BAD_RCODE)
	{
		fprintf(stderr, "ERROR: BAD RESPONSE CODE\n");
	}

	else if (errorCode == DNS_BAD_NAME)
	{
		fprintf(stderr, "ERROR: BAD NAME\n");
	}

	else if (errorCode == BAD_IP_ADDR)
	{
		fprintf(stderr, "ERROR: BAD IP ADDRESS");
	}

	else
	{
		fprintf(stderr, "ERROR: FAILED WITH ERROR CODE 0x%x\n", errorCode);
	}
	errorCode = 0;
}

/**
 * Prints textual error message.
 */
void PrintError()
{
	perror("ERROR");
}