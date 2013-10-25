#ifndef _SSAX_LOADER_H_
#define _SSAX_LOADER_H_

#include <string>
#include "SsMotion.h"

struct SsaxLoader
{
public:
    enum ResultCode
    {
        SUCCESS,
        XML_PARSE_ERROR,
        
        END_OF_RESULT_CODE
    };
    
	/**
	 * ssaxファイルをロード
	 */
	static ss::SsMotion::Ptr load(const std::string& filename, ResultCode* result = 0);
};

#endif	// ifndef _SSAX_LOADER_H_
