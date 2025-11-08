#include <FileReader.h>

namespace VRE
{
	void FileReader::ReadShaderFile(const std::string &fileName) {
		std::ifstream file(fileName, std::ios::ate | std::ios::binary);
		if (!file.is_open())
		{
			throw std::runtime_error("Could not open file!");
		}
		file.close();
	}

}