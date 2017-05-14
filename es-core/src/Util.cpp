#include "Util.h"
#include "resources/ResourceManager.h"
#include "platform.h"
#include <signal.h>
#include <wait.h>
#include <iostream>
#include <fstream>
#include "Log.h"
#include <ctime>

namespace fs = boost::filesystem;

std::string strToUpper(const char* from)
{
	std::string str(from);
	for(unsigned int i = 0; i < str.size(); i++)
		str[i] = toupper(from[i]);
	return str;
}

std::string& strToUpper(std::string& str)
{
	for(unsigned int i = 0; i < str.size(); i++)
		str[i] = toupper(str[i]);

	return str;
}

std::string strToUpper(const std::string& str)
{
	return strToUpper(str.c_str());
}


#if defined(_WIN32) && _MSC_VER < 1800
float round(float num)
{
	return (float)((int)(num + 0.5f));
}
#endif

Eigen::Affine3f& roundMatrix(Eigen::Affine3f& mat)
{
	mat.translation()[0] = round(mat.translation()[0]);
	mat.translation()[1] = round(mat.translation()[1]);
	return mat;
}

Eigen::Affine3f roundMatrix(const Eigen::Affine3f& mat)
{
	Eigen::Affine3f ret = mat;
	roundMatrix(ret);
	return ret;
}

Eigen::Vector3f roundVector(const Eigen::Vector3f& vec)
{
	Eigen::Vector3f ret = vec;
	ret[0] = round(ret[0]);
	ret[1] = round(ret[1]);
	ret[2] = round(ret[2]);
	return ret;
}

Eigen::Vector2f roundVector(const Eigen::Vector2f& vec)
{
	Eigen::Vector2f ret = vec;
	ret[0] = round(ret[0]);
	ret[1] = round(ret[1]);
	return ret;
}

// embedded resources, e.g. ":/font.ttf", need to be properly handled too
std::string getCanonicalPath(const std::string& path)
{
	if(path.empty() || !boost::filesystem::exists(path))
		return path;

	return boost::filesystem::canonical(path).generic_string();
}

// expands "./my/path.sfc" to "[relativeTo]/my/path.sfc"
// if allowHome is true, also expands "~/my/path.sfc" to "/home/pi/my/path.sfc"
fs::path resolvePath(const fs::path& path, const fs::path& relativeTo, bool allowHome)
{
	// nothing here
	if(path.begin() == path.end())
		return path;

	if(*path.begin() == ".")
	{
		fs::path ret = relativeTo;
		for(auto it = ++path.begin(); it != path.end(); ++it)
			ret /= *it;
		return ret;
	}

	if(allowHome && *path.begin() == "~")
	{
		fs::path ret = getHomePath();
		for(auto it = ++path.begin(); it != path.end(); ++it)
			ret /= *it;
		return ret;
	}

	return path;
}

fs::path removeCommonPathUsingStrings(const fs::path& path, const fs::path& relativeTo, bool& contains)
{
#ifdef WIN32
	std::wstring pathStr = path.c_str();
	std::wstring relativeToStr = relativeTo.c_str();
#else
	std::string pathStr = path.c_str();
	std::string relativeToStr = relativeTo.c_str();
#endif
	if (pathStr.find_first_of(relativeToStr) == 0) {
		contains = true;
		return pathStr.substr(relativeToStr.size() + 1);
	}
	else {
		contains = false;
		return path;
	}
}

// example: removeCommonPath("/home/pi/roms/nes/foo/bar.nes", "/home/pi/roms/nes/") returns "foo/bar.nes"
fs::path removeCommonPath(const fs::path& path, const fs::path& relativeTo, bool& contains)
{
	// if either of these doesn't exist, fs::canonical() is going to throw an error
	if(!fs::exists(path) || !fs::exists(relativeTo))
	{
		contains = false;
		return path;
	}

	// if it's a symlink we don't want to apply fs::canonical on it, otherwise we'll lose the current parent_path
	fs::path p = (fs::is_symlink(path) ? fs::canonical(path.parent_path()) / path.filename() : fs::canonical(path));
	fs::path r = fs::canonical(relativeTo);

	if(p.root_path() != r.root_path())
	{
		contains = false;
		return p;
	}

	fs::path result;

	// find point of divergence
	auto itr_path = p.begin();
	auto itr_relative_to = r.begin();
	while(*itr_path == *itr_relative_to && itr_path != p.end() && itr_relative_to != r.end())
	{
		++itr_path;
		++itr_relative_to;
	}

	if(itr_relative_to != r.end())
	{
		contains = false;
		return p;
	}

	while(itr_path != p.end())
	{
		if(*itr_path != fs::path("."))
			result = result / *itr_path;

		++itr_path;
	}

	contains = true;
	return result;
}

// usage: makeRelativePath("/path/to/my/thing.sfc", "/path/to") -> "./my/thing.sfc"
// usage: makeRelativePath("/home/pi/my/thing.sfc", "/path/to", true) -> "~/my/thing.sfc"
fs::path makeRelativePath(const fs::path& path, const fs::path& relativeTo, bool allowHome)
{
	bool contains = false;

	fs::path ret = removeCommonPath(path, relativeTo, contains);
	if(contains)
	{
		// success
		ret = "." / ret;
		return ret;
	}

	if(allowHome)
	{
		contains = false;
		std::string homePath = getHomePath();
		ret = removeCommonPath(path, homePath, contains);
		if(contains)
		{
			// success
			ret = "~" / ret;
			return ret;
		}
	}

	// nothing could be resolved
	return path;
}

boost::posix_time::ptime string_to_ptime(const std::string& str, const std::string& fmt)
{
	std::istringstream ss(str);
	ss.imbue(std::locale(std::locale::classic(), new boost::posix_time::time_input_facet(fmt))); //std::locale handles deleting the facet
	boost::posix_time::ptime time;
	ss >> time;

	return time;
}

#ifdef _RPI_
int getFreeMemory()
{
  int returnValue;
  const int BUFFER_SIZE = 1000;
  char buffer[BUFFER_SIZE];
  FILE *fInput;
  int loop;
  int len;
  char ch;
  returnValue = -1;
  fInput = fopen("/proc/meminfo","r");
  if (fInput != NULL)
  {
    while (!feof(fInput))
    {
      fgets(buffer,BUFFER_SIZE-1,fInput);
      if (feof(fInput))
      {
        break;
      }
      buffer[BUFFER_SIZE-1] = 0;
      // Look for serial number
      if (strncmp(buffer,"MemFree:",8)==0)
      {
        // Extract mem free from the line.
        for(loop=0;loop<BUFFER_SIZE;loop++)
        {
          ch = buffer[loop];
          if (ch == ':')
          {
             returnValue = 0;
             continue;
          }
          if (ch == 0)
          {
              break;
          }
          if (returnValue >=0)
          {
             if (ch >='A')
             {
                break;
             }
             if ((ch >='0') && (ch <='9'))
             {
                returnValue = returnValue * 10 + (ch-'0');
             }
          }
        }
        break;
      }
    } 
    fclose(fInput);
  }
  return returnValue;
}

int getFreeMaxGPUMemory()
{
	return 0;
	FILE * f = popen( "sudo vcdbg reloc | grep \"[0-9]* free memory\" | grep -o \"[0-9\\.]*[MK]\" | grep -o \"[0-9\\.]*\"", "r" );
    if ( f == 0 ) {
        LOG(LogError) << "Couldn't get GPU Memory!";
        return 0;
    }
    std::string mem;
    const int BUFSIZE = 1000;
    char buf[ BUFSIZE ];
    while( fgets( buf, BUFSIZE,  f ) ) {
      mem += buf;        
    }
    pclose( f );
    //LOG(LogError) << "Memory: " << mem;
    int out = 0;

    if (mem == "")
    	mem = "0";

    try {
    	out = std::stoi(mem);
    } 
    catch (int e)
    {
    	out = 0;
    }

    return out;

}

int getFreeGPUMemory()
{
	// TO DO
	// sudo vcdbg reloc
	// sudo vcdbg reloc | grep "largest free block"
	// /opt/vc/lib /opt/vc/bin/vcdbg reloc
	// https://github.com/nezticle/RaspberryPi-BuildRoot/wiki/VideoCore-Tools
	// see https://github.com/MilhouseVH/bcmstat/blob/master/bcmstat.sh#L586

	/*pid_t pid = fork();
	if (!pid)
	{
		// do stuff
		execl(getenv("SHELL"),"sh","-c","sudo vcdbg reloc | grep \"largest free block is [0-9]*\" | grep -o \"[0-9]*\" > /home/pi/.emulationstation/mem.log",NULL);
		
	    exit(1);
	}
	else {
		// now the ptm file handle is used to send data
		// to the process and to receive output from the process
		int status;
		waitpid(pid, &status, 0);
		std::string mem;
		std::ifstream infile;
		infile.open ("/home/pi/.emulationstation/mem.log");
	    getline(infile,mem); // Saves the line in STRING.
	    infile.close();

		return std::stoi(mem);
	}*/

	return 0;

	FILE * f = popen( "sudo vcdbg reloc | grep \"largest free block is [0-9\\.]*\" | grep -o \"[0-9\\.]*\"", "r" );
    if ( f == 0 ) {
        LOG(LogError) << "Couldn't get GPU Memory!";
        return 0;
    }
    std::string mem;
    const int BUFSIZE = 1000;
    char buf[ BUFSIZE ];
    while( fgets( buf, BUFSIZE,  f ) ) {
      mem += buf;        
    }
    pclose( f );
    //LOG(LogError) << "Memory: " << mem;

	int out = 0;

	if (mem == "")
    	mem = "0";

    try {
    	out = std::stoi(mem);
    } 
    catch (int e)
    {
    	out = 0;
    }

    return out;

}
#endif