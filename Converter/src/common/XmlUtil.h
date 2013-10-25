#ifndef _XML_UTIL_H_
#define _XML_UTIL_H_

#include <string>

namespace xmlutil
{
	/** XMLファイルのエンコーディングを得る */
	std::string getEncoding(const std::string& filename);
};

#endif	// _XML_UTIL_H_
