#pragma once
#include <string>
#include <vector>
#include  "Structs.h"


//Length 	Chunk type 	Chunk data 	    CRC
//4 bytes 	4 bytes 	Length bytes 	4 bytes

struct chunk
{
	uint32_t length;
	char type[4];
	char* data;
	uint32_t CRC;

	const static uint32_t Chunk_Limit = INT32_MAX; //2^20
};





class PNG
{
public:
	PNG(std::wstring path);
	PNG();
	~PNG();

	void Read(std::wstring path);
	void ReadPixels(std::vector<pixel8>& pixels, int width, int height, int type = 2, int depth = 8, int compression = 0, int filter = 0, int interlace = 0);
	void Write(std::wstring path);

	std::vector<pixel8> GetPixels();
	int GetWidth();
	int GetHeight();
	int GetDepth();
	int GetChannels();

	template <typename T>
	T swap_endian(T u);

private:

	void DataToPixel(char* data, uint32_t length);
	void MergeIDATChucks(std::vector<chunk*> IDATChunks);

	uint32_t UncompressedLength();

	void Filter(int type, uint32_t current, char* data);
	void Unfilter(uint32_t current, uint32_t length, char* data);
	//filters
	char Sub(uint32_t index, char* data);
	char Up(uint32_t index, char* data);
	char Average(uint32_t index, char* data);
	char Paeth(uint32_t index, char* data); //unimplemented


	//write funcs
	chunk* Make_IHDR();
	std::vector<chunk*>* Make_IDAT();
	void writeChunk(chunk* chk, std::ostream& writer);

	void WriteColor(const pixel8& src, char* dest);
	void WriteGray(const pixel8& src, char* dest);
	void WriteAlpha(const pixel8& src, char* dest);
	void WritePalette(const pixel8& src, char* dest);



	//read funcs
	void Read_IHDR(chunk* data);
	void Read_IDAT();

	void ReadColor(pixel8& dest,const char* src);
	void ReadGray(pixel8& dest, const char* src);
	void ReadAlpha(pixel8& dest, const char* src);
	void ReadPalette(pixel8& dest, const char* src); //unimplemented
	

	char m_header[8];

	//internal IHDR (header)
	uint32_t m_width, m_height;
	uint8_t m_depth{ 8 }, m_colortype{ 2 }, m_compression{ 0 }, m_filter{ 0 }, m_interlace{0};

	//internal IDAT (data)
	uint32_t m_length{0};
	char* m_data;

	std::vector<pixel8> m_pixels;



};


