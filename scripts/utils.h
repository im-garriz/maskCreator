#include <iterator>
#include <algorithm>
#include <string>
#include <vector>
#include <filesystem>

// I declase extension extern so as to be read from main.cpp
extern std::string extension;

namespace utils
{
	typedef std::vector<std::string> stringvec;

	void read_directory(const std::string& name, stringvec& v);
}