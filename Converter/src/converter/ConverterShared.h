//
//  ConverterShared.h
//
//  Created by Hiroki Azumada on 2013/04/17.
//

#ifndef __ConverterShared__
#define __ConverterShared__

#include <boost/shared_ptr.hpp>
#include <boost/filesystem/path.hpp>

#include "SsaxLoader.h"
#include "SsPlayerConverter.h"

namespace ConverterShared
{
    /** ssaxファイルを読み込む */
    ss::SsMotion::Ptr loadSsax(const boost::filesystem::path& ssaxPath, SsPlayerConverterResultCode& resultCode);

};

#endif /* defined(__ConverterShared__) */
