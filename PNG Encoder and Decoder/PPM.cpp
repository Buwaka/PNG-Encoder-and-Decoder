#include "pch.h"
#include "PPM.h"

#include <fstream>
#include <iostream>


PPM::PPM()
{
}

PPM::PPM(std::wstring path, std::vector<pixel8>& pixels, int width, int height, int type)
{
	Write(path, pixels, width, height, type);
}


PPM::~PPM()
{
}

bool PPM::Write(std::wstring path, std::vector<pixel8>& pixels, int width, int height, int type)
{
	//ppm test
	std::ofstream ppm;

	ppm.open(path, std::ios::out);

	if (!ppm.is_open())
	{
		std::cout << "failed to open write file\n";
		return false;
	}


	std::string header = "P" + std::to_string(m_type) + "\n" + std::to_string(width) + " " + std::to_string(height) + " " 
	+ std::to_string((int)std::pow(2, m_depth) - 1) + " \n";

	ppm.write(header.data(), header.size());

	for (size_t i = 0; i < pixels.size(); i++)
	{
		ppm << (unsigned int)pixels[i].r << " ";
		ppm << (unsigned int)pixels[i].g << " ";
		ppm << (unsigned int)pixels[i].b << " \n";
	}



	if (ppm.bad())
	{
		std::cout << "failed to write file\n";
		ppm.close();
		return false;
	}

	ppm.close();


	return true;
}
