#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include <ctype.h>
#include <wchar.h>
#include <getopt.h>

#define SMS_MSG_MAX_SIZE 255
#define NUMBER_IN_7BIT_ASCII 0xD0
#define TIMESTAMP_STR_SIZE 21
#define TIMESTAMP_BUF_SIZE 7


typedef struct sms_entry {
	char dbId;
	char status;
	char* message_raw;
	unsigned char message_length;
	wchar_t message_ucs[255];
	unsigned char smscenter[255];
	unsigned char smscenter_length;
	wchar_t smssender[255];
	unsigned char smssender_length;
	char timestamp[TIMESTAMP_STR_SIZE];
	unsigned char protocol;
	unsigned char encoding_scheme;
} sms_entry_t;

typedef enum LOCALE_SUPPORT {
	LOCALE_SUPPORTED, LOCALE_NOT_SUPPORTED
} locale_support;

int gsmString2Bytes(const char* pSrc, unsigned char* pDst, size_t nSrcLength)
{
	size_t i;
	for (i = 0; i < nSrcLength; i += 2) {
		if (*pSrc >= '0' && *pSrc <= '9')
			*pDst = (*pSrc - '0') << 4;
		else
			*pDst = (*pSrc - 'A' + 10) << 4;

		pSrc++;

		if (*pSrc >= '0' && *pSrc <= '9')
			*pDst |= *pSrc - '0';
		else
			*pDst |= *pSrc - 'A' + 10;

		pSrc++;
		pDst++;
	}

	return nSrcLength / 2;
}

int gsmDecode7bit(const unsigned char* pSrc, wchar_t* pDst, int nSrcLength)
{
	int nSrc = 0;
	int nDst = 0;
	int nByte = 0;
	unsigned char nLeft = 0;

	while (nDst < nSrcLength) {
		*pDst = ((*pSrc << nByte) | nLeft) & 0x7f;
		if (*pDst < 32 )
			*pDst = 127;

		nLeft = *pSrc >> (7-nByte);

		pDst++;
		nDst++;
		nByte++;

		if (nByte == 7) {
			*pDst = nLeft;

			if (*pDst < 32 )
				*pDst = 127;

			pDst++;
			nDst++;
			nByte = 0;
			nLeft = 0;
		}
		pSrc++;
		nSrc++;
	}
	*pDst = 0;

	return nDst;
}

void swapByteArray(unsigned char* array, size_t length)
{
	unsigned char tmp;
	size_t i;
	for (i = 0 ; i < length; i++) {
		tmp = array[i];
		array[i] = (tmp >> 4) | (tmp << 4);
	}
}

void semiOctetToWString(unsigned char* array, wchar_t* outStr, size_t length)
{
	unsigned char tmp;
	size_t i, k;

	outStr[0]='+';

	for (k = 0, i = 1; k < length * 2; k++, i += 2) {
		outStr[i] = 0;
		outStr[i] = (array[k] >> 4) + 48;
		tmp = (array[k] & 0x0F);
		outStr[i+1] = tmp == 0xf ? '\0' : (array[k] & 0x0F) + 48;
	}
}

void convertTimestamp(unsigned char *timestampBuf, sms_entry_t* entry)
{
	unsigned char tmp[TIMESTAMP_STR_SIZE];
	size_t i;

	swapByteArray(timestampBuf, TIMESTAMP_BUF_SIZE);
	for (i = 0; i < TIMESTAMP_BUF_SIZE; i++) {
		tmp[i] = ((timestampBuf[i] >> 4)*10) + ((timestampBuf[i] & 0x0F));
	}

	sprintf((char*)entry->timestamp, "%02u/%02u/%02u %02u:%02u:%02u+%02u",
	        (unsigned)tmp[2], (unsigned)tmp[1], (unsigned)tmp[0], (unsigned)tmp[3],
	        (unsigned)tmp[4], (unsigned)tmp[5], (unsigned)tmp[6]);
}

void convertMessage(sms_entry_t* entry, locale_support localeSupport)
{
	unsigned char tmp[SMS_MSG_MAX_SIZE] = {0};
	unsigned char tmpChar;
	unsigned char bufPos = 2;
	size_t i, k;

	if (entry->dbId < 0 || entry->status < 0)
		return;

	gsmString2Bytes(entry->message_raw, tmp, strlen(entry->message_raw));

	entry->smscenter_length = tmp[0] - 1; /* -1 remove tel phone type field */
	memcpy(entry->smscenter, tmp + bufPos, entry->smscenter_length);
	swapByteArray(entry->smscenter, entry->smscenter_length);
	bufPos += entry->smscenter_length + 1; /* Remove first octet of SMS_DELIVER */

	entry->smssender_length = tmp[bufPos];
	if (entry->smssender_length % 2 != 0)
		entry->smssender_length++;

	entry->smssender_length /= 2;
	bufPos += 1;
	tmpChar = tmp[bufPos];
	bufPos += 1;
	if ((tmpChar & NUMBER_IN_7BIT_ASCII) == NUMBER_IN_7BIT_ASCII)
		gsmDecode7bit(tmp + bufPos, entry->smssender, entry->smssender_length);
	else {
		swapByteArray(tmp + bufPos, entry->smssender_length);
		semiOctetToWString(tmp + bufPos, entry->smssender, entry->smssender_length);
	}
	bufPos += entry->smssender_length;

	entry->protocol = tmp[bufPos];
	bufPos += 1;
	entry->encoding_scheme = tmp[bufPos];
	bufPos += 1;
	convertTimestamp(tmp + bufPos, entry);
	bufPos += 7;
	entry->message_length = tmp[bufPos];
	bufPos += 1;

	switch(entry->encoding_scheme & 0x0F) {
		case 0: /* 7Bit message */
			gsmDecode7bit(tmp + bufPos, entry->message_ucs, entry->message_length);
		break;
		case 4: /* 8Bit message */
			memcpy(entry->message_ucs, tmp + bufPos, entry->message_length);
		break;
		case 8: /* UCS-2 message */
			for (k = 0, i = 0; i < entry->message_length; k++, i += 2) {
				wchar_t tmpWChar = 0;
				tmpWChar = (tmp[bufPos+i] << 8) | tmp[bufPos+i+1];
				if (localeSupport == LOCALE_SUPPORTED)
					entry->message_ucs[k] = tmpWChar;
				else
					entry->message_ucs[k] = (tmpWChar < 20 || tmpWChar > 125) ? L' ' : tmpWChar;
			}
			entry->message_ucs[entry->message_length] = L'\0';
		break;
		default:
			swprintf(entry->message_ucs, 32, L"ERR Strange encoding occurs: %d!", entry->encoding_scheme);
	}
}

sms_entry_t* parse_modem_response(FILE* fp, size_t* count)
{
	ssize_t read;
	size_t len, dbLen = 0;
	unsigned tmp1;
	unsigned tmp2;
	unsigned tmp3, successfullyParsed;
	sms_entry_t* db = NULL;
	sms_entry_t* entry = NULL;
	char* currentLine = NULL;
	unsigned int add_entry = 0;

	while ((read = getline(&currentLine, &len, fp)) != -1) {
		read = strlen(currentLine);
		if (read < 2)
			continue;
		if ((strncmp("OK", currentLine, 2) == 0) || (strncmp("ERROR", currentLine, 5) == 0))
			break;

		if (strncmp("+CMGL: ", currentLine, 7) == 0) { /* +CMGL line, resize db */
			db = realloc(db, (dbLen + 1) * sizeof(sms_entry_t));
			entry = &db[dbLen];
			dbLen += 1;
			memset(entry, 0, sizeof(sms_entry_t));
			/* +CMGL: 18,1,,89 */
			successfullyParsed = sscanf(currentLine,
			                            "+CMGL: %u,%u,,%ud\n", &tmp1, &tmp2, &tmp3);
			                            entry->dbId = successfullyParsed == 3 ? (char)tmp1 : -1;
			                            entry->status = successfullyParsed == 3 ? (char)tmp2 : -1;
			                            entry->message_raw = NULL;
			if (successfullyParsed == 3)
				add_entry = 1;
		}
		else { /* PDU Line, copy PDU to db */
			if (add_entry == 1) { /* avoid SIGSEGV */
				entry->message_raw = calloc((read + 1), sizeof(char));
				strcpy(entry->message_raw, currentLine);
				entry->message_length = read;
				add_entry = 0;
			}
		}
	}
	*count = dbLen;
	free(currentLine);
	return db;
}

const char* translateMessageStatus(const unsigned char numericStatus)
{
	switch (numericStatus) {
		case 0: return "REC UNREAD";
		case 1: return "REC READ";
		case 2: return "STO UNREAD";
		case 3: return "STO READ";
		case 4: return "ALL"; /* ?? What does it mean? added for sake */
		default: return "ERROR";
	}
}

int main(int argc, char** argv)
{
	size_t entries_count, i;
	FILE *infile = stdin;
	FILE *outfile = stdout;
	sms_entry_t *parsed_entries;
	char pduEntries = 0;
	locale_support ucsSupport = LOCALE_NOT_SUPPORTED;

	int c;
	const char* short_opt = "hsf:w:";
	struct option long_opt[] =
	{
		{"help",  no_argument,       NULL, 'h'},
		{"stdin", no_argument,       NULL, 'h'},
		{"file",  required_argument, NULL, 'f'},
		{"out",   required_argument, NULL, 'w'},
		{NULL,    0,                 NULL,  0 }
	};

	while ((c = getopt_long (argc, argv, short_opt, long_opt, NULL)) != -1) {
		switch (c) {
			case 0:
			case -1:
			break;
			case 's':
				infile = stdin;
			break;
			case 'f':
				infile = fopen(optarg, "r");
				if (infile == NULL) {
					fprintf(stderr, "ERR Cannot open file %s!\n", optarg);
					return 1;
				}
			break;
			case 'w':
				outfile = fopen(optarg, "w");
				if (outfile == NULL) {
					fprintf(stderr, "ERR Cannot open output file %s!\n", optarg);
					return 1;
				}
			break;
			case 'h':
				printf("Usage: \
				        \n\t-h, --help  Print this help \
				        \n\t-f, --file  FILE: Get input from file instead of stdin \
				        \n\t-w, --out   FILE: Use output file instead of stdout \
				        \n\t-s, --stdin FILE: Get input from stdin (default)\n");
				return 0;
			break;
			case '*':
			case ':':
			case '?':
				fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
				return(-2);
			default:
				fprintf(stderr, "%s: invalid option -- %c\n", argv[0], c);
				fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
				return(-2);
		}
	};

	parsed_entries = parse_modem_response(infile, &entries_count);

	if (setlocale(LC_ALL,  "C.UTF-8") != NULL)
		ucsSupport = LOCALE_SUPPORTED;

	for (i = 0; i < entries_count; i++) {
		sms_entry_t* entry = &parsed_entries[i];
		if (entry->status > 0) {
			pduEntries++;
			convertMessage(entry, ucsSupport);
			fwprintf(outfile, L"ID: %u [%s][%s][%ls]: %ls\n",
			         entry->dbId,
			         translateMessageStatus(entry->status),
			         entry->timestamp,
			         entry->smssender,
			         entry->message_ucs);
		}
		free(entry->message_raw);
	}
	free(parsed_entries);
	fclose(infile);

	if (entries_count == 0)
		fwprintf(stderr, L"No parseable entries found!\n");
	else if (pduEntries == 0 && entries_count > 0)
		fwprintf(stderr, L"No correct PDU entries found!\n");

	return 0;
}
