#ifndef _SSF_LOADER_H_
#define _SSF_LOADER_H_

#include <string>
#include <vector>
#include "SsMotion.h"

struct SsfLoader
{
public:
	/**
	 * ssfファイルをロード 
	 */
	static ss::SsImageList::Ptr load(const std::string& filename);
};

#endif	// ifndef _SSF_LOADER_H_
