#include <iterator>
#include <algorithm>
#include <string>
#include <vector>
#include <filesystem>

namespace utils
{
	typedef std::vector<std::string> stringvec;

	struct path_leaf_string
	{
	    std::string operator()(const std::filesystem::directory_entry& entry) const
	    {
	        return entry.path().filename();
	    }
	};

	void read_directory(const std::string& name, stringvec& v);
}