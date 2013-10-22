#ifndef _SS_MOTION_UTIL_H_
#define _SS_MOTION_UTIL_H_

#include <vector>
#include "SsMotion.h"

namespace ss
{
namespace utilities
{

	/** ノードと、その子孫ノードをリスト化する */
	std::vector<SsNode::ConstPtr> listTreeNodes(SsNode::ConstPtr node);

	/** 最大のIdを返す*/
	int maxPartId(SsMotion::ConstPtr& motion);

}	// namesapce utilities
}	// namespace ss

#endif	// ifndef _SS_MOTION_UTIL_H_
