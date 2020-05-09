#include "utils.h"

namespace utils
{
	void read_directory(const std::string& name, stringvec& v)
	{
	    std::filesystem::path p(name);
	    std::filesystem::directory_iterator it(p);
	    std::filesystem::directory_iterator end;
	    //std::transform(start, end, std::back_inserter(v), path_filename());
	    for(auto& p: std::filesystem::recursive_directory_iterator(name))
	    {
	    	if(std::filesystem::is_regular_file(*it) && it->path().extension() == extension)
	    	{

	    		v.push_back(it->path().filename());
	    	}
	    	it++;
	    }
	}

}