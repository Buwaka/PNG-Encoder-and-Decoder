#include "pch.h" 
#include <iostream> 
#include <string>
#include <fstream> 

#include "PNG.h"
#include "PPM.h"




int main()
{

	std::wstring input;

	std::cout << "PNG file to read (with extension, type 'END' to terminate program): ";
	std::getline(std::wcin, input);

	while (input != L"END")
	{




		PNG copy(input);

		std::vector<pixel8> temp = copy.GetPixels();

		//ppm test
		PPM ppm(input + L".ppm", temp, copy.GetWidth(), copy.GetHeight());
		std::cout << "\nWrote ppm file.";


		copy.Write(input + L"_copy.png");
		std::wcout << L"\nWrote png straight from copied in data stream. " + input + L"_copy.png";


	

		PNG newpng;

		newpng.ReadPixels(temp, copy.GetWidth(), copy.GetHeight());

		newpng.Write(input + L"_scratch.png");
		std::wcout << L"\nWrote png from raw rgb values. " + input + L"_scratch.png";

		std::cout << "\nPNG file to read (with extension, type 'END' to terminate program): ";
		std::getline(std::wcin, input);

	}
}