#pragma once
#include <stdint.h>

class CRC32
{
public:
	CRC32() = delete;
	~CRC32() = delete;
	CRC32(const CRC32&) = delete;
	CRC32(CRC32&&) = delete;
	CRC32& operator=(const CRC32&) = delete;
	CRC32& operator=(CRC32&&) = delete;


	static uint32_t GenerateCRC32(char* buf, int len);
	static uint32_t GeneratePNGCRC32(char* type, int len1, char * buf, int len2);

private:

	static void init_table();
	static uint32_t Update_CRC32(uint32_t crc, char *buf, int len);
	static uint32_t Update_PNGCRC32(uint32_t crc, char* type, int len1, char * buf, int len2);


	static uint32_t crc_table[256];
	static bool table_computed;
};

