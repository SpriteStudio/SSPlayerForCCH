//
//  DebugUtil.h
//

#ifndef __StaticLib__DebugUtil__
#define __StaticLib__DebugUtil__

#include <iostream>
#include <vector>
#include "SsMotionDecoder.h"

namespace ss
{

namespace debug
{

	void dumpPrefix(std::ostream& out, int frameNo);
	void dumpSuffix(std::ostream& out);

	void dumpFrameParamList(std::ostream& out, const std::vector<SsMotionFrameDecoder::FrameParam>& paramList);
	void dumpFrameParam(std::ostream& out, const ss::SsMotionFrameDecoder::FrameParam& param);


}	// namespace debug

}	// namespace ss

#endif /* defined(__StaticLib__DebugUtil__) */
