#pragma once
#include <fstream>
#include <vector>

namespace VRE
{
	//TO-DO: Refactor this into a file system that can read shaders/textures and other files
	class FileReader
	{
		public:
			static std::vector<char> ReadShaderFile(const std::string& fileName);
	};

}

