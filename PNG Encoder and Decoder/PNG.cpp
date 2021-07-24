#include "pch.h"
#include "PNG.h"
#include <fstream>
#include <cctype>
#include <iostream> 
#include <algorithm>

#include <zlib.h>
#include "CRC32.h"

PNG::PNG(std::wstring path)
{
	Read(path);
}

PNG::PNG()
{
}

PNG::~PNG()
{
	delete[] m_data;
}

std::vector<pixel8> PNG::GetPixels()
{
	return m_pixels;
}

int PNG::GetWidth()
{
	return m_width;
}

int PNG::GetHeight()
{
	return m_height;
}

int PNG::GetDepth()
{
	return m_depth;
}

int PNG::GetChannels()
{
	int channels;

	switch (m_colortype)
	{
	case 0: channels = 1;
		break;
	case 3: channels = 1;
		break;
	case 2: channels = 3;
		break;
	case 4:	channels = 2;
		break;
	case 6: channels = 4;
		break;
	default: channels = 1;
		break;
	}

	return channels;
}

void PNG::Read(std::wstring  path)
{
	std::ifstream reader;

	reader.open(path, std::ios::binary | std::ios::in);

	//8 bytes worth of header
	reader.read((char *)&m_header, sizeof(char) * 8);


	//read full file
	bool end = false;
	std::vector<chunk*> IDATChunks;
	do
	{
		chunk *temp = new chunk{};
		//length
		reader.read((char *)&(temp->length), sizeof(temp->length));
		//type
		reader.read((char *)&(temp->type), sizeof(temp->type));

		//check if end chunk is reached to stop process
		if (std::strstr(temp->type, "IEND") == nullptr)
		{
			temp->length = swap_endian(temp->length);

			temp->data = new char[temp->length];

			//data
			reader.read(temp->data, temp->length);

			//checksum
			reader.read((char *)&(temp->CRC), sizeof(temp->CRC));

			//is critical chunk?
			if (std::isupper(temp->type[0]))
			{
				//IHDR
				if (std::strstr(temp->type, "IHDR"))
				{
					//process header
					Read_IHDR(temp);
				}
				//IDAT
				else if (std::strstr(temp->type, "IDAT"))
				{
					//merge later
					IDATChunks.push_back(temp);
				}
				//PLTE
				else if (std::strstr(temp->type, "PLTE"))
				{
					//not implemented
				}
				else //not actually a critical chunk, skip
				{
					delete temp;
				}
			}
			else
			{
				delete temp;
			}
		}
		else
			end = true;
		
	} while (!reader.eof() && !end); //or IEND

	reader.close();


	MergeIDATChucks(IDATChunks);

	Read_IDAT();

}

void PNG::ReadPixels(std::vector<pixel8>& pixels, int width, int height, int type, int depth, int compression, int filter, int interlace)
{
	m_width = width;
	m_height = height;
	m_colortype = type;
	m_depth = depth;
	m_compression = compression;
	m_filter = filter;
	m_interlace = interlace;

	if (m_length > 0)
	{
		delete[] m_data;
	}

	m_data = new char[((m_width * m_height) * (m_depth / 8) * GetChannels()) + m_height];

	unsigned long index = 0;
	for (size_t i = 0; i < m_height; i++)
	{
		int hgt = i * m_width;

		m_data[index] = (uint32_t)m_filter;
		index++;

		for (size_t j = 0; j < m_width; j++)
		{
			switch (m_colortype)
			{

				//grayscale
			case 0:
				WriteGray(pixels[j + hgt], &m_data[index]);
				index++;
				break;

				//truecolor (rgb)
			case 2:
				WriteColor(pixels[j + hgt], &m_data[index]);
				index += 3;
				break;

				//indexed (palette)
			case 3:
				WritePalette(pixels[j + hgt], &m_data[index]);
				index++;
				break;

				//grayscale & alpha
			case 4:
				WriteGray(pixels[j + hgt], &m_data[index]);
				index++;

				WriteAlpha(pixels[j + hgt], &m_data[index]);
				index++;
				break;

				//truecolor & alpha
			case 6:
				WriteColor(pixels[j + hgt], &m_data[index]);
				index += 3;

				WriteAlpha(pixels[j + hgt], &m_data[index]);
				index++;
				break;
			}
		}
	}

	m_length = index;


	uint32_t outputLength = compressBound(m_length);
	char* output = new char[outputLength];

	z_stream deflstream;
	deflstream.zalloc = Z_NULL;
	deflstream.zfree = Z_NULL;
	deflstream.opaque = Z_NULL;
	deflstream.data_type = Z_BINARY;

	deflstream.avail_in = m_length; // size of input
	deflstream.next_in = (Bytef *)m_data; // input char array
	deflstream.avail_out = outputLength; // size of output
	deflstream.next_out = (Bytef *)output; // output char array

	//// the actual compression work.
	if (int ret = deflateInit(&deflstream, m_compression) != Z_OK)
		std::cout << "failed to init inflate, error code: " << ret;
	else
	{
		if (deflate(&deflstream, Z_FINISH) < 0)
		{
			std::cout << deflstream.msg << std::endl;
		}
		else
		{
			delete[] m_data;
			m_data = output;
			m_length = deflstream.total_out;
		}
		deflateEnd(&deflstream);
	}



}

void PNG::Write(std::wstring path)
{
	std::ofstream writer;
	writer.open(path, std::ios::binary | std::ios::out);

	if (!writer.is_open()) 
	{
		std::cout << "failed to open write file\n";
		return;
	}

	//png header
	char header[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
	writer.write(header, 8);

	//IHDR
	chunk* IHDR = Make_IHDR();
	writeChunk(IHDR, writer);


	//IDAT
	std::vector<chunk*>* IDATChunks;
	IDATChunks = Make_IDAT();
	for (size_t i = 0; i< IDATChunks->size(); i++)
	{
		writeChunk((*IDATChunks)[i], writer);
	}

	//IEND
	//length
	uint32_t end{0};
	writer.write((const char *)&end, 4);
	memcpy(&end, "IEND", 4);
	writer.write((const char *)&end, 4);
	end = 0;
	end = CRC32::GenerateCRC32((char*)&end, 4);
	writer.write((const char *)&end, 4);


	if (writer.bad()) 
	{
		std::cout << "failed to write file\n";
		writer.close();
		return;
	}

	writer.close();

	delete IHDR;
	for (size_t i = 0; i < IDATChunks->size(); i++)
	{
		delete (*IDATChunks)[i];
	}
	delete IDATChunks;

}

void PNG::DataToPixel(char* data, uint32_t length)
{
	//resize pixelcount
	m_pixels.reserve((m_width * m_height));

	if (data[0] != 0)
	{
		std::cout << "unsupported filter type, image will be garbage\n";
	}

	uint32_t index = 0;
	uint32_t pixels = 0;
	while ( index < length)
	{
		//process filter byte per scanline (width)
		if (pixels % m_width == 0)
		{
			//BytesPerPixel
			int bpp = GetChannels() * (m_depth / 8);
			uint32_t scanlinewidth = (bpp * m_width) + 1; //plus filter byte
			Unfilter(index, scanlinewidth, data);
			index++;
		}


			pixel8 temp;
		
	
		switch (m_colortype)
		{

			//grayscale
		case 0:
			ReadGray(temp, (const char*)&data[index]);
			index++;
			break;

			//truecolor (rgb)
		case 2: 
			ReadColor(temp, (const char*)&data[index]);
			index += 3; 
			break;

			//indexed (palette)
		case 3:
			ReadPalette(temp, (const char*)&data[index]);
			index++;
			break;

			//grayscale & alpha
		case 4:
			ReadGray(temp, (const char*)&data[index]);
			index++;

			ReadAlpha(temp, (const char*)&data[index]);
			index++;
			break;

			//truecolor & alpha
		case 6: 
			ReadColor(temp, (const char*)&data[index]);
			index += 3; 

			ReadAlpha(temp, (const char*)&data[index]);
			index++;
			break;
		}
		m_pixels.push_back( temp);
		pixels++;
		
	}
}

void PNG::MergeIDATChucks(std::vector<chunk*> IDATChunks)
{
	//merge IDAT chunks by standard
	uint32_t newsize = 0;
	for (auto& chk : IDATChunks)
	{
		newsize += chk->length;
	}

	m_length = newsize;
	m_data = new char[newsize];
	uint64_t index = 0;
	for (size_t i = 0; i < IDATChunks.size(); i++)
	{
		memcpy(&m_data[index], IDATChunks[i]->data, IDATChunks[i]->length);
		index += IDATChunks[i]->length;

		//memcpy(&m_data[index], &IDATChunks[i]->CRC, 4);

		delete IDATChunks[i]->data;
		delete IDATChunks[i];
	}
}

void PNG::Filter(int type, uint32_t index, char * data)
{
	//unimplemented
}

//index is start of scanline including filter byte, length is length of scanline, pointer to data
void PNG::Unfilter(uint32_t current, uint32_t length, char * data)
{
	char filterByte = data[current];
	uint32_t index = 1;

	while (index < length)
	{

		switch (filterByte)
		{
			//None
		case 0: //do nothing
			break;

			//Sub
		case 1: data[index] = data[index] + Sub(index,data);
			break;

			//Up
		case 2: data[current + index] = data[current + index] + Up(current + index, data);
			break;

			//Average
		case 3: data[current + index] = data[current + index] + Average(current + index, data);
			break;

			//Paeth
		case 4: data[current + index] = data[current + index] + Paeth(current + index, data);
			break;
		}

		index ++;
	}
}

char PNG::Sub(uint32_t index, char* data)
{
	//BytesPerPixel
	uint8_t bpp = GetChannels() * (m_depth / 8);
	if (bpp > index)
		return data[index];
	else
		return data[index - bpp];
}

char PNG::Up(uint32_t index, char* data)
{
	//BytesPerPixel
	uint8_t bpp = GetChannels() * (m_depth / 8);

	//full scanline plus filter byte
	uint32_t prior = (bpp * m_width) + 1;

	if (prior > index)
		return data[index];
	else
		return data[index - prior];
}

char PNG::Average(uint32_t index, char* data)
{
	//BytesPerPixel
	uint8_t bpp = GetChannels() * (m_depth / 8);

	//full scanline plus filter byte
	uint32_t prior = (bpp * m_width) + 1;

	uint32_t up;
	if (prior > index)
		up = data[index];
	else
		up = data[index - prior];

	uint32_t pre;
	if (bpp > index)
		pre = data[index];
	else
		pre = data[index - bpp];

	//floor((Raw(x - bpp) + Prior(x)) / 2)
	return (char) std::floorf(float(data[index - bpp] + data[index - prior]) / 2);
}

char PNG::Paeth(uint32_t index, char* data)
{
	//unimplemented
	return 0;
}

chunk * PNG::Make_IHDR()
{
	chunk *temp = new chunk;
	uint32_t eSwap;

	//it contains (in this order) the image's width (4 bytes), height (4 bytes), bit depth (1 byte), color type (1 byte), compression method (1 byte), filter method (1 byte), and interlace method (1 byte) (13 data bytes total)
	//CRC needs to be calculated with type
	char* IHDR = new char[13];



	//IHDR
	eSwap = swap_endian(m_width);
	memcpy(&IHDR[0], &eSwap, 4);

	eSwap = swap_endian(m_height);
	memcpy(&IHDR[4], &eSwap, 4);

	IHDR[8] = m_depth;
	IHDR[9] = m_colortype;
	IHDR[10] = m_compression;
	IHDR[11] = m_filter;
	IHDR[12] = m_interlace;

	//length
	temp->length = 13;
	
		//type
	memcpy(temp->type, "IHDR", 4);

	//data
	temp->data = new char[13];
	memcpy(temp->data, IHDR, 13);

	//crc32
	temp->CRC = CRC32::GeneratePNGCRC32(temp->type, 4, IHDR, 13);


	delete IHDR;

	return temp;
}

std::vector<chunk*>* PNG::Make_IDAT()
{
	unsigned int numChunks = (int) std::ceilf((float)m_length / chunk::Chunk_Limit);

	std::vector<chunk*>* chunks = new std::vector<chunk*>;

	uint32_t fullLength = m_length;
	unsigned long index = 0;
	for (size_t i = 0; i < numChunks; i++)
	{
		chunk* temp = new chunk;

		//length
		temp->length = std::min(fullLength, chunk::Chunk_Limit);

		//type
		memcpy(temp->type, "IDAT", 4);

		//data
		temp->data = new char[temp->length];
		memcpy(temp->data, &m_data[index], temp->length);

		//crc32
		temp->CRC = CRC32::GeneratePNGCRC32(temp->type,4, temp->data, temp->length);

		fullLength -= temp->length;

		chunks->push_back(temp);
	}


	return chunks;
}

void PNG::writeChunk(chunk * chk, std::ostream & writer)
{

	uint32_t eswap;

	eswap = swap_endian(chk->length);
	writer.write((const char*)&eswap, 4);

	writer.write((const char*)&chk->type, 4);

	writer.write((const char*) chk->data, chk->length);

	eswap = swap_endian(chk->CRC);
	writer.write((const char*)&eswap, 4);

}

void PNG::WriteColor(const pixel8 & src, char * dest)
{
	//obligatory: byte = 8 bits
	//m_depth(bits per channel) / 8 = bytes per channel

	//red
	memcpy((char*)&dest[0], &src.r, m_depth / 8);
	//green
	memcpy((char*)&dest[1], &src.g, m_depth / 8);
	//blue
	memcpy((char*)&dest[2], &src.b, m_depth / 8);
}

void PNG::WriteGray(const pixel8 & src, char * dest)
{
	//gray
	uint8_t temp =  std::max(std::min( (int) std::round(float(src.r + src.g + src.b) / 3),255),0);
	memcpy((char*)&dest[0], &temp, m_depth / 8);
}

void PNG::WriteAlpha(const pixel8 & src, char * dest)
{
	memcpy((char*)&dest[0], &src.a, m_depth / 8);
}

void PNG::WritePalette(const pixel8 & src, char * dest)
{
	//unimplemented
}

uint32_t PNG::UncompressedLength()
{
	uint32_t bytesPerChannel = m_depth / 8;
	uint32_t pixelsize = GetChannels() * bytesPerChannel;

	uint32_t filterbytes = m_height;

	uint32_t total = ((m_width * m_height) * pixelsize) + filterbytes;

	return total;
}

void PNG::Read_IHDR(chunk* data)
{
	//it contains (in this order) the image's width (4 bytes), height (4 bytes), bit depth (1 byte), color type (1 byte), compression method (1 byte), filter method (1 byte), and interlace method (1 byte) (13 data bytes total)

	memcpy((char*)&m_width, &data->data[0], 4 * sizeof(char));
	m_width = swap_endian(m_width);

	memcpy((char*)&m_height, &data->data[4], 4 * sizeof(char));
	m_height = swap_endian(m_height);


	//1 byte don't need to be endian swapped
	memcpy((char*)&m_depth, &data->data[8], 1 * sizeof(char));

	memcpy((char*)&m_colortype, &data->data[9], 1 * sizeof(char));

	memcpy((char*)&m_compression, &data->data[10], 1 * sizeof(char));

	memcpy((char*)&m_filter, &data->data[11], 1 * sizeof(char));

	memcpy((char*)&m_interlace, &data->data[12], 1 * sizeof(char));

	delete data->data;
	delete data;


}

void PNG::Read_IDAT()
{
	//reading zlib header, turned out to be unnecessary 
	{
		int index = 0;
		char CMF = m_data[index];
		index++;

		//big endian, so 
		char CM = CMF & 0b00001111;
		char CINFO = CMF & 0b11110000;
		//For CM = 8, CINFO is the base-2 logarithm of the LZ77 window	size, minus eight(CINFO = 7 indicates a 32K window size).

		char FLG = m_data[index];
		index++;

		char FCHECK = FLG & 0b11111000;
		//The FCHECK value must be such that CMF and FLG, when viewed as a 16 - bit unsigned integer stored in MSB order(CMF * 256 + FLG),	is a multiple of 31. //effort
		char FDICT = FLG & 0b00000100;
		char FLEVEl = FLG & 0b00000011;

		char DICTID[4];
		if (FDICT > 0)
		{
			memcpy(DICTID, &m_data[index], 4);
			swap_endian(DICTID);
			index += 4;
		}
	}


	uint32_t outputLength = UncompressedLength();
	char* output = new char[outputLength];

	z_stream infstream;
	infstream.zalloc = Z_NULL;
	infstream.zfree = Z_NULL;
	infstream.opaque = Z_NULL;
	infstream.data_type = Z_BINARY;

	infstream.avail_in = m_length ; // size of input
	infstream.next_in = (Bytef *)m_data; // input char array
	infstream.avail_out = outputLength; // size of output
	infstream.next_out = (Bytef *)output; // output char array

	//// the actual DE-compression work.
	if (int ret = inflateInit(&infstream) != Z_OK)
		std::cout<<"failed to init inflate, error code: " << ret;
	else
	{
		if (inflate(&infstream, Z_FINISH) < 0)
		{
			std::cout << infstream.msg << std::endl;
		}
		else
		{
			DataToPixel(output, infstream.total_out);
		}
		inflateEnd(&infstream);
	}


	delete output;
}

void PNG::ReadColor(pixel8 & dest, const char* src)
{
	//obligatory: byte = 8 bits
	//m_depth(bits per channel) / 8 = bytes per channel

	//red
	memcpy((char*)&dest.r, &src[0], m_depth / 8);
	//green
	memcpy((char*)&dest.g, &src[1], m_depth / 8);
	//blue
	memcpy((char*)&dest.b, &src[2], m_depth / 8);
}

void PNG::ReadGray(pixel8 & dest, const char* src)
{
	memcpy((char*)&dest.r, &src[0], m_depth / 8);
	dest.b = dest.g = dest.r;
}

void PNG::ReadAlpha(pixel8 & dest, const char* src)
{
	memcpy((char*)&dest.a, &src[0], m_depth / 8);
}

void PNG::ReadPalette(pixel8 & dest, const char* src)
{
	//unimplemented
	std::cout << "index colors are unsupported, image will fail or be garbage\n";
}


template <typename T>
T PNG::swap_endian(T u)
{
	static_assert (CHAR_BIT == 8, "CHAR_BIT != 8");

	union
	{
		T u;
		unsigned char u8[sizeof(T)];
	} source, dest;

	source.u = u;

	for (size_t k = 0; k < sizeof(T); k++)
		dest.u8[k] = source.u8[sizeof(T) - k - 1];

	return dest.u;
}


