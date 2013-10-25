
#include "XmlUtil.h"
#include <fstream>
#include <iostream>
#include <boost/xpressive/xpressive.hpp>


namespace xmlutil
{

/** XMLファイルのエンコーディングを得る */
std::string getEncoding(const std::string& filename)
{
	using namespace boost::xpressive;

	std::string enc = "";
	std::ifstream ifs(filename.c_str(), std::ifstream::binary | std::ifstream::in);
	if (ifs)
	{
		ifs.seekg(0, std::ios::end);
		size_t length = static_cast<size_t>(ifs.tellg());
		ifs.seekg(0, std::ios::beg);

		char* buffer = new char[length];
		ifs.read(buffer, length);
		std::string buf(buffer);

		sregex declpt = sregex::compile("<\\?xml [^>]+\\?>");
		smatch m;
		if (regex_search(buf, m, declpt))
		{
			std::string decl = m.str();
			//std::cout << decl << std::endl;

			sregex encpt = sregex::compile("encoding=[\"']([\\w\\-]+)[\"']");
			if (regex_search(decl, m, encpt))
			{
				enc = m[1];
				//std::cout << enc << std::endl;
			}
		}

		delete [] buffer;
	}
	return enc;
}


};  // xmlutil

