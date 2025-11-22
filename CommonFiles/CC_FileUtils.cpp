#include "CC_FileUtils.h"

namespace Cc
{
	std::string StripPathToFileName(const std::string& path)
	{
		std::filesystem::path p(path);
		std::string result = p.filename().string();
		return result;
	}
}


