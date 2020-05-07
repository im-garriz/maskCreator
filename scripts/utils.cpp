#include "utils.h"

namespace utils
{
	void read_directory(const std::string& name, stringvec& v)
	{
	    std::filesystem::path p(name);
	    std::filesystem::directory_iterator start(p);
	    std::filesystem::directory_iterator end;
	    std::transform(start, end, std::back_inserter(v), path_leaf_string());
	}

}