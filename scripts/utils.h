#include <iterator>
#include <algorithm>
#include <string>
#include <vector>
#include <filesystem>

extern std::string extension;

namespace utils
{
	typedef std::vector<std::string> stringvec;

	void read_directory(const std::string& name, stringvec& v);
}