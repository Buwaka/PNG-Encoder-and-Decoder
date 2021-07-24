#include "pch.h"
#include "CRC32.h"

bool CRC32::table_computed = false;
uint32_t CRC32::crc_table[256];

void CRC32::init_table()
{

	for (int n = 0; n < 256; n++) {
		uint32_t c = (uint32_t)n;
		for (int k = 0; k < 8; k++) {
			if (c & 1)
				c = 0xedb88320L ^ (c >> 1);
			else
				c = c >> 1;
		}
		crc_table[n] = c;
	}
	table_computed = true;
}


uint32_t CRC32::Update_CRC32(uint32_t crc, char *buf, int len)
{
	uint32_t c = crc;

	for (int n = 0; n < len; n++) {
		c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
	}
	return c;
}

uint32_t CRC32::Update_PNGCRC32(uint32_t crc, char* type, int len1, char * buf, int len2)
{
	uint32_t c = crc;

	for (int n = 0; n < len1; n++) {
		c = crc_table[(c ^ type[n]) & 0xff] ^ (c >> 8);
	}

	for (int n = 0; n < len2; n++) {
		c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
	}
	return c;
}

uint32_t CRC32::GenerateCRC32(char * buf, int len)
{
	if (!table_computed)
		init_table();



	return Update_CRC32(0xffffffffL, buf, len) ^ 0xffffffffL;
}

uint32_t CRC32::GeneratePNGCRC32(char* type, int len1, char * buf, int len2)
{
	if (!table_computed)
		init_table();



	return Update_PNGCRC32(0xffffffffL,type, len1, buf, len2) ^ 0xffffffffL;
}