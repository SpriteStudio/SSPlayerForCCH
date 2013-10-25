//
//  ConverterShared.cpp
//
//  Created by Hiroki Azumada on 2013/04/17.
//

#include "ConverterShared.h"



namespace fs = boost::filesystem;

using namespace ss;
using boost::shared_ptr;


namespace ConverterShared {

    /** ssaxファイルを読み込む */
    SsMotion::Ptr loadSsax(const fs::path& ssaxPath, SsPlayerConverterResultCode& resultCode)
    {
        SsaxLoader::ResultCode result;
        SsMotion::Ptr motion = SsaxLoader::load(ssaxPath.generic_string(), &result);
        switch (result)
        {
            case SsaxLoader::SUCCESS:
                resultCode = SSPC_SUCCESS;
                break;
                
            case SsaxLoader::XML_PARSE_ERROR:
                resultCode = SSPC_SSAX_PARSE_FAILED;
                break;
                
            default:
                break;
        }
        return motion;
    }
    
};

