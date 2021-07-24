#pragma once
#include <vector>
#include <string>
#include "Structs.h"

class PPM
{
public:
	PPM();
	PPM(std::wstring path, std::vector<pixel8>& pixels, int width, int height, int type = 3);
	~PPM();

	bool Write(std::wstring path, std::vector<pixel8>& pixels, int width, int height, int type = 3);

private:
	int m_width, m_height, m_type{3}, m_depth{8};
};

