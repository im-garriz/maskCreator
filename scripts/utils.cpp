#include "utils.h"

namespace utils
{
	void read_directory(const std::string& name, stringvec& v)
	{
	    std::filesystem::path p(name);
	    std::filesystem::directory_iterator it(p);
	    std::filesystem::directory_iterator end;

	    for(auto& p: std::filesystem::recursive_directory_iterator(name))
	    {
	    	if(std::filesystem::is_regular_file(*it) && it->path().extension() == extension)
	    	{
	    		// If the file has the desired extension, I store it on v
	    		v.push_back(it->path().filename());
	    	}
	    	it++;
	    }
	}

}