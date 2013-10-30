#ifndef _COCOS2D_SAVER_H_
#define _COCOS2D_SAVER_H_

#include <iostream>
#include <string>
#include "SsMotion.h"
#include "TextEncoding.h"

struct Cocos2dSaver
{
	/** cocos2dプレイヤー形式で出力する */
	static void save(
		std::ostream& out,
		bool binaryFormatMode,
		textenc::Encoding outEncoding,
		ss::SsMotion::Ptr motion,
		ss::SsImageList::ConstPtr optImageList,
		const std::string& prefixLabel,
		const std::string& creatorComment
		);
};

#endif	// ifndef _COCOS2D_SAVER_H_
