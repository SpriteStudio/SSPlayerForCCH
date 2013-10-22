
#include "SsMotionUtil.h"
#include <boost/foreach.hpp>

namespace ss
{
namespace utilities
{

	static void listTreeNodesSub(std::vector<SsNode::ConstPtr>& list, SsNode::ConstPtr node)
	{
		list.push_back(node);
		BOOST_FOREACH( SsNode::ConstPtr child, node->getChildren() )
		{
			listTreeNodesSub(list, child);
		}
	}

	/** ノードと、その子孫ノードをリスト化する */
	std::vector<SsNode::ConstPtr> listTreeNodes(SsNode::ConstPtr node)
	{
		std::vector<SsNode::ConstPtr> list;
		listTreeNodesSub(list, node);
		return list;
	}


}	// namesapce utilities
}	// namespace ss

